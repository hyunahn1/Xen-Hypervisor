# PiRacer Head Unit

> An automotive-grade infotainment system built with Qt6, running on Raspberry Pi 4 under Yocto Linux.
> Designed for the SEA:ME (Software Engineering for Automotive & Mobility Engineers) program.

---

## Overview

The Head Unit is the driver-facing touchscreen application in a dual-display PiRacer system. It provides media playback, ambient LED lighting control, gear selection, and system settings through a 1280×720 fixed-resolution interface — all communicating with an Instrument Cluster process over VSOMEIP peer-to-peer IPC.

The project prioritizes **real automotive HMI patterns**: hardware-abstracted IPC/LED layers with mock implementations for rapid development, Qt6/Qt5 dual-compatibility, graceful multimedia degradation, and a polished dark-themed UI designed to look credible on a real dashboard.

---

## Features

| Category | Feature |
|----------|---------|
| **Media** | MP3/WAV/FLAC/M4A playback · Folder-based playlist · Embedded cover art extraction · Album-art-driven dynamic UI theming |
| **Lighting** | 5 presets (Night, Day, Relax, Sport, Focus) · Custom color swatches · Brightness slider · Full-window ambient glow overlay |
| **Gear** | Touch-based P/R/N/D selection · Custom 270° arc ring display · Last-Write-Wins conflict resolution across input sources |
| **Settings** | Speed unit toggle (km/h / mph) · Build info / IPC status panel |
| **System** | Car-style boot splash animation · Tab slide transitions · Real-time speed/gear/IPC status bar · Cross-tab gear panel |

---

## Architecture

```
┌─────────────────────────── MainWindow (1280×720) ────────────────────────────┐
│  ┌─────────────────────────── TabBar (48px) ─────────────────────────────┐   │
│  │  [Media]          [Lighting]          [Gear]          [Settings]      │   │
│  └────────────────────────────────────────────────────────────────────────┘   │
│                                                                               │
│  ┌─────────────────── QStackedWidget (636px) ────────────────────────────┐   │
│  │                                                                        │   │
│  │   MediaScreen  │  AmbientScreen  │  GearScreen  │  SettingsScreen     │   │
│  │   ───────────  │  ─────────────  │  ──────────  │  ──────────────     │   │
│  │   MediaPlayer  │  ILedController │  GearRing    │  Speed unit toggle  │   │
│  │   Playlist     │  GlowOverlay ◄──┼──signal      │  About info panel   │   │
│  │   CoverArt     │  Presets/Swatches│  P/R/N/D     │                     │   │
│  │                │                 │  PRND buttons│                     │   │
│  │   [GearPanel]  │  [GearPanel]    │              │  [GearPanel]        │   │
│  └────────────────────────────────────────────────────────────────────────┘   │
│                                                                               │
│  ┌─────────────────────── StatusBar (36px) ──────────────────────────────┐   │
│  │  ◉ 12.5 km/h          [Gear: D]          ◉ IPC Connected             │   │
│  └────────────────────────────────────────────────────────────────────────┘   │
│                                                                               │
│  ░░░░░░░░░░░░░ GlowOverlay (transparent, top/bottom gradient) ░░░░░░░░░░░░   │
│  ▓▓▓▓▓▓▓▓▓▓▓▓ SplashScreen (boot only, self-destructs) ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓   │
└───────────────────────────────────────────────────────────────────────────────┘

       ▲ GearStateManager (signals: gearChanged)
       │         ▲ IVehicleDataProvider (Mock / VSOMEIP)
       │         │         ▲ ILedController (Mock / PWM / WS2812)
       │         │         │
   GearPanel  StatusBar  AmbientScreen
   GearScreen
       │
   GearStateManager ──► VSomeIPClient ──► Instrument Cluster (VSOMEIP)
                                          Service 0x1234, Instance 0x0001
                                          Events: speed(0x8001), gear(0x8002), battery(0x8003)
```

### Layer Breakdown

```
src/
├── MainWindow             → Root widget, layout, animation orchestration
├── screens/               → 4-tab content (one class per tab)
├── widgets/               → Reusable UI components (TabBar, StatusBar, GearPanel, GlowOverlay, SplashScreen)
├── ipc/                   → Vehicle data & gear state (interface + mock + VSOMEIP stub)
├── media/                 → Audio playback & playlist (Qt Multimedia wrapper)
└── led/                   → LED hardware abstraction (interface + mock)
```

