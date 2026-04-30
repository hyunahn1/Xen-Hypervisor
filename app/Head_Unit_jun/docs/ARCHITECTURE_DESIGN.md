# PiRacer Head Unit - Architecture Design Document

## 📋 Document Information
- **Version**: 2.0.0
- **Date**: 2026-02-20
- **Status**: Updated (비판적 평가 Round 0 반영)
- **변경 이력**: Section 7 Revisions 참조

---

## 1. Requirements Traceability

### 1.1 프로젝트 요구사항 → 아키텍처 매핑

| ID | 요구사항 (HeadUnit.md) | 아키텍처 대응 | 검증 방법 |
|----|------------------------|---------------|-----------|
| REQ-01 | Head Unit Qt 앱, RPi, Instrument Cluster와 동시 실행 | 분리된 프로세스, 멀티 디스플레이, systemd 자동 기동 | VER-01 |
| REQ-02 | Ambient lighting 제어 | Head Unit → ILedController → LED 하드웨어 | VER-02 |
| REQ-03 | 기어 선택 표시 | GearScreen 위젯, VSOMEIP gear 이벤트 수신 | VER-03 |
| REQ-04 | 미디어 앱 (음악/비디오 재생·제어) | MediaScreen, Qt Multimedia (QMediaPlayer) | VER-04 |
| REQ-05 | IPC (Cluster ↔ Head Unit) | VSOMEIP peer-to-peer (Service Discovery) | VER-05 |
| REQ-06 | CAN (속도 센서 데이터) | Arduino→CAN→Cluster (CAN Reader), VSOMEIP로 HU 전달 | VER-06 |
| REQ-07 | Yocto 빌드 시스템 | Yocto meta-layer, recipe 작성 | VER-07 |
| REQ-08 | 기어 터치 제어 (사용자 결정) | GearScreen P/R/N/D 버튼 → GearStateManager → VSOMEIP publish | VER-08 |
| REQ-09 | 기어 게임패드 제어 | Gamepad Python 서비스 → GearStateManager → VSOMEIP publish | VER-09 |

### 1.2 제약사항

| ID | 제약 | 영향 |
|----|------|------|
| C-01 | PiRacer 기어 = 스로틀 부호 (게임패드 아날로그) | 기어 터치 → 실제 모터 직접 제어 불가, 기어 상태 표시+의도만 전달 (§2.4 상세) |
| C-02 | 디스플레이 2대 (Cluster DSI 1280×400, Head Unit HDMI) | RPi DSI + HDMI 동시 출력 설정 필요 |
| C-03 | 속도 = Arduino→CAN→Cluster | Cluster가 CAN 소켓 소유, HU는 VSOMEIP로 수신 |
| C-04 | Ambient LED 하드웨어 미확정 | 조명 모듈 인터페이스 추상화, 하드웨어 수령 후 구체화 |
| C-05 | VSOMEIP = peer-to-peer | 중앙 브로커 없음, 각 앱에 vsomeip.json 설정 필요 |

---

## 2. System Architecture

