# IPC (VSOMEIP) 동작 검증 가이드

Head Unit와 Instrument Cluster 간 기어 상태 IPC 통신이 제대로 되는지 확인하는 방법입니다.

## 1. 사전 준비

### VSOMEIP 설정 파일

- **Head Unit**: `config/vsomeip_headunit.json`
- **Instrument Cluster**: `instrument_cluster/config/vsomeip_cluster.json`

설정 파일 경로는 `VSOMEIP_CONFIGURATION` 환경 변수로 지정하거나, 실행 디렉터리에 config가 있어야 합니다.

### 실행 순서 (중요)

VSOMEIP에서 **Instrument Cluster**가 `"routing": "InstrumentCluster"`로 라우팅 역할을 하므로, **반드시 Instrument Cluster를 먼저 실행**해야 합니다.

---

## 2. 검증 절차

### 터미널 1: Instrument Cluster (먼저 실행)

```bash
cd /home/seame2026/piracer/Head_Unit_jun/instrument_cluster/build
export VSOMEIP_CONFIGURATION=$(pwd)/../config/vsomeip_cluster.json
./PiRacerDashboard
```

### 터미널 2: Head Unit

```bash
cd /home/seame2026/piracer/Head_Unit_jun/build_ipc/bin
# config는 소스 config/ 또는 make install 후 bin/config/ 에 있음
export VSOMEIP_CONFIGURATION=/home/seame2026/piracer/Head_Unit_jun/config/vsomeip_headunit.json
./hu_shell
```

---

## 3. 동작 확인 방법

### (1) 콘솔 로그로 확인

앱 실행 시 다음 로그가 나오면 IPC 이벤트가 전달되고 있는 것입니다.

**Head Unit에서 기어(P/R/N/D) 터치 시:**
```
[VSomeIP] publishGear: D
[VSomeIP] publishGear: R
...
```

**Instrument Cluster 콘솔에서:**
```
[VSomeIP] gearReceived: D
[VSomeIP] gearReceived: R
...
```

Head Unit에서 기어를 바꿀 때마다 Cluster 쪽에 위 로그가 출력되면 **IPC 동작 확인된 것**입니다.

### (2) 화면으로 확인

- **Head Unit**: 왼쪽 기어 패널에서 P / R / N / D 터치
- **Instrument Cluster**: 중앙/우측 방향 표시(F/P/R)가 기어에 맞게 바뀌는지 확인
  - D 터치 → F(Forward) 표시
  - R 터치 → R(Reverse) 표시
  - P, N 터치 → N(Neutral) 표시

---

## 4. 문제 발생 시 체크리스트

| 현상 | 확인할 것 |
|------|-----------|
| Cluster에 `gearReceived` 로그 안 나옴 | 1) Cluster를 먼저 실행했는지<br>2) `VSOMEIP_CONFIGURATION` 경로가 올바른지<br>3) 방화벽에서 30490, 30509, 30510 포트 차단 여부 |
| `vsomeip` 관련 에러 | `libvsomeip3` 설치 확인: `dpkg -l | grep vsomeip` |
| Config 파일을 못 찾음 | `VSOMEIP_CONFIGURATION`에 절대 경로 사용 권장 |

---

## 5. 게임패드 + 모터 제어

**Instrument Cluster만 실행하면 됩니다.** piracer_bridge에 게임패드+모터 제어가 통합되어 있음.
`run_gamepad_service.sh` 실행 불필요.

- **Cluster 실행** → piracer_bridge가 자동 시작 → 게임패드 동작
- **X**=전진, **B**=후진, **Y**/A=중립 | 스틱=스로틀/스티어링

---

## 6. 빠른 테스트 (한 터미널에서)

**VSOMEIP_CONFIGURATION은 환경 변수라 매 세션마다 사라집니다.** 아래 스크립트 사용 권장.

```bash
# Cluster 먼저 (백그라운드)
./run_cluster.sh &

# 2초 대기 후 Head Unit 실행
sleep 2
./run_headunit.sh
```

또는 수동으로:
```bash
# Cluster 먼저
cd instrument_cluster/build
VSOMEIP_CONFIGURATION=../config/vsomeip_cluster.json ./PiRacerDashboard &

sleep 2
cd ../..
VSOMEIP_CONFIGURATION=$(pwd)/config/vsomeip_headunit.json ./build_ipc/bin/hu_shell
```

Head Unit에서 기어를 바꿀 때마다 두 앱 콘솔에 `[VSomeIP]` 로그가 나오면 IPC가 정상 동작하는 것입니다.
