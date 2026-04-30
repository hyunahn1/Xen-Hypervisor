#!/usr/bin/env python3
"""
PiRacer Bridge - Python to Qt Communication
============================================

Reads PiRacer vehicle data (battery, power, etc.) and outputs it in JSON format
to stdout for communication with the Qt application.

Author: Ahn Hyunjun
Date: 2026-02-16
Version: 1.0.0
"""

import json
import os
import subprocess
import sys
import time
import struct
import fcntl
from enum import Enum
from typing import Dict, Optional

# i2c-dev 모듈 자동 로드 (PiRacer HAT I2C 통신 필요)
try:
    subprocess.run(["modprobe", "i2c-dev"], check=False, capture_output=True)
except Exception:
    pass

from battery import BatteryMonitor, calculate_battery_percent

# Ensure project root is importable regardless of current working directory.
PROJECT_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
if PROJECT_ROOT not in sys.path:
    sys.path.insert(0, PROJECT_ROOT)

try:
    from piracer.vehicles import PiRacerStandard, PiRacerPro
    PIRACER_AVAILABLE = True
except ImportError:
    PIRACER_AVAILABLE = False
    print("Warning: piracer-py not installed. Running in simulation mode.", file=sys.stderr)

try:
    from piracer.gamepads import ShanWanGamepad
    GAMEPAD_AVAILABLE = True
except ImportError:
    GAMEPAD_AVAILABLE = False
    print("Warning: ShanWanGamepad not available. Direction fallback enabled.", file=sys.stderr)

try:
    import can
    CAN_AVAILABLE = True
except ImportError:
    CAN_AVAILABLE = False
    print("Warning: python-can not installed. CAN input disabled.", file=sys.stderr)


class DriveMode(Enum):
    NEUTRAL = "N"
    DRIVE = "F"
    REVERSE = "R"
    BRAKE = "N"


