# PDC Architecture

## 1. Scope

PDC(Park Distance Control)는 후진 시 초음파 센서 거리 정보를 받아 후방 카메라 화면 위에 단계별 시각 경고와 청각 경고를 제공한다.

이 기능은 Head Unit shell에 통합한다. 이유는 현재 후진 카메라 창의 생명주기를 `ShellWindow`가 이미 관리하고 있고, PDC overlay는 다른 tab/module이 아니라 후방 카메라 화면 위에 직접 그려져야 하기 때문이다.

## 2. Existing System Fit

```
Gear input / gamepad / IPC
        |
        v
GearStateManager
        |
        v
ShellWindow::onGearChanged()
        |
        +-- Gear != R -> close ReverseCameraWindow, stop PDC alert
        |
        +-- Gear == R -> show ReverseCameraWindow
                         |
                         v
                  Camera frame paint
                         |
                         v
                  PDC overlay paint
```

추가할 PDC 경로:

```
Ultrasonic ECU / Arduino
        |
       CAN
        |
        v
SocketCanPdcProvider
        |
        v
PdcController
  - validate
  - timeout handling
  - smoothing
  - warning level calculation
        |
        +--------------------+
        |                    |
        v                    v
ReverseCameraWindow      PdcBeepController
visual overlay           audible warning
```

## 3. Proposed Components

| Component | Layer | Responsibility |
|-----------|-------|----------------|
| `PdcSensorReading` | core model | 센서별 거리, timestamp, validity를 담는 값 객체 |
| `PdcState` | core model | 전체 PDC 상태: sensor array, nearest distance, warning level |
| `IPdcSensorProvider` | core interface | CAN/mock 입력을 동일한 signal로 노출 |
| `MockPdcSensorProvider` | core/mock | 카메라 overlay 개발용 fake distance pattern |
| `SocketCanPdcProvider` | core/can | `can0`에서 초음파 CAN frame 수신 및 decode |
| `PdcController` | shell/core glue | provider 입력을 필터링하고 UI/beep용 상태로 변환 |
| `PdcOverlayPainter` | shell/widgets | 후방 카메라 위 guide line, zone, sensor bar 렌더링 |
| `PdcBeepController` | shell/audio | warning level에 따른 beep interval 제어 |

## 4. Data Model

### Sensor Layout

현재 하드웨어는 HC-SR04-style 후방 중앙 1채널을 기준으로 한다. 모듈 전면의 두 원형 부품은 송신/수신 트랜스듀서이며, 소프트웨어 모델에서는 하나의 `rear_center` 거리 센서로 취급한다. 4채널 PDC는 CAN `0x350` 확장 frame으로 지원 가능하게 남긴다.

| Index | Name | 위치 |
|-------|------|------|
| 0 | rear_center | 중앙 후방 |

좌/우 또는 전방 센서가 추가되면 같은 model에 `rear_left`, `rear_right`, `front_*`를 추가하고, overlay는 현재 `R` gear에서는 rear만 표시한다.

### Warning Levels

| Level | Distance | Color | Audible |
|-------|----------|-------|---------|
| `Off` | no valid data or gear != R | none | off |
| `Far` | `> 120 cm` | muted green | off or slow |
| `Near` | `60-120 cm` | green | slow beep |
| `Caution` | `30-60 cm` | yellow | medium beep |
| `Critical` | `< 30 cm` | red | fast or continuous beep |

거리 threshold는 실차 튜닝 값으로 config화한다.

## 5. Runtime Behavior

1. Shell starts and creates `PdcController`.
2. `PdcController` owns one provider:
   - development: `MockPdcSensorProvider`
   - target hardware: `SocketCanPdcProvider("can0")`
3. Gear changes to `R`.
4. Shell opens `ReverseCameraWindow` and connects `PdcController::stateChanged` to `ReverseCameraWindow::setPdcState`.
5. Provider emits raw sensor distances.
6. Controller drops invalid/stale values, smooths jitter, computes nearest distance and warning level.
7. Reverse camera repaints frame plus PDC overlay.
8. Beep controller changes interval according to warning level.
9. Gear leaves `R`: camera closes, overlay state resets, beep stops.

## 6. Fault Handling

| Fault | Detection | Fallback |
|-------|-----------|----------|
| CAN interface missing | provider cannot open `can0` | UI shows camera only, logs warning |
| No PDC frame | no update for `> 300 ms` | mark PDC stale, hide colored bars, stop beep |
| Out-of-range distance | distance < 2 cm or > 400 cm | sensor invalid, do not use for nearest |
| Partial sensor failure | one sensor stale/invalid | draw that sector gray, keep valid sensors active |
| Camera unavailable | existing placeholder path | still draw PDC overlay on placeholder for debugging |

## 7. Threading Model

SocketCAN read should use `QSocketNotifier` on the Qt event loop, matching the existing `SerialReader` pattern in the instrument cluster. This avoids a worker thread for the first version.

If later the CAN parsing becomes heavier, provider can move to a `QThread` while keeping the same `IPdcSensorProvider` signal contract.

## 8. Integration Point Decision

`PdcController` should be created in `ShellWindow`, not inside `ReverseCameraWindow`.

Reasons:

- Gear state and window lifecycle already live in `ShellWindow`.
- Beep must stop even if the camera widget is destroyed.
- Future settings/config can be shared with the shell.
- `ReverseCameraWindow` remains a view: camera frame plus overlay state.

## 9. Minimal Code Shape

```cpp
struct PdcSensorReading {
    QString name;
    float distanceCm = -1.0f;
    bool valid = false;
    qint64 timestampMs = 0;
};

enum class PdcWarningLevel {
    Off,
    Far,
    Near,
    Caution,
    Critical
};

struct PdcState {
    QVector<PdcSensorReading> rearSensors;
    float nearestDistanceCm = -1.0f;
    PdcWarningLevel warningLevel = PdcWarningLevel::Off;
    bool stale = true;
};

class IPdcSensorProvider : public QObject {
    Q_OBJECT
public:
    using QObject::QObject;
    virtual void start() = 0;
    virtual void stop() = 0;

signals:
    void readingsChanged(const QVector<PdcSensorReading> &readings);
    void faultChanged(const QString &message);
};
```

## 10. Open Questions

| Question | Owner | How to close |
|----------|-------|--------------|
| 초음파 CAN ID와 payload format은 무엇인가? | vehicle/CAN owner | `candump can0` 로그와 송신 코드 확인 |
| 속도 데이터는 HU가 직접 CAN에서 볼 것인가, Cluster/VSOMEIP에서 받을 것인가? | architecture | 현 구조상 speed는 Cluster->VSOMEIP가 우선이다. PDC safety gating에는 이 경로를 재사용한다. |
| Beep 출력 장치는 무엇인가? | hardware | Qt sound, ALSA tone, buzzer GPIO 중 실제 하드웨어 확인 |
| 실제 카메라 FOV에 guide line 좌표를 맞출 것인가? | UI/hardware | 정적 overlay 후 실차 캘리브레이션 |
