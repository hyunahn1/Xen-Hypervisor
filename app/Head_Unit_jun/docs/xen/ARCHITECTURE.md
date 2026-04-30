# PiRacer Xen Hypervisor Architecture

## 1. Executive Decision

The recommended Xen architecture for this project is a managed-Dom0 design:

```text
Raspberry Pi 4
  Xen hypervisor
    Dom0: minimal control domain
      - Xen toolstack
      - network backend
      - block backend
      - optional CAN/camera/display backend during phase 1
    DomU-HU: Head Unit domain
      - hu_shell QtWayland compositor
      - media/settings/ambient/reverse camera/PDC UI
    DomU-Cluster: Instrument Cluster domain
      - PiRacerDashboard
      - speed/gear/battery display
```

Do not start with a fully dom0less design. Dom0less is attractive for fast boot, but this project needs easier debugging, domain restart, logs, and staged device assignment first. After the managed design is stable, dom0less can be evaluated as an optimization.

## 2. Current System Assessment

The current non-Xen system already has good separation at the application level:

| Area | Current State | Xen Impact |
|------|---------------|------------|
| Head Unit | QtWayland compositor, eglfs DRM master, module supervision | Good candidate for `DomU-HU`, but display ownership must be decided carefully |
| Cluster | Separate Qt app, currently rendered as Wayland client under HU compositor | Can become `DomU-Cluster`, but direct DSI passthrough is harder than keeping cluster as HU-rendered Wayland client |
| IPC | VSOMEIP peer-to-peer over local network stack | Maps cleanly to inter-domain virtual networking |
| CAN | SocketCAN used by IC and now PDC provider paths | Start with Dom0 owning CAN, expose data via service; direct CAN passthrough can be phase 2 |
| Camera | `libcamerasrc` inside HU reverse camera | Prefer keeping camera in HU only if CSI/libcamera stack is assigned or proxied there |
| PDC | Sensor value from CAN, stable risk estimator in HU shell | Good portfolio feature; can demonstrate cross-domain data flow later |

Important correction: the current cluster is not an independent display owner in the deployed HU image. It connects to `hu_shell` as a Wayland client and is shown through the HU compositor. Xen must either preserve this model over inter-domain networking, or split physical displays with a more complex display-passthrough design.

## 3. Target Domain Boundary

### 3.1 Phase 1: Practical Portfolio Xen

```text
                         +----------------------------+
                         |          Dom0              |
                         |  xl/xenstored/netback      |
                         |  can0 owner or bridge svc  |
                         |  SSH/log collection        |
                         +------+----------+----------+
                                |          |
                       xen-netfront     xen-netfront
                                |          |
+-------------------------------+--+    +--+----------------------------+
| DomU-HU                          |    | DomU-Cluster                  |
| hu_shell                         |    | PiRacerDashboard              |
| QtWayland compositor             |<---| Wayland client or VSOMEIP app |
| reverse camera + PDC overlay     |    | speed/gear display            |
| VSOMEIP client/service           |<-->| VSOMEIP service/client        |
+----------------------------------+    +-------------------------------+
```

Phase 1 keeps the visual stack mostly intact:

- DomU-HU owns the physical display path and runs `hu_shell`.
- DomU-Cluster connects to HU over virtual network using the existing Wayland/VSOMEIP model where possible.
- Dom0 owns management, networking, and logs.
- CAN can initially stay in Dom0 or HU for simplicity, but the architecture decision should be explicit.

This gives the strongest demo fastest: two isolated Linux guests, same automotive UI, cross-domain IPC, reboot/restart one guest without killing the other.

### 3.2 Phase 2: Stronger Isolation

```text
Dom0
  - no UI
  - owns Xen toolstack and backend drivers only
  - CAN backend service
DomU-HU
  - HDMI/display stack
  - camera/PDC/reverse overlay
DomU-Cluster
  - DSI display stack or virtual display client
  - instrument cluster
DomU-Control
  - gamepad / PiRacer motor control / battery bridge
```

Add `DomU-Control` only after HU and Cluster domains are stable. It is the right final location for motor/gamepad control because it separates vehicle control from infotainment.

## 4. Hardware Ownership Matrix

| Resource | Phase 1 Owner | Phase 2 Owner | Rationale |
|----------|---------------|---------------|-----------|
| HDMI/primary display | DomU-HU | DomU-HU | HU compositor already owns eglfs/DRM |
| DSI cluster display | DomU-HU compositor renders cluster | DomU-Cluster or HU compositor | Direct DSI passthrough is possible only after boot/display stability is proven |
| CSI camera | DomU-HU or Dom0 proxy | DomU-HU | Reverse camera is a HU feature; passthrough may need DT/IOMMU work |
| CAN `can0` | Dom0 or DomU-HU | Dom0 backend or DomU-Control | Safety data should not be coupled to infotainment long-term |
| WiFi/Ethernet | Dom0 bridge/NAT | Dom0 bridge/NAT | Keep networking centralized |
| Storage | Dom0 block backend | Dom0 block backend | Enables rollback/OTA later |
| Gamepad | DomU-Control or Dom0 proxy | DomU-Control | Vehicle control should be separated from HU UI |
| Touch | DomU-HU | DomU-HU | HU owns user interaction |

