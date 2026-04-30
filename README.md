# Xen-Hypervisor

Lightweight Xen architecture workspace for the PiRacer Head Unit project.

This repository intentionally carries only the source code, project Yocto layer,
and architecture documents. Heavy Yocto build outputs, downloads, and sstate
caches are not copied.

## Layout

```text
.
├── app/Head_Unit_jun/       # Head Unit + Instrument Cluster source payload
├── docs/xen/                # Xen architecture plan
├── docs/ota/                # OTA/Xen compatibility contract
├── yocto/
│   ├── docker-build.sh      # Docker-based BitBake wrapper
│   ├── flash.sh             # SD card flashing helper
│   ├── Dockerfile           # Yocto build container
│   └── meta-piracer/        # Project layer and image recipes
└── docs/YOCTO_BUILD_GUIDE.md
```

## Required Yocto Layers

The large upstream layers are not stored in this repository. Clone or add them
under `yocto/` before building:

```bash
cd yocto
git clone -b scarthgap git://git.yoctoproject.org/poky
git clone -b scarthgap git://git.openembedded.org/meta-openembedded
git clone -b scarthgap git://git.yoctoproject.org/meta-raspberrypi
git clone -b scarthgap https://github.com/meta-qt5/meta-qt5.git
```

For Xen work, add `meta-virtualization` as the next layer:

```bash
cd yocto
git clone -b scarthgap https://git.yoctoproject.org/meta-virtualization
```

`yocto/docker-build.sh` generates `build-rpi/conf/bblayers.conf` for the copied
layout and points the app recipes at `app/Head_Unit_jun`.

## Build

```bash
cd yocto
./docker-build.sh piracer-hu-image
```

The first clean build will still take a long time because Yocto must download
and rebuild dependencies. Subsequent builds are faster once `downloads/`,
`sstate-cache/`, and `tmp/` exist locally.
