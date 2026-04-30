#!/bin/bash
# Head Unit 부팅 자동 시작 설치 스크립트 (개발 환경용)
# 사용법: sudo bash install_autostart.sh

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SERVICE_NAME="hu-shell.service"
SERVICE_PATH="/etc/systemd/system/${SERVICE_NAME}"

# 빌드된 바이너리 경로 자동 탐색
if [ -x "${SCRIPT_DIR}/build_ipc/bin/hu_shell" ]; then
    HU_BIN="${SCRIPT_DIR}/build_ipc/bin/hu_shell"
elif [ -x "${SCRIPT_DIR}/build/bin/hu_shell" ]; then
    HU_BIN="${SCRIPT_DIR}/build/bin/hu_shell"
else
    echo "[ERROR] hu_shell 바이너리를 찾을 수 없습니다."
    echo "  먼저 빌드하세요: mkdir -p build && cd build && cmake .. && make -j4"
    exit 1
fi

CONFIG_PATH="${SCRIPT_DIR}/config/vsomeip_headunit.json"
echo "[INFO] 바이너리: ${HU_BIN}"
echo "[INFO] VSOMEIP 설정: ${CONFIG_PATH}"

# Qt 플랫폼 자동 감지 (X11 또는 EGLFS)
if [ -n "${DISPLAY}" ]; then
    QT_PLATFORM="xcb"
    DISPLAY_ENV="Environment=DISPLAY=${DISPLAY}"
    XAUTH_ENV="Environment=XAUTHORITY=${XAUTHORITY:-/root/.Xauthority}"
    AFTER_TARGET="graphical.target"
    WANTED_BY="graphical.target"
else
    QT_PLATFORM="eglfs"
    DISPLAY_ENV=""
    XAUTH_ENV=""
    AFTER_TARGET="multi-user.target"
    WANTED_BY="multi-user.target"
fi

echo "[INFO] Qt 플랫폼: ${QT_PLATFORM}"

# 서비스 파일 생성
cat > "${SERVICE_PATH}" << EOF
[Unit]
Description=PiRacer Head Unit Shell
After=network.target ${AFTER_TARGET}
Wants=${AFTER_TARGET}

[Service]
Type=simple
User=root
${DISPLAY_ENV}
${XAUTH_ENV}
Environment=QT_QPA_PLATFORM=${QT_PLATFORM}
Environment=VSOMEIP_CONFIGURATION=${CONFIG_PATH}
WorkingDirectory=$(dirname "${HU_BIN}")
ExecStartPre=/bin/sleep 3
ExecStart=${HU_BIN}
Restart=on-failure
RestartSec=3

[Install]
WantedBy=${WANTED_BY}
EOF

echo "[INFO] 서비스 파일 생성: ${SERVICE_PATH}"

# systemd 재로드 및 서비스 활성화
systemctl daemon-reload
systemctl enable "${SERVICE_NAME}"

echo ""
echo "=== 설치 완료 ==="
echo "  서비스 시작:   sudo systemctl start ${SERVICE_NAME}"
echo "  상태 확인:     sudo systemctl status ${SERVICE_NAME}"
echo "  로그 확인:     sudo journalctl -u ${SERVICE_NAME} -f"
echo "  서비스 중지:   sudo systemctl stop ${SERVICE_NAME}"
echo "  자동시작 해제: sudo systemctl disable ${SERVICE_NAME}"
echo ""
echo "다음 재부팅 시 자동으로 시작됩니다."