---

## Technical Highlights

### 1. AVFoundation Playback Retry Logic

Qt6's AVFoundation backend on macOS introduces a notorious timing race: `QMediaPlayer::play()` may return successfully but leave the media clock frozen at position 0 ms. The backend reports `PlayingState` while the decoder has not actually started producing audio frames. This is not a Qt bug but an AVFoundation async pipeline characteristic.

**Solution** — a multi-layered safety net in `MediaPlayer::play()`:

```cpp
// Immediate attempt
m_playOnLoad = true;
m_qPlayer->play();

// Safety net 1: durationChanged(>0) is the most reliable "media is ready" indicator
connect(m_qPlayer, &QMediaPlayer::durationChanged, this, [this](qint64 dur) {
    if (dur > 0 && m_playOnLoad) m_qPlayer->play();
});

// Safety net 2: LoadedMedia / BufferedMedia status transitions
connect(m_qPlayer, &QMediaPlayer::mediaStatusChanged, this, [this](auto status) {
    if ((status == LoadedMedia || status == BufferedMedia) && m_playOnLoad)
        m_qPlayer->play();
});

// Safety net 3: Timed retries — nudge decoder clock if still at 0 ms after 300 ms
for (const int delay : {120, 300, 600, 1000}) {
    QTimer::singleShot(delay, this, [this, delay] {
        if (!m_playOnLoad || m_qPlayer->position() > 0) { m_playOnLoad = false; return; }
        if (delay >= 300) {
            m_qPlayer->setPosition(1); // 1 ms nudge to unlock decoder
            if (m_qPlayer->playbackState() == PlayingState) m_qPlayer->pause();
        }
        m_qPlayer->play();
    });
}

// Confirmation: position advancing means playback is real
connect(m_qPlayer, &QMediaPlayer::positionChanged, this, [this](qint64 pos) {
    if (pos > 0 && m_playOnLoad) m_playOnLoad = false;
});
```

`isPlaying()` also guards against the false-positive: it returns `false` if `m_playOnLoad && position() == 0`, preventing the UI play/pause button from toggling into an incorrect state.

---

### 2. Album Art → Dynamic UI Color Extraction

When a track is loaded, its embedded cover art is extracted from `QMediaMetaData::CoverArtImage`. The dominant vibrant color is then computed and applied to the play button, progress bar fill, and playlist selection highlight — making every track feel distinct.

**Algorithm** (`MediaScreen::extractVibrantColor`):

```
1. Downsample pixmap to 64×64 (speed: 4096 pixels is enough for palette analysis)
2. For each pixel, convert to HSV
3. Reject: v < 40 (near-black) or v > 220 (near-white) — UI backgrounds don't work as accents
4. Reject: s < 40 (desaturated grays and neutrals)
5. Weight each accepted pixel by saturation (s/255.0) so vivid hues dominate dull ones
6. Compute weighted RGB average
7. Clamp to readable range: saturation ≥ 160, value ≥ 190 (always vibrant, always visible)
```

When no embedded art is present (common with MP3 files lacking APIC frames), a deterministic gradient placeholder is generated from the track title:

```cpp
// Same title → same hue every time (qHash with fixed seed)
QColor colorFromTitle(const QString &title) {
    return QColor::fromHsv(int(qHash(title, 42u) % 360), 200, 220);
}
```

This ensures color sync works for every track regardless of metadata quality.

---

### 3. Screen Slide Transition

`QStackedWidget` provides no built-in animation for page switches. The transition is implemented by capturing pixmap snapshots of the outgoing and incoming pages, overlaying them as `QLabel` children of the central widget, and running a `QParallelAnimationGroup` to slide both simultaneously.

```cpp
// 1. Capture outgoing page before switch
QPixmap oldSnap = m_contentStack->grab();

// 2. Switch underlying stack (new page renders underneath)
m_contentStack->setCurrentIndex(index);

// 3. Capture incoming page
QPixmap newSnap = m_contentStack->grab();

// 4. Overlay both as floating labels
auto *outLabel = new QLabel(central);
outLabel->setPixmap(oldSnap);   outLabel->setGeometry(stackRect); outLabel->raise();

auto *inLabel  = new QLabel(central);
inLabel->setPixmap(newSnap);    inLabel->setGeometry(forward ? offRight : offLeft); inLabel->raise();

// 5. Animate in parallel (300 ms, OutCubic easing)
QParallelAnimationGroup: outLabel → offLeft (or offRight), inLabel → stackRect
```

