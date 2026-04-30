"""
battery_monitor.py
==================

Battery percentage estimation for PiRacer dashboard / instrument cluster.

Goal:
- Stable, realistic percent from pack voltage
- Avoid bouncing due to load sag (acceleration)
- Work even if current readings are imperfect

Assumptions:
- Default chemistry: LiPo (common for 2S/3S RC packs)
- Percent is based on a typical LiPo open-circuit / resting curve.
"""

from __future__ import annotations
from dataclasses import dataclass
from typing import Optional


def _clamp(x: float, lo: float, hi: float) -> float:
    return max(lo, min(hi, x))


# Typical LiPo per-cell curve: (volts_per_cell, percent)
_LIPO_CURVE = [
    (4.20, 100.0),
    (4.10,  90.0),
    (4.00,  80.0),
    (3.92,  70.0),
    (3.85,  60.0),
    (3.79,  50.0),
    (3.74,  40.0),
    (3.70,  30.0),
    (3.66,  20.0),
    (3.62,  10.0),
    (3.50,   0.0),
]


def _interp_curve(v_cell: float) -> float:
    """Piecewise linear interpolation on _LIPO_CURVE."""
    if v_cell >= _LIPO_CURVE[0][0]:
        return _LIPO_CURVE[0][1]
    if v_cell <= _LIPO_CURVE[-1][0]:
        return _LIPO_CURVE[-1][1]

    for (v_hi, p_hi), (v_lo, p_lo) in zip(_LIPO_CURVE, _LIPO_CURVE[1:]):
        if v_hi >= v_cell >= v_lo:
            t = (v_cell - v_lo) / (v_hi - v_lo)
            return p_lo + t * (p_hi - p_lo)

    return 0.0


def detect_cells_from_voltage(voltage: float) -> int:
    """
    Simple auto-detection based on pack voltage.
    Works for typical LiPo voltage ranges.
    """
    if voltage <= 0:
        return 2

    # Rough thresholds for LiPo packs
    # 2S: ~6.0..8.4, 3S: ~9.0..12.6, 4S: ~12.0..16.8
    if voltage < 9.0:
        return 2
    if voltage < 12.0:
        return 3
    return 4


def calculate_battery_percent(voltage: float, cells: Optional[int] = None) -> float:
    """
    Voltage -> percent using LiPo curve.
    cells: force cell count; if None, auto-detect.
    """
    if voltage <= 0:
        return 0.0

    if cells is None:
        cells = detect_cells_from_voltage(voltage)

    v_cell = voltage / float(cells)
    return _clamp(_interp_curve(v_cell), 0.0, 100.0)


@dataclass
class BatteryMonitor:
    """
    Dashboard-oriented battery monitor.

    It produces a stable percent using:
    1) Optional sag compensation (voltage + I*R)
    2) LiPo voltage curve mapping
    3) EMA smoothing (alpha)

    Notes:
    - capacity_mah is kept for compatibility; this version is primarily voltage-based.
    - If you want true coulomb counting, you need accurate current + dt + calibration.
    """
    capacity_mah: float = 2500.0
    alpha: float = 0.12
    cells: Optional[int] = None          # None = auto-detect
    internal_resistance_ohm: float = 0.10  # tune: 0.05~0.20 typical small packs/wiring
    enable_sag_compensation: bool = True

    _filtered_percent: Optional[float] = None

    def update(self, voltage: float, current_ma: float) -> float:
        """
        Update monitor using latest sensor readings.

        Args:
            voltage: pack voltage in V
            current_ma: battery current in mA (positive when discharging)

        Returns:
            filtered percent (0..100)
        """
        if voltage <= 0:
            return 0.0

        # Convert current to A for sag compensation
        current_a = float(current_ma) / 1000.0

        # Compensate for load sag: estimate "resting" voltage
        v_eff = float(voltage)
        if self.enable_sag_compensation and current_a > 0.0:
            v_eff = v_eff + current_a * float(self.internal_resistance_ohm)

        p_raw = calculate_battery_percent(v_eff, cells=self.cells)

        # EMA smoothing
        if self._filtered_percent is None:
            self._filtered_percent = p_raw
        else:
            self._filtered_percent = self.alpha * p_raw + (1.0 - self.alpha) * self._filtered_percent

        # Optional display stabilizer: don’t jump upward too quickly after sag release
        # (Keeps gauge from popping up instantly when you let go of throttle)
        # You can comment this out if you dislike it.
        if p_raw > self._filtered_percent:
            self._filtered_percent = min(self._filtered_percent + 0.8, p_raw)

        return _clamp(self._filtered_percent, 0.0, 100.0)
