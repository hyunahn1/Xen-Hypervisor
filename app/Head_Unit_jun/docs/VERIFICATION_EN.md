# PiRacer Head Unit - Verification Plan

## 📋 Document Information
- **Version**: 1.0.0
- **Date**: 2026-02-20
- **Traceability**: ARCHITECTURE_DESIGN_EN.md REQ-ID → VER-ID

---

## 1. Verification Strategy

### 1.1 V-Model

```
ARCHITECTURE_DESIGN_EN.md   VERIFICATION_EN.md (this doc)
       │                            │
   [Design]  ←── criteria ──→  [Verification]
       │                            │
   [Impl]    ←── execute ──→  [Testing]
       │                            │
       └── optimization ──→  [CRITICAL_EVALUATION_EN.md]
```

### 1.2 Verification Levels

| Level | Scope | Purpose |
|-------|-------|---------|
| **VER-Unit** | Class/function | Logic correctness |
| **VER-Integ** | Module comms (IPC, LED) | Interface compliance |
| **VER-Sys** | Full system | Requirements |
| **VER-Perf** | Perf, stability | NFR |
| **VER-User** | UI/UX | Usability |

---

## 2. Requirements → Verification Mapping

| REQ-ID | Requirement | VER-ID | Method | Pass Criteria |
|--------|-------------|--------|--------|---------------|
| REQ-01 | HU Qt app, Cluster co-run | VER-01 | Boot, process check | Both apps run, no crash |
| REQ-02 | Ambient lighting | VER-02 | LED color/brightness, HW | LED responds, no error |
| REQ-03 | Gear display | VER-03 | Gear change → UI update | Within 200ms |
| REQ-04 | Media (audio/video) | VER-04 | Play/pause/volume/seek/video | All controls work |
| REQ-05 | IPC (VSOMEIP) | VER-05 | Cluster↔HU data exchange | Speed·gear sync |
| REQ-06 | CAN speed data | VER-06 | Cluster CAN rx, IPC, StatusBar | Data path OK |
| REQ-07 | Yocto | VER-07 | Image build, app run | TC-YOCTO all pass |
| REQ-08 | Gear touch | VER-08 | HU touch → gear change | Cluster reflects, Python receives |
| REQ-09 | Gear gamepad button | VER-09 | Gamepad button → gear | HU·Cluster both reflect |

---

## 3. Unit Verification

### 3.1 IPC Client (VSOMEIP)

| TC-ID | Test | Input | Expected | Pass |
|-------|------|-------|----------|------|
| TC-IPC-001 | speed event rx | Mock 15.0 km/h | callback, 15.0 | Value match |
| TC-IPC-002 | gear event rx | Mock "D" | callback, "D" | Value match |
| TC-IPC-003 | gear publish | Touch "R" | VSOMEIP publish | Sent |
| TC-IPC-004 | Disconnect | Broker stop | Retry, error handle | No crash |

### 3.2 Media Player

| TC-ID | Test | Pass |
|-------|------|------|
| TC-MED-001 | MP3 play | 5s play, no error |
| TC-MED-002 | Pause/Resume | Correct state |
| TC-MED-003 | Seek | Position reflects |
| TC-MED-004 | Volume 0–100% | Output changes |
| TC-MED-005 | Video MP4 | Video+audio, no error |
| TC-MED-006 | Video fullscreen overlay | Touch show/hide controls |
| TC-MED-007 | Video CPU | < 50% (RPi4) |
| TC-MED-008 | Unsupported format | Skip, no crash |

### 3.3 Gear Logic

| TC-ID | Test | Input | Expected | Pass |
|-------|------|------|----------|------|
| TC-GEAR-001 | Last-Write-Wins | Throttle F → Touch R | R | Touch wins |
| TC-GEAR-002 | Last-Write-Wins | Touch R → Throttle F | F | Throttle wins |
| TC-GEAR-003 | Invalid value | "X" | Keep previous | Filtered |

### 3.4 LED Controller

| TC-ID | Test | Pass |
|-------|------|------|
| TC-LED-001 | setColor(255,0,0) | Mock called |
| TC-LED-002 | setBrightness(80) | Mock called |
| TC-LED-003 | Init fail | Error return, no crash |

---

## 4. Integration Verification

### 4.1 Cluster ↔ Head Unit IPC

| TC-ID | Test | Procedure | Pass |
|-------|------|------------|------|
| TC-INT-001 | Speed sync | Cluster speed input → HU rx | Within 500ms |
| TC-INT-002 | Gear sync C→HU | Cluster gear change | HU reflects |
| TC-INT-003 | Gear sync HU→C | HU touch R | Cluster shows R |
| TC-INT-004 | Bidirectional same time | Both inputs | Last-Write-Wins, consistent |

### 4.2 CAN → Cluster → IPC

| TC-ID | Test | Pass |
|-------|------|------|
| TC-INT-005 | Arduino CAN → Cluster | Cluster speed display |
| TC-INT-006 | Cluster → IPC → HU | HU receives speed |

