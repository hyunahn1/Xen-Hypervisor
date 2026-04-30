# PDC CAN Signal Contract

## 1. Goal

초음파 센서와 속도 센서가 CAN으로 들어온다는 가정은 있지만, 현재 repository 안에서는 PDC 초음파 CAN ID와 payload format이 확정되어 있지 않다. 이 문서는 구현 전에 확정해야 할 신호 계약과 확인 절차를 정의한다.

## 2. Required Signals

| Signal | Direction | Required | Purpose |
|--------|-----------|----------|---------|
| rear_center_distance_cm | CAN -> HU | yes | 현재 HC-SR04-style 후방 중앙 obstacle distance |
| rear_left/right_distance_cm | CAN -> HU | optional | 향후 다채널 PDC 확장용 sector distance |
| vehicle_speed_kmh | CAN/VSOMEIP -> HU | recommended | PDC activation guard, warning suppression above threshold |
| gear_state | internal/VSOMEIP -> HU | yes | PDC activation only in `R` |
| pdc_sensor_health | CAN -> HU | optional | sensor disconnected or out-of-range diagnostics |

## 3. Provisional CAN Mapping

아래 mapping은 2026-04-24에 `root@192.168.86.35`에서 확인한 live CAN 결과와 향후 4센서 확장 frame을 함께 반영한다.

| CAN ID | DLC | Byte Layout | Scale | Notes |
|--------|-----|-------------|-------|-------|
| `0x123` | 8 | B0 speed 후보, B1 rear_center distance 후보, B2-B7 zero | uint8, km/h/cm | observed live: `00 48/49 00 00 00 00 00 00`, about 5 Hz |
| `0x350` | 8 | B0-B1 RL, B2-B3 RML, B4-B5 RMR, B6-B7 RR | uint16 little-endian, cm | proposed future 4-sensor distances |
| `0x351` | 1 | B0 bitmask sensor valid | bit set = valid | optional validity |

현재 `SocketCanPdcProvider`는 `0x123`의 B1을 `rear_center` 단일 후방 거리로 표시한다. `0x350`이 들어오면 향후 4개 후방 센서를 개별 decode한다. 실제 format이 다르면 provider decode만 바꾸고, 상위 `PdcController`와 UI는 유지한다.

### 3.1 Live Capture Summary

Command:

```bash
ssh root@192.168.86.35 "candump -tz -n 80 -T 3000 can0"
```

Observed sample:

```text
(000.000000)  can0  123   [8]  00 49 00 00 00 00 00 00
(000.200799)  can0  123   [8]  00 48 00 00 00 00 00 00
(003.403546)  can0  123   [8]  00 4B 00 00 00 00 00 00
```

Interpretation:

- `0x123` arrives about every 200 ms.
- B0 stayed `0x00`, matching stationary speed.
- B1 moved around `0x47-0x4B` decimal 71-75, a plausible rear-center ultrasonic distance in cm.
- Need one physical check: move an obstacle closer/farther and confirm B1 tracks distance.

## 4. Discovery Procedure

### 4.1 Interface Check

```bash
ip link show can0
```

Expected:

- `can0` exists.
- state is `UP`.
- bitrate matches sender, usually `500000`.

### 4.2 Raw Capture

```bash
candump -tz can0
```

검증 방법:

1. 차를 정지 상태에 둔다.
2. 장애물을 센서 앞 150 cm, 100 cm, 50 cm, 20 cm에 순서대로 둔다.
3. 각 위치에서 `candump` 로그를 5초씩 저장한다.
4. 변하는 CAN ID와 byte를 찾는다.
5. 각 byte 값과 실제 거리 사이의 scale/endian을 계산한다.

### 4.3 Replay Test

capture가 확정되면 개발 PC 또는 RPi에서 replay로 UI를 검증한다.

```bash
canplayer -I pdc_sample.log
```

또는 수동 송신:

```bash
cansend can0 350#9600640032001E00
```

위 예시는 150, 100, 50, 30 cm를 little-endian uint16으로 보낸다는 가정이다.

## 5. Decode Rules

| Rule | Behavior |
|------|----------|
| DLC 부족 | frame drop, warning log |
| unknown CAN ID | ignore |
| distance `0` | invalid 또는 critical 중 실제 센서 정의에 맞춰 결정 |
| distance `< 2 cm` | invalid로 처리 |
| distance `> 400 cm` | out of range, invalid로 처리 |
| no frame for `> 300 ms` | PDC state stale |

## 6. Speed Source Decision

현재 Head Unit은 `VSomeIPClient`를 통해 speed event를 받는 구조가 있다. PDC는 속도 gating에 이 값을 우선 사용한다.

권장 정책:

| Condition | PDC |
|-----------|-----|
| Gear != R | off |
| Gear == R and speed <= 10 km/h | active |
| Gear == R and speed > 10 km/h | visual reduced, beep off |
| speed unknown | active, but log `speed unavailable` |

## 7. Yocto Notes

Target image에 필요한 항목:

- SocketCAN kernel support
- `iproute2` for `ip link`
- `can-utils` for `candump`, `cansend`, `canplayer`
- app permission to open raw CAN socket

개발 중에는 `can-utils`를 이미지에 넣어야 신호 확인 속도가 빨라진다.
