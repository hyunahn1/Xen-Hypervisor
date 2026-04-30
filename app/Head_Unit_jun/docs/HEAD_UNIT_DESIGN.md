# PiRacer Head Unit - Design Specification

## 📋 Document Information
- **Project Name**: PiRacer Head Unit Qt Application
- **Version**: 1.0.0
- **Date**: 2026-02-20
- **Design Style**: Modern In-Car Infotainment (Spotify/Apple CarPlay inspired)

---

## 1. Architecture Decisions (ADR Summary)

| 항목 | 결정 |
|------|------|
| **기어 제어** | 스로틀(게임패드) + Head Unit 터치 둘 다 사용, 공용 기어 상태 via IPC |
| **IPC** | VSOMEIP (요구사항 준수) |
| **Ambient LED** | 월요일 하드웨어 수령 후 제어 방식 결정 |
| **미디어** | 현대적 세련된 UI, 디테일한 디자인 |
| **네비게이션** | 상단 4탭: 미디어 | 조명 | 기어 | 설정 |

---

## 2. Display & Layout

### 2.1 기본 규격
- **해상도**: 1280 × 720 (가정, 실제 디스플레이 수령 시 조정)
- **방향**: Landscape
- **터치**: 지원 (버튼 최소 터치 영역 44×44pt 권장)

### 2.2 전체 구조

```
┌────────────────────────────────────────────────────────────────────────────────────┐
│  [ 미디어 ]  [ 조명 ]  [ 기어 ]  [ 설정 ]     ← Tab Bar (48px 높이)                  │
├────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                    │
│                                                                                    │
│                     Content Area (탭별 화면)                                        │
│                        1280 × 636 px                                              │
│                                                                                    │
│                                                                                    │
├────────────────────────────────────────────────────────────────────────────────────┤
│  ◉ 35.2 km/h   [기어: D]   ◉ IPC 연결됨   🔊 재생 중: Track Name   ← StatusBar (36px)│
└────────────────────────────────────────────────────────────────────────────────────┘
```

### 2.3 StatusBar (하단 상태 바)

- **높이**: 36px
- **배경**: `#111114` (메인 배경보다 약간 더 어두움)
- **구성** (좌→우):
  - 속도: `◉ 35.2 km/h` (IPC 미연결 시 `✕ --- km/h`)
  - 기어: `[기어: D]` (현재 기어 상태)
  - IPC 상태: `◉ IPC 연결됨` / `⚠ IPC 재연결 중` / `✕ IPC 없음`
  - 현재 재생 중인 곡: `🔊 Track Name` (미디어 재생 시)
- **색상**: Text Muted(기본), Success(연결), Danger(오류), Accent Primary(재생)

---

## 3. Design System

### 3.1 Color Palette

| 용도 | Hex | RGB | 사용처 |
|------|-----|-----|--------|
| **Background Primary** | `#0D0D0F` | (13, 13, 15) | 메인 배경 |
| **Background Secondary** | `#1A1A1E` | (26, 26, 30) | 카드, 패널 |
| **Background Tertiary** | `#252529` | (37, 37, 41) | 호버, 선택됨 |
| **Text Primary** | `#FFFFFF` | (255, 255, 255) | 제목, 주요 텍스트 |
| **Text Secondary** | `#B3B3B7` | (179, 179, 183) | 부제목, 메타 |
| **Text Muted** | `#6E6E73` | (110, 110, 115) | 비활성 |
| **Accent Primary** | `#00D4AA` | (0, 212, 170) | CTA, 재생, 포커스 |
| **Accent Secondary** | `#6366F1` | (99, 102, 241) | 보조 강조 (조명 등) |
| **Progress Fill** | `#00D4AA` | — | 프로그레스 바 채움 |
| **Progress Track** | `#2C2C30` | (44, 44, 48) | 프로그레스 바 배경 |
| **Danger** | `#FF4757` | (255, 71, 87) | 후진(R), 경고 |
| **Success** | `#34C759` | (52, 199, 89) | 전진(F), 확인 |

### 3.2 Typography

| 용도 | Font | Size | Weight | 적용 |
|------|------|------|--------|------|
| **Tab Label** | Inter / SF Pro | 14pt | Semibold 600 | 탭 바 |
| **Section Title** | Inter | 20pt | Bold 700 | 화면 제목 |
| **Card Title** | Inter | 16pt | Semibold 600 | 곡명, 플레이리스트명 |
| **Body** | Inter | 14pt | Regular 400 | 일반 텍스트 |
| **Meta** | Inter | 12pt | Regular 400 | 아티스트, 시간 |
| **Caption** | Inter | 11pt | Regular 400 | 힌트, 부가정보 |

*Inter 없을 경우: Noto Sans KR, Roboto 대체*

### 3.3 Spacing Scale (px)
- **xs**: 4
- **sm**: 8
- **md**: 16
- **lg**: 24
- **xl**: 32
- **2xl**: 48

### 3.4 Border Radius
- **Small**: 8px (버튼, 작은 카드)
- **Medium**: 12px (카드, 패널)
- **Large**: 16px (앨범 아트, 메인 카드)
- **Full**: 9999px (원형, pill 버튼)

