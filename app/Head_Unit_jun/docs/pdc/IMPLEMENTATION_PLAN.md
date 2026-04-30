# PDC Implementation Plan

## Phase 0: Confirm Inputs

| Task | Output |
|------|--------|
| Capture `candump can0` while moving obstacle | sample logs for 150/100/50/20 cm |
| Confirm PDC CAN ID and endian/scale | update `CAN_SIGNAL_CONTRACT.md` |
| Confirm speed source | choose VSOMEIP speed or direct CAN |
| Confirm beep hardware | Qt audio, ALSA, or GPIO buzzer decision |

## Phase 1: UI-First Mock

Goal: prove the rear camera overlay before real CAN is ready.

Status: implemented in first pass. `PdcTypes`, `IPdcSensorProvider`, `MockPdcSensorProvider`, `PdcController`, `PdcOverlayPainter`, and `ReverseCameraWindow::setPdcState()` were added. Local `hu_shell` build passes.

Files to add:

| File | Purpose |
|------|---------|
| `core/interfaces/IPdcSensorProvider.h` | provider signal contract |
| `core/models/PdcTypes.h` | `PdcState`, `PdcSensorReading`, `PdcWarningLevel` |
| `core/ipc/MockPdcSensorProvider.h/.cpp` | fake distance pattern |
| `shell/PdcController.h/.cpp` | state calculation and stale timeout |
| `shell/widgets/PdcOverlayPainter.h/.cpp` | draw guide lines and sectors |

Files to modify:

| File | Change |
|------|--------|
| `shell/widgets/ReverseCameraWindow.h/.cpp` | add `setPdcState()` and call overlay painter in `paintEvent()` |
| `shell/ShellWindow.h/.cpp` | own `PdcController`, connect it to camera while gear is `R` |
| `core/CMakeLists.txt`, `shell/CMakeLists.txt` | add new sources |

## Phase 2: SocketCAN Provider

Goal: replace mock values with real ultrasonic sensor CAN data.

Status: implemented in first pass with two decode paths:

- observed live frame `0x123`: B1 is treated as the current hardware's single `rear_center` distance.
- proposed future frame `0x350`: four little-endian uint16 distances map to RL/RML/RMR/RR.

Files to add:

| File | Purpose |
|------|---------|
| `core/ipc/SocketCanPdcProvider.h/.cpp` | open `can0`, read frames with `QSocketNotifier`, decode distances |

Implementation notes:

- Reuse the SocketCAN style from `instrument_cluster/src/serial/SerialReader.cpp`.
- Keep decode constants in one namespace.
- Emit invalid sensor readings instead of inventing fake distance.
- Log unknown/fault states once per transition to avoid flooding.

## Phase 3: Audible Warning

Goal: add beep feedback with clean shutdown behavior.

Status: first pass implemented with `PdcBeepController` and `QApplication::beep()`. Hardware-specific buzzer/ALSA output can replace this later without changing PDC state calculation.

Files to add:

| File | Purpose |
|------|---------|
| `shell/PdcBeepController.h/.cpp` | map warning level to timer interval and sound output |

Rules:

- Beep only when gear is `R`.
- Beep only when PDC state is not stale.
- Beep can be disabled by config for quiet demos.
- Critical warning stops immediately when state becomes safe.

## Phase 4: Config and Yocto

Goal: make thresholds and provider mode configurable on target image.

Config proposal:

```json
{
  "pdc": {
    "provider": "socketcan",
    "canInterface": "can0",
    "enabledInReverseOnly": true,
    "maxActiveSpeedKmh": 10.0,
    "staleTimeoutMs": 300,
    "thresholdsCm": {
      "near": 120,
      "caution": 60,
      "critical": 30
    },
    "beepEnabled": true,
    "debugOverlay": false
  }
}
```

Yocto tasks:

| Task | File |
|------|------|
| Ensure `can-utils` available for validation | `yocto/build-rpi/conf/local.conf` or image recipe |
| Ensure CAN interface brought up | systemd/network config or boot script |
| Install PDC config | `yocto/meta-piracer/recipes-hu/headunit/` |

## Phase 5: Calibration

| Calibration | Method |
|-------------|--------|
| Distance thresholds | place obstacle at known distances and compare UI level |
| Overlay geometry | align guide lines with rear camera FOV |
| Sensor sector mapping | block one sensor at a time and verify correct screen sector |
| Beep cadence | driver feedback: too noisy vs too late |

## Proposed Commit Order

1. Add PDC docs and signal contract.
2. Add PDC types, mock provider, controller unit behavior.
3. Add overlay painter and connect to `ReverseCameraWindow`.
4. Add Shell integration behind mock/config flag.
5. Add SocketCAN provider.
6. Add beep controller.
7. Add Yocto/config integration.
8. Run hardware verification and tune thresholds.
