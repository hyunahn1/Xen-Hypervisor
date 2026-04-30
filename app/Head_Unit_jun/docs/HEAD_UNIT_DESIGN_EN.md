# PiRacer Head Unit - Design Specification

## 📋 Document Information
- **Project**: PiRacer Head Unit Qt Application
- **Version**: 1.0.0
- **Date**: 2026-02-20
- **Design Style**: Modern In-Car Infotainment (Spotify/Apple CarPlay inspired)

---

## 1. Architecture Decisions (ADR Summary)

| Item | Decision |
|------|----------|
| **Gear control** | Throttle (gamepad) + Head Unit touch both; shared gear state via IPC |
| **IPC** | VSOMEIP (requirement) |
| **Ambient LED** | Control method TBD after hardware receipt (Monday) |
| **Media** | Modern, polished UI with detailed design |
| **Navigation** | Top 4 tabs: Media | Lighting | Gear | Settings |

---

## 2. Display & Layout

### 2.1 Base Specs
- **Resolution**: 1280 × 720 (assumed; adjust per actual display)
- **Orientation**: Landscape
- **Touch**: Supported (min touch target 44×44pt)

### 2.2 Overall Structure

```
┌────────────────────────────────────────────────────────────────────────────────────┐
│  [ Media ]  [ Lighting ]  [ Gear ]  [ Settings ]     ← Tab Bar (48px)                │
├────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                    │
│                     Content Area (tab screens)                                      │
│                        1280 × 636 px                                              │
│                                                                                    │
├────────────────────────────────────────────────────────────────────────────────────┤
│  ◉ 35.2 km/h   [Gear: D]   ◉ IPC Connected   🔊 Playing: Track Name  ← StatusBar 36px│
└────────────────────────────────────────────────────────────────────────────────────┘
```

### 2.3 StatusBar (Bottom)

- **Height**: 36px
- **Background**: `#111114`
- **Items** (L→R):
  - Speed: `◉ 35.2 km/h` (IPC off: `✕ --- km/h`)
  - Gear: `[Gear: D]`
  - IPC: `◉ IPC Connected` / `⚠ IPC Reconnecting` / `✕ IPC Unavailable`
  - Playing: `🔊 Track Name`
- **Colors**: Text Muted (default), Success (connected), Danger (error), Accent Primary (playing)

---

## 3. Design System

### 3.1 Color Palette

| Use | Hex | RGB | Usage |
|-----|-----|-----|-------|
| **Background Primary** | `#0D0D0F` | (13, 13, 15) | Main background |
| **Background Secondary** | `#1A1A1E` | (26, 26, 30) | Cards, panels |
| **Background Tertiary** | `#252529` | (37, 37, 41) | Hover, selected |
| **Text Primary** | `#FFFFFF` | (255, 255, 255) | Titles, main text |
| **Text Secondary** | `#B3B3B7` | (179, 179, 183) | Subtitles, meta |
| **Text Muted** | `#6E6E73` | (110, 110, 115) | Inactive |
| **Accent Primary** | `#00D4AA` | (0, 212, 170) | CTA, play, focus |
| **Accent Secondary** | `#6366F1` | (99, 102, 241) | Secondary accent |
| **Progress Fill** | `#00D4AA` | — | Progress bar fill |
| **Progress Track** | `#2C2C30` | (44, 44, 48) | Progress bar track |
| **Danger** | `#FF4757` | (255, 71, 87) | Reverse (R), warning |
| **Success** | `#34C759` | (52, 199, 89) | Forward (F), confirm |

### 3.2 Typography

| Use | Font | Size | Weight |
|-----|------|------|--------|
| **Tab Label** | Inter / SF Pro | 14pt | Semibold 600 |
| **Section Title** | Inter | 20pt | Bold 700 |
| **Card Title** | Inter | 16pt | Semibold 600 |
| **Body** | Inter | 14pt | Regular 400 |
| **Meta** | Inter | 12pt | Regular 400 |
| **Caption** | Inter | 11pt | Regular 400 |

*Fallback: Noto Sans KR, Roboto*

### 3.3 Spacing (px)
xs: 4, sm: 8, md: 16, lg: 24, xl: 32, 2xl: 48

### 3.4 Border Radius
Small: 8px, Medium: 12px, Large: 16px, Full: 9999px

### 3.5 Shadows
Card: `0 2px 8px rgba(0,0,0,0.24)`, Floating: `0 4px 20px rgba(0,0,0,0.32)`

---

## 4. Tab Bar

### 4.1 Layout
- Height: 48px, Padding: 24px L/R
- Icon + text or text only (min touch 88×48px)

### 4.2 Tabs

| Tab | Icon | Label | Description |
|-----|------|-------|-------------|
| 1 | 🎵 | Media | Audio/video playback |
| 2 | 💡 | Lighting | Ambient LED control |
| 3 | ⚙ | Gear | P/R/N/D display and touch |
| 4 | 🔧 | Settings | Calibration, theme |

### 4.3 States
- Default: Text Secondary, no bg
- Selected: Accent Primary text, 2px underline
- Hover: Text Primary, bg Tertiary

---

## 5. Media App – Detail

### 5.1 Now Playing Card
- Size: Content width × 280px
- Layout: Album art (160×160) | Track info + controls
- Progress bar: 6px, scrubber, time display
- Controls: 48×48 (56×56 play/pause), volume slider 120px

### 5.2 Playlist / Recently Played
- Horizontal scroll, 120×120 cover cards + 36px text
- Empty state: large icon, "No media to play", "Add from folder" CTA

### 5.3 Video Mode
- Media tab toggle: Music | Video
- Video: full content area, overlay controls on touch

### 5.4 Animation
- Tab switch: 200ms ease-out, fade + slide
- Card hover: 150ms, scale 1.02
- Button press: 100ms, scale 0.95

---

## 6. Gear Screen

- Current gear: 72pt Bold, center top
- P/R/N/D buttons: 2×2 grid or horizontal 4
- Size: 120×80px
- Colors: P=Gray, R=Red, N=Yellow, D=Green
- Selected: 3px Accent Primary border
- Input source label: "Throttle" or "Touch"

---

## 7. Ambient Lighting Screen

- Presets: Auto / Night / Day / Custom
- Color picker: Hue ring + brightness slider
- Favorites: 4–6 saved colors
- *To detail after hardware receipt*

---

## 8. Settings Screen

- List: title + arrow/switch
- Items: unit (km/h ↔ mph), theme, calibration, info

---

## 9. Implementation (Qt)

- Style: QSS or QML
- Media: Qt Multimedia (QMediaPlayer, QAudioOutput)
- Scroll: QScrollArea + QWidget flow
- Icons: Material, Phosphor, or SVG

---

## 10. File Structure

*Synced with ARCHITECTURE_DESIGN_EN.md Section 7*

```
Head_Unit/
├── docs/
├── src/ main.cpp, MainWindow.*, widgets/, screens/, ipc/, media/, led/
├── config/ vsomeip_headunit.json
├── resources/ headunit.qrc, icons/, fonts/
└── CMakeLists.txt (or headunit.pro)
```

---

**Approval**: ________________  
**Date**: 2026-02-20