### 2.1 상위 다이어그램 (VSOMEIP peer-to-peer 정정)

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
│  │  │ (스로틀 기반 F/R/P 판단) │    │       │            │                 │   │
│  │  └────────────┬─────────────┘    │       │   ┌────────▼────────────┐    │   │
│  │               │                  │       │   │ GearStateManager    │    │   │
│  │  ┌────────────▼─────────────┐    │       │   │ (터치 기어 의도)    │    │   │
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
│  │ Python 서비스│──────────────────────────►│   ILedController → LED HW    │   │
│  └──────────────┘  gear 의도 전달           └──────────────────────────────┘   │
│                                                                                 │
│  VSOMEIP Service Discovery: UDP Multicast (lo, 127.255.255.255)                │
│  ──────────────────────────────────────────────────────────────────────────── │
│  Cluster: offer 0x1234/0x0001   ←→   Head Unit: find 0x1234/0x0001           │
└─────────────────────────────────────────────────────────────────────────────────┘
```

### 2.2 컴포넌트 정의

| 컴포넌트 | 책임 | 인터페이스 |
|----------|------|------------|
| **Instrument Cluster** | CAN 속도 수신, GearStateManager(스로틀 기반), VSOMEIP offer | CAN (socketcan), VSOMEIP |
| **Head Unit** | 4탭 UI, 미디어, 조명, 터치 기어, VSOMEIP subscribe/publish | VSOMEIP, ILedController |
| **Gamepad Python 서비스** | 게임패드 입력 읽기, 스로틀→PiRacer 제어, 기어 의도 → Head Unit 전달 | Python piracer-py, (VSOMEIP or stdin) |
| **GearStateManager** | 기어 상태 보관·전파, Last-Write-Wins | 내부 클래스 |
| **ILedController** | LED 하드웨어 추상 인터페이스 | GPIO/PWM/WS2812 (구현 클래스 교체 가능) |
| **VSomeIPService (Cluster)** | speed/gear/battery 이벤트 offer | VSOMEIP |
| **VSomeIPClient (HU)** | speed/gear/battery 이벤트 subscribe | VSOMEIP |

### 2.3 데이터 흐름

| 데이터 | 소스 | 경로 | 목적지 |
|--------|------|------|--------|
| 속도 | Arduino | CAN → Cluster CANReader | Cluster UI, VSOMEIP 0x8001 → HU StatusBar |
| 기어(스로틀 자동) | Gamepad 아날로그 | Python 서비스 → PiRacer throttle → GearStateManager | Cluster UI, VSOMEIP 0x8002 → HU GearScreen |
| 기어(터치) | HU GearScreen | GearStateManager → VSOMEIP publish 0x8002 | Cluster 수신 → 표시 |
| 기어(게임패드 버튼) | Gamepad 버튼 | Python 서비스 → VSOMEIP publish 0x8002 | Cluster·HU 수신 |
| 배터리 | PiRacer Python bridge | Python → Cluster | Cluster UI, VSOMEIP 0x8003 → HU (선택) |
| LED 명령 | HU AmbientScreen | ILedController | LED 하드웨어 |

### 2.4 기어 제어 상세 (제약 C-01 대응)

#### 현실적 제약
PiRacer의 기어(전진/후진)는 `piracer.set_throttle_percent(value)`의 부호로 결정됩니다.
- 양수 → 전진(F), 음수 → 후진(R), 0 → 정지
- 게임패드 아날로그 스틱이 이미 스로틀을 직접 제어하고 있으므로, **터치 기어 선택이 모터를 직접 제어하면 게임패드와 충돌** 발생

#### 설계 결정: 기어 "의도 + 표시" 분리 방식

```
[터치 기어 선택 R]
       │
       ▼
GearStateManager (HU)
 - gearIntent = "R"
 - VSOMEIP publish gear = "R"
       │
       ▼
Cluster GearScreen: "R" 표시    (표시 역할)
HU GearScreen: "R" 표시         (표시 역할)
       │
       ▼
Python Gamepad 서비스:
 - subscribe gear 이벤트 수신
 - gearIntent == "R" 이면 throttle 부호 강제 음수 적용
 - gearIntent == "P" or "N" 이면 throttle = 0 유지
 - gearIntent == "D" 이면 throttle 부호 양수 적용
