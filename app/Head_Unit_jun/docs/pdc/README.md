# PDC Integration Workspace

이 디렉토리는 후방 카메라 화면에 Park Distance Control(PDC) 경고 overlay를 얹기 위한 설계, 구현 순서, 검증 계획을 모아 둔다.

## 현재 확인한 사실

| 항목 | 위치 | 확인 내용 |
|------|------|-----------|
| 후진 기어 처리 | `shell/ShellWindow.cpp` | `GearState::R` 진입 시 `ReverseCameraWindow`를 생성하고 표시한다. |
| 후방 카메라 렌더링 | `shell/widgets/ReverseCameraWindow.cpp` | GStreamer frame 또는 placeholder만 그린다. PDC 선/경고 overlay는 아직 없다. |
| 차량 데이터 인터페이스 | `core/interfaces/IVehicleDataProvider.h` | speed, gear, battery 중심이다. PDC 전용 distance 상태는 없다. |
| IPC 구조 | `core/ipc/VSomeIPClient.cpp` | 현재 HU는 VSOMEIP로 speed event를 받고 gear를 publish한다. |
| CAN 예시 | `instrument_cluster/src/serial/SerialReader.cpp` | Cluster 쪽에 SocketCAN 기반 speed 수신 패턴이 있다. PDC CAN receiver 작성 시 참고 가능하다. |

## 문서 구성

| 문서 | 목적 |
|------|------|
| [ARCHITECTURE.md](./ARCHITECTURE.md) | PDC 기능을 Head Unit 구조에 넣는 전체 아키텍처 |
| [CAN_SIGNAL_CONTRACT.md](./CAN_SIGNAL_CONTRACT.md) | 초음파/속도 CAN 신호 계약과 확인 절차 |
| [UI_OVERLAY_SPEC.md](./UI_OVERLAY_SPEC.md) | 후방 카메라 위 빨강/노랑/초록 선, 센서 영역, 경고 단계 UI 사양 |
| [IMPLEMENTATION_PLAN.md](./IMPLEMENTATION_PLAN.md) | 실제 코드 반영 순서와 파일 단위 작업 목록 |
| [VERIFICATION_PLAN.md](./VERIFICATION_PLAN.md) | 단위/통합/실차 검증 항목 |

## 목표 동작

1. 기어가 `R`이면 후방 카메라 창을 표시한다.
2. PDC 센서 거리를 CAN 또는 mock provider에서 수신한다.
3. 센서별 거리를 안정화하고 가장 가까운 장애물 기준으로 위험 단계를 계산한다.
4. 카메라 frame 위에 초록/노랑/빨강 guide line과 센서별 proximity bar를 그린다.
5. 위험 단계에 맞춰 beep 주기를 바꾸고, 너무 가까우면 연속 경고를 낸다.
6. 기어가 `R`이 아니거나 센서 데이터가 stale이면 PDC overlay와 beep를 안전하게 끈다.

## 우선순위

1. `ReverseCameraWindow`에 PDC overlay painter를 추가한다. Done.
2. CAN 신호가 확정되기 전에는 `MockPdcSensorProvider`로 UI를 먼저 검증한다. Done.
3. `SocketCanPdcProvider`를 붙이고 `candump can0` 로그로 decode를 확정한다. First pass done with `0x123` B1.
4. Yocto recipe와 `local.conf`에 필요한 CAN/GStreamer/Qt 의존성을 최종 반영한다.
