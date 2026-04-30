# PDC Verification Plan

## 1. Verification Strategy

PDC는 후진 중 운전자에게 경고하는 기능이라 false negative가 가장 위험하다. 검증은 아래 순서로 진행한다.

1. Model/controller unit test
2. Overlay visual test with mock data
3. CAN decode replay test
4. Reverse gear integration test
5. Hardware-in-the-loop parking scenario

## 2. Requirements Mapping

| REQ-ID | Requirement | Verification |
|--------|-------------|--------------|
| PDC-REQ-01 | PDC activates only in reverse gear | PDC-INT-001 |
| PDC-REQ-02 | Sensor distance is shown on rear camera view | PDC-UI-001, PDC-UI-002 |
| PDC-REQ-03 | Warning level changes by distance | PDC-UNIT-002, PDC-HIL-002 |
| PDC-REQ-04 | Audible warning follows warning level | PDC-UNIT-004, PDC-HIL-003 |
| PDC-REQ-05 | Missing/stale CAN data is fail-safe | PDC-UNIT-003, PDC-INT-003 |
| PDC-REQ-06 | System runs on Yocto target | PDC-SYS-001 |

## 3. Unit Tests

| TC-ID | Target | Input | Expected |
|-------|--------|-------|----------|
| PDC-UNIT-001 | CAN decode | frame with 150/100/50/30 cm | four valid readings |
| PDC-UNIT-002 | Warning calculation | nearest 125/80/45/20 cm | Far/Near/Caution/Critical |
| PDC-UNIT-003 | Stale timeout | no reading for `> 300 ms` | state stale, level Off |
| PDC-UNIT-004 | Beep mapping | Near/Caution/Critical | 800/350/100 ms interval |
| PDC-UNIT-005 | Invalid distance | 0 cm, 999 cm | sensor invalid, not nearest |

## 4. UI Verification

| TC-ID | Scenario | Pass Criteria |
|-------|----------|---------------|
| PDC-UI-001 | Mock provider sweeps 150 -> 20 cm | lines change green -> yellow -> red |
| PDC-UI-002 | Only left sensor close | left sector emphasized |
| PDC-UI-003 | Sensor data stale | overlay dims or hides, no red false warning |
| PDC-UI-004 | Camera unavailable placeholder | PDC overlay still visible for debugging |
| PDC-UI-005 | 5 minute reverse camera run | no visible flicker, no crash |

## 5. Integration Tests

| TC-ID | Scenario | Steps | Pass Criteria |
|-------|----------|-------|---------------|
| PDC-INT-001 | Reverse activation | gear D -> R -> D | camera and overlay open on R, close on D |
| PDC-INT-002 | CAN replay | replay known `candump` log | overlay follows recorded distances |
| PDC-INT-003 | CAN unplug/stale | stop CAN frames while in R | beep stops, stale state shown |
| PDC-INT-004 | Speed gating | gear R with speed > 10 km/h | beep suppressed |
| PDC-INT-005 | Provider fallback | no `can0` on dev PC | app starts, camera still works |

## 6. Hardware-in-the-Loop Tests

| TC-ID | Scenario | Pass Criteria |
|-------|----------|---------------|
| PDC-HIL-001 | Obstacle at 150 cm | far/green only |
| PDC-HIL-002 | Obstacle at 50 cm | yellow warning, medium beep |
| PDC-HIL-003 | Obstacle at 20 cm | red warning, fast/continuous beep |
| PDC-HIL-004 | Obstacle moves left to right | highlighted sector follows position |
| PDC-HIL-005 | One sensor blocked/disconnected | that sensor invalid, other sensors still work |

## 7. Performance

| Metric | Target | Method |
|--------|--------|--------|
| Overlay paint cost | `< 3 ms/frame` | add temporary `QElapsedTimer` around paint |
| Sensor-to-overlay latency | `< 100 ms` | timestamp at CAN receive and paint |
| CAN stale detection | `300-500 ms` | stop sender and measure UI fallback |
| CPU overhead | `< 5%` additional | compare `top` with camera only vs PDC overlay |

## 8. Test Log Template

| Date | Build | TC-ID | Result | Notes |
|------|-------|-------|--------|-------|
| | | PDC-UNIT-001 | Pass/Fail | |
| | | PDC-UI-001 | Pass/Fail | |
| | | PDC-INT-001 | Pass/Fail | |
| | | PDC-HIL-001 | Pass/Fail | |