### 3.5 Shadows
- **Card**: `0 2px 8px rgba(0,0,0,0.24)`
- **Floating**: `0 4px 20px rgba(0,0,0,0.32)`

---

## 4. Tab Bar (상단 네비게이션)

### 4.1 레이아웃
- **높이**: 48px
- **패딩**: 좌우 24px, 상하 0
- **아이콘 + 텍스트** 또는 **텍스트만** (터치 영역 최소 88×48px)

### 4.2 탭 목록

| 탭 | 아이콘(선택) | 라벨 | 설명 |
|----|--------------|------|------|
| 1 | 🎵 | 미디어 | 음악/비디오 재생 |
| 2 | 💡 | 조명 | Ambient LED 제어 |
| 3 | ⚙ | 기어 | P/R/N/D 표시 및 터치 선택 |
| 4 | 🔧 | 설정 | 캘리브레이션, 테마 등 |

### 4.3 상태
- **기본**: Text Secondary, 배경 없음
- **선택됨**: Accent Primary 텍스트, 하단 2px Accent Primary 언더라인
- **호버**: Text Primary, 배경 Background Tertiary

---

## 5. Media App – 상세 디자인

### 5.1 구조 개요

```
┌─────────────────────────────────────────────────────────────────┐
│  [ 미디어 ]  [ 조명 ]  [ 기어 ]  [ 설정 ]                         │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│   ┌─────────────────────────────────────────────────────────┐   │
│   │              Now Playing Card (메인)                     │   │
│   │  ┌──────────┐                                            │   │
│   │  │          │   Track Name                               │   │
│   │  │  Album   │   Artist Name                              │   │
│   │  │  Art     │                                            │   │
│   │  │ (160×160)│   ━━━━━━━━━━●──────────  1:23 / 3:45      │   │
│   │  │          │                                            │   │
│   │  └──────────┘   [ ⏮ ]  [ ⏯ ]  [ ⏭ ]     🔊 ━━━●━━━    │   │
│   └─────────────────────────────────────────────────────────┘   │
│                                                                 │
│   Recently Played / Playlists                                    │
│   ┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐                  │
│   │ Cover│ │ Cover│ │ Cover│ │ Cover│ │ Cover│   ← 가로 스크롤    │
│   │ 120² │ │ 120² │ │ 120² │ │ 120² │ │ 120² │                  │
│   │ Title│ │ Title│ │ Title│ │ Title│ │ Title│                  │
│   └──────┘ └──────┘ └──────┘ └──────┘ └──────┘                  │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### 5.2 Now Playing Card (재생 중 카드)

#### 5.2.1 컨테이너
- **크기**: Content 영역 너비 × 280px
- **배경**: Background Secondary, 12px radius, shadow
- **패딩**: 24px 전체
- **레이아웃**: 좌측 앨범 아트 | 우측 정보 + 컨트롤

#### 5.2.2 앨범 아트
- **크기**: 160×160px
- **radius**: 12px
- **placeholder**: 그라데이션 `#2C2C30 → #1A1A1E` + 음표 아이콘
- **애니메이션**: 재생 중 subtle rotation (선택) 또는 펄스 글로우

#### 5.2.3 트랙 정보
- **곡명**: 18pt Bold, Text Primary, 최대 1줄 ellipsis
- **아티스트**: 14pt Regular, Text Secondary, 최대 1줄 ellipsis
- **간격**: 4px

#### 5.2.4 프로그레스 바 (Scrubber)
- **높이**: 6px
- **트랙**: Background Tertiary, 6px radius
- **채움**: Accent Primary, 6px radius
- **썸**: 14×14px 원, Accent Primary, 호버 시 18×18
- **터치**: 드래그로 시크 가능
- **시간 표시**: 12pt, Text Muted | "1:23 / 3:45" | Text Muted

#### 5.2.5 재생 컨트롤
- **버튼 크기**: 48×48px (터치 영역)
- **아이콘**: 24×24px, Text Primary
- **재생/일시정지**: 56×56px (중앙만 더 크게)
- **간격**: 16px
- **호버**: Accent Primary tint
- **눌림**: scale 0.95

#### 5.2.6 볼륨 컨트롤
- **위치**: 재생 버튼 우측
- **아이콘**: 스피커 20×20
- **슬라이더**: 가로 120px, 높이 4px
- **0%**: mute 아이콘
- **100%**: speaker max 아이콘

### 5.3 플레이리스트 / 최근 재생 섹션

#### 5.3.1 섹션 헤더
- **라벨**: "최근 재생" 또는 "플레이리스트"
- **스타일**: 14pt Semibold, Text Secondary
- **우측**: "전체 보기" 링크 (12pt, Accent Primary)

#### 5.3.2 트랙/앨범 카드 (가로 스크롤)
- **카드 크기**: 120×120 (커버) + 36 (텍스트 영역)
- **커버**: 120×120, 8px radius
- **제목**: 12pt Semibold, 1줄 ellipsis
- **서브타이틀**: 11pt, Text Muted, 1줄 ellipsis
- **간격**: 12px
- **스크롤**: 수평 QScrollArea, snap to item