## 5. CPU and Memory Budget

Raspberry Pi 4 has 4 Cortex-A72 cores. Start simple:

| Domain | vCPU | Suggested pinning | RAM | Notes |
|--------|------|-------------------|-----|------|
| Dom0 | 1 | CPU0 | 512 MB | management only, no heavy UI |
| DomU-HU | 2 | CPU1-2 | 1536-2048 MB | QtWayland, camera, media |
| DomU-Cluster | 1 | CPU3 | 512-768 MB | deterministic UI |

If the HU camera path drops frames, give DomU-HU 2 CPUs and pin Cluster to one CPU. Avoid overcommitting until boot and UI latency are measured.

## 6. Inter-Domain Communication

### 6.1 Primary IPC

Use virtual Ethernet between domains and keep VSOMEIP as the application protocol:

```text
DomU-Cluster 10.0.10.20 <---- xen bridge ----> 10.0.10.10 DomU-HU
```

Why:

- Minimal code changes.
- Matches the project requirement for SOME/IP.
- Easy to inspect with `tcpdump`.
- Later OTA/network policy can be implemented in Dom0.

### 6.2 Alternative IPC

For a future control domain, consider a smaller message path for safety/control data:

- vchan for low-level domain-to-domain messages.
- virtio or Xen grant-table based transport if you need lower overhead.
- Keep VSOMEIP for portfolio-visible automotive middleware.

## 7. Boot Architecture

```text
Firmware/U-Boot
  -> Xen hypervisor
     -> Dom0 Linux kernel + initramfs/rootfs
        -> xenstored/xl toolstack
        -> create DomU-HU from /etc/xen/hu.cfg
        -> create DomU-Cluster from /etc/xen/cluster.cfg
        -> health monitor waits for guest heartbeat
```

Phase 1 can use `xl create` from systemd services in Dom0. Domain configuration should live in versioned files:

```text
/etc/xen/hu.cfg
/etc/xen/cluster.cfg
/usr/lib/piracer-xen/start-domains.sh
```

## 8. Yocto Integration Plan

Current repo already has image skeletons:

- `piracer-dom0-image.bb`
- `piracer-hu-image.bb`
- `piracer-cluster-image.bb`

Required Yocto work:

| Step | Change |
|------|--------|
| Add virtualization layer | Add `meta-virtualization` to `bblayers.conf` |
| Build Xen | Add Xen packages/toolstack to `piracer-dom0-image` |
| Kernel config | Enable Xen guest/backend options for Dom0 and DomU kernels |
| Guest rootfs | Keep `piracer-hu-image` and `piracer-cluster-image` as DomU root filesystems |
| Domain configs | Add recipe to install `/etc/xen/*.cfg` into Dom0 |
| Networking | Add Dom0 bridge config and static IPs for domains |
| Logging | Dom0 collects `xl dmesg`, guest console logs, app logs |

## 9. Domain Config Sketch

Example only. Final values depend on kernel, dtb, rootfs partition paths, and device assignment result.

```python
# /etc/xen/hu.cfg
name = "domu-hu"
type = "pv"
vcpus = 2
memory = 1792
vif = [ "bridge=xenbr0,mac=00:16:3e:10:00:10" ]
disk = [ "phy:/dev/mmcblk0p3,xvda,w" ]
extra = "root=/dev/xvda rw console=hvc0"
```

```python
# /etc/xen/cluster.cfg
name = "domu-cluster"
type = "pv"
vcpus = 1
memory = 768
vif = [ "bridge=xenbr0,mac=00:16:3e:10:00:20" ]
disk = [ "phy:/dev/mmcblk0p4,xvda,w" ]
extra = "root=/dev/xvda rw console=hvc0"
```

On ARM, Linux guests are commonly PVH-like device-tree-described guests rather than x86-style fully emulated PCs. Treat these configs as direction, not copy-paste final.

## 10. Safety and Failure Model

| Failure | Expected Behavior | Verification |
|---------|-------------------|--------------|
| HU DomU crashes | Dom0 restarts HU; Cluster remains alive or shows stale IPC | Kill HU domain, observe restart |
| Cluster DomU crashes | HU remains alive; cluster domain restarts | Kill Cluster domain |
| VSOMEIP unavailable | UI shows IPC unavailable/stale state | Block inter-domain network |
| CAN unavailable | PDC state stale; no false distance | Bring down CAN/backend |
| Camera unavailable | Reverse camera placeholder, PDC overlay still works | Disconnect camera |
| Dom0 overloaded | Guests still have pinned CPU budget | Stress Dom0 CPU0 |
| Guest memory leak | Affected guest restarts only | cgroup/guest memory pressure test |

## 11. Verification Plan

### 11.1 Build Verification