```

즉, **모터 제어는 항상 Python 서비스**가 담당하며, Head Unit은 기어 의도를 VSOMEIP로 전달하고 Python 서비스가 이를 반영합니다.

#### P / N 기어 동작 정의

| 기어 | 표시 조건 | 모터 동작 |
|------|-----------|-----------|
| **P (Parking)** | 속도 ≤ 0.15 km/h 시 자동 표시 (Cluster와 동일), 또는 터치 선택 | throttle = 0 강제, 게임패드 스로틀 무시 |
| **N (Neutral)** | 터치로 선택 가능 | throttle = 0 (P와 동일 동작) |
| **D (Drive)** | 터치로 선택 또는 스로틀 양수 | throttle 양수 허용 |
| **R (Reverse)** | 터치로 선택 또는 스로틀 음수 | throttle 음수 허용 |

> **Note**: N은 P와 모터 동작은 동일하되, 이름/색상만 다르게 표시하여 "이동 준비 중" 의미로 구분합니다.

#### 이중 입력 충돌 해결 (Last-Write-Wins)

```
터치 입력 시간: T=100ms → gear = "R"
게임패드 입력: T=150ms → gear = "D"
결과: gear = "D" (150ms가 마지막)
```

Python 서비스와 HU가 동시에 publish하면 가장 최근 이벤트가 적용됩니다.

---

## 3. Design Decisions & Rationale

### 3.1 ADR (Architecture Decision Record)

| ADR-ID | 결정 | 대안 | 선정 이유 |
|--------|------|------|-----------|
| ADR-01 | IPC = VSOMEIP (peer-to-peer) | D-Bus, Socket | 프로젝트 요구사항 명시, 자동차 표준 |
| ADR-02 | 기어 이중 입력 (게임패드 버튼 + 터치) | 단일 소스 | 사용자 요구 |
| ADR-03 | Last-Write-Wins 기어 충돌 | 우선순위 규칙 | 구현 단순성, 최신 의도 존중 |
| ADR-04 | Cluster가 CAN 소켓 소유 | HU 직접 CAN | 단일 소켓 원칙, 기존 Cluster 재활용 |
| ADR-05 | Head Unit 별도 디스플레이 (HDMI) | 화면 분할 | 독립 실행, 해상도 자유도 |
| ADR-06 | 4탭 상단 네비게이션 | 하단/사이드 | 터치 접근성, 인포테인먼트 관례 |
| ADR-07 | LED 제어 인터페이스 추상화 | 직접 GPIO | 하드웨어 미확정, 구현 클래스 교체 용이 |
| ADR-08 | 기어 터치 = 의도 전달, 모터 제어는 Python 서비스 | HU 직접 모터 제어 | PiRacer 스로틀 충돌 방지, 책임 분리 |
| ADR-09 | N 기어 = P와 동일 모터 동작 | 별도 동작 | PiRacer API 제약 (스로틀 부호만 존재) |
| ADR-10 | 속도 HU 표시 = StatusBar (소형) | 별도 탭 | 요구사항 충족, 화면 공간 절약 |
| ADR-11 | Gamepad = 별도 Python 서비스 | Cluster 내부 처리 | 기어 의도 VSOMEIP publish, Cluster 독립성 유지 |

### 3.2 리스크 & 완화

| 리스크 | 영향 | 완화 |
|--------|------|------|
| VSOMEIP 학습 곡선 | 일정 지연 | 예제·튜토리얼 선행, D-Bus 폴백 준비 |
| 기어 스로틀/터치 동시 입력 | 혼란 | Last-Write-Wins, Python 서비스 기어 의도 구독 |
| LED 하드웨어 다양성 | 인터페이스 변경 | ILedController 추상화, 어댑터 패턴 |
| RPi 멀티 디스플레이 | 설정 복잡 | /boot/config.txt 설정 (§8 참조) |
| VSOMEIP Service Discovery 타이밍 | 앱 시작 시 이벤트 누락 | 부팅 순서 정의 (§5), 재시도 로직 |

---

## 4. Interface Specification

### 4.1 VSOMEIP 서비스 정의

**Service ID**: `0x1234`  
**Instance ID**: `0x0001`  
**Protocol**: UDP (Service Discovery) + TCP (데이터)

| Event Group | Event ID | 이름 | 페이로드 타입 | 페이로드 내용 | 방향 | 주기/트리거 |
|-------------|----------|------|--------------|--------------|------|-------------|
| 0x0001 | 0x8001 | speed | float32 (4 bytes) | km/h | Cluster→HU | 100ms |
| 0x0001 | 0x8002 | gear | uint8 (1 byte) | P=0, R=1, N=2, D=3 | 양방향 | 변경 시 |
| 0x0001 | 0x8003 | battery | {float32 voltage, float32 percent} (8 bytes) | V, % | Cluster→HU | 500ms |

### 4.2 vsomeip.json 설정 파일

#### Instrument Cluster (서비스 제공자)
**파일 경로**: `/etc/vsomeip/vsomeip_cluster.json`

```json
{
    "unicast": "127.0.0.1",
    "logging": {
        "level": "warning",
        "console": "false"
    },
    "applications": [
        {
            "name": "InstrumentCluster",
            "id": "0x1001"
        }
    ],
    "services": [
        {
            "service": "0x1234",
            "instance": "0x0001",
            "reliable": {
                "port": "30509"
            },
            "unreliable": "30510"
        }
    ],
    "events": [
        {
            "service": "0x1234",
            "instance": "0x0001",
            "event": "0x8001",
            "is_field": false,
            "update-cycle": "100"
        },
        {
            "service": "0x1234",
            "instance": "0x0001",
            "event": "0x8002",
            "is_field": true,
            "update-cycle": "0"
        },
        {
            "service": "0x1234",
            "instance": "0x0001",
            "event": "0x8003",
            "is_field": false,
            "update-cycle": "500"
        }
    ],
    "eventgroups": [
        {
            "service": "0x1234",
            "instance": "0x0001",
            "eventgroup": "0x0001",
            "events": ["0x8001", "0x8002", "0x8003"]
        }
    ],
    "routing": "InstrumentCluster",
    "service-discovery": {
        "enable": "true",
        "multicast": "224.0.0.1",
        "port": "30490",
        "protocol": "udp"
    }
}
```

#### Head Unit (서비스 구독자)
**파일 경로**: `/etc/vsomeip/vsomeip_headunit.json`

```json
{
    "unicast": "127.0.0.1",
    "logging": {
        "level": "warning",
        "console": "false"
    },
    "applications": [
        {
            "name": "HeadUnit",
            "id": "0x1002"
        }
    ],
    "routing": "InstrumentCluster",
    "service-discovery": {
        "enable": "true",
        "multicast": "224.0.0.1",
        "port": "30490",
        "protocol": "udp"
    }
}
```

> **Note**: Head Unit은 별도 서비스를 offer하지 않고, gear 이벤트(0x8002)를 Cluster 쪽으로 notify합니다. Cluster도 0x8002를 subscribe하여 HU 터치 기어 변경을 수신합니다.

### 4.3 LED 제어 인터페이스 (추상)

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

// 하드웨어 확정 후 구현 (어댑터 패턴)
// class PwmLedController : public ILedController { ... };
// class WS2812LedController : public ILedController { ... };
```