### 4.3 LED Control

| TC-ID | Test | Pass |
|-------|------|------|
| TC-INT-007 | HU color select → LED | LED color changes |
| TC-INT-008 | Brightness slider → LED | LED brightness changes |

---

## 5. System Verification

### 5.1 Functionality

| TC-ID | Scenario | Steps | Pass |
|-------|----------|-------|------|
| TC-SYS-001 | Media flow | File select→play→pause→volume→next | All OK |
| TC-SYS-002 | Gear flow | Throttle F → Touch R → Touch D | C·HU reflect |
| TC-SYS-003 | Lighting flow | Color select→save→preset | LED reacts, save OK |
| TC-SYS-004 | Tab switch | 4 tabs in order | Switch OK, state kept |

### 5.2 Multi-Display

| TC-ID | Test | Pass |
|-------|------|------|
| TC-SYS-005 | Cluster DSI + HU HDMI | Correct app per display |
| TC-SYS-006 | Power reboot | Both apps auto-start |

---

## 6. Performance Verification

| TC-ID | Item | Target | Method | Pass |
|-------|------|--------|--------|------|
| TC-PERF-001 | UI response | < 100ms | Touch→response | Avg < 100ms |
| TC-PERF-002 | IPC latency | < 50ms | Publish→rx | Within 50ms |
| TC-PERF-003 | CPU (HU) | < 25% | top, 1min avg | ≤ 25% |
| TC-PERF-004 | Memory (HU) | < 150MB | /proc, valgrind | ≤ 150MB |
| TC-PERF-005 | Media CPU | < 15% | During play | ≤ 15% |

---

## 7. Stability Verification

| TC-ID | Test | Time/Repeat | Pass |
|-------|------|-------------|------|
| TC-STAB-001 | Long run | 2h | No crash/freeze, memory Δ < 10MB |
| TC-STAB-002 | IPC disconnect | Broker restart 5× | Auto reconnect, data restore |
| TC-STAB-003 | Media repeat | 100 tracks | No error |
| TC-STAB-004 | Fast tab switch | 30× in 1min | No crash |

---

## 8. Usability Verification

| TC-ID | Item | Method | Pass |
|-------|------|--------|------|
| TC-UI-001 | Tab readability | 3 evaluators 1–5 | Avg ≥ 4 |
| TC-UI-002 | Media control intuitiveness | First-time play/pause | 90% success |
| TC-UI-003 | Gear button touch area | Min 44×44pt | All comply |
| TC-UI-004 | Sunlight readability | Outdoor check | Text readable |

---

## 8a. Yocto Verification

| TC-ID | Test | Procedure | Pass |
|-------|------|------------|------|
| TC-YOCTO-001 | Yocto image build | `bitbake piracer-image` | Build OK, image created |
| TC-YOCTO-002 | RPi flash/boot | Image to SD, boot | Boot OK, apps auto-start |
| TC-YOCTO-003 | VSOMEIP package | bitbake -e / image check | vsomeip included |
| TC-YOCTO-004 | Qt Multimedia | Image Qt libs | qtmultimedia included |
| TC-YOCTO-005 | systemd auto-start | systemctl status | 4 services active |
| TC-YOCTO-006 | CAN init | ip link show can0 | can0 UP, bitrate 500000 |
| TC-YOCTO-007 | vsomeip.json deploy | /etc/vsomeip/ check | Both configs exist |

---

## 9. Test Execution Log

| TC-ID | Date | Result | Notes |
|-------|------|--------|-------|
| TC-IPC-001 ~ 004 | | Pass/Fail | |
| TC-MED-001 ~ 008 | | Pass/Fail | |
| TC-GEAR-001 ~ 003 | | Pass/Fail | |
| TC-LED-001 ~ 003 | | Pass/Fail | |
| TC-INT-001 ~ 008 | | Pass/Fail | |
| TC-SYS-001 ~ 006 | | Pass/Fail | |
| TC-PERF-001 ~ 005 | | Pass/Fail | |
| TC-STAB-001 ~ 004 | | Pass/Fail | |
| TC-YOCTO-001 ~ 007 | | Pass/Fail | |
| TC-UI-001 ~ 004 | | Pass/Fail | |

---

## 10. Verification Summary

### 10.1 Pass Criteria
- **Unit**: All TC-Unit Pass
- **Integration**: All TC-INT Pass
- **System**: All TC-SYS Pass
- **Performance**: TC-PERF 4/5+ Pass
- **Stability**: TC-STAB 3/4+ Pass
- **Usability**: TC-UI 3/4+ Pass

### 10.2 Traceability to Critical Evaluation
- Failures → CRITICAL_EVALUATION_EN.md **Issues**
- Pass with improvement room → **Improvements**

---

*Verification results feed into CRITICAL_EVALUATION_EN.md for architecture optimization.*
