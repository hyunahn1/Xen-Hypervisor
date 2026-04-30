# PiRacer Head Unit - Architecture Optimization Cycle

## 📋 Document Information
- **Version**: 1.0.0
- **Date**: 2026-02-20
- **목적**: 설계 → 검증 → 테스트 → 비판적 평가 → 최적화의 반복 프로세스 정의

---

## 1. Process Overview

```
                    ┌──────────────────────┐
                    │  ARCHITECTURE_DESIGN  │
                    │  (아키텍처 설계서)    │
                    └──────────┬───────────┘
                               │
                               ▼
                    ┌──────────────────────┐
                    │  VERIFICATION        │
                    │  (검증 기준·케이스)   │
                    └──────────┬───────────┘
                               │
                               ▼
                    ┌──────────────────────┐
                    │  IMPLEMENTATION      │
                    │  (구현)              │
                    └──────────┬───────────┘
                               │
                               ▼
                    ┌──────────────────────┐
                    │  TESTING             │
                    │  (테스트 실행)        │
                    └──────────┬───────────┘
                               │
                               ▼
                    ┌──────────────────────┐
                    │  CRITICAL_EVALUATION │
                    │  (비판적 평가)        │
                    └──────────┬───────────┘
                               │
                               ▼
                    ┌──────────────────────┐
                    │  OPTIMIZATION        │
                    │  (아키텍처 수정)      │
                    └──────────┬───────────┘
                               │
                               └───────────────┐
                                               │
                                               ▼
                                        (다음 사이클)
```

---

## 2. Step-by-Step Process

### Step 1: Architecture Design
**문서**: `ARCHITECTURE_DESIGN.md`

- [ ] 요구사항 → 아키텍처 매핑 (REQ-xx)
- [ ] 컴포넌트·인터페이스 정의
- [ ] ADR 작성 (결정 + rationale)
- [ ] 리스크 및 완화 전략
- [ ] NFR 목표 설정

**출력**: 설계서 초안, VERIFICATION.md 입력용 기준

---

### Step 2: Verification Planning
**문서**: `VERIFICATION.md`

- [ ] REQ-ID → VER-ID 매핑
- [ ] 단위/통합/시스템 테스트 케이스 정의
- [ ] 성능·안정성 기준 수치화
- [ ] 통과 조건 명시
- [ ] Test Execution Log 템플릿 준비

**출력**: 검증 계획서, 테스트 시나리오

---

### Step 3: Implementation
**문서**: `HEAD_UNIT_DESIGN.md`, 코드

- [ ] 설계서대로 구현
- [ ] 단위 테스트 코드 (TC-Unit)
- [ ] 변경 시 ARCHITECTURE_DESIGN과 불일치 확인

**출력**: 구현물, 단위 테스트 결과

---

### Step 4: Testing
**문서**: `VERIFICATION.md` (Test Execution Log)

- [ ] VERIFICATION.md 테스트 케이스 실행
- [ ] 결과 기록 (Pass/Fail/미실행)
- [ ] 실패 시 로그·재현 방법 저장
- [ ] 성능 측정 데이터 저장

**출력**: 테스트 결과 요약, 통과율

---

### Step 5: Critical Evaluation
**문서**: `CRITICAL_EVALUATION.md`

- [ ] Test Summary 작성
- [ ] Issues (실패·심각 이슈) 나열
- [ ] Root Cause Analysis
- [ ] Improvements (개선점) 나열
- [ ] Trade-off 재검토
- [ ] Architecture Impact 정리
- [ ] 다음 사이클 권장사항

**출력**: 비판적 평가 보고서

---

### Step 6: Optimization
**문서**: `ARCHITECTURE_DESIGN.md`

- [ ] CRITICAL_EVALUATION의 Architecture Impact 반영
- [ ] ADR 추가/수정
- [ ] Revisions 섹션 업데이트
- [ ] 필요 시 VERIFICATION.md 보완

**출력**: 갱신된 아키텍처 설계서

---

## 3. Cycle Triggers

| 트리거 | 액션 |
|--------|------|
| **정기** | 주 1회 또는 마일스톤 완료 시 전체 사이클 |
| **실패** | Critical/High 이슈 발견 시 즉시 비판적 평가 → 최적화 |
| **요구 변경** | 신규 요구 시 ARCHITECTURE_DESIGN 수정 → VERIFICATION 업데이트 |
| **리스크 발생** | ADR의 리스크 실현 시 재평가·최적화 |

---

## 4. Document Dependencies

```
ARCHITECTURE_DESIGN.md
    │
    ├──→ VERIFICATION.md (REQ → VER 매핑)
    │
    └──→ CRITICAL_EVALUATION.md (테스트 결과 입력)
              │
              └──→ ARCHITECTURE_DESIGN.md (Revisions, ADR 업데이트)
```

---

## 5. Checklist per Cycle

### Before Cycle
- [ ] ARCHITECTURE_DESIGN.md 현재 Rev 확인
- [ ] VERIFICATION.md TC 목록 확인
- [ ] 테스트 환경 준비 (하드웨어, 미들웨어)

### During Cycle
- [ ] 구현 중 설계 이탈 시 문서 우선 수정
- [ ] 테스트 결과 즉시 기록

### After Cycle
- [ ] CRITICAL_EVALUATION 완료
- [ ] ARCHITECTURE_DESIGN Revisions 업데이트
- [ ] 다음 사이클 스코프 확정

---

## 6. Version Control

| 문서 | 변경 시 | 규칙 |
|------|---------|------|
| ARCHITECTURE_DESIGN | ADR, Revisions | Rev 증가 |
| VERIFICATION | TC 추가/수정 | 버전 메타 업데이트 |
| CRITICAL_EVALUATION | 라운드 추가 | Round 번호 증가 |
| OPTIMIZATION_CYCLE | 프로세스 변경 | 버전 메타 업데이트 |

---

*이 프로세스는 설계의 품질을 유지하고, 테스트 기반으로 아키텍처를 지속적으로 최적화하기 위한 것입니다.*
