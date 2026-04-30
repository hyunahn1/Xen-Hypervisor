# PiRacer Head Unit - System Architecture (Summary)

> [한국어](./ARCHITECTURE.md) | **Detailed Design**: [ARCHITECTURE_DESIGN_EN.md](./ARCHITECTURE_DESIGN_EN.md) | **Verification**: [VERIFICATION_EN.md](./VERIFICATION_EN.md) | **Critical Evaluation**: [CRITICAL_EVALUATION_EN.md](./CRITICAL_EVALUATION_EN.md) | **Optimization Cycle**: [OPTIMIZATION_CYCLE_EN.md](./OPTIMIZATION_CYCLE_EN.md)

## 1. Overview

```
┌─────────────────┐     CAN      ┌──────────────────────────────────────────────────────┐
│  Arduino        │─────────────→│  Raspberry Pi                                          │
│  + Speed Sensor │              │  ┌─────────────────┐    VSOMEIP    ┌─────────────────┐ │
└─────────────────┘              │  │ Instrument      │◄─────────────►│  Head Unit      │ │
                                 │  │ Cluster         │               │  (Qt)           │ │
┌─────────────────┐              │  └────────┬────────┘               └────────┬────────┘ │
│  Gamepad        │─────────────→│           │                                  │         │
│  (Bluetooth)    │   Throttle   │           │  Gear state                      │         │
└─────────────────┘              │           │  Speed, etc.                     │         │
                                 │           │                                  │         │
                                 │  ┌────────▼────────┐               ┌────────▼────────┐ │
                                 │  │  Display 1      │               │  Display 2      │ │
                                 │  │  (DSI 1280×400)  │               │  (HDMI, etc.)   │ │
                                 │  └─────────────────┘               └─────────────────┘ │
                                 └──────────────────────────────────────────────────────┘
```

## 2. Data Flow

### 2.1 Speed
- **Path**: Arduino → CAN → Instrument Cluster
- **Head Unit**: Receives speed from Cluster via IPC (optional display)

### 2.2 Gear
- **Input 1**: Gamepad throttle → F/R/N
- **Input 2**: Head Unit touch → P/R/N/D
- **Shared state**: Last-Write-Wins, propagated to both apps via VSOMEIP

### 2.3 Ambient LED
- Head Unit → (control command) → LED driver (GPIO/Python service)
- *To be detailed after hardware receipt*

## 3. VSOMEIP Structure (Draft)

| Service | Method/Event | Direction | Purpose |
|---------|--------------|-----------|---------|
| VehicleData | speed | Cluster→HU | Speed transfer |
| VehicleData | gear | Bidirectional | Gear state sync |
| VehicleData | battery | Cluster→HU | Battery (optional) |

## 4. Tab Screens

| Tab | Main Features |
|-----|---------------|
| Media | Now Playing, playlist, volume |
| Lighting | LED color/brightness control |
| Gear | P/R/N/D display and touch selection |
| Settings | Calibration, theme, etc. |
