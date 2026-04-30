# PiRacer Head Unit - Architecture Optimization Cycle

## 📋 Document Information
- **Version**: 1.0.0
- **Date**: 2026-02-20
- **Purpose**: Define iterative process: Design → Verify → Test → Critical Evaluation → Optimize

---

## 1. Process Overview

```
                    ┌──────────────────────┐
                    │  ARCHITECTURE_DESIGN  │
                    │  (Architecture Spec)  │
                    └──────────┬───────────┘
                               │
                               ▼
                    ┌──────────────────────┐
                    │  VERIFICATION        │
                    │  (Criteria & Cases)  │
                    └──────────┬───────────┘
                               │
                               ▼
                    ┌──────────────────────┐
                    │  IMPLEMENTATION      │
                    └──────────┬───────────┘
                               │
                               ▼
                    ┌──────────────────────┐
                    │  TESTING             │
                    └──────────┬───────────┘
                               │
                               ▼
                    ┌──────────────────────┐
                    │  CRITICAL_EVALUATION │
                    └──────────┬───────────┘
                               │
                               ▼
                    ┌──────────────────────┐
                    │  OPTIMIZATION        │
                    │  (Architecture Edit) │
                    └──────────┬───────────┘
                               │
                               └───────────────┐
                                               ▼
                                        (Next cycle)
```

---

## 2. Step-by-Step Process

### Step 1: Architecture Design
**Document**: `ARCHITECTURE_DESIGN_EN.md`
- [ ] Requirements → Architecture mapping (REQ-xx)
- [ ] Component and interface definition
- [ ] ADR (decision + rationale)
- [ ] Risk and mitigation
- [ ] NFR targets

### Step 2: Verification Planning
**Document**: `VERIFICATION_EN.md`
- [ ] REQ-ID → VER-ID mapping
- [ ] Unit/Integration/System test case definition
- [ ] Performance/stability metrics
- [ ] Pass criteria
- [ ] Test Execution Log template

### Step 3: Implementation
**Document**: `HEAD_UNIT_DESIGN_EN.md`, code
- [ ] Implement per design
- [ ] Unit test code (TC-Unit)
- [ ] Check alignment with ARCHITECTURE_DESIGN

### Step 4: Testing
**Document**: `VERIFICATION_EN.md` (Test Execution Log)
- [ ] Run VERIFICATION test cases
- [ ] Record results (Pass/Fail/Not Run)
- [ ] Save logs and repro steps on failure
- [ ] Store performance data

### Step 5: Critical Evaluation
**Document**: `CRITICAL_EVALUATION_EN.md`
- [ ] Test Summary
- [ ] Issues (failures, serious issues)
- [ ] Root Cause Analysis
- [ ] Improvements
- [ ] Trade-off re-review
- [ ] Architecture Impact
- [ ] Next cycle recommendations

### Step 6: Optimization
**Document**: `ARCHITECTURE_DESIGN_EN.md`
- [ ] Apply Architecture Impact from CRITICAL_EVALUATION
- [ ] Add/update ADR
- [ ] Update Revisions
- [ ] Adjust VERIFICATION if needed

---

## 3. Cycle Triggers

| Trigger | Action |
|---------|--------|
| **Scheduled** | Weekly or milestone completion → full cycle |
| **Failure** | Critical/High issue → immediate evaluation → optimize |
| **Requirement change** | New requirement → update DESIGN → VERIFICATION |
| **Risk realized** | ADR risk occurs → re-evaluate and optimize |

---

## 4. Document Dependencies

```
ARCHITECTURE_DESIGN_EN.md
    ├──→ VERIFICATION_EN.md (REQ → VER)
    └──→ CRITICAL_EVALUATION_EN.md (test results)
              └──→ ARCHITECTURE_DESIGN_EN.md (Revisions, ADR)
```

---

## 5. Checklist per Cycle

### Before Cycle
- [ ] Confirm current Rev of ARCHITECTURE_DESIGN
- [ ] Review VERIFICATION TC list
- [ ] Prepare test environment (HW, middleware)

### During Cycle
- [ ] If design drift during impl → update doc first
- [ ] Record test results as soon as available

### After Cycle
- [ ] Complete CRITICAL_EVALUATION
- [ ] Update ARCHITECTURE_DESIGN Revisions
- [ ] Define next cycle scope

---

## 6. Version Control

| Document | On change | Rule |
|----------|-----------|------|
| ARCHITECTURE_DESIGN | ADR, Revisions | Increment Rev |
| VERIFICATION | TC add/change | Update version |
| CRITICAL_EVALUATION | Round add | Increment Round |
| OPTIMIZATION_CYCLE | Process change | Update version |

---

*This process maintains design quality and continuously optimizes architecture based on tests.*
