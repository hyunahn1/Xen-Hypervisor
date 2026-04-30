# PDC UI Overlay Specification

## 1. Objective

후진 카메라 화면 위에 실제 자동차처럼 거리별 색상 guide line과 센서별 경고를 표시한다. 화면은 운전자가 후진 중 빠르게 읽어야 하므로 텍스트보다 색, 위치, beep 패턴을 우선한다.

## 2. Target View

Current camera window:

- Widget: `ReverseCameraWindow`
- Size: `640 x 400`
- Paint source: camera frame or placeholder pixmap

PDC overlay는 `paintEvent()`에서 camera frame을 먼저 그리고 그 위에 그린다.

```
┌────────────────────────────────────────┐
│              REAR CAMERA               │
│                                        │
│       green guide zone                 │
│      /----------------\                │
│     /------------------\               │
│    yellow warning zone                 │
│   /----------------------\             │
│  red critical zone                     │
│ /--------------------------\           │
│                                        │
│ [RL] [RML] [RMR] [RR] sensor bars      │
└────────────────────────────────────────┘
```

## 3. Visual Elements

| Element | Purpose | Notes |
|---------|---------|-------|
| Distance guide lines | 차량 후방 거리감을 표시 | green/yellow/red curved trapezoid lines |
| Sensor sector fill | 어느 방향이 가까운지 표시 | left to right 4 sectors |
| Nearest distance label | debug/development feedback | release에서는 작게 또는 settings flag로 숨김 |
| Stale indicator | 센서 데이터 없음 | gray/dim overlay, no beep |

## 4. Color Rules

| Level | Color | Suggested RGBA |
|-------|-------|----------------|
| Far | green | `rgba(45, 220, 120, 150)` |
| Near | green | `rgba(45, 220, 120, 210)` |
| Caution | yellow | `rgba(255, 210, 64, 220)` |
| Critical | red | `rgba(255, 64, 64, 235)` |
| Stale/Invalid | gray | `rgba(150, 150, 150, 90)` |

## 5. Distance Mapping

| Distance | UI |
|----------|----|
| `> 120 cm` | only far guide line visible |
| `60-120 cm` | green zone emphasized |
| `30-60 cm` | yellow zone emphasized |
| `< 30 cm` | red zone emphasized, critical sector pulses |

Sensor sector intensity:

```
intensity = clamp((150 cm - distanceCm) / 120 cm, 0.0, 1.0)
```

Closer obstacle means stronger alpha.

## 6. Layout Coordinates

Initial static coordinates for `640 x 400`:

| Zone | Y position | Width top/bottom | Color |
|------|------------|------------------|-------|
| Far | 155 | 170 / 330 | green |
| Near | 215 | 240 / 430 | green |
| Caution | 275 | 320 / 540 | yellow |
| Critical | 335 | 420 / 620 | red |

These are starting points. Final values should be calibrated with the actual rear camera angle.

## 7. Beep Mapping

| Level | Interval |
|-------|----------|
| Off/Far | no beep |
| Near | 800 ms |
| Caution | 350 ms |
| Critical | 100 ms or continuous tone |

Beep must stop immediately when:

- gear leaves `R`
- PDC data becomes stale
- speed exceeds configured PDC limit
- `ReverseCameraWindow` closes

## 8. Developer Debug Mode

During implementation, enable a small debug strip at the bottom:

```
RL 145cm | RML 82cm | RMR 38cm | RR -- | nearest 38cm
```

The debug strip should be behind a compile-time or runtime flag so final demo can look clean.

## 9. Acceptance Criteria

| ID | Criteria |
|----|----------|
| UI-01 | Overlay appears only while reverse camera is visible. |
| UI-02 | Green/yellow/red lines are visible over both real frame and placeholder. |
| UI-03 | Closest sensor sector is visually obvious within 100 ms of new data. |
| UI-04 | Stale/missing sensor data never shows false red. |
| UI-05 | Overlay drawing does not drop camera update below target 30 fps on RPi. |
