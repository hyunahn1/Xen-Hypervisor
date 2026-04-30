# PiRacer Head Unit - Verification Plan

## 📋 Document Information
- **Version**: 1.0.0
- **Date**: 2026-02-20
- **Traceability**: ARCHITECTURE_DESIGN.md REQ-ID → VER-ID 매핑

---

## 1. Verification Strategy

### 1.1 V-Model

```
ARCHITECTURE_DESIGN.md          VERIFICATION.md (본 문서)
       │                                 │
       ▼                                 ▼
   [설계 단계]  ←──── 검증 기준 정의 ────→  [검증 단계]
       │                                 │
       ▼                                 ▼
   [구현]        ←──── 테스트 실행 ──────→  [테스트]
       │                                 │
       ▼                                 ▼
   [배포]        ←──── 비판적 평가 ──────→  [CRITICAL_EVALUATION.md]
       │                                 │
       └─────── 최적화 사이클 ───────────┘
```

### 1.2 Verification Levels

| Level | 범위 | 목적 |
|-------|------|------|
| **VER-Unit** | 개별 클래스/함수 | 로직 정확성 |
| **VER-Integ** | 모듈 간 통신 (IPC, LED) | 인터페이스 준수 |
| **VER-Sys** | 전체 시스템 | 요구사항 충족 |
| **VER-Perf** | 성능, 안정성 | NFR 충족 |
| **VER-User** | UI/UX | 사용성 |

---

## 2. Requirements → Verification Mapping

| REQ-ID | 요구사항 | VER-ID | 검증 방법 | 통과 기준 |
|--------|----------|--------|-----------|-----------|
| REQ-01 | Head Unit Qt 앱, Cluster 동시 실행 | VER-01 | 시스템 기동, 프로세스 확인 | 두 앱 정상 실행, 크래시 없음 |
| REQ-02 | Ambient lighting 제어 | VER-02 | LED 색/밝기 변경, 하드웨어 확인 | LED 반응, 오류 없음 |
| REQ-03 | 기어 표시 | VER-03 | 기어 변경 시 UI 업데이트 | 200ms 이내 반영 |
| REQ-04 | 미디어 재생·제어 (음악/비디오) | VER-04 | 재생/일시정지/볼륨, 시크, 비디오 | 모든 컨트롤 동작 |
| REQ-05 | IPC (VSOMEIP) | VER-05 | Cluster↔HU 데이터 교환 | 속도·기어 동기화 |
| REQ-06 | CAN 속도 데이터 | VER-06 | Cluster CAN 수신, IPC 전달, StatusBar 표시 | 데이터 경로 정상 |
| REQ-07 | Yocto | VER-07 | 이미지 빌드, 앱 실행 | TC-YOCTO 전체 통과 |
| REQ-08 | 기어 터치 제어 | VER-08 | HU 터치 → 기어 변경 | Cluster에 반영, 기어 의도 Python에 전달 |
| REQ-09 | 기어 게임패드 버튼 제어 | VER-09 | 게임패드 버튼 → 기어 변경 | HU·Cluster 모두 반영 |

---

## 3. Unit Verification

### 3.1 IPC Client (VSOMEIP)

| TC-ID | 테스트 내용 | 입력 | 기대 출력 | 통과 조건 |
|-------|-------------|------|-----------|-----------|
| TC-IPC-001 | speed 이벤트 수신 | Mock 15.0 km/h | callback 호출, 값 15.0 | 값 일치 |
| TC-IPC-002 | gear 이벤트 수신 | Mock "D" | callback 호출, 값 "D" | 값 일치 |
| TC-IPC-003 | gear publish | "R" 터치 | VSOMEIP로 publish | 전송 확인 |
| TC-IPC-004 | 연결 끊김 | Broker 중지 | 재연결 시도, 에러 처리 | 크래시 없음 |

### 3.2 Media Player

| TC-ID | 테스트 내용 | 통과 조건 |
|-------|-------------|-----------|
| TC-MED-001 | MP3 재생 | 5초 재생, 오류 없음 |
| TC-MED-002 | 일시정지/재개 | 상태 전환 정확 |
| TC-MED-003 | 시크 | 위치 변경 반영 |
| TC-MED-004 | 볼륨 0~100% | 출력 변화 |
| TC-MED-005 | 비디오 파일 재생 (MP4) | 영상+음성 재생, 오류 없음 |
| TC-MED-006 | 비디오 풀스크린 컨트롤 오버레이 | 터치 시 컨트롤 표시/숨김 |
| TC-MED-007 | 비디오 재생 중 CPU 사용 | 재생 중 CPU < 50% (RPi4 기준) |
| TC-MED-008 | 지원 안 되는 포맷 처리 | 건너뜀, 크래시 없음 |

### 3.3 Gear Logic

| TC-ID | 테스트 내용 | 입력 | 기대 | 통과 조건 |
|-------|-------------|------|------|-----------|
| TC-GEAR-001 | Last-Write-Wins | 스로틀 F → 터치 R | R | 터치 우선 |
| TC-GEAR-002 | Last-Write-Wins | 터치 R → 스로틀 F | F | 스로틀 우선 |
| TC-GEAR-003 | 유효하지 않은 값 | "X" | 이전 값 유지 | 필터링 |

