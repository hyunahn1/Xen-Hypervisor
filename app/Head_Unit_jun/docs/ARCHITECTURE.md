# PiRacer Head Unit - System Architecture (요약)

> [English](./ARCHITECTURE_EN.md) | **상세 설계**: [ARCHITECTURE_DESIGN.md](./ARCHITECTURE_DESIGN.md) | **PDC 설계**: [pdc/README.md](./pdc/README.md) | **검증**: [VERIFICATION.md](./VERIFICATION.md) | **비판적 평가**: [CRITICAL_EVALUATION.md](./CRITICAL_EVALUATION.md) | **최적화 사이클**: [OPTIMIZATION_CYCLE.md](./OPTIMIZATION_CYCLE.md)

## 1. Overview

```
┌─────────────────┐     CAN      ┌──────────────────────────────────────────────────────┐
│  Arduino        │─────────────→│  Raspberry Pi                                          │
│  + Speed Sensor │              │  ┌─────────────────┐    VSOMEIP    ┌─────────────────┐ │
└─────────────────┘              │  │ Instrument      │◄─────────────►│  Head Unit      │ │
                                 │  │ Cluster         │               │  (Qt)           │ │
┌─────────────────┐              │  └────────┬────────┘               └────────┬────────┘ │
│  Gamepad        │─────────────→│           │                                  │         │
│  (Bluetooth)    │   스로틀     │           │  기어 상태                        │         │
└─────────────────┘              │           │  속도 등                          │         │
                                 │           │                                  │         │
                                 │  ┌────────▼────────┐               ┌────────▼────────┐ │
                                 │  │  Display 1      │               │  Display 2      │ │
                                 │  │  (DSI 1280×400) │               │  (HDMI 등)      │ │
                                 │  └─────────────────┘               └─────────────────┘ │
                                 └──────────────────────────────────────────────────────┘
```

## 2. Data Flow

### 2.1 속도
- **경로**: Arduino → CAN → Instrument Cluster
- **Head Unit**: IPC로 Cluster에서 속도 수신 (선택적 표시)

### 2.2 기어
- **입력 소스 1**: 게임패드 스로틀 → F/R/N
- **입력 소스 2**: Head Unit 터치 → P/R/N/D
- **공용 상태**: Last-Write-Wins, VSOMEIP로 양쪽 앱에 전파

### 2.3 Ambient LED
- Head Unit → (제어 명령) → LED 드라이버 (GPIO/Python 서비스)
- *하드웨어 수령 후 상세 정의*

## 3. VSOMEIP 구조 (초안)

| Service | Method/Event | 방향 | 용도 |
|---------|--------------|------|------|
| VehicleData | speed | Cluster→HU | 속도 전달 |
| VehicleData | gear | Bidirectional | 기어 상태 동기화 |
| VehicleData | battery | Cluster→HU | 배터리 (선택) |

## 4. 탭별 화면

| 탭 | 주요 기능 |
|----|-----------|
| 미디어 | Now Playing, 플레이리스트, 볼륨 |
| 조명 | LED 색상/밝기 제어 |
| 기어 | P/R/N/D 표시 및 터치 선택 |
| 설정 | 캘리브레이션, 테마 등 |
