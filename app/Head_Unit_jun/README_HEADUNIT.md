# PiRacer Head Unit

Qt application for PiRacer infotainment. Architecture: [docs/ARCHITECTURE_DESIGN.md](docs/ARCHITECTURE_DESIGN.md)

## Structure (scaffold)

```
Head_Unit/
├── headunit.pro              # Qt project
├── src/
│   ├── main.cpp
│   ├── MainWindow.*
│   ├── widgets/ TabBar, StatusBar
│   ├── screens/ MediaScreen, AmbientScreen, GearScreen, SettingsScreen
│   ├── ipc/ IVehicleDataProvider, MockVehicleDataProvider, GearStateManager, VSomeIPClient (TODO)
│   ├── media/ MediaPlayer, PlaylistModel (stubs)
│   └── led/ ILedController, MockLedController
├── config/ vsomeip_headunit.json
├── resources/ headunit.qrc
└── python/ gamepad_service.py (stub)
```

## Build (CMake)

```bash
mkdir build && cd build
cmake ..
make
./head_unit
```

Or open `CMakeLists.txt` in Qt Creator.

## Implemented

- 4-tab layout (Media, Lighting, Gear, Settings)
- GearScreen: P/R/N/D touch buttons, Last-Write-Wins
- StatusBar: speed, gear, IPC status (Mock data)
- GearStateManager

## TODO (for Sonnet/GPT)

1. **VSomeIPClient** – Replace MockVehicleDataProvider with VSOMEIP
2. **MediaScreen** – Now Playing card, QMediaPlayer, playlist
3. **AmbientScreen** – Color picker, presets (after LED hardware)
4. **gamepad_service.py** – VSOMEIP gear publish
5. **Yocto** – Recipe, systemd services