### 3.4 LED Controller (인터페이스)

| TC-ID | 테스트 내용 | 통과 조건 |
|-------|-------------|-----------|
| TC-LED-001 | setColor(255,0,0) | Mock 호출 확인 |
| TC-LED-002 | setBrightness(80) | Mock 호출 확인 |
| TC-LED-003 | 초기화 실패 | 에러 반환, 크래시 없음 |

---

## 4. Integration Verification

### 4.1 Cluster ↔ Head Unit IPC

| TC-ID | 테스트 내용 | 절차 | 통과 조건 |
|-------|-------------|------|-----------|
| TC-INT-001 | 속도 동기화 | Cluster에 속도 입력 → HU 수신 | 500ms 이내 동기화 |
| TC-INT-002 | 기어 동기화 (Cluster→HU) | Cluster 기어 변경 | HU 화면 반영 |
| TC-INT-003 | 기어 동기화 (HU→Cluster) | HU 터치 R | Cluster 화면 R |
| TC-INT-004 | 양방향 동시 변경 | 양쪽 동시 입력 | Last-Write-Wins, 일관성 |

### 4.2 CAN → Cluster → IPC

| TC-ID | 테스트 내용 | 통과 조건 |
|-------|-------------|-----------|
| TC-INT-005 | Arduino CAN → Cluster | Cluster 속도 표시 |
| TC-INT-006 | Cluster → IPC → HU | HU에서 속도 수신 (표시 선택) |

### 4.3 LED 제어

| TC-ID | 테스트 내용 | 통과 조건 |
|-------|-------------|-----------|
| TC-INT-007 | HU 색상 선택 → LED | 실제 LED 색 변경 |
| TC-INT-008 | 밝기 슬라이더 → LED | LED 밝기 변화 |

---

## 5. System Verification

### 5.1 Functionality

| TC-ID | 시나리오 | 단계 | 통과 조건 |
|-------|----------|------|-----------|
| TC-SYS-001 | 미디어 전체 플로우 | 파일 선택→재생→일시정지→볼륨→다음곡 | 모든 단계 정상 |
| TC-SYS-002 | 기어 전체 플로우 | 스로틀 F → 터치 R → 터치 D | Cluster·HU 모두 반영 |
| TC-SYS-003 | 조명 전체 플로우 | 색 선택→저장→프리셋 변경 | LED 반응, 저장 확인 |
| TC-SYS-004 | 탭 전환 | 4탭 순차 터치 | 전환 정상, 상태 유지 |

### 5.2 Multi-Display

| TC-ID | 테스트 내용 | 통과 조건 |
|-------|-------------|-----------|
| TC-SYS-005 | Cluster DSI + HU HDMI | 각 디스플레이에 올바른 앱 표시 |
| TC-SYS-006 | 전원 재부팅 | 두 앱 자동 기동 |

---

## 6. Performance Verification

| TC-ID | 항목 | 목표 | 측정 방법 | 통과 조건 |
|-------|------|------|-----------|-----------|
| TC-PERF-001 | UI 응답 | < 100ms | 터치→화면 반응 시간 | 평균 < 100ms |
| TC-PERF-002 | IPC 지연 | < 50ms | 이벤트 발행→수신 | 50ms 이내 |
| TC-PERF-003 | CPU (HU) | < 25% | top, 1분 평균 | 25% 이하 |
| TC-PERF-004 | 메모리 (HU) | < 150MB | /proc 또는 valgrind | 150MB 이하 |
| TC-PERF-005 | 미디어 재생 CPU | < 15% | 재생 중 측정 | 15% 이하 |

---

## 7. Stability Verification

| TC-ID | 테스트 내용 | 시간/반복 | 통과 조건 |
|-------|-------------|-----------|-----------|
| TC-STAB-001 | 장시간 실행 | 2시간 | 크래시·프리징 없음, 메모리 증가 < 10MB |
| TC-STAB-002 | IPC 단절 | Broker 재시작 5회 | 자동 재연결, 데이터 복구 |
| TC-STAB-003 | 미디어 반복 | 100곡 연속 재생 | 오류 없음 |
| TC-STAB-004 | 탭 빠른 전환 | 1분간 30회 전환 | 크래시 없음 |

---

## 8. Usability Verification

| TC-ID | 항목 | 방법 | 통과 조건 |
|-------|------|------|-----------|
| TC-UI-001 | 탭 가독성 | 3명 평가 (1-5점) | 평균 ≥ 4 |
| TC-UI-002 | 미디어 컨트롤 직관성 | 첫 사용자 재생/일시정지 | 90% 성공 |
| TC-UI-003 | 기어 버튼 터치 영역 | 44×44pt 최소 | 모든 버튼 준수 |
| TC-UI-004 | 햇빛 가독성 | 실외 확인 | 텍스트 읽기 가능 |

---

## 8a. Yocto Verification