class PiRacerBridge:
    """
    Bridge class that reads PiRacer data and outputs it as JSON.
    """
    
    # Update interval (seconds)
    UPDATE_INTERVAL = 0.5
    # Speed CAN ID based on current field logs (candump: "can0 123 [8] ..")
    SPEED_CAN_ID = 0x123
    DRIVE_MODE_SNAPSHOT_PATH = "/tmp/piracer_drive_mode.json"
    
    def __init__(self, vehicle_type: str = "standard"):
        """
        Initialize PiRacer bridge.

        Args:
            vehicle_type: "standard" or "pro"
        """
        self.vehicle_type = vehicle_type
        self.piracer: Optional[PiRacerStandard] = None
        self.last_throttle = 0.0
        self.can_bus = None
        self.last_can_speed_kmh = 0.0
        self.last_can_rpm = 0.0
        self.battery_monitor = BatteryMonitor(capacity_mah=2500.0, alpha=0.12, cells=3)
        self.gamepad = None
        self.drive_mode = DriveMode.NEUTRAL
        self.prev_buttons = {"x": False, "a": False, "b": False, "y": False}
        
        if PIRACER_AVAILABLE:
            try:
                if vehicle_type == "pro":
                    self.piracer = PiRacerPro()
                else:
                    self.piracer = PiRacerStandard()
                print("PiRacer initialized successfully!", file=sys.stderr)
            except Exception as e:
                print(f"Error initializing PiRacer: {e}", file=sys.stderr)
                self.piracer = None
        else:
            print("Running in simulation mode", file=sys.stderr)

        if CAN_AVAILABLE:
            try:
                self.can_bus = can.interface.Bus(channel="can0", bustype="socketcan")
                print("CAN bus initialized on can0", file=sys.stderr)
            except Exception as e:
                print(f"Warning: failed to open can0: {e}", file=sys.stderr)
                self.can_bus = None

        if GAMEPAD_AVAILABLE:
            try:
                self.gamepad = ShanWanGamepad()
                jsdev = getattr(self.gamepad, "jsdev", None)
                if jsdev is not None:
                    fd = jsdev.fileno()
                    flags = fcntl.fcntl(fd, fcntl.F_GETFL)
                    fcntl.fcntl(fd, fcntl.F_SETFL, flags | os.O_NONBLOCK)
                print("Gamepad initialized for drive direction", file=sys.stderr)
            except Exception as e:
                print(f"Warning: failed to initialize gamepad: {e}", file=sys.stderr)
                self.gamepad = None

    def read_can_data(self) -> Dict:
        result: Dict = {}
        if self.can_bus is None:
            return result

        # Non-blocking read up to 10 messages per cycle
        for _ in range(10):
            try:
                msg = self.can_bus.recv(timeout=0.0)
            except Exception:
                break

            if msg is None:
                break

            if msg.arbitration_id == self.SPEED_CAN_ID and len(msg.data) >= 1:
                # Format 1) 1-byte integer speed (km/h) - current candump format
                # e.g. 11 00 00 00 ... -> 17 km/h
                if len(msg.data) >= 4 and msg.data[1] == 0 and msg.data[2] == 0 and msg.data[3] == 0:
                    self.last_can_speed_kmh = float(int(msg.data[0]))
                    result["speed_kmh"] = self.last_can_speed_kmh
                elif len(msg.data) >= 4:
                    # Format 2) 4-byte float speed (km/h)
                    try:
                        self.last_can_speed_kmh = float(struct.unpack("<f", msg.data[0:4])[0])
                        result["speed_kmh"] = self.last_can_speed_kmh
                    except struct.error:
                        pass

            elif msg.arbitration_id == (self.SPEED_CAN_ID + 1) and len(msg.data) >= 4:
                try:
                    self.last_can_rpm = float(struct.unpack("<f", msg.data[0:4])[0])
                    result["rpm"] = self.last_can_rpm
                except struct.error:
                    pass

        return result
    
    def get_direction_from_throttle(self, throttle: float) -> str:
        """
        Determine direction from throttle value.

        Args:
            throttle: Throttle value (-1.0 ~ 1.0)

        Returns:
            "F" (Forward), "R" (Reverse), or "N" (Neutral)
        """
        if throttle > 0.05:  # Dead zone
            return "F"
        elif throttle < -0.05:
            return "R"
        else:
            return "N"

    def _write_drive_mode_snapshot(self) -> None:
        """drive_mode를 파일에 기록 (Head Unit/Cluster 동기화용)"""
        try:
            with open(self.DRIVE_MODE_SNAPSHOT_PATH, "w", encoding="utf-8") as f:
                json.dump({"direction": self.drive_mode.value}, f)
        except Exception as e:
            print(f"Warning: drive_mode write failed: {e}", file=sys.stderr)

    def read_gamepad_and_apply_motor(self) -> None:
        """
        게임패드 읽기 → 기어(X/A/B/Y) + 스틱(스로틀/스티어링) → 모터 제어.
        hu_motor_control 역할 통합. Head Unit 터치(파일) 먼저 확인.
        """
        # Head Unit(IPC)에서 온 기어가 있으면 반영
        self.update_drive_mode_from_snapshot()

        if self.gamepad is None:
            if self.piracer is not None:
                self.piracer.set_throttle_percent(0.0)
                self.piracer.set_steering_percent(0.0)
            return

        try:
            gamepad_input = self.gamepad.read_data()
        except (BlockingIOError, OSError, Exception):
            return

        # 스틱: 우측 Y=스로틀, 좌측 X=스티어링
        throttle_raw = float(getattr(
            getattr(gamepad_input, "analog_stick_right", None), "y", 0.0
        ) or 0.0)
        steering_raw = float(getattr(
            getattr(gamepad_input, "analog_stick_left", None), "x", 0.0
        ) or 0.0)

        # 버튼으로 기어 변경 (X=F, B=R, Y/A=N)
        x_pressed = bool(getattr(gamepad_input, "button_x", False))
        a_pressed = bool(getattr(gamepad_input, "button_a", False))
        b_pressed = bool(getattr(gamepad_input, "button_b", False))
        y_pressed = bool(getattr(gamepad_input, "button_y", False))

        if x_pressed and not self.prev_buttons["x"]:
            self.drive_mode = DriveMode.DRIVE
            self._write_drive_mode_snapshot()
        if b_pressed and not self.prev_buttons["b"]:
            self.drive_mode = DriveMode.REVERSE
            self._write_drive_mode_snapshot()
        if y_pressed and not self.prev_buttons["y"]:
            self.drive_mode = DriveMode.NEUTRAL
            self._write_drive_mode_snapshot()
        if a_pressed and not self.prev_buttons["a"]:
            self.drive_mode = DriveMode.BRAKE
            self._write_drive_mode_snapshot()

        self.prev_buttons["x"] = x_pressed
        self.prev_buttons["a"] = a_pressed
        self.prev_buttons["b"] = b_pressed
        self.prev_buttons["y"] = y_pressed

        # 모터 제어 - piracer HAT 없으면 기어 UI만 업데이트하고 스킵
        if self.piracer is None:
            return

        # 기어에 맞게 스로틀 적용
        if self.drive_mode == DriveMode.NEUTRAL or self.drive_mode == DriveMode.BRAKE:
            throttle = 0.0
        elif self.drive_mode == DriveMode.REVERSE:
            throttle = -abs(throttle_raw)
        else:
            throttle = abs(throttle_raw)

        self.last_throttle = throttle
        self.piracer.set_throttle_percent(throttle)
        self.piracer.set_steering_percent(steering_raw)

    def update_drive_mode_from_gamepad(self) -> None:
        """
        레거시: drive_mode만 업데이트 (read_piracer_data에서 호출)
        Motor 제어는 read_gamepad_and_apply_motor에서 처리.
        """
        if self.gamepad is None:
            return

        try:
            gamepad_input = self.gamepad.read_data()
        except BlockingIOError:
            return
        except OSError as e:
            if e.errno in (11, 35):
                return
            return
        except Exception:
            return

        x_pressed = bool(getattr(gamepad_input, "button_x", False))
        a_pressed = bool(getattr(gamepad_input, "button_a", False))
        b_pressed = bool(getattr(gamepad_input, "button_b", False))
        y_pressed = bool(getattr(gamepad_input, "button_y", False))

        if x_pressed and not self.prev_buttons["x"]:
            self.drive_mode = DriveMode.DRIVE
        if b_pressed and not self.prev_buttons["b"]:
            self.drive_mode = DriveMode.REVERSE
        if y_pressed and not self.prev_buttons["y"]:
            self.drive_mode = DriveMode.NEUTRAL
        if a_pressed and not self.prev_buttons["a"]:
            self.drive_mode = DriveMode.BRAKE

        self.prev_buttons["x"] = x_pressed
        self.prev_buttons["a"] = a_pressed
        self.prev_buttons["b"] = b_pressed
        self.prev_buttons["y"] = y_pressed

    def update_drive_mode_from_snapshot(self) -> bool:
        """
        Read drive mode from external control script snapshot.
        Returns True when direction was updated from snapshot.
        """
        try:
            stat = os.stat(self.DRIVE_MODE_SNAPSHOT_PATH)
        except OSError:
            return False

        # Ignore stale snapshot.
        if (time.time() - stat.st_mtime) > 2.0:
            return False

        try:
            with open(self.DRIVE_MODE_SNAPSHOT_PATH, "r", encoding="utf-8") as fp:
                data = json.load(fp)
        except (OSError, ValueError, json.JSONDecodeError):
            return False

        direction = str(data.get("direction", "")).strip().upper()
        if direction == "F":
            self.drive_mode = DriveMode.DRIVE
            return True
        if direction == "R":
            self.drive_mode = DriveMode.REVERSE
            return True
        if direction == "N":
            self.drive_mode = DriveMode.NEUTRAL
            return True
        return False
    
    def read_piracer_data(self) -> Dict:
        """
        Read actual data from PiRacer.
        drive_mode는 read_gamepad_and_apply_motor에서 유지됨.
        """
        can_data = self.read_can_data()
        
        try:
            if self.piracer is not None:
                voltage = self.piracer.get_battery_voltage()
                current = self.piracer.get_battery_current()
                power = self.piracer.get_power_consumption()
            else:
                sim = self.get_simulation_data()
                return sim

            percent = self.battery_monitor.update(voltage, current)
            direction = self.drive_mode.value
            if self.gamepad is None:
                # Fallback when gamepad is not connected
                direction = self.get_direction_from_throttle(self.last_throttle)
                if "speed_kmh" in can_data and abs(float(can_data["speed_kmh"])) > 0.1 and direction == "N":
                    direction = "F"

            output = {
                "battery": {
                    "voltage": round(voltage, 2),
                    "percent": round(percent, 1),
                    "current": round(current, 1),
                    "power": round(power, 2)
                },
                "direction": direction,
                "timestamp": time.time()
            }
            if "speed_kmh" in can_data:
                output["speed_kmh"] = round(float(can_data["speed_kmh"]), 2)
            if "rpm" in can_data:
                output["rpm"] = round(float(can_data["rpm"]), 1)
            return output
        except Exception as e:
            print(f"Error reading PiRacer data: {e}", file=sys.stderr)
            return self.get_simulation_data()
    
    def get_simulation_data(self) -> Dict:
        """
        Generate simulation data (for testing).

        Returns:
            Simulation data dictionary
        """
        # Time-varying simulation data
        t = time.time()
        voltage = 7.8 + 0.2 * (t % 10) / 10  # 7.8 ~ 8.0V variation
        percent = calculate_battery_percent(voltage)
        
        return {
            "battery": {
                "voltage": round(voltage, 2),
                "percent": round(percent, 1),
                "current": round(150.0 + 50.0 * (t % 5) / 5, 1),  # 150~200 mA
                "power": round(voltage * 0.15, 2)  # Watts
            },
            "direction": "N",
            "timestamp": t
        }
    
    MOTOR_INTERVAL = 0.02  # 모터 제어 주기 (50Hz, hu_motor_control과 동일)

    def run(self):
        """
        Main loop: 모터 제어(0.02s) + JSON 출력(0.5s) 통합.
        게임패드 X/A/B/Y=기어, 스틱=스로틀/스티어링. run_gamepad_service.sh 불필요.
        """
        print("PiRacer Bridge started (gamepad + motor 통합)", file=sys.stderr)
        print("  X=전진, B=후진, Y/A=중립 | 스틱=스로틀/스티어링", file=sys.stderr)
        print(f"Update interval: {self.UPDATE_INTERVAL}s", file=sys.stderr)
        print("-" * 50, file=sys.stderr)
        
        iteration = 0
        try:
            while True:
                self.read_gamepad_and_apply_motor()
                iteration += 1
                if iteration % 25 == 0:
                    data = self.read_piracer_data()
                    json_output = json.dumps(data)
                    print(json_output)
                    sys.stdout.flush()
                time.sleep(self.MOTOR_INTERVAL)
        
        except KeyboardInterrupt:
            print("\nPiRacer Bridge stopped by user", file=sys.stderr)
        except Exception as e:
            print(f"Fatal error: {e}", file=sys.stderr)
            sys.exit(1)


def main():
    """
    Main entry point.
    """
    import argparse
    
    parser = argparse.ArgumentParser(description="PiRacer Bridge")
    parser.add_argument(
        "--type",
        choices=["standard", "pro"],
        default="standard",
        help="PiRacer vehicle type (default: standard)"
    )
    parser.add_argument(
        "--interval",
        type=float,
        default=0.5,
        help="Update interval in seconds (default: 0.5)"
    )
    
    args = parser.parse_args()
    
    bridge = PiRacerBridge(vehicle_type=args.type)
    bridge.UPDATE_INTERVAL = args.interval
    
    bridge.run()


if __name__ == "__main__":
    main()
