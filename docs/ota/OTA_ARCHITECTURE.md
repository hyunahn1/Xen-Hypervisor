# Secure OTA Architecture — Head Unit (PiRacer)

## Overview

This document defines the Secure OTA architecture applied to the existing PiRacer Head Unit project.  
It satisfies **ota1.md Level 1–3** requirements: mutual authentication (PKI/mTLS), firmware integrity (SHA-256 + ECDSA digital signature), version management (rollback prevention), and encrypted transport (TLS 1.3).

> Xen compatibility note: this first OTA design is an in-Head-Unit application updater. It can be implemented in parallel with the Xen work as long as the downloader/verifier/version modules are kept independent from the final apply mechanism. In the Xen architecture, the long-term update orchestrator moves to Dom0 and targets individual DomU images. See `XEN_COMPATIBILITY.md`.

---

## System Diagram

```
╔══════════════════════════════════════════════════════════════╗
║                        OTA SERVER                            ║
║                                                              ║
║  ┌─────────────────┐      ┌──────────────────────────────┐  ║
║  │  FastAPI (HTTPS)│      │   Mosquitto MQTT Broker      │  ║
║  │  Port: 8443     │      │   Port: 8883  (TLS 1.3)      │  ║
║  │                 │      │                              │  ║
║  │  POST /upload   │      │  Topic: ota/notify           │  ║
║  │  GET  /firmware │      │  Payload: JSON               │  ║
║  │  GET  /version  │      │  {version, hash, url, sig}   │  ║
║  └────────┬────────┘      └──────────────┬───────────────┘  ║
║           │                              │                   ║
║  ┌────────▼──────────────────────────────▼───────────────┐  ║
║  │              PKI / Certificate Authority               │  ║
║  │  ca.crt  │  server.crt/key  │  client.crt/key         │  ║
║  │  Firmware signing: ECDSA P-256 (private key)          │  ║
║  └───────────────────────────────────────────────────────┘  ║
╚══════════════════════════════════════════════════════════════╝
                     ║  mTLS (TLS 1.3)  ║
                     ║  HTTPS + MQTTS   ║
╔══════════════════════════════════════════════════════════════╗
║              HEAD UNIT — Raspberry Pi (Yocto)                ║
║                                                              ║
║  ┌───────────────────────────────────────────────────────┐  ║
║  │                  OTA Manager (C++)                    │  ║
║  │                                                       │  ║
║  │  ┌──────────────┐   ┌────────────────────────────┐   │  ║
║  │  │  MQTT Client │   │    Firmware Verifier        │   │  ║
║  │  │  (Paho C++)  │   │  - SHA-256 hash check       │   │  ║
║  │  │  TLS 1.3     │   │  - ECDSA signature verify   │   │  ║
║  │  │  mTLS auth   │   │  - Version rollback check   │   │  ║
║  │  └──────┬───────┘   └────────────┬───────────────┘   │  ║
║  │         │                        │                    │  ║
║  │  ┌──────▼────────────────────────▼───────────────┐   │  ║
║  │  │            OtaController (Qt C++)             │   │  ║
║  │  │  - Receives MQTT notification                 │   │  ║
║  │  │  - Downloads firmware via HTTPS (libcurl)     │   │  ║
║  │  │  - Verifies hash + signature                  │   │  ║
║  │  │  - Applies update (shell script)              │   │  ║
║  │  │  - Signals ShellWindow: OTA status UI         │   │  ║
║  │  └───────────────────────────────────────────────┘   │  ║
║  └───────────────────────────────────────────────────────┘  ║
║                                                              ║
║  ┌───────────────────────────────────────────────────────┐  ║
║  │           Existing Head Unit Application               │  ║
║  │  ShellWindow → Wayland Compositor                     │  ║
║  │  Modules: media / call / navigation / settings / ...  │  ║
║  │  VSomeIP ↔ Instrument Cluster (speed/gear/battery)    │  ║
║  └───────────────────────────────────────────────────────┘  ║
╚══════════════════════════════════════════════════════════════╝
```

---