### 4.4 기어 상태 프로토콜

```cpp
enum class GearState : uint8_t {
    P = 0,  // Parking: throttle = 0, 속도 0 시 자동 또는 터치
    R = 1,  // Reverse: throttle < 0 허용
    N = 2,  // Neutral: throttle = 0 (P와 동일 동작)
    D = 3   // Drive: throttle > 0 허용
};

struct GearEvent {
    GearState gear;
    QString source;  // "throttle" | "touch" | "button"
    qint64 timestamp;
};
```

---

## 5. Boot Sequence (부팅 순서)

### 5.1 전체 부팅 흐름

```
[Power ON]
    │
    ▼ ~5s
[Yocto Linux Kernel 부팅]
    │
    ▼ ~3s
[systemd default.target]
    │
    ├──► [CAN 인터페이스 초기화]  (can-init.service)
    │       ip link set can0 type can bitrate 500000
    │       ip link set can0 up
    │       → 완료 신호: can0 UP
    │
    ├──► [DSI 디스플레이 활성화]  (자동, 커널 드라이버)
    │       → Display 1 (Cluster)
    │
    ├──► [HDMI 디스플레이 활성화]  (자동, 커널 드라이버)
    │       → Display 2 (Head Unit)
    │
    ▼
[piracer-gamepad.service 시작]  (After: can-init.service)
    │   Python 서비스: 게임패드 읽기, 스로틀 제어, 기어 의도 publish
    │   → 준비 완료 (VSOMEIP 연결 대기)
    │
    ▼
[instrument-cluster.service 시작]  (After: can-init.service)
    │   VSOMEIP_APPLICATION_NAME=InstrumentCluster
    │   VSOMEIP_CONFIGURATION=/etc/vsomeip/vsomeip_cluster.json
    │   → VSOMEIP Service offer (0x1234/0x0001)
    │   → Service Discovery 시작 (UDP Multicast 224.0.0.1:30490)
    │   → Display 1 (DSI) 에 렌더링
    │
    ▼ (After: instrument-cluster.service)
[head-unit.service 시작]  (After: instrument-cluster.service)
    │   VSOMEIP_APPLICATION_NAME=HeadUnit
    │   VSOMEIP_CONFIGURATION=/etc/vsomeip/vsomeip_headunit.json
    │   → VSOMEIP Service Discovery: 0x1234/0x0001 탐색
    │   → 탐색 성공 시 subscribe (0x8001, 0x8002, 0x8003)
    │   → Display 2 (HDMI) 에 렌더링
    │
    ▼
[정상 운영 상태]
    Cluster: CAN 속도 수신 → VSOMEIP publish
    Head Unit: VSOMEIP subscribe → UI 업데이트
    Gamepad: 스로틀 → PiRacer 모터, 버튼/터치 → VSOMEIP gear publish
```