| ID | Test | Expected |
|----|------|----------|
| XEN-BLD-001 | `bitbake piracer-dom0-image` | image contains Xen toolstack |
| XEN-BLD-002 | `bitbake piracer-hu-image` | HU DomU rootfs builds |
| XEN-BLD-003 | `bitbake piracer-cluster-image` | Cluster DomU rootfs builds |
| XEN-BLD-004 | inspect deploy artifacts | Dom0, HU, Cluster images present |

### 11.2 Boot Verification

| ID | Test | Expected |
|----|------|----------|
| XEN-BOOT-001 | serial boot log | Xen banner appears before Dom0 Linux |
| XEN-BOOT-002 | `xl info` | Xen toolstack can query hypervisor |
| XEN-BOOT-003 | `xl list` | Dom0 plus HU/Cluster domains visible |
| XEN-BOOT-004 | guest console | HU and Cluster reach login/app start |

### 11.3 Functional Verification

| ID | Test | Expected |
|----|------|----------|
| XEN-FUNC-001 | HU display starts | Main shell visible |
| XEN-FUNC-002 | Cluster app starts | Cluster surface or domain UI visible |
| XEN-FUNC-003 | VSOMEIP speed event | HU status receives Cluster speed |
| XEN-FUNC-004 | Reverse gear | Camera window opens, PDC overlay active |
| XEN-FUNC-005 | CAN frame replay | PDC updates from CAN/backend |

### 11.4 Isolation Verification

| ID | Test | Expected |
|----|------|----------|
| XEN-ISO-001 | crash HU process | Cluster domain remains running |
| XEN-ISO-002 | reboot DomU-HU | Dom0 and Cluster unaffected |
| XEN-ISO-003 | network isolate Cluster | HU reports IPC stale, no crash |
| XEN-ISO-004 | CPU stress HU | Cluster remains responsive within target frame budget |
| XEN-ISO-005 | camera failure | HU stays up with placeholder |

## 12. Implementation Roadmap

### Milestone 0: Baseline Freeze

- Tag current non-Xen image as known-good.
- Record boot video and verification logs.
- Keep current HU image bootable as fallback.

### Milestone 1: Dom0 Boots Xen

- Add `meta-virtualization`.
- Build `piracer-dom0-image` with Xen.
- Confirm `xl info` on Raspberry Pi 4.

### Milestone 2: One Guest Boots

- Boot `piracer-hu-image` as a DomU with virtual disk and virtual network.
- Do not pass display yet if it blocks progress.
- Confirm SSH and basic process startup.

### Milestone 3: HU Display Path

- Decide one of:
  - HU DomU owns DRM/display, if passthrough is stable.
  - Dom0 owns display and runs a thin compositor/proxy, if passthrough is painful.
- Preferred portfolio route: HU DomU owns display, Dom0 remains non-UI.

### Milestone 4: Cluster Domain

- Boot Cluster DomU.
- Connect it to HU via virtual network.
- Reuse current VSOMEIP/Wayland flow if possible.

### Milestone 5: Device Split

- Move CAN ownership to Dom0 backend or Control DomU.
- Keep camera in HU DomU or proxy frames from Dom0.
- Add failure handling and guest restart policy.

### Milestone 6: OTA-Ready Layout

- Use Dom0 as the update orchestrator.
- Store HU/Cluster rootfs images separately.
- Add version manifest and rollback plan.

OTA can be developed in parallel with the Xen work if the OTA apply step is abstracted. The OTA server, mTLS transport, artifact download, hash/signature verification, and version policy are reusable. The Xen-sensitive part is only the applier: current non-Xen OTA may unpack an app bundle locally, while the final Xen architecture should let Dom0 stage and activate per-domain artifacts. See `../OTA/XEN_COMPATIBILITY.md`.

## 13. Open Technical Risks

| Risk | Why It Matters | Mitigation |
|------|----------------|------------|
| RPi4 display passthrough | DRM/KMS and GPU paths may not isolate cleanly | Start with one display owner; keep serial recovery |
| CSI camera passthrough | libcamera pipeline may require device-tree/IOMMU tuning | Keep camera in same domain as HU; fallback placeholder |
| CAN ownership conflict | Current code has both IC and HU paths reading CAN | Define single owner; expose data through VSOMEIP/backend |
| VSOMEIP multicast across bridge | Service discovery may need explicit unicast config | Use fixed domain IPs and explicit configs |
| Memory pressure | Qt + camera + media in HU is heavy | Pin CPUs, set RAM budget, measure |

## 14. Source References

- Xen Project documentation: https://xenbits.xen.org/docs/
- Xen ARM passthrough notes: https://xenbits.xen.org/docs/unstable/misc/arm/passthrough.txt
- Xen dom0less design: https://xenbits.xen.org/docs/unstable/features/dom0less.html
- Xen Project wiki: https://wiki.xenproject.org/wiki/Xen_Project_Software_Overview
- OpenEmbedded meta-virtualization layer: https://git.yoctoproject.org/meta-virtualization
- COVESA vsomeip: https://github.com/COVESA/vsomeip