## Security Levels (from ota1.md)

| Level | Requirement | Implementation |
|-------|-------------|----------------|
| **1** | PKI mutual authentication | mTLS — server + client certificates signed by CA |
| **2** | Digital signature + hash | ECDSA P-256 signature on firmware; SHA-256 integrity check |
| **2** | Version rollback prevention | `/etc/ota/version.json` — reject if new ≤ current |
| **3** | Encrypted transport | TLS 1.3 on MQTT (port 8883) and HTTPS (port 8443) |
| **3** | Secure session | Paho MQTT TLS options; no session persistence on client |

---

## Component Specification

### 1. OTA Server

**Language:** Python 3.11  
**Framework:** FastAPI + Uvicorn (HTTPS, port 8443)  
**MQTT Broker:** Mosquitto 2.x

#### File Structure
```
ota_server/
├── main.py               # FastAPI app
├── routes/
│   ├── upload.py         # POST /upload — receive firmware + sign
│   └── firmware.py       # GET /firmware/{version} — HTTPS download
├── mqtt/
│   └── publisher.py      # Publish ota/notify after upload
├── pki/
│   ├── ca.crt
│   ├── server.crt
│   ├── server.key
│   └── signing.key       # ECDSA private key for firmware signing
├── firmware/             # Stored firmware binaries
│   └── {version}/
│       ├── headunit.tar.gz
│       ├── headunit.tar.gz.sig   # ECDSA signature
│       └── metadata.json         # {version, sha256, size, timestamp}
└── mosquitto.conf
```

#### MQTT Notification Payload (Topic: `ota/notify`)
```json
{
  "version": "2.1.0",
  "sha256": "a3f4b2c1...",
  "url": "https://192.168.x.x:8443/firmware/2.1.0/headunit.tar.gz",
  "signature": "base64-encoded-ECDSA-sig",
  "timestamp": "2026-04-27T10:00:00Z"
}
```

#### Firmware Signing (on upload)
```
openssl dgst -sha256 -sign signing.key -out headunit.tar.gz.sig headunit.tar.gz
```

---

### 2. PKI Setup (One-time, OpenSSL)

```bash
# 1. Create CA
openssl ecparam -name prime256v1 -genkey -noout -out ca.key
openssl req -new -x509 -days 3650 -key ca.key -out ca.crt -subj "/CN=OTA-CA"

# 2. Server certificate (OTA server)
openssl ecparam -name prime256v1 -genkey -noout -out server.key
openssl req -new -key server.key -out server.csr -subj "/CN=ota-server"
openssl x509 -req -in server.csr -CA ca.crt -CAkey ca.key -CAcreateserial -days 365 -out server.crt

# 3. Client certificate (Head Unit)
openssl ecparam -name prime256v1 -genkey -noout -out client.key
openssl req -new -key client.key -out client.csr -subj "/CN=headunit-001"
openssl x509 -req -in client.csr -CA ca.crt -CAkey ca.key -CAcreateserial -days 365 -out client.crt

# 4. Firmware signing key
openssl ecparam -name prime256v1 -genkey -noout -out signing.key
openssl ec -in signing.key -pubout -out signing.pub  # deploy to Head Unit
```

**Head Unit stores:**
- `/etc/ota/certs/ca.crt`
- `/etc/ota/certs/client.crt`
- `/etc/ota/certs/client.key`
- `/etc/ota/certs/signing.pub`  ← firmware signature verification only

---

### 3. OTA Manager — Head Unit (C++/Qt)

**New module added to the existing CMake project.**

#### File Structure (added to `Head_Unit_jun/`)
```
ota/
├── CMakeLists.txt
├── OtaManager.h          # Main controller — integrates with ShellWindow
├── OtaManager.cpp
├── MqttOtaClient.h       # Paho MQTT C++ wrapper with TLS
├── MqttOtaClient.cpp
├── FirmwareVerifier.h    # SHA-256 + ECDSA verification (OpenSSL)
├── FirmwareVerifier.cpp
├── FirmwareDownloader.h  # HTTPS download (libcurl + TLS)
├── FirmwareDownloader.cpp
├── VersionManager.h      # Read/write /etc/ota/version.json
└── VersionManager.cpp
```