### 5.2 systemd 서비스 파일 예시

```ini
# /etc/systemd/system/can-init.service
[Unit]
Description=CAN Interface Initialization
After=network.target

[Service]
Type=oneshot
RemainAfterExit=yes
ExecStart=/bin/sh -c 'ip link set can0 type can bitrate 500000 && ip link set can0 up'

[Install]
WantedBy=multi-user.target
```

```ini
# /etc/systemd/system/instrument-cluster.service
[Unit]
Description=PiRacer Instrument Cluster
After=can-init.service
Requires=can-init.service

[Service]
Type=simple
Environment=VSOMEIP_APPLICATION_NAME=InstrumentCluster
Environment=VSOMEIP_CONFIGURATION=/etc/vsomeip/vsomeip_cluster.json
Environment=DISPLAY=:0
ExecStart=/usr/bin/instrument_cluster
Restart=on-failure
RestartSec=5

[Install]
WantedBy=graphical.target
```

```ini
# /etc/systemd/system/head-unit.service
[Unit]
Description=PiRacer Head Unit
After=instrument-cluster.service
Wants=instrument-cluster.service

[Service]
Type=simple
Environment=VSOMEIP_APPLICATION_NAME=HeadUnit
Environment=VSOMEIP_CONFIGURATION=/etc/vsomeip/vsomeip_headunit.json
Environment=DISPLAY=:1
ExecStart=/usr/bin/head_unit
Restart=on-failure
RestartSec=5

[Install]
WantedBy=graphical.target
```

```ini
# /etc/systemd/system/piracer-gamepad.service
[Unit]
Description=PiRacer Gamepad Service
After=instrument-cluster.service

[Service]
Type=simple
ExecStart=/usr/bin/python3 /usr/lib/piracer/gamepad_service.py
Restart=on-failure
RestartSec=3

[Install]
WantedBy=graphical.target
```

### 5.3 VSOMEIP Service Discovery 타이밍