An `m_animating` flag prevents stacking multiple transitions when the user taps tabs quickly. The `GlowOverlay` is re-raised after every animation to remain on top of the content.

---

### 4. Boot Splash Animation (SplashScreen)

A full-window overlay with three chained `QVariantAnimation` phases, all driven from a single `runPhase()` method:

| Phase | Duration | Easing | Effect |
|-------|----------|--------|--------|
| 0 — Logo reveal | 700 ms | `OutQuart` | Logo fades in (0→1 opacity) with 18px upward drift |
| 1 — Progress bar | 1600 ms | Linear | Bar fills left-to-right with glowing teal leading edge |
| 2 — Fade out | 550 ms | `InCubic` | Entire overlay fades to transparent, revealing main UI |

`WA_TranslucentBackground` enables true compositing transparency during phase 2 — the parent's content shows through as the overlay alpha decreases, producing a clean reveal without a jarring cut.

---

### 5. Ambient Glow Overlay

`GlowOverlay` sits permanently at the top of the Z-order, covering the entire 1280×720 surface. It carries `WA_TransparentForMouseEvents` so all touch/click events pass through to the content below.

```cpp
// paintEvent draws two linear gradients — top strip and bottom strip
QLinearGradient topGrad(0, 0, 0, glowHeight);
topGrad.setColorAt(0, QColor(r, g, b, alpha)); // full alpha at edge
topGrad.setColorAt(1, QColor(r, g, b, 0));     // transparent at center
```

The alpha value is proportional to the user-selected brightness (`alpha = 85 × brightness / 100`), capping at 85 to remain a subtle frame effect rather than washing out the content.

---

### 6. Hardware Abstraction via Interfaces

Both the vehicle data source and the LED controller are accessed exclusively through pure abstract interfaces:

```cpp
// Strategy pattern — swap implementations without touching screens
class IVehicleDataProvider : public QObject { ... }; // signals: speedChanged, gearChanged, batteryChanged
class ILedController       : public QObject { ... }; // setColor, setBrightness, setPreset, off

// Development builds
IVehicleDataProvider* provider = new MockVehicleDataProvider(this); // simulates sin-wave speed
ILedController*       leds     = new MockLedController(this);       // qDebug output only

// Production builds (Raspberry Pi + Yocto)
IVehicleDataProvider* provider = new VSomeIPClient(this);           // VSOMEIP event subscriber
ILedController*       leds     = new WS2812LedController(this);     // real hardware
```

Swapping from mock to production requires changing two lines in `MainWindow.cpp`. All screens are completely unaware of which implementation is active.

---

### 7. Gear State Conflict Resolution

Three input sources can request gear changes simultaneously: capacitive touch (GearPanel / GearScreen), throttle events from the Instrument Cluster, and physical buttons. `GearStateManager` applies a **Last-Write-Wins** policy — the most recent input always wins, with no priority ordering. Each change carries a source tag (`"touch"`, `"throttle"`, `"button"`) displayed in the GearScreen's status label.

```cpp
void GearStateManager::setGear(GearState gear, const QString &source) {
    m_gear       = gear;
    m_lastSource = source;
    emit gearChanged(m_gear, m_lastSource); // all subscribers update simultaneously
}
```

---

### 8. Qt5 / Qt6 Dual Compatibility

The codebase conditionally compiles for both Qt versions, maintaining a single source tree for development (macOS/Qt6) and production (Yocto/Qt5 or Qt6):

```cpp
#if QT_VERSION_MAJOR >= 6
    m_audioOut = new QAudioOutput(this);          // Qt6: separate audio output object
    m_qPlayer->setAudioOutput(m_audioOut);
    m_audioOut->setVolume(0.5f);

    connect(m_qPlayer, &QMediaPlayer::playbackStateChanged, ...); // Qt6 API
    QMediaMetaData::CoverArtImage                 // Qt6 metadata API
#else
    m_qPlayer->setVolume(50);                     // Qt5: volume on player directly
    connect(m_qPlayer, &QMediaPlayer::stateChanged, ...);         // Qt5 API
    // Cover art not available in Qt5 QMediaPlayer
#endif
```

