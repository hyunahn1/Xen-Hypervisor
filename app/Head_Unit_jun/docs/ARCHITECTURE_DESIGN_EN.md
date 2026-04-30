# PiRacer Head Unit - Architecture Design Document

## 📋 Document Information
- **Version**: 2.0.0
- **Date**: 2026-02-20
- **Status**: Updated (Critical Evaluation Round 0 reflected)
- **Change History**: See Section 10 Revisions

---

## 1. Requirements Traceability

### 1.1 Project Requirements → Architecture Mapping

| ID | Requirement (HeadUnit.md) | Architecture Response | Verification |
|----|---------------------------|------------------------|--------------|
| REQ-01 | Head Unit Qt app, RPi, Instrument Cluster co-execution | Separate processes, multi-display, systemd auto-start | VER-01 |
| REQ-02 | Ambient lighting control | Head Unit → ILedController → LED hardware | VER-02 |
| REQ-03 | Gear selection display | GearScreen widget, VSOMEIP gear event subscription | VER-03 |
| REQ-04 | Media app (audio/video playback/control) | MediaScreen, Qt Multimedia (QMediaPlayer) | VER-04 |
| REQ-05 | IPC (Cluster ↔ Head Unit) | VSOMEIP peer-to-peer (Service Discovery) | VER-05 |
| REQ-06 | CAN (speed sensor data) | Arduino→CAN→Cluster (CAN Reader), VSOMEIP to HU | VER-06 |
| REQ-07 | Yocto build system | Yocto meta-layer, recipe creation | VER-07 |
| REQ-08 | Gear touch control (user decision) | GearScreen P/R/N/D buttons → GearStateManager → VSOMEIP publish | VER-08 |
| REQ-09 | Gear gamepad control | Gamepad Python service → GearStateManager → VSOMEIP publish | VER-09 |

### 1.2 Constraints

| ID | Constraint | Impact |
|----|------------|--------|
| C-01 | PiRacer gear = throttle sign (gamepad analog) | Touch gear → no direct motor control; gear state/ intent display only (§2.4 detail) |
| C-02 | Two displays (Cluster DSI 1280×400, Head Unit HDMI) | RPi DSI + HDMI simultaneous output config required |
| C-03 | Speed = Arduino→CAN→Cluster | Cluster owns CAN socket; HU receives via VSOMEIP |
| C-04 | Ambient LED hardware unconfirmed | Lighting module interface abstracted; to be detailed after hardware receipt |
| C-05 | VSOMEIP = peer-to-peer | No central broker; each app needs vsomeip.json config |

---

## 2. System Architecture

