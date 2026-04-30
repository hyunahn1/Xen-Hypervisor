#!/bin/bash
# Head Unit (hu_shell) - VSOMEIP 설정 포함
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
export VSOMEIP_CONFIGURATION="${SCRIPT_DIR}/config/vsomeip_headunit.json"
export DISPLAY="${DISPLAY:-:0}"

# hu_shell 위치: build_ipc/bin 또는 build/bin
if [ -x "${SCRIPT_DIR}/build_ipc/bin/hu_shell" ]; then
    cd "${SCRIPT_DIR}/build_ipc/bin"
    exec ./hu_shell "$@"
elif [ -x "${SCRIPT_DIR}/build/bin/hu_shell" ]; then
    cd "${SCRIPT_DIR}/build/bin"
    exec ./hu_shell "$@"
else
    echo "hu_shell not found. Build first:"
    echo "  cd ${SCRIPT_DIR} && mkdir -p build && cd build && cmake .. && make"
    exit 1
fi