#### OtaManager Interface
```cpp
class OtaManager : public QObject {
    Q_OBJECT
public:
    explicit OtaManager(QObject *parent = nullptr);
    void start();  // connect MQTT, begin listening

signals:
    void updateAvailable(QString version);      // → ShellWindow: show dialog
    void downloadProgress(int percent);         // → ShellWindow: progress bar
    void updateReady();                         // → ShellWindow: "Restart to apply"
    void updateFailed(QString reason);          // → ShellWindow: error toast

private slots:
    void onNotification(QByteArray payload);    // from MqttOtaClient
    void onDownloadFinished(QString filePath);  // from FirmwareDownloader

private:
    bool verifyAndApply(const QString &filePath, const QByteArray &sig, const QString &expectedHash);

    MqttOtaClient      *m_mqtt       = nullptr;
    FirmwareDownloader *m_downloader = nullptr;
    FirmwareVerifier   *m_verifier   = nullptr;
    VersionManager     *m_version    = nullptr;

    QString  m_pendingVersion;
    QString  m_pendingHash;
    QByteArray m_pendingSignature;
};
```

#### MqttOtaClient (TLS 1.3 + mTLS)
```cpp
// Paho MQTT C++ with TLS options
mqtt::connect_options buildConnectOptions() {
    mqtt::ssl_options ssl;
    ssl.set_trust_store("/etc/ota/certs/ca.crt");
    ssl.set_key_store("/etc/ota/certs/client.crt");
    ssl.set_private_key("/etc/ota/certs/client.key");
    ssl.set_ssl_version(MQTT_SSL_VERSION_TLS_1_3);
    ssl.set_verify(true);  // verify server cert against CA

    mqtt::connect_options opts;
    opts.set_ssl(ssl);
    opts.set_keep_alive_interval(60);
    return opts;
}
// Subscribe: "ota/notify", QoS 1
```

#### FirmwareVerifier (OpenSSL)
```cpp
// Step 1: SHA-256 hash check
bool verifyHash(const QString &filePath, const QString &expectedHex);

// Step 2: ECDSA P-256 signature verification
bool verifySignature(const QString &filePath,
                     const QByteArray &signature,
                     const QString &pubKeyPath);  // /etc/ota/certs/signing.pub
```

#### VersionManager
```cpp
// /etc/ota/version.json: {"version": "2.0.0", "installed_at": "2026-04-27T..."}
struct OtaVersion {
    int major, minor, patch;
    bool operator>=(const OtaVersion &o) const;
};

OtaVersion currentVersion();
bool isRollback(const OtaVersion &incoming);   // returns true if incoming ≤ current
void writeVersion(const OtaVersion &v);
```

#### Update Apply Script (`/etc/ota/apply_update.sh`)
```bash
#!/bin/bash
set -e
FIRMWARE_PATH=$1
TARGET_DIR="/opt/headunit"

systemctl stop headunit.service
tar -xzf "$FIRMWARE_PATH" -C "$TARGET_DIR"
systemctl start headunit.service
```

The script above is a non-Xen prototype. In the current Yocto image the service is `hu-shell.service` or `/etc/init.d/hu-shell`, and the installed binaries live under `/usr/bin`. For Xen, replace this script with an `IUpdateApplier` implementation so the same secure download/verify path can apply either:

- a development app bundle inside `DomU-HU`, or
- a signed DomU rootfs/image artifact through Dom0.

---

### 4. ShellWindow Integration

Add OtaManager to ShellWindow with minimal changes:

```cpp
// ShellWindow.h — add:
#include "OtaManager.h"
OtaManager *m_otaManager = nullptr;

// ShellWindow.cpp — setupModules():
m_otaManager = new OtaManager(this);
connect(m_otaManager, &OtaManager::updateAvailable,
        this, [this](QString v){ showOtaNotification(v); });
connect(m_otaManager, &OtaManager::updateFailed,
        this, [this](QString r){ showToast("OTA Failed: " + r); });
m_otaManager->start();
```