```
t=0ms    Head Unit 시작, find 0x1234/0x0001 요청 (offer 대기)
t=0~200ms  Cluster이 아직 offer하지 않았으면 재시도 (SD Initial Delay)
t=~500ms  Cluster offer 완료 → HU 수신 → subscribe 완료
t=~600ms  첫 speed 이벤트 수신 (100ms 주기)
```

> **Note**: HU가 Cluster보다 먼저 시작될 경우를 대비하여 VSomeIPClient에 **재시도 로직** 필요 (find 요청을 최대 10회, 500ms 간격 재시도).

---

## 6. Error Handling Policy (에러 처리 정책)

### 6.1 에러 상황별 처리

| 상황 | 감지 방법 | UI 처리 | 복구 동작 |
|------|-----------|---------|-----------|
| VSOMEIP 연결 없음 | subscribe timeout (5s) | StatusBar "---" 표시, 황색 경고 아이콘 | 5초 간격 재탐색 |
| CAN 데이터 없음 | speed 이벤트 10초 미수신 | 속도 "---" 표시, 연결 끊김 메시지 | 자동 재연결 없음 (HW 문제) |
| VSOMEIP 이벤트 유실 | 이전 이벤트 대비 100ms 초과 | 마지막 값 유지, 3회 누락 시 "---" | 다음 이벤트 정상 수신 시 자동 복구 |
| LED 초기화 실패 | init() false 반환 | AmbientScreen에 "LED 연결 없음" 표시, 기능 비활성화 | 재시도 없음 (HW 교체 필요) |
| 미디어 파일 없음 | PlaylistModel 빈 상태 | 빈 상태 화면: 음표 아이콘 + "파일 없음" | 폴더 재스캔 버튼 |
| 미디어 코덱 오류 | QMediaPlayer errorOccurred | 해당 파일 건너뜀, 다음 파일 재생 | - |
| 게임패드 연결 끊김 | Bluetooth/USB 이벤트 | Gamepad 서비스 Restart (systemd) | 자동 재시작 |

### 6.2 에러 표시 UI 규칙

```
StatusBar (항상 표시):
┌──────────────────────────────────────────────────────────────────────────┐
│  ◉ Cluster: 연결됨   ⚠ IPC: 연결 끊김   🔊 미디어 재생 중              │
└──────────────────────────────────────────────────────────────────────────┘

- 정상: 초록 ◉
- 경고: 황색 ⚠ (재연결 중)
- 오류: 빨간 ✕ (연결 없음)
```

### 6.3 에러 로그 정책

- 로그 위치: `/var/log/piracer/headunit.log`
- 수준: ERROR (에러), WARN (경고), INFO (상태 변화)
- 로테이션: 최대 10MB, 3개 파일

---

## 7. Module Structure (파일 구조)