### 2.1 Top-Level Diagram (VSOMEIP peer-to-peer)

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                         Raspberry Pi (Yocto Linux)                              │
│                                                                                 │
│  ┌──────────────┐  CAN (socketcan)                                              │
│  │ Arduino      │──────────────────────────────────────────────────────────┐   │
│  │ Speed Sensor │                                                           │   │
│  └──────────────┘                                                           │   │
│                                                                             ↓   │
│  ┌──────────────────────────────────┐       ┌──────────────────────────────┐   │
│  │   Instrument Cluster (Qt)        │       │   Head Unit (Qt)             │   │
│  │  ┌──────────────────────────┐    │       │   ┌─────────────────────┐    │   │
│  │  │ CANReader                │    │       │   │ VSomeIPClient       │    │   │
│  │  │ socketcan read           │    │       │   │ subscribe:          │    │   │
│  │  └────────────┬─────────────┘    │       │   │  0x8001 speed       │    │   │
│  │               │ speed, gear      │       │   │  0x8002 gear        │    │   │
│  │  ┌────────────▼─────────────┐    │       │   │  0x8003 battery     │    │   │
│  │  │ GearStateManager         │    │       │   └────────┬────────────┘    │   │
│  │  │ (throttle-based F/R/P)   │    │       │            │                 │   │
│  │  └────────────┬─────────────┘    │       │   ┌────────▼────────────┐    │   │
│  │               │                  │       │   │ GearStateManager    │    │   │
│  │  ┌────────────▼─────────────┐    │       │   │ (touch gear intent) │    │   │
│  │  │ VSomeIPService           │    │       │   └────────┬────────────┘    │   │
│  │  │ offer:                   │◄───┼───────┼───────────►│ publish 0x8002  │   │
│  │  │  0x8001 speed            │    │       │            │                 │   │
│  │  │  0x8002 gear             │    │       │   ┌────────▼────────────┐    │   │
│  │  │  0x8003 battery          │    │       │   │ Screens (4 Tabs)    │    │   │
│  │  └──────────────────────────┘    │       │   │  MediaScreen        │    │   │
│  │                                  │       │   │  GearScreen         │    │   │
│  │  [Display 1: DSI 1280×400]       │       │   │  AmbientScreen      │    │   │
│  └──────────────────────────────────┘       │   │  SettingsScreen     │    │   │
│                                             │   └─────────────────────┘    │   │
│  ┌──────────────┐                           │                              │   │
│  │ Gamepad      │  Bluetooth/USB            │   [Display 2: HDMI]          │   │
│  │ Python svc   │──────────────────────────►│   ILedController → LED HW    │   │
│  └──────────────┘  gear intent delivery     └──────────────────────────────┘   │
│                                                                                 │
│  VSOMEIP Service Discovery: UDP Multicast (lo, 127.255.255.255)                │
│  ──────────────────────────────────────────────────────────────────────────── │
│  Cluster: offer 0x1234/0x0001   ←→   Head Unit: find 0x1234/0x0001           │
└─────────────────────────────────────────────────────────────────────────────────┘
```

### 2.2 Component Definition

| Component | Responsibility | Interface |
|-----------|----------------|-----------|
| **Instrument Cluster** | CAN speed rx, GearStateManager (throttle), VSOMEIP offer | CAN (socketcan), VSOMEIP |
| **Head Unit** | 4-tab UI, media, lighting, touch gear, VSOMEIP subscribe/publish | VSOMEIP, ILedController |
| **Gamepad Python service** | Gamepad input, throttle→PiRacer, gear intent → Head Unit | Python piracer-py, (VSOMEIP or stdin) |
| **GearStateManager** | Gear state storage/propagation, Last-Write-Wins | Internal class |
| **ILedController** | LED hardware abstraction | GPIO/PWM/WS2812 (swappable impl) |
| **VSomeIPService (Cluster)** | speed/gear/battery event offer | VSOMEIP |
| **VSomeIPClient (HU)** | speed/gear/battery event subscribe | VSOMEIP |

### 2.3 Data Flow

| Data | Source | Path | Destination |
|------|--------|------|-------------|
| Speed | Arduino | CAN → Cluster CANReader | Cluster UI, VSOMEIP 0x8001 → HU StatusBar |
| Gear (throttle auto) | Gamepad analog | Python svc → PiRacer throttle → GearStateManager | Cluster UI, VSOMEIP 0x8002 → HU GearScreen |
| Gear (touch) | HU GearScreen | GearStateManager → VSOMEIP publish 0x8002 | Cluster receive → display |
| Gear (gamepad button) | Gamepad button | Python svc → VSOMEIP publish 0x8002 | Cluster·HU receive |
| Battery | PiRacer Python bridge | Python → Cluster | Cluster UI, VSOMEIP 0x8003 → HU (optional) |
| LED command | HU AmbientScreen | ILedController | LED hardware |

### 2.4 Gear Control Detail (Constraint C-01)

#### Practical Constraint
PiRacer gear (F/R) is determined by `piracer.set_throttle_percent(value)` sign.
- Positive → Forward (F), Negative → Reverse (R), Zero → Stop
- Gamepad analog stick already directly controls throttle; **touch gear selection directly controlling motor would conflict** with gamepad.

#### Design Decision: Gear "Intent + Display" Separation

```
[Touch gear select R]
       │
       ▼
GearStateManager (HU)
 - gearIntent = "R"
 - VSOMEIP publish gear = "R"
       │
       ▼
Cluster GearScreen: "R" display    (display role)
HU GearScreen: "R" display         (display role)
       │
       ▼
Python Gamepad service:
 - subscribe gear event
 - gearIntent == "R" → force throttle sign negative
 - gearIntent == "P" or "N" → keep throttle = 0
 - gearIntent == "D" → throttle sign positive
