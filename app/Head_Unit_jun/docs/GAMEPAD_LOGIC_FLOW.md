# 게임패드 로직 흐름

## 현재 구조 (통합됨)

```
┌─────────────────────────────────────────────────────┐
│ Instrument Cluster (PiRacerDashboard)                │
│   └── piracer_bridge.py (자동 시작)                  │
│       ├── ShanWanGamepad: X/A/B/Y=기어, 스틱=스로틀/스티어링 │
│       ├── set_throttle_percent / set_steering_percent │
│       ├── 배터리/CAN → Cluster 표시                   │
│       └── drive_mode.json 쓰기 (Head Unit 동기화)    │
└─────────────────────────────────────────────────────┘
          ▲
          │ IPC (기어)
┌─────────┴───────────┐
│ Head Unit           │
│ (터치 P/R/N/D)      │
│ → IPC → Cluster     │
│   → setGearFromIPC  │
└─────────────────────┘
```

## run_gamepad_service.sh는 더 이상 필요 없음

piracer_bridge에 모터 제어가 통합되어 있어, **Cluster만 실행하면 게임패드로 조종 가능**합니다.

| 버튼/스틱 | 동작 |
|-----------|------|
| X | 전진(F) |
| B | 후진(R) |
| Y, A | 중립(N) |
| 우측 스틱 Y | 스로틀 |
| 좌측 스틱 X | 스티어링 |

### 실행 방법

```bash
cd instrument_cluster/build
DISPLAY=:0 VSOMEIP_CONFIGURATION=../config/vsomeip_cluster.json ./PiRacerDashboard
```

이 한 줄이면 게임패드로 모터 제어 가능. `run_gamepad_service.sh` 실행 불필요.