#### 5.3.3 빈 상태 (재생 목록 없음)
- **아이콘**: 큰 음표 또는 폴더 (64×64, Text Muted)
- **메시지**: "재생할 미디어가 없습니다"
- **CTA**: "폴더에서 추가" 버튼

### 5.4 비디오 모드
- **전환**: 미디어 탭 내 "음악 | 비디오" 토글
- **비디오**: 전체 Content 영역 재생, 컨트롤 오버레이 (호버/터치 시 표시)
- **컨트롤**: 하단 60px, 반투명 배경, 프로그레스 + 재생/일시정지/볼륨

### 5.5 애니메이션
- **탭 전환**: 200ms ease-out, fade + 약간의 slide
- **카드 호버**: 150ms, scale 1.02 또는 밝기 증가
- **버튼 프레스**: 100ms, scale 0.95
- **프로그레스 바**: 부드러운 interpolation (100ms)

---

## 6. Gear Screen

### 6.1 레이아웃
- **현재 기어 표시**: 대형 텍스트 (72pt Bold), 중앙 상단
- **P/R/N/D 버튼**: 2×2 그리드 또는 가로 4개
- **버튼 크기**: 120×80px
- **색상**: P=Gray, R=Red, N=Yellow, D=Green
- **선택됨**: 테두리 3px Accent Primary, 배경 약간 밝게

### 6.2 입력 소스 표시
- 작은 라벨: "스로틀" 또는 "터치" (마지막 변경 소스)

---

## 7. Ambient Lighting Screen

### 7.1 기본 구조 (하드웨어 확정 전)
- **프리셋**: 자동/밤/주간/커스텀
- **컬러 피커**: Hue 링 + 밝기 슬라이더
- **즐겨찾기**: 저장된 색 4–6개
- *월요일 하드웨어 수령 후 구체화*

---

## 8. 설정 화면

- **목록 형태**: 제목 + 우측 화살표/스위치
- 항목: 단위 (km/h ↔ mph), 테마, 캘리브레이션, 정보

---

## 9. 구현 참고 (Qt)

- **스타일**: QSS (Qt Style Sheets) 또는 QML
- **미디어**: Qt Multimedia (QMediaPlayer, QAudioOutput)
- **스크롤**: QScrollArea + QWidget flow
- **아이콘**: Material Icons, Phosphor, 또는 SVG

---

## 10. 파일 구조

*ARCHITECTURE_DESIGN.md Section 7과 동기화된 최종 구조*

```
Head_Unit/
├── docs/
│   ├── ARCHITECTURE_DESIGN.md
│   ├── ARCHITECTURE.md
│   ├── HEAD_UNIT_DESIGN.md    ← 본 문서
│   ├── VERIFICATION.md
│   ├── CRITICAL_EVALUATION.md
│   └── OPTIMIZATION_CYCLE.md
│
├── src/
│   ├── main.cpp                      ← 앱 진입점, QApplication, DISPLAY=:1 설정
│   ├── MainWindow.h / MainWindow.cpp ← TabBar + Content 영역 + StatusBar
│   │
│   ├── widgets/
│   │   ├── TabBar.h / TabBar.cpp            ← 상단 4탭 네비게이션
│   │   └── StatusBar.h / StatusBar.cpp      ← 하단: 속도·IPC 상태·에러 표시
│   │
│   ├── screens/
│   │   ├── MediaScreen.h / MediaScreen.cpp  ← 미디어 탭 (NowPlayingCard 포함)
│   │   ├── AmbientScreen.h / AmbientScreen.cpp  ← 조명 탭
│   │   ├── GearScreen.h / GearScreen.cpp    ← 기어 탭 (P/R/N/D 터치 버튼)
│   │   └── SettingsScreen.h / SettingsScreen.cpp
│   │
│   ├── ipc/
│   │   ├── IVehicleDataProvider.h           ← 추상 인터페이스 (Mock 교체 가능)
│   │   ├── VSomeIPClient.h / VSomeIPClient.cpp  ← VSOMEIP 구독·발행
│   │   └── GearStateManager.h / GearStateManager.cpp  ← Last-Write-Wins
│   │
│   ├── media/
│   │   ├── MediaPlayer.h / MediaPlayer.cpp  ← QMediaPlayer 래퍼
│   │   └── PlaylistModel.h / PlaylistModel.cpp
│   │
│   └── led/
│       ├── ILedController.h                 ← 추상 인터페이스
│       ├── PwmLedController.h / PwmLedController.cpp    ← PWM 구현
│       └── WS2812LedController.h / WS2812LedController.cpp
│
├── config/
│   └── vsomeip_headunit.json               ← VSOMEIP 설정
│
├── resources/
│   ├── headunit.qrc
│   ├── icons/                              ← SVG 아이콘
│   └── fonts/                             ← Inter, Roboto Mono
│
└── CMakeLists.txt (또는 headunit.pro)
```

---

**승인**: ________________  
**날짜**: 2026-02-20