```

**Motor control is always handled by the Python service**; Head Unit conveys gear intent via VSOMEIP and the Python service applies it.

#### P / N Gear Behavior

| Gear | Display condition | Motor behavior |
|------|-------------------|----------------|
| **P (Parking)** | Speed ≤ 0.15 km/h auto (same as Cluster), or touch select | throttle = 0 enforced, gamepad throttle ignored |
| **N (Neutral)** | Touch selectable | throttle = 0 (same as P) |
| **D (Drive)** | Touch select or throttle positive | Allow positive throttle |
| **R (Reverse)** | Touch select or throttle negative | Allow negative throttle |

> **Note**: N behaves like P in motor terms but is shown separately as "ready to move" by name/color.

#### Dual Input Conflict (Last-Write-Wins)

```
Touch input: T=100ms → gear = "R"
Gamepad input: T=150ms → gear = "D"
Result: gear = "D" (150ms is last)
```

The most recent event wins when both Python service and HU publish.

---

## 3. Design Decisions & Rationale

### 3.1 ADR (Architecture Decision Record)

| ADR-ID | Decision | Alternative | Rationale |
|--------|----------|-------------|-----------|
| ADR-01 | IPC = VSOMEIP (peer-to-peer) | D-Bus, Socket | Project requirement, automotive standard |
| ADR-02 | Dual gear input (gamepad + touch) | Single source | User requirement |
| ADR-03 | Last-Write-Wins gear conflict | Priority rules | Simplicity, respects latest intent |
| ADR-04 | Cluster owns CAN socket | HU direct CAN | Single-socket principle, reuse Cluster |
| ADR-05 | Head Unit separate display (HDMI) | Split screen | Independent run, resolution freedom |
| ADR-06 | 4-tab top navigation | Bottom/side | Touch accessibility, infotainment convention |
| ADR-07 | LED control interface abstraction | Direct GPIO | Hardware unconfirmed, easy impl swap |
| ADR-08 | Gear touch = intent relay; motor control in Python | HU direct motor | Avoid PiRacer throttle conflict, separation of concerns |
| ADR-09 | N gear = same motor behavior as P | Separate behavior | PiRacer API limitation (throttle sign only) |
| ADR-10 | Speed HU display = StatusBar (compact) | Separate tab | Meets requirement, saves space |
| ADR-11 | Gamepad = separate Python service | Cluster internal | Gear intent VSOMEIP publish, Cluster independence |

### 3.2 Risks & Mitigation

| Risk | Impact | Mitigation |
|------|--------|------------|
| VSOMEIP learning curve | Schedule delay | Example/tutorial first, D-Bus fallback |
| Gear throttle/touch simultaneous input | Confusion | Last-Write-Wins, Python svc gear intent subscribe |
| LED hardware diversity | Interface change | ILedController abstraction, adapter pattern |
| RPi multi-display | Config complexity | /boot/config.txt settings (§8) |
| VSOMEIP Service Discovery timing | Event loss at startup | Boot sequence (§5), retry logic |

---

## 4. Interface Specification

### 4.1 VSOMEIP Service Definition

**Service ID**: `0x1234`  
**Instance ID**: `0x0001`  
**Protocol**: UDP (Service Discovery) + TCP (data)

| Event Group | Event ID | Name | Payload Type | Payload | Direction | Cycle/Trigger |
|-------------|----------|------|--------------|---------|-----------|---------------|
| 0x0001 | 0x8001 | speed | float32 (4B) | km/h | Cluster→HU | 100ms |
| 0x0001 | 0x8002 | gear | uint8 (1B) | P=0,R=1,N=2,D=3 | Bidirectional | On change |
| 0x0001 | 0x8003 | battery | {float32 V, float32 %} (8B) | V, % | Cluster→HU | 500ms |

### 4.2 vsomeip.json Configuration

#### Instrument Cluster (Service provider)
**Path**: `/etc/vsomeip/vsomeip_cluster.json`

```json
{
    "unicast": "127.0.0.1",
    "logging": { "level": "warning", "console": "false" },
    "applications": [{ "name": "InstrumentCluster", "id": "0x1001" }],
    "services": [{ "service": "0x1234", "instance": "0x0001", "reliable": { "port": "30509" }, "unreliable": "30510" }],
    "events": [
        { "service": "0x1234", "instance": "0x0001", "event": "0x8001", "is_field": false, "update-cycle": "100" },
        { "service": "0x1234", "instance": "0x0001", "event": "0x8002", "is_field": true, "update-cycle": "0" },
        { "service": "0x1234", "instance": "0x0001", "event": "0x8003", "is_field": false, "update-cycle": "500" }
    ],
    "eventgroups": [{ "service": "0x1234", "instance": "0x0001", "eventgroup": "0x0001", "events": ["0x8001","0x8002","0x8003"] }],
    "routing": "InstrumentCluster",
    "service-discovery": { "enable": "true", "multicast": "224.0.0.1", "port": "30490", "protocol": "udp" }
}
```

#### Head Unit (subscriber)
**Path**: `/etc/vsomeip/vsomeip_headunit.json`

```json
{
    "unicast": "127.0.0.1",
    "logging": { "level": "warning", "console": "false" },
    "applications": [{ "name": "HeadUnit", "id": "0x1002" }],
    "routing": "InstrumentCluster",
    "service-discovery": { "enable": "true", "multicast": "224.0.0.1", "port": "30490", "protocol": "udp" }
}
```

> **Note**: Head Unit does not offer a service; it notifies gear event (0x8002) to Cluster. Cluster also subscribes to 0x8002 for HU touch gear changes.

### 4.3 LED Control Interface (Abstract)

```cpp
// led/ILedController.h
class ILedController {
public:
    virtual ~ILedController() = default;
    virtual bool init() = 0;
    virtual void setColor(uint8_t r, uint8_t g, uint8_t b) = 0;
    virtual void setBrightness(uint8_t percent) = 0;  // 0~100
    virtual void setPreset(const QString &presetName) = 0;
    virtual void off() = 0;
};
```

### 4.4 Gear State Protocol

```cpp
enum class GearState : uint8_t { P=0, R=1, N=2, D=3 };
struct GearEvent { GearState gear; QString source; qint64 timestamp; };
```

---

## 5. Boot Sequence

### 5.1 Overall Flow

```
[Power ON] → [Yocto Kernel] → [systemd]
    ├── can-init.service (CAN up)
    ├── DSI, HDMI displays
    ├── piracer-gamepad.service (After: can-init)
    ├── instrument-cluster.service (VSOMEIP offer, Display 1)
    ├── head-unit.service (VSOMEIP find, subscribe, Display 2)
    └── [Normal operation]