A CMake `find_package(Qt Multimedia OPTIONAL)` gate controls whether `HU_MULTIMEDIA_AVAILABLE` is defined, enabling fully stub-based UI development on machines without Qt Multimedia installed.

---

## Qt Modules & Libraries

| Module | Purpose | Key Classes Used |
|--------|---------|-----------------|
| `Qt::Core` | Foundation | `QObject`, `QTimer`, `QVariant`, `QUrl`, `QAbstractAnimation`, `QVariantAnimation`, `QPropertyAnimation`, `QParallelAnimationGroup`, `qHash` |
| `Qt::Widgets` | UI Framework | `QMainWindow`, `QStackedWidget`, `QLabel`, `QPushButton`, `QSlider`, `QListView`, `QAbstractListModel`, `QFrame`, `QFileDialog` |
| `Qt::Widgets` (painting) | Custom rendering | `QPainter`, `QLinearGradient`, `QRadialGradient`, `QPixmap`, `QImage`, `QFont`, `QEasingCurve` |
| `Qt::Multimedia` (optional) | Audio playback | `QMediaPlayer`, `QAudioOutput` (Qt6), `QMediaMetaData` (Qt6), `QMediaContent` (Qt5) |

**No third-party dependencies.** The project is self-contained within Qt, making it straightforward to cross-compile for Yocto Linux.

---

## Design Patterns

| Pattern | Where Applied | Why |
|---------|--------------|-----|
| **Observer** (Qt signals/slots) | Throughout all layers | Decoupled updates — screens never poll, they react |
| **Strategy** | `IVehicleDataProvider`, `ILedController` | Swap mock ↔ real hardware with zero screen-level changes |
| **Adapter** | `MockVehicleDataProvider`, `MockLedController` | Isolate hardware dependencies during development |
| **Model-View** | `PlaylistModel` (QAbstractListModel) + `QListView` | Standard Qt separation of data from display |
| **Template Method** | `SplashScreen::runPhase()` | Defines animation skeleton, phase implementations vary |
| **Composite** | `QParallelAnimationGroup` in slide transition | Two independent animations composed into one synchronized group |

---

## Project Structure

```
Head_Unit/
├── CMakeLists.txt
├── src/
│   ├── main.cpp
│   ├── MainWindow.h / .cpp           — Root window, tab orchestration, slide animation
│   ├── widgets/
│   │   ├── TabBar.h / .cpp           — 4-tab navigation with active underline indicator
│   │   ├── StatusBar.h / .cpp        — Speed · Gear · IPC connection status
│   │   ├── GearPanel.h / .cpp        — Compact vertical gear selector (shared across tabs)
│   │   ├── GlowOverlay.h / .cpp      — Full-window transparent ambient glow
│   │   └── SplashScreen.h / .cpp     — 3-phase boot animation overlay
│   ├── screens/
│   │   ├── MediaScreen.h / .cpp      — Media playback, playlist, cover art, color sync
│   │   ├── AmbientScreen.h / .cpp    — LED color presets, brightness, glow control
│   │   ├── GearScreen.h / .cpp       — Arc ring display, P/R/N/D touch input
│   │   └── SettingsScreen.h / .cpp   — Speed unit, about/build info
│   ├── ipc/
│   │   ├── IVehicleDataProvider.h    — Abstract interface (speed, gear, battery signals)
│   │   ├── MockVehicleDataProvider   — Sinusoidal speed simulation, battery decay
│   │   ├── GearStateManager          — Gear state hub, Last-Write-Wins arbitration
│   │   └── VSomeIPClient             — VSOMEIP subscriber/publisher (stub, TODO)
│   ├── media/
│   │   ├── MediaPlayer.h / .cpp      — QMediaPlayer wrapper + AVFoundation retry logic
│   │   └── PlaylistModel.h / .cpp    — QAbstractListModel for audio file list
│   └── led/
│       ├── ILedController.h          — Abstract interface (setColor, setBrightness, off)
│       └── MockLedController         — qDebug stub for development
├── config/
│   └── vsomeip_headunit.json         — VSOMEIP service/instance/event configuration
├── resources/
│   └── headunit.qrc
└── docs/
    ├── ARCHITECTURE_DESIGN.md
    ├── HEAD_UNIT_DESIGN.md
    └── VERIFICATION.md
```

