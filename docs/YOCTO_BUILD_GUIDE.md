# PiRacer Head Unit — Yocto Build & Deployment Guide

> **Audience:** New team members
> **Host Environment:** Ubuntu 24.04 + Docker (Ubuntu 22.04) build environment
> **Target:** Raspberry Pi 4 (aarch64, Yocto Scarthgap 5.0.16)

---

## Table of Contents

1. [Project Structure](#1-project-structure)
2. [Prerequisites](#2-prerequisites)
3. [Docker Build Environment Setup](#3-docker-build-environment-setup)
4. [First Time Only: Initialize Submodules](#4-first-time-only-initialize-submodules)
5. [Building the Image with BitBake](#5-building-the-image-with-bitbake)
6. [Flashing the SD Card](#6-flashing-the-sd-card)
7. [Booting the Raspberry Pi & SSH Access](#7-booting-the-raspberry-pi--ssh-access)
8. [Useful Debug Commands on the Pi](#8-useful-debug-commands-on-the-pi)
9. [Partial Rebuild After Recipe Changes](#9-partial-rebuild-after-recipe-changes)
10. [Key Configuration Files](#10-key-configuration-files)
11. [Bug Fixes Made Today](#11-bug-fixes-made-today)
12. [Remaining Work](#12-remaining-work)
13. [Troubleshooting](#13-troubleshooting)

---

## 1. Project Structure

```
Head_Unit/
├── Head_Unit_jun/          ← Qt5 source code (CMake project)
│   ├── shell/              ← hu_shell: Wayland Compositor + tab UI
│   ├── modules/
│   │   ├── media/          ← Media player module
│   │   ├── youtube/        ← YouTube WebView module
│   │   ├── call/           ← Phone call module
│   │   ├── navigation/     ← OpenStreetMap navigation module
│   │   ├── ambient/        ← Ambient light module
│   │   └── settings/       ← Settings module
│   ├── core/               ← VSomeIP client, interfaces
│   └── instrument_cluster/ ← Instrument cluster (separate image)
│
└── yocto/                  ← Yocto build system
    ├── Dockerfile           ← Ubuntu 22.04 build environment
    ├── docker-build.sh      ← Docker build automation script
    ├── flash.sh             ← SD card flash script
    ├── poky/                ← Yocto Poky (Scarthgap) [submodule]
    ├── meta-openembedded/   ← OE layer collection [submodule]
    ├── meta-raspberrypi/    ← RPi board support [submodule]
    ├── meta-qt5/            ← Qt5 recipes [submodule]
    ├── meta-piracer/        ← Our custom layer
    │   ├── recipes-hu/headunit/         ← headunit build recipe
    │   └── recipes-core/images/         ← image recipe
    └── build-rpi/           ← BitBake build directory
        └── conf/
            ├── local.conf   ← Build settings (threads, PACKAGECONFIG, etc.)
            └── bblayers.conf ← Layer path list
```

### Architecture Overview

```
┌─────────────────────────────────────────────┐
│  hu_shell  (QT_QPA_PLATFORM=eglfs)          │
│  - Wayland Compositor (QtWaylandCompositor)  │
│  - Renders tab bar, gear panel, status bar   │
│  - Launches module processes as children     │
└────────────┬────────────────────────────────┘
             │ Wayland Protocol (wayland-hu socket)
             │ Unix Socket IPC (ModuleBridge)
    ┌────────┴────────────────────────────────┐
    │  Module Processes  (QT_QPA_PLATFORM=wayland)
    │  hu_module_media / hu_module_youtube    │
    │  hu_module_call  / hu_module_navigation │
    │  hu_module_ambient / hu_module_settings │
    └─────────────────────────────────────────┘
             │ vSomeIP (UDP/TCP)
    ┌────────┴────────────────────────────────┐
    │  Instrument Cluster (separate process)  │
    │  Sends: speed, gear, battery data       │
    └─────────────────────────────────────────┘
```

---

## 2. Prerequisites

### Tools Required on Host

```bash
# Docker
sudo apt install docker.io
sudo usermod -aG docker $USER   # requires re-login
newgrp docker                    # or re-login

# SD card flash tool
sudo apt install bmap-tools

# Git
sudo apt install git
```

### Clone the Repository

```bash
git clone <repo-url> Head_Unit
cd Head_Unit
```

---

## 3. Docker Build Environment Setup

> **Why Docker?**
> Ubuntu 24.04 blocks Yocto's `pseudo` (virtual root FS tool) via AppArmor restrictions.
> You must build inside an Ubuntu 22.04 Docker container.

### Build the Docker Image (once)

```bash
cd Head_Unit/yocto

docker build \
  --build-arg UID=$(id -u) \
  --build-arg GID=$(id -g) \
  --build-arg USERNAME=yocto \
  -t yocto-builder:ubuntu22 \
  .
```

Verify:

```bash
docker images | grep yocto
# yocto-builder   ubuntu22   xxxxxxxxxxxx   ...   812MB
```

---

## 4. First Time Only: Initialize Submodules

```bash
cd Head_Unit/yocto

git submodule update --init poky
git submodule update --init meta-openembedded
git submodule update --init meta-raspberrypi
git submodule update --init meta-qt5
```

> **Note:** Without submodules, BitBake will fail to find the layers and error out immediately.

---

## 5. Building the Image with BitBake

### 5-1. Automated Build (Recommended)

```bash
cd Head_Unit/yocto
SKIP_DOCKER_BUILD=1 bash docker-build.sh piracer-hu-image
```

- `SKIP_DOCKER_BUILD=1` — skip Docker image rebuild (use if already built)
- Available targets: `piracer-hu-image` | `piracer-cluster-image` | `headunit`

### 5-2. Manual Build (Full Control)

```bash
# bblayers.conf uses absolute paths starting with /home/seame/SEA_ME/Head_Unit
# The Docker mount path MUST match the host path exactly
docker run --rm \
  --memory=12g --cpus=10 \
  -v /home/seame/SEA_ME/Head_Unit:/home/seame/SEA_ME/Head_Unit \
  -e HOME=/home/yocto \
  yocto-builder:ubuntu22 \
  bash -c '
    cd /home/seame/SEA_ME/Head_Unit/yocto
    source poky/oe-init-build-env build-rpi
    bitbake piracer-hu-image
  '
```

### 5-3. Build Time & Resources

| Item | Value |
|------|-------|
| Full build (first time) | ~3–5 hours (6116 tasks) |
| headunit only rebuild | ~5–10 minutes |
| qtwayland rebuild | ~30–60 minutes |
| RAM recommended | 12 GB+ (set Docker memory limit) |
| Disk space | ~100 GB+ required |
| Thread settings | `BB_NUMBER_THREADS=8`, `PARALLEL_MAKE=-j8` |

> **Warning:** Setting threads too high (e.g. 16×16 = 256 parallel jobs) causes OOM and system freeze.
> Keep `BB_NUMBER_THREADS=8` and `PARALLEL_MAKE=-j8` in `local.conf`.

### Check Build Output

```bash
ls yocto/build-rpi/tmp/deploy/images/raspberrypi4-64/*.wic*
# piracer-hu-image-raspberrypi4-64.rootfs-YYYYMMDDHHMMSS.wic.bz2
# piracer-hu-image-raspberrypi4-64.rootfs-YYYYMMDDHHMMSS.wic.bmap
```

---

## 6. Flashing the SD Card

### 6-1. Identify the SD Card Device

```bash
lsblk -d -o NAME,SIZE,TYPE,MOUNTPOINT | grep -v loop
# sda    119.1G disk          ← SD card (via USB reader)
# nvme0n1 476.9G disk         ← Internal SSD (DO NOT TOUCH)
```

### 6-2. Automated Flash Script

```bash
cd Head_Unit/yocto
bash flash.sh /dev/sda     # change device name to match your setup
```

- The script automatically finds the latest `.wic.bz2` image and flashes it using `bmaptool`
- Type `yes` when prompted — takes ~30–60 seconds

### 6-3. Manual Flash with bmaptool

```bash
# Unmount partitions first
sudo umount /dev/sda1 /dev/sda2

# Flash
sudo bmaptool copy \
  yocto/build-rpi/tmp/deploy/images/raspberrypi4-64/piracer-hu-image-raspberrypi4-64.rootfs.wic.bz2 \
  /dev/sda
```

---

## 7. Booting the Raspberry Pi & SSH Access

### Boot Sequence

1. Insert SD card into RPi4
2. Make sure WiFi AP `SEA:ME WiFi Access` (password: `1fy0u534m3`) is nearby
3. Power on → boot takes ~30 seconds
4. Find the IP address (check router admin page or connect HDMI)

### SSH Access

```bash
# No password required (debug-tweaks image)
ssh root@<Pi IP address>

# Example (IP may change — use hostname if unsure)
ssh root@raspberrypi4-64.lan
# or by IP
ssh root@192.168.86.30
```

> If you see an SSH host key error after reflashing:
> ```bash
> ssh-keygen -R '192.168.86.56'
> ```

### Check WiFi Config

```bash
# On the Pi
cat /etc/wpa_supplicant.conf
cat /etc/network/interfaces
```

---

## 8. Useful Debug Commands on the Pi

### Check Running Processes

```bash
ps aux | grep hu_
# Should show: hu_shell, hu_module_media, hu_module_youtube, etc.
```

### Restart hu_shell with Debug Logs

```bash
# Kill existing process
killall hu_shell 2>/dev/null

# Prepare XDG_RUNTIME_DIR (Wayland socket location)
mkdir -p /run/user/0
chmod 700 /run/user/0

# Run manually with log output
QT_QPA_PLATFORM=eglfs \
XDG_RUNTIME_DIR=/run/user/0 \
QT_FONT_DPI=96 \
VSOMEIP_CONFIGURATION=/usr/bin/config/vsomeip_headunit.json \
/usr/bin/hu_shell 2>&1
```

### Check Qt Plugins

```bash
# Platform plugins (wayland must be present for modules to work)
ls /usr/lib/plugins/platforms/
# libqeglfs.so  libqwayland-egl.so  libqwayland-generic.so  ...

# Wayland compositor server-side plugins (for hu_shell compositor)
ls /usr/lib/plugins/wayland-graphics-integration-server/
```

### SysVinit Service (NOT systemd)

```bash
# Start / Stop / Restart
/etc/init.d/hu-shell start
/etc/init.d/hu-shell stop
/etc/init.d/hu-shell restart
```

### vSomeIP Status

```bash
# In hu_shell logs:
# "[VSomeIP] IC service 0x1234 UNAVAILABLE" → Instrument Cluster is off
# "[VSomeIP] IC service 0x1234 AVAILABLE"   → IC is connected
```

### Installed Binaries

```bash
ls /usr/bin/hu_*
# hu_shell
# hu_module_media
# hu_module_youtube
# hu_module_call
# hu_module_navigation
# hu_module_ambient
# hu_module_settings

ls /usr/bin/config/
# vsomeip_headunit.json
```

---

## 9. Partial Rebuild After Recipe Changes

### After Modifying C++ Source (Head_Unit_jun/)

```bash
docker run --rm \
  --memory=12g --cpus=10 \
  -v /home/seame/SEA_ME/Head_Unit:/home/seame/SEA_ME/Head_Unit \
  -e HOME=/home/yocto \
  yocto-builder:ubuntu22 \
  bash -c '
    cd /home/seame/SEA_ME/Head_Unit/yocto
    source poky/oe-init-build-env build-rpi > /dev/null 2>&1
    bitbake -c cleanall headunit
    bitbake piracer-hu-image
  '
```

### After Modifying Image Recipe Only (IMAGE_INSTALL changes)

```bash
bitbake piracer-hu-image   # fast — no recompile needed
```

### After Modifying qtwayland PACKAGECONFIG

```bash
bitbake -c cleanall qtwayland headunit
bitbake piracer-hu-image
```

### Full Clean (when build is corrupted)

```bash
bitbake -c cleanall <package-name>
# Example: bitbake -c cleanall gcc binutils qtbase
```

---

## 10. Key Configuration Files

### `yocto/build-rpi/conf/local.conf`

```bitbake
# Parallel build (safe for 12–15 GB RAM)
BB_NUMBER_THREADS = "8"
PARALLEL_MAKE = "-j8"

# Enable Wayland + OpenGL distro features
DISTRO_FEATURES:append = " wayland opengl"

# qtbase: enable eglfs + wayland QPA (no X11)
PACKAGECONFIG:append:pn-qtbase = " eglfs linuxfb wayland"
PACKAGECONFIG:remove:pn-qtbase = "examples tests xcb xcb-gl"

# qtwayland: enable client (for modules) + server (for compositor) + EGL
# ⚠️ Using an invalid value like "compositor" disables ALL features silently!
PACKAGECONFIG:pn-qtwayland = "wayland-client wayland-server wayland-egl"
```

### `yocto/meta-piracer/recipes-hu/headunit/headunit_git.bb`

```bitbake
# Use local source tree directly via EXTERNALSRC
EXTERNALSRC = "${TOPDIR}/../../Head_Unit_jun"

# SysVinit init.d auto-start (NOT systemd!)
inherit cmake_qt5 pkgconfig update-rc.d externalsrc
INITSCRIPT_NAME = "hu-shell"
INITSCRIPT_PARAMS = "defaults 99"
```

### `yocto/meta-piracer/recipes-core/images/piracer-hu-image.bb`

```bitbake
IMAGE_INSTALL:append = " \
    headunit \
    qtbase-plugins \
    qtwayland \          # Wayland compositor libraries
    qtwayland-plugins \  # libqwayland-egl.so etc. (client QPA plugins)
    vsomeip3 \
    ttf-dejavu-sans \
    fontconfig \
"
```

### `yocto/meta-piracer/recipes-hu/headunit/files/hu-shell.init`

```sh
# /etc/init.d/hu-shell (SysVinit service script)
export QT_QPA_PLATFORM=eglfs        # hu_shell uses eglfs (direct DRM/KMS)
export XDG_RUNTIME_DIR=/run/user/0   # Wayland socket directory
export VSOMEIP_CONFIGURATION=/usr/bin/config/vsomeip_headunit.json
```

---

## 11. Bug Fixes Made Today

### Bug 1: All Modules Crash — `qt.qpa.plugin: Could not find the Qt platform plugin "wayland"`

**Root Cause:**
`local.conf` had `PACKAGECONFIG:pn-qtwayland = "compositor"`.
`"compositor"` is **not a valid option** in the qtwayland recipe — it simply doesn't exist.
→ BitBake configured qtwayland with **all features disabled** (`-no-feature-wayland-client`, `-no-feature-wayland-server`, etc.)
→ No `libqwayland-egl.so` or `libqwayland-generic.so` was built
→ Module processes crash immediately when trying to use `QT_QPA_PLATFORM=wayland`

**Fix:**

```bitbake
# Before (wrong)
PACKAGECONFIG:pn-qtwayland = "compositor"

# After (correct)
PACKAGECONFIG:pn-qtwayland = "wayland-client wayland-server wayland-egl"
```

| Option | Purpose |
|--------|---------|
| `wayland-server` | Required for hu_shell to use the Qt Wayland Compositor API |
| `wayland-client` | Required for module processes to use `QT_QPA_PLATFORM=wayland` |
| `wayland-egl` | Required for EGL-accelerated rendering in Wayland clients |

**Rebuild command after fix:**

```bash
bitbake -c cleanall qtwayland headunit
bitbake piracer-hu-image
```

**Verification:**

```bash
# Check config.summary after build
cat build-rpi/tmp/work/cortexa72-poky-linux/qtwayland/5.15.13+git/build/config.summary
# Qt Wayland Client ........................ yes  ← must be "yes"
# Qt Wayland Compositor .................... yes  ← must be "yes"
```

---

### Bug 2: hu_shell Did Not Auto-Start (systemd recipe on SysVinit image)

**Root Cause:**
The headunit recipe used `inherit systemd` and installed `hu-shell.service`,
but the Yocto image uses **SysVinit**, not systemd — so the service never started.

**Fix:**
- Replaced `inherit systemd` with `inherit update-rc.d`
- Wrote `hu-shell.init` (SysVinit init.d script) and installed it to `/etc/init.d/`

---

### Bug 3: rootfs File Conflict (interfaces, wpa_supplicant.conf)

**Root Cause:**
The headunit recipe tried to copy `interfaces` and `wpa_supplicant.conf` directly into rootfs,
but conflicted with `init-ifupdown` and `wpa-supplicant` packages owning the same files.

**Fix:**
Moved WiFi config to image recipe using `ROOTFS_POSTPROCESS_COMMAND`:

```bitbake
ROOTFS_POSTPROCESS_COMMAND:append = " setup_wifi_config; "
setup_wifi_config() {
    printf 'ctrl_interface=/var/run/wpa_supplicant\n...\nnetwork={\n    ssid="SEA:ME WiFi Access"\n    psk="1fy0u534m3"\n}\n' \
        > ${IMAGE_ROOTFS}/etc/wpa_supplicant.conf
    chmod 600 ${IMAGE_ROOTFS}/etc/wpa_supplicant.conf
}
```

---

### Bug 4: BitBake OOM / System Freeze

**Root Cause:** `BB_NUMBER_THREADS=16` + `PARALLEL_MAKE=-j16` = 256 parallel processes exceeds available RAM.

**Fix:**
```bitbake
BB_NUMBER_THREADS = "8"
PARALLEL_MAKE = "-j8"
```
Also add `--memory=12g --cpus=10` when running Docker.

---

## 12. Remaining Work

### 🔴 High Priority

| Task | Description |
|------|-------------|
| **Instrument Cluster vSomeIP integration** | Boot the IC, verify vSomeIP communication with Head Unit. Currently hu_shell logs show `IC service 0x1234 UNAVAILABLE`. |
| **Gear panel live data** | After vSomeIP link is up, verify speed/gear/battery data appears in the gear panel overlay. |

### 🟡 Medium Priority

| Task | Description |
|------|-------------|
| **YouTube module** | Add `qtwebengine` to the image. Adds ~2–4 hours to build time and significantly increases image size. |
| **Navigation module** | Same requirement as YouTube — needs `qtwebengine`. Uses OpenStreetMap (no API key needed). |
| **Instrument Cluster image build** | Build `piracer-cluster-image` and flash to a separate SD card for IC hardware. |

### 🟢 Improvements

| Task | Description |
|------|-------------|
| **Externalize WiFi credentials** | SSID and password are currently hardcoded in the recipe. Should be configurable at build time. |
| **Log persistence** | No journald in this image — set up log file output for hu_shell. |
| **Media module testing** | Test actual audio file playback. |

---

## 13. Troubleshooting

### BitBake "No reply from server in 30s"

```bash
# Cause: OOM kill or leftover server process
rm -f build-rpi/bitbake.lock
# Reduce memory usage and retry
```

### Docker layer path error

```
ERROR: The following layer directories do not exist:
ERROR:    /home/seame/SEA_ME/Head_Unit/yocto/poky/meta
```

```bash
# Cause: bblayers.conf uses absolute host paths
# The Docker -v mount path MUST match the host path exactly:
-v /home/seame/SEA_ME/Head_Unit:/home/seame/SEA_ME/Head_Unit  # ✅ correct
-v /home/seame/SEA_ME/Head_Unit:/workspace                     # ❌ wrong
```

### SSH host key error after reflashing

```bash
ssh-keygen -R '192.168.86.30'
ssh root@192.168.86.30
```

### SD card "Device or resource busy" during flash

```bash
sudo umount /media/seame/boot /media/seame/root1
# or
sudo umount /dev/sda1 /dev/sda2
```

### Modules not appearing (check Wayland plugins)

```bash
# On the Pi
ls /usr/lib/plugins/platforms/ | grep wayland
# If libqwayland-egl.so and libqwayland-generic.so are missing
# → fix PACKAGECONFIG:pn-qtwayland and rebuild the image
```

### Verify qtwayland was built correctly after rebuild

```bash
cat build-rpi/tmp/work/cortexa72-poky-linux/qtwayland/5.15.13+git/build/config.summary
# Qt Wayland Client ........................ yes
# Qt Wayland Compositor .................... yes
```

---

## Quick Reference — Frequently Used Commands

```bash
# ── Build ─────────────────────────────────────────────────────────
SKIP_DOCKER_BUILD=1 bash docker-build.sh piracer-hu-image

# ── Partial rebuild (after modifying C++ source) ──────────────────
# Inside Docker container:
bitbake -c cleanall headunit && bitbake piracer-hu-image

# ── Flash SD card ─────────────────────────────────────────────────
bash flash.sh /dev/sda

# ── SSH into the Pi ───────────────────────────────────────────────
ssh root@raspberrypi4-64.lan   # hostname (recommended — works even if IP changes)

# ── Manually run hu_shell on Pi (with logs) ───────────────────────
killall hu_shell 2>/dev/null; sleep 1
mkdir -p /run/user/0 && chmod 700 /run/user/0
QT_QPA_PLATFORM=eglfs XDG_RUNTIME_DIR=/run/user/0 \
VSOMEIP_CONFIGURATION=/usr/bin/config/vsomeip_headunit.json \
/usr/bin/hu_shell 2>&1

# ── Check Wayland plugins on Pi ───────────────────────────────────
ls /usr/lib/plugins/platforms/ | grep wayland

# ── Reset SSH host key ────────────────────────────────────────────
ssh-keygen -R '192.168.86.56'
```