| TC-ID | 테스트 내용 | 절차 | 통과 조건 |
|-------|-------------|------|-----------|
| TC-YOCTO-001 | Yocto 이미지 빌드 | `bitbake piracer-image` 실행 | 빌드 오류 없음, 이미지 생성 |
| TC-YOCTO-002 | RPi 플래시·부팅 | 이미지 SD 카드 기록, RPi 부팅 | 정상 부팅, 두 앱 자동 기동 |
| TC-YOCTO-003 | VSOMEIP 패키지 포함 | `bitbake -e` 또는 이미지 내 확인 | vsomeip 라이브러리 포함 |
| TC-YOCTO-004 | Qt Multimedia 패키지 포함 | 이미지 내 Qt 라이브러리 확인 | qtmultimedia 포함 |
| TC-YOCTO-005 | systemd 서비스 자동 기동 | 부팅 후 `systemctl status` 확인 | 4개 서비스 active(running) |
| TC-YOCTO-006 | CAN 인터페이스 초기화 | 부팅 후 `ip link show can0` | can0 UP, bitrate 500000 |
| TC-YOCTO-007 | vsomeip.json 배포 | `/etc/vsomeip/` 경로 확인 | 두 설정 파일 존재 |

---

## 9. Test Execution Log (템플릿)

| TC-ID | 실행일 | 결과 | 비고 |
|-------|--------|------|------|
| TC-IPC-001 | | Pass/Fail | |
| TC-IPC-002 | | Pass/Fail | |
| TC-IPC-003 | | Pass/Fail | |
| TC-IPC-004 | | Pass/Fail | |
| TC-MED-001 | | Pass/Fail | |
| TC-MED-002 | | Pass/Fail | |
| TC-MED-003 | | Pass/Fail | |
| TC-MED-004 | | Pass/Fail | |
| TC-MED-005 | | Pass/Fail | |
| TC-MED-006 | | Pass/Fail | |
| TC-MED-007 | | Pass/Fail | |
| TC-MED-008 | | Pass/Fail | |
| TC-GEAR-001 | | Pass/Fail | |
| TC-GEAR-002 | | Pass/Fail | |
| TC-GEAR-003 | | Pass/Fail | |
| TC-LED-001 | | Pass/Fail | |
| TC-LED-002 | | Pass/Fail | |
| TC-LED-003 | | Pass/Fail | |
| TC-INT-001 | | Pass/Fail | |
| TC-INT-002 | | Pass/Fail | |
| TC-INT-003 | | Pass/Fail | |
| TC-INT-004 | | Pass/Fail | |
| TC-INT-005 | | Pass/Fail | |
| TC-INT-006 | | Pass/Fail | |
| TC-INT-007 | | Pass/Fail | |
| TC-INT-008 | | Pass/Fail | |
| TC-SYS-001 | | Pass/Fail | |
| TC-SYS-002 | | Pass/Fail | |
| TC-SYS-003 | | Pass/Fail | |
| TC-SYS-004 | | Pass/Fail | |
| TC-SYS-005 | | Pass/Fail | |
| TC-SYS-006 | | Pass/Fail | |
| TC-PERF-001 | | Pass/Fail | |
| TC-PERF-002 | | Pass/Fail | |
| TC-PERF-003 | | Pass/Fail | |
| TC-PERF-004 | | Pass/Fail | |
| TC-PERF-005 | | Pass/Fail | |
| TC-STAB-001 | | Pass/Fail | |
| TC-STAB-002 | | Pass/Fail | |
| TC-STAB-003 | | Pass/Fail | |
| TC-STAB-004 | | Pass/Fail | |
| TC-YOCTO-001 | | Pass/Fail | |
| TC-YOCTO-002 | | Pass/Fail | |
| TC-YOCTO-003 | | Pass/Fail | |
| TC-YOCTO-004 | | Pass/Fail | |
| TC-YOCTO-005 | | Pass/Fail | |
| TC-YOCTO-006 | | Pass/Fail | |
| TC-YOCTO-007 | | Pass/Fail | |
| TC-UI-001 | | Pass/Fail | |
| TC-UI-002 | | Pass/Fail | |
| TC-UI-003 | | Pass/Fail | |
| TC-UI-004 | | Pass/Fail | |

---

## 10. Verification Summary

### 10.1 Pass Criteria (전체 통과 조건)
- **Unit**: 모든 TC-Unit Pass
- **Integration**: 모든 TC-INT Pass
- **System**: 모든 TC-SYS Pass
- **Performance**: TC-PERF 4/5 이상 Pass
- **Stability**: TC-STAB 3/4 이상 Pass
- **Usability**: TC-UI 3/4 이상 Pass

### 10.2 Traceability to Critical Evaluation
- 미통과 항목 → CRITICAL_EVALUATION.md **Issues** 섹션에 기재
- 통과하였으나 개선 여지 → **Improvements** 섹션에 기재

---

*검증 결과는 CRITICAL_EVALUATION.md 비판적 평가에 입력되어 아키텍처 최적화에 활용됩니다.*