---

## Build & Run

### Prerequisites

- Qt 6.2+ (Qt 5.15 also supported)
- CMake 3.16+
- C++17 compiler (GCC 10+, Clang 12+, MSVC 2019+)
- Qt Multimedia (optional — falls back to UI-only stub mode)

### macOS / Linux (Development)

```bash
git clone <repo>
cd Head_Unit

cmake -S . -B build
cmake --build build --parallel

# macOS
open build/head_unit.app

# Linux
./build/head_unit
```

### Raspberry Pi 4 / Yocto (Target)

```bash
# Cross-compile via Yocto SDK
source /opt/poky/<version>/environment-setup-cortexa72-poky-linux
cmake -S . -B build-rpi -DCMAKE_TOOLCHAIN_FILE=$OECORE_NATIVE_SYSROOT/...
cmake --build build-rpi --parallel

# Deploy via scp
scp build-rpi/head_unit pi@piracer:/opt/headunit/
```

```bash
# systemd service on target (DISPLAY=:1 for HDMI output)
[Unit]
Description=PiRacer Head Unit
After=network.target vsomeipd.service

[Service]
Environment=DISPLAY=:1
ExecStart=/opt/headunit/head_unit
Restart=on-failure
```

### CMake Feature Flags

| Flag | Effect |
|------|--------|
| `HU_MULTIMEDIA_AVAILABLE` (auto-detected) | Real Qt Multimedia audio playback |
| _(not defined)_ | Stub mode — full UI without audio |

---

## Known Technical Challenges

### AVFoundation Backend (macOS / Qt6)
Qt6's AVFoundation backend initializes media asynchronously. Calling `play()` immediately after `setSource()` succeeds at the API level but may leave the clock frozen at 0 ms. The retry mechanism in `MediaPlayer::play()` handles this with a combination of signal-driven retries and timed position nudges. This is not required on GStreamer (Linux) or Windows Media Foundation.

### VSOMEIP Service Discovery Timing
In the production VSOMEIP integration (not yet implemented), the Head Unit must handle the case where the Instrument Cluster process starts before VSOMEIP service discovery completes. The `VSomeIPClient` stub is structured to emit `connectionStatusChanged(false)` initially, retrying `find_service()` on a timer until the cluster is reachable.

### Yocto Multi-Display Configuration
The dual-display setup (Instrument Cluster on DSI, Head Unit on HDMI) requires specific `/boot/config.txt` overlays and systemd service ordering to ensure each process gets the correct `DISPLAY` environment variable. Both processes must start after `vsomeipd.service`.

---

## VSOMEIP IPC Specification

The Head Unit and Instrument Cluster communicate over a local UDP socket using VSOMEIP service discovery:

| Parameter | Value |
|-----------|-------|
| Service ID | `0x1234` |
| Instance ID | `0x0001` |
| Speed event | `0x8001` — `float32`, 100 ms periodic |
| Gear event | `0x8002` — `uint8` (0=P, 1=R, 2=N, 3=D), change-triggered |
| Battery event | `0x8003` — `float32 voltage + float32 percent`, 500 ms periodic |

The Head Unit **subscribes** to all three events and **publishes** `0x8002` when the user selects a gear via touch, enabling the Instrument Cluster to reflect the driver's gear selection.

---

## Roadmap

- [x] Complete 4-tab HMI with all screens
- [x] Qt Multimedia integration with AVFoundation retry logic
- [x] Album art color extraction and dynamic theming
- [x] Slide transition animations (300ms OutCubic)
- [x] Boot splash animation (3-phase, ~2.9s)
- [x] Full-window ambient glow overlay
- [x] GearStateManager with Last-Write-Wins arbitration
- [x] Mock implementations for hardware-free development
- [ ] VSomeIPClient — subscribe speed/gear/battery, publish gear touch events
- [ ] Real LED hardware driver (PWM GPIO or WS2812 via SPI)
- [ ] Yocto meta-layer recipe and systemd service files
- [ ] Gamepad Python service (VSOMEIP gear publisher via physical controller)
- [ ] CAN bus integration for real vehicle speed data

---

## Author

**Ahn Hyunjun**
SEA:ME Program — DES Project, Cohort 2026
Automotive Embedded Software Engineering

---

*Built with Qt 6 · C++17 · CMake · VSOMEIP · Yocto Linux*