---

### 5. OTA Update Flow (Step by Step)

```
[Developer]
    │  POST /upload  (firmware + version)
    ▼
[OTA Server]
    │  1. Compute SHA-256 of firmware
    │  2. Sign with ECDSA signing.key → .sig file
    │  3. Store firmware + metadata
    │  4. Publish MQTT ota/notify {version, sha256, url, signature}
    ▼
[MQTT Broker]  ←── TLS 1.3 + mTLS ──►  [Head Unit: MqttOtaClient]
                                              │
                                              │  5. Receive notification
                                              │  6. Check version → rollback? → reject
                                              │  7. Download firmware via HTTPS (libcurl)
                                              │  8. Verify SHA-256 hash
                                              │  9. Verify ECDSA signature (signing.pub)
                                              │  10. Signal ShellWindow → show UI
                                              │  11. User confirms (or auto-apply)
                                              │  12. apply_update.sh → restart service
                                              ▼
                                         [Updated Head Unit]
```

---

### 6. CMake Changes

```cmake
# ota/CMakeLists.txt
find_package(OpenSSL REQUIRED)
find_package(CURL REQUIRED)
# Paho MQTT C++ must be installed: libpaho-mqtt3as-dev + paho-mqtt-cpp

add_library(hu_ota STATIC
    OtaManager.cpp
    MqttOtaClient.cpp
    FirmwareVerifier.cpp
    FirmwareDownloader.cpp
    VersionManager.cpp
)
target_link_libraries(hu_ota PUBLIC
    Qt5::Core
    Qt5::Network
    OpenSSL::SSL
    OpenSSL::Crypto
    CURL::libcurl
    paho-mqttpp3
    paho-mqtt3as
)

# Root CMakeLists.txt — add:
add_subdirectory(ota)
# shell/CMakeLists.txt — add hu_ota to target_link_libraries
```

---

### 7. Dependencies

| Library | Purpose | Install |
|---------|---------|---------|
| `paho-mqtt-cpp` | MQTT client with TLS | `apt install libpaho-mqttpp-dev` |
| `libpaho-mqtt3as` | Async MQTT (required by paho-cpp) | `apt install libpaho-mqtt3as-dev` |
| `OpenSSL` | SHA-256, ECDSA verify | `apt install libssl-dev` |
| `libcurl` | HTTPS firmware download | `apt install libcurl4-openssl-dev` |
| `mosquitto` | MQTT broker (server side) | `apt install mosquitto` |
| `FastAPI` | OTA web server | `pip install fastapi uvicorn` |

---

### 8. Verification Checklist (Self-Assessment)

- [ ] **mTLS**: Head Unit rejected without valid client certificate
- [ ] **Hash check**: Tampered firmware file is rejected (hash mismatch)
- [ ] **Signature check**: Firmware signed by wrong key is rejected
- [ ] **Rollback prevention**: Version 1.9 rejected when current is 2.0
- [ ] **TLS version**: Confirm TLS 1.3 with `openssl s_client -connect server:8883`
- [ ] **MQTT QoS**: Topic `ota/notify` delivered with QoS 1
- [ ] **UI integration**: ShellWindow shows OTA notification and progress

---

### 9. Directory Layout (final project structure)

```
Head_Unit_jun/
├── core/               ← unchanged
├── shell/              ← add OtaManager connection
├── modules/            ← unchanged
├── ota/                ← NEW: OTA Manager module
│   ├── CMakeLists.txt
│   ├── OtaManager.{h,cpp}
│   ├── MqttOtaClient.{h,cpp}
│   ├── FirmwareVerifier.{h,cpp}
│   ├── FirmwareDownloader.{h,cpp}
│   └── VersionManager.{h,cpp}
└── CMakeLists.txt      ← add_subdirectory(ota)

ota_server/             ← NEW: server-side (Python)
├── main.py
├── routes/
├── mqtt/
├── pki/
└── mosquitto.conf
```
