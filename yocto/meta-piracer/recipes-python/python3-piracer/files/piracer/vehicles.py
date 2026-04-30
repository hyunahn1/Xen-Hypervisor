# Copyright (C) 2022 twyleg (modified for Yocto: uses smbus2 instead of adafruit-blinka)
import math
import time
from abc import abstractmethod

try:
    import smbus2
    SMBUS2_AVAILABLE = True
except ImportError:
    SMBUS2_AVAILABLE = False


class _PCA9685:
    """Minimal PCA9685 PWM driver via smbus2 (replaces adafruit_pca9685)."""
    MODE1     = 0x00
    PRESCALE  = 0xFE
    LED0_ON_L = 0x06
    PWM_MAX   = 65535

    def __init__(self, bus, address: int = 0x40):
        self._bus  = bus
        self._addr = address
        self._bus.write_byte_data(self._addr, self.MODE1, 0x00)
        time.sleep(0.005)

    def _set_freq(self, freq_hz: float):
        prescale = max(3, int(25_000_000.0 / (4096 * freq_hz) - 1))
        old = self._bus.read_byte_data(self._addr, self.MODE1)
        self._bus.write_byte_data(self._addr, self.MODE1, (old & 0x7F) | 0x10)
        self._bus.write_byte_data(self._addr, self.PRESCALE, prescale)
        self._bus.write_byte_data(self._addr, self.MODE1, old)
        time.sleep(0.005)
        self._bus.write_byte_data(self._addr, self.MODE1, old | 0xA1)

    def _set_pwm(self, channel: int, on: int, off: int):
        reg = self.LED0_ON_L + 4 * channel
        self._bus.write_i2c_block_data(self._addr, reg, [
            on & 0xFF, on >> 8, off & 0xFF, off >> 8
        ])

    class _Channel:
        def __init__(self, pca, ch):
            self._pca = pca
            self._ch  = ch

        @property
        def duty_cycle(self):
            return 0

        @duty_cycle.setter
        def duty_cycle(self, value: int):
            # value: 0-65535
            if value == 0:
                self._pca._set_pwm(self._ch, 0, 0)
            elif value >= 65535:
                self._pca._set_pwm(self._ch, 4096, 0)
            else:
                off = int(value * 4096 // 65536)
                self._pca._set_pwm(self._ch, 0, off)

    def __init_subclass__(cls, **kw):
        super().__init_subclass__(**kw)

    @property
    def channels(self):
        return [self._Channel(self, i) for i in range(16)]

    @property
    def frequency(self):
        return 50.0

    @frequency.setter
    def frequency(self, freq_hz: float):
        self._set_freq(freq_hz)


class _INA219:
    """Minimal INA219 voltage/current reader via smbus2."""
    REG_BUS_VOLTAGE   = 0x02
    REG_SHUNT_VOLTAGE = 0x01
    REG_CURRENT       = 0x04

    def __init__(self, bus, addr: int = 0x41):
        self._bus  = bus
        self._addr = addr

    def _read_word(self, reg: int) -> int:
        raw = self._bus.read_word_data(self._addr, reg)
        return ((raw & 0xFF) << 8) | (raw >> 8)

    @property
    def bus_voltage(self) -> float:
        try:
            raw = self._read_word(self.REG_BUS_VOLTAGE)
            return (raw >> 3) * 0.004
        except Exception:
            return 7.8

    @property
    def shunt_voltage(self) -> float:
        try:
            raw = self._read_word(self.REG_SHUNT_VOLTAGE)
            if raw > 32767:
                raw -= 65536
            return raw * 0.00001
        except Exception:
            return 0.0

    @property
    def current(self) -> float:
        try:
            raw = self._read_word(self.REG_CURRENT)
            if raw > 32767:
                raw -= 65536
            return raw * 0.1
        except Exception:
            return 150.0

    @property
    def power(self) -> float:
        return self.bus_voltage * abs(self.current) / 1000.0


class PiRacerBase:
    PWM_RESOLUTION    = 16
    PWM_MAX_RAW_VALUE = math.pow(2, PWM_RESOLUTION) - 1
    PWM_FREQ_50HZ     = 50.0
    PWM_WAVELENGTH_50HZ = 1.0 / PWM_FREQ_50HZ

    @classmethod
    def _set_channel_active_time(cls, t: float, pwm_controller: _PCA9685, channel: int):
        raw = int(cls.PWM_MAX_RAW_VALUE * (t / cls.PWM_WAVELENGTH_50HZ))
        pwm_controller.channels[channel].duty_cycle = raw

    @classmethod
    def _get_50hz_duty_cycle_from_percent(cls, value: float) -> float:
        return 0.0015 + (value * 0.0005)

    def __init__(self):
        if not SMBUS2_AVAILABLE:
            raise ImportError("smbus2 not available — cannot initialize PiRacer HAT")
        self.i2c_bus = smbus2.SMBus(1)

    def _warmup(self):
        self.set_steering_percent(0.0)
        self.set_throttle_percent(0.0)
        time.sleep(2.0)

    def get_battery_voltage(self) -> float:
        return self.battery_monitor.bus_voltage + self.battery_monitor.shunt_voltage

    def get_battery_current(self) -> float:
        return self.battery_monitor.current

    def get_power_consumption(self) -> float:
        return self.battery_monitor.power

    @abstractmethod
    def set_steering_percent(self, value: float): pass

    @abstractmethod
    def set_throttle_percent(self, value: float): pass


class PiRacerPro(PiRacerBase):
    PWM_STEERING_CHANNEL  = 0
    PWM_THROTTLE_CHANNEL  = 1

    def __init__(self):
        super().__init__()
        self.pwm_controller = _PCA9685(self.i2c_bus, address=0x40)
        self.pwm_controller.frequency = self.PWM_FREQ_50HZ
        self.battery_monitor = _INA219(self.i2c_bus, addr=0x42)
        self._warmup()

    def set_steering_percent(self, value: float):
        self._set_channel_active_time(
            self._get_50hz_duty_cycle_from_percent(-value),
            self.pwm_controller, self.PWM_STEERING_CHANNEL)

    def set_throttle_percent(self, value: float):
        self._set_channel_active_time(
            self._get_50hz_duty_cycle_from_percent(value),
            self.pwm_controller, self.PWM_THROTTLE_CHANNEL)


class PiRacerStandard(PiRacerBase):
    PWM_STEERING_CHANNEL               = 0
    PWM_THROTTLE_CHANNEL_LEFT_MOTOR_IN1  = 5
    PWM_THROTTLE_CHANNEL_LEFT_MOTOR_IN2  = 6
    PWM_THROTTLE_CHANNEL_LEFT_MOTOR_PWM  = 7
    PWM_THROTTLE_CHANNEL_RIGHT_MOTOR_IN1 = 1
    PWM_THROTTLE_CHANNEL_RIGHT_MOTOR_IN2 = 2
    PWM_THROTTLE_CHANNEL_RIGHT_MOTOR_PWM = 0

    def __init__(self):
        super().__init__()
        self.steering_pwm_controller = _PCA9685(self.i2c_bus, address=0x40)
        self.steering_pwm_controller.frequency = self.PWM_FREQ_50HZ
        self.throttle_pwm_controller = _PCA9685(self.i2c_bus, address=0x60)
        self.throttle_pwm_controller.frequency = self.PWM_FREQ_50HZ
        self.battery_monitor = _INA219(self.i2c_bus, addr=0x41)
        self._warmup()

    def set_steering_percent(self, value: float):
        self._set_channel_active_time(
            self._get_50hz_duty_cycle_from_percent(-value),
            self.steering_pwm_controller, self.PWM_STEERING_CHANNEL)

    def set_throttle_percent(self, value: float):
        ch = self.throttle_pwm_controller.channels
        if value > 0:
            ch[self.PWM_THROTTLE_CHANNEL_LEFT_MOTOR_IN1].duty_cycle  = int(self.PWM_MAX_RAW_VALUE)
            ch[self.PWM_THROTTLE_CHANNEL_LEFT_MOTOR_IN2].duty_cycle  = 0
            ch[self.PWM_THROTTLE_CHANNEL_RIGHT_MOTOR_IN1].duty_cycle = 0
            ch[self.PWM_THROTTLE_CHANNEL_RIGHT_MOTOR_IN2].duty_cycle = int(self.PWM_MAX_RAW_VALUE)
        else:
            ch[self.PWM_THROTTLE_CHANNEL_LEFT_MOTOR_IN1].duty_cycle  = 0
            ch[self.PWM_THROTTLE_CHANNEL_LEFT_MOTOR_IN2].duty_cycle  = int(self.PWM_MAX_RAW_VALUE)
            ch[self.PWM_THROTTLE_CHANNEL_RIGHT_MOTOR_IN1].duty_cycle = int(self.PWM_MAX_RAW_VALUE)
            ch[self.PWM_THROTTLE_CHANNEL_RIGHT_MOTOR_IN2].duty_cycle = 0

        raw = int(self.PWM_MAX_RAW_VALUE * abs(value))
        ch[self.PWM_THROTTLE_CHANNEL_LEFT_MOTOR_PWM].duty_cycle  = raw
        ch[self.PWM_THROTTLE_CHANNEL_RIGHT_MOTOR_PWM].duty_cycle = raw