```

### 5.2 systemd Service Examples

See Korean document for full examples of: can-init, instrument-cluster, head-unit, piracer-gamepad.

### 5.3 VSOMEIP Service Discovery Timing

- t=0ms: HU starts, find 0x1234/0x0001
- t=~500ms: Cluster offer complete → HU subscribe
- t=~600ms: First speed event (100ms cycle)
- **Retry**: VSomeIPClient find retry (max 10, 500ms interval)

---

## 6. Error Handling Policy

### 6.1 Error Handling by Situation

| Situation | Detection | UI | Recovery |
|-----------|-----------|-----|----------|
| VSOMEIP disconnected | subscribe timeout 5s | StatusBar "---", yellow warning | Retry every 5s |
| CAN data absent | speed event 10s missing | Speed "---", disconnect msg | No auto (HW issue) |
| VSOMEIP event loss | >100ms since last | Keep last; 3 misses → "---" | Auto when next OK |
| LED init fail | init() false | "LED not connected", disable | No retry |
| No media files | PlaylistModel empty | Empty state + "No files" | Rescan button |
| Media codec error | QMediaPlayer errorOccurred | Skip file, next | - |
| Gamepad disconnect | BT/USB event | systemd Restart | Auto restart |

### 6.2 Error Display Rules

- StatusBar: ◉ OK, ⚠ Warning, ✕ Error

### 6.3 Log Policy

- Path: `/var/log/piracer/headunit.log`
- Level: ERROR, WARN, INFO
- Rotation: 10MB, 3 files

---

## 7. Module Structure (File Layout)

```
Head_Unit/
├── docs/ (ARCHITECTURE_DESIGN, HEAD_UNIT_DESIGN, VERIFICATION, etc.)
├── src/
│   ├── main.cpp, MainWindow.*
│   ├── widgets/ TabBar.*, StatusBar.*
│   ├── screens/ MediaScreen.*, AmbientScreen.*, GearScreen.*, SettingsScreen.*
│   ├── ipc/ IVehicleDataProvider.*, VSomeIPClient.*, GearStateManager.*
│   ├── media/ MediaPlayer.*, PlaylistModel.*
│   └── led/ ILedController.*, PwmLedController.*, WS2812LedController.*
├── config/ vsomeip_headunit.json
├── resources/ headunit.qrc, icons/, fonts/
├── python/ gamepad_service.py
└── CMakeLists.txt (or headunit.pro)
```

---

## 8. Display Configuration (RPi Multi-Display)

### 8.1 /boot/config.txt

```ini
dtoverlay=vc4-kms-v3d
hdmi_force_hotplug=1
hdmi_group=1
hdmi_mode=16   # 1080p60
```

### 8.2 X11 Display Mapping

- Cluster: DISPLAY=:0 (DSI)
- Head Unit: DISPLAY=:1 (HDMI)

---

## 9. Non-Functional Requirements

| NFR | Target | Verification |
|-----|--------|--------------|
| Responsiveness | UI input < 100ms | TC-PERF-001 |
| IPC latency | VSOMEIP event < 50ms | TC-PERF-002 |
| Stability | 2h continuous | TC-STAB-001 |
| Resources | Memory < 150MB, CPU < 25% | TC-PERF-003, -004 |
| Compatibility | Yocto build/run | TC-YOCTO-001~003 |
| Error recovery | VSOMEIP reconnect < 10s | TC-STAB-002 |

---

## 10. Revisions

| Rev | Date | Changes | Critical Eval |
|-----|------|---------|---------------|
| 1.0 | 2026-02-20 | Draft | - |
| 2.0 | 2026-02-20 | VSOMEIP peer-to-peer, gear dual input, boot sequence, P/N gear, error policy, vsomeip.json, file structure, StatusBar speed | Round 0 |

---

*This document is updated periodically based on VERIFICATION and CRITICAL_EVALUATION results.*