```
Head_Unit/
├── docs/
│   ├── ARCHITECTURE_DESIGN.md    ← 본 문서
│   ├── ARCHITECTURE.md           ← 요약
│   ├── HEAD_UNIT_DESIGN.md       ← UI 디자인 상세
│   ├── VERIFICATION.md
│   ├── CRITICAL_EVALUATION.md
│   └── OPTIMIZATION_CYCLE.md
│
├── src/
│   ├── main.cpp                  ← 앱 진입점, QApplication, DISPLAY 설정
│   ├── MainWindow.h
│   ├── MainWindow.cpp            ← TabBar + Content 컨테이너, StatusBar
│   │
│   ├── widgets/
│   │   ├── TabBar.h / TabBar.cpp            ← 상단 4탭 네비게이션
│   │   └── StatusBar.h / StatusBar.cpp      ← 하단: 속도 표시, IPC 상태, 에러
│   │
│   ├── screens/
│   │   ├── MediaScreen.h / MediaScreen.cpp  ← 미디어 탭
│   │   ├── AmbientScreen.h / AmbientScreen.cpp  ← 조명 탭
│   │   ├── GearScreen.h / GearScreen.cpp    ← 기어 탭 (P/R/N/D 터치)
│   │   └── SettingsScreen.h / SettingsScreen.cpp  ← 설정 탭
│   │
│   ├── ipc/
│   │   ├── IVehicleDataProvider.h           ← 추상 인터페이스 (테스트용 Mock 교체 가능)
│   │   ├── VSomeIPClient.h / VSomeIPClient.cpp  ← VSOMEIP 구독·발행 구현
│   │   └── GearStateManager.h / GearStateManager.cpp  ← 기어 상태 보관·충돌 해결
│   │
│   ├── media/
│   │   ├── MediaPlayer.h / MediaPlayer.cpp  ← QMediaPlayer 래퍼
│   │   └── PlaylistModel.h / PlaylistModel.cpp  ← QAbstractListModel 기반
│   │
│   └── led/
│       ├── ILedController.h                 ← 추상 인터페이스
│       ├── PwmLedController.h / PwmLedController.cpp    ← PWM 구현 (하드웨어 확정 후)
│       └── WS2812LedController.h / WS2812LedController.cpp  ← WS2812 구현 (선택)
│
├── config/
│   └── vsomeip_headunit.json               ← VSOMEIP 설정 (§4.2)
│
├── resources/
│   ├── headunit.qrc
│   ├── icons/                              ← SVG 아이콘 (Material/Phosphor)
│   └── fonts/                             ← Inter, Roboto Mono 등
│
├── python/                                 ← Gamepad 서비스 (Cluster 측)
│   └── gamepad_service.py                 ← 게임패드 → 스로틀 + 기어 VSOMEIP publish
│
└── CMakeLists.txt (또는 headunit.pro)
```

---

## 8. Display Configuration (RPi 멀티 디스플레이)

### 8.1 /boot/config.txt 설정

```ini
# DSI (Display 1 - Instrument Cluster)
display_auto_detect=0
dtoverlay=vc4-kms-v3d
max_framebuffers=2

# HDMI (Display 2 - Head Unit)
hdmi_force_hotplug=1
hdmi_group=1
hdmi_mode=16       # 1080p60 (실제 디스플레이에 맞게 조정)
```

### 8.2 X11 디스플레이 매핑 (필요 시)

```bash
# Cluster: :0 (DSI)
export DISPLAY=:0
./instrument_cluster &

# Head Unit: :1 (HDMI)
export DISPLAY=:1
./head_unit &
```

---

## 9. Non-Functional Requirements

| NFR | 목표 | 검증 |
|-----|------|------|
| 응답성 | UI 입력 100ms 이내 반응 | TC-PERF-001 |
| IPC 지연 | VSOMEIP 이벤트 50ms 이내 전달 | TC-PERF-002 |
| 안정성 | 2시간 이상 무중단 | TC-STAB-001 |
| 리소스 | 메모리 < 150MB, CPU < 25% | TC-PERF-003, TC-PERF-004 |
| 호환성 | Yocto 이미지에서 빌드·실행 | TC-YOCTO-001~003 |
| 에러 복구 | VSOMEIP 재연결 < 10s | TC-STAB-002 |

---

## 10. Revisions (최적화 이력)

| Rev | 날짜 | 변경 내용 | 비판적 평가 결과 |
|-----|------|-----------|------------------|
| 1.0 | 2026-02-20 | 초안 | - |
| 2.0 | 2026-02-20 | VSOMEIP peer-to-peer 수정, 기어 이중 입력 경로 상세화, 부팅 순서 추가, P/N 기어 정의, 에러 처리 정책, vsomeip.json 설정, 파일 구조 통일, 속도 표시 StatusBar 결정 | 비판적 평가 Round 0 (설계 검토) |

---

*이 문서는 VERIFICATION.md 검증 결과 및 CRITICAL_EVALUATION.md 비판적 평가를 반영하여 주기적으로 갱신됩니다.*
