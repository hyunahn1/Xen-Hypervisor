# OTA and Xen Compatibility Plan

## 1. Decision

OTA and Xen can be developed in parallel, but they must not share the same "apply update" assumption.

Recommended split:

| Owner | Workstream | Stable Contract |
|-------|------------|-----------------|
| OTA team | Server, mTLS, notification, download, hash/signature verification, version policy | Produce a verified update artifact plus metadata |
| Xen team | Dom0, DomU-HU, DomU-Cluster boot, domain lifecycle, storage layout | Provide domain/image targets and an apply API |

The OTA team should build the secure transport and verification pipeline now. The Xen team should build the domain layout now. They merge at the **Update Applier** boundary.

## 2. Why Parallel Work Is Safe

The current OTA design is mostly independent from Xen until the final apply step:

```text
MQTT notify
  -> HTTPS download
  -> SHA-256 check
  -> ECDSA signature verify
  -> version rollback check
  -> APPLY UPDATE   <-- Xen-sensitive boundary
```

Everything before `APPLY UPDATE` is reusable in both non-Xen and Xen systems.

## 3. Current OTA Gaps Before Xen Merge

| Gap | Current OTA Document | Xen-Compatible Requirement |
|-----|----------------------|----------------------------|
| Apply target | `/opt/headunit` | Domain/image target such as `domu-hu`, `domu-cluster`, or `dom0` |
| Service name | `headunit.service` | Current service is `hu-shell.service` or SysV `/etc/init.d/hu-shell`; Xen may restart a whole DomU |
| Rollback | version JSON only | Keep version JSON, but add slot/image state for domain artifacts |
| Artifact type | `headunit.tar.gz` | Add typed artifacts: app bundle, DomU rootfs, kernel/dtb, config |
| Orchestrator | Head Unit process | Long-term Xen orchestrator should live in Dom0 |
| Failure handling | restart local service | Dom0 should stop/start/reboot target domain and rollback if health check fails |

## 4. Stable Artifact Manifest

Use a manifest format that works before and after Xen:

```json
{
  "schema": 1,
  "version": "2.1.0",
  "target": "domu-hu",
  "artifact_type": "app-bundle",
  "url": "https://ota-server:8443/firmware/2.1.0/domu-hu.tar.zst",
  "sha256": "hex",
  "signature": "base64-ecdsa-signature",
  "requires_reboot": false,
  "min_current_version": "2.0.0",
  "rollback_allowed": true,
  "health_check": {
    "type": "systemd",
    "name": "hu-shell.service",
    "timeout_sec": 30
  }
}
```

Recommended target values:

| Target | Meaning |
|--------|---------|
| `dom0` | Xen manager/root image update |
| `domu-hu` | Head Unit guest update |
| `domu-cluster` | Instrument Cluster guest update |
| `domu-control` | Future vehicle-control guest update |

Recommended artifact types:

| Artifact Type | Phase | Applies Where |
|---------------|-------|---------------|
| `app-bundle` | OTA first phase | Inside current non-Xen HU or DomU-HU |
| `rootfs-image` | Xen phase | Dom0 writes inactive guest rootfs slot |
| `kernel-dtb` | Xen phase | Dom0 updates boot artifacts |
| `config` | Both | Service/domain configuration only |

## 5. Apply Interface

The OTA implementation should depend on an interface, not a shell script:

```cpp
class IUpdateApplier {
public:
    virtual ~IUpdateApplier() = default;
    virtual bool stage(const UpdateManifest &manifest, const QString &artifactPath) = 0;
    virtual bool activate(const UpdateManifest &manifest) = 0;
    virtual bool healthCheck(const UpdateManifest &manifest) = 0;
    virtual bool rollback(const UpdateManifest &manifest) = 0;
};
```

Implementations:

| Implementation | Used When | Behavior |
|----------------|-----------|----------|
| `LocalAppBundleApplier` | Current non-Xen demo | Stop `hu-shell`, unpack app bundle, restart |
| `Dom0GuestImageApplier` | Xen final architecture | Stop target DomU, write inactive image/slot, update domain config, boot target, health check |
| `ConfigOnlyApplier` | Safe config update | Apply config and restart affected service |

## 6. Recommended Work Order

### OTA Team Can Build Now

- OTA server with FastAPI and Mosquitto.
- mTLS certificate flow.
- ECDSA signing and SHA-256 verification.
- Manifest parser.
- Version manager.
- `LocalAppBundleApplier` for current non-Xen demo.
- UI notification/progress in Head Unit.

### Xen Team Can Build Now

- Dom0 image with Xen toolstack.
- DomU-HU and DomU-Cluster boot images.
- Inter-domain network.
- Domain start/restart service.
- Stable IP naming: `domu-hu`, `domu-cluster`.
- Guest health checks.

### Merge Point

When Xen boots two guests reliably:

1. Move OTA orchestrator from `DomU-HU` to Dom0, or keep a thin UI client in HU and run the applier in Dom0.
2. Change manifest `artifact_type` from `app-bundle` to `rootfs-image`.
3. Replace `LocalAppBundleApplier` with `Dom0GuestImageApplier`.
4. Keep the same server, mTLS, verifier, signature, hash, and version logic.

## 7. Final Xen-Aware OTA Architecture

```text
OTA Server
  FastAPI + Mosquitto + signing key
        |
        | mTLS HTTPS/MQTTS
        v
Dom0 OTA Orchestrator
  - receives notification
  - downloads artifact
  - verifies hash/signature/version
  - stages inactive DomU image
  - stops/starts target domain
  - health-checks guest
  - rolls back on failure
        |
        +--> DomU-HU thin OTA UI client
        +--> DomU-Cluster status optional
```

DomU-HU may still display update progress, but it should not be the only component capable of applying system-level updates once Xen is introduced.

## 8. Answer to "Xen First or OTA First?"

If only one person were doing both, do Xen first. But with two people, parallel work is better:

- Team member: implement OTA server/security/verifier/local demo.
- You: implement Xen domain architecture.
- Shared contract: manifest + `IUpdateApplier`.

The only thing to avoid is hardcoding OTA around `/opt/headunit` and `headunit.service`. Keep the apply layer replaceable and the work will merge cleanly.
