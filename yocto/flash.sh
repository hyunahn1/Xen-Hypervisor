#!/bin/bash
# =============================================================================
# PiRacer Head Unit - SD 카드 플래시 스크립트
# 사용법:
#   ./flash.sh <SD카드 장치>
#
# 예시:
#   ./flash.sh /dev/sdb       # SD카드가 /dev/sdb인 경우
#   ./flash.sh /dev/mmcblk0   # 내장 SD 리더인 경우
#
# 주의:
#   - 올바른 장치를 지정하지 않으면 데이터가 영구 삭제됩니다!
#   - sudo 권한이 필요합니다.
# =============================================================================
set -e

# ── 경로 설정 ────────────────────────────────────────────────────────────────
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DEPLOY_DIR="${SCRIPT_DIR}/build-rpi/tmp/deploy/images/raspberrypi4-64"

# ── 색상 출력 ────────────────────────────────────────────────────────────────
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

log_info()  { echo -e "${BLUE}[INFO]${NC}  $*"; }
log_ok()    { echo -e "${GREEN}[OK]${NC}    $*"; }
log_warn()  { echo -e "${YELLOW}[WARN]${NC}  $*"; }
log_error() { echo -e "${RED}[ERROR]${NC} $*" >&2; }

# ── 인수 검사 ────────────────────────────────────────────────────────────────
if [ -z "$1" ]; then
    log_error "SD 카드 장치를 지정하세요."
    echo ""
    echo "사용법: $0 <장치>"
    echo "예시:   $0 /dev/sdb"
    echo ""
    echo "현재 연결된 블록 장치:"
    lsblk -d -o NAME,SIZE,MODEL,TRAN | grep -v loop
    exit 1
fi

DEVICE="$1"

# ── 장치 검사 ────────────────────────────────────────────────────────────────
if [ ! -b "${DEVICE}" ]; then
    log_error "장치가 존재하지 않습니다: ${DEVICE}"
    echo "사용 가능한 장치 목록:"
    lsblk -d -o NAME,SIZE,MODEL,TRAN | grep -v loop
    exit 1
fi

# 내장 디스크 보호
if echo "${DEVICE}" | grep -qE "^/dev/nvme"; then
    log_error "내장 디스크로 보이는 장치입니다: ${DEVICE}"
    log_error "SD 카드 장치를 정확히 지정하세요. (예: /dev/sda, /dev/mmcblk0)"
    exit 1
fi

# ── 이미지 파일 탐색 ─────────────────────────────────────────────────────────
if [ ! -d "${DEPLOY_DIR}" ]; then
    log_error "deploy 디렉토리가 없습니다. 먼저 빌드를 실행하세요:"
    log_error "  ./docker-build.sh"
    exit 1
fi

# .wic.bz2 → .wic.zst → .wic 순으로 탐색
WIC_BZ2=$(ls "${DEPLOY_DIR}"/*.wic.bz2 2>/dev/null | grep -v "^$" | sort -V | tail -1 || true)
WIC_ZST=$(ls "${DEPLOY_DIR}"/*.wic.zst 2>/dev/null | grep -v "^$" | sort -V | tail -1 || true)
WIC=$(ls "${DEPLOY_DIR}"/*.wic 2>/dev/null | grep -v "^$" | grep -v ".bz2" | grep -v ".zst" | sort -V | tail -1 || true)

if [ -n "${WIC_BZ2}" ]; then
    IMAGE_FILE="${WIC_BZ2}"
    IMAGE_TYPE="wic.bz2"
elif [ -n "${WIC_ZST}" ]; then
    IMAGE_FILE="${WIC_ZST}"
    IMAGE_TYPE="wic.zst"
elif [ -n "${WIC}" ]; then
    IMAGE_FILE="${WIC}"
    IMAGE_TYPE="wic"
else
    log_error "플래시 가능한 이미지 파일(.wic/.wic.bz2/.wic.zst)이 없습니다."
    log_error "deploy 디렉토리: ${DEPLOY_DIR}"
    ls -la "${DEPLOY_DIR}"/ 2>/dev/null || true
    exit 1
fi

# ── 확인 ─────────────────────────────────────────────────────────────────────
echo "============================================================"
echo "  PiRacer SD 카드 플래시"
echo "============================================================"
log_info "이미지 파일: $(basename ${IMAGE_FILE})"
log_info "대상 장치:   ${DEVICE}"
echo ""

# 장치 정보 표시
log_info "장치 정보:"
lsblk "${DEVICE}" || true
echo ""

log_warn "경고: ${DEVICE}의 모든 데이터가 삭제됩니다!"
read -rp "계속하시겠습니까? (yes/no): " CONFIRM
if [ "${CONFIRM}" != "yes" ]; then
    log_info "취소되었습니다."
    exit 0
fi

# ── 마운트 해제 ──────────────────────────────────────────────────────────────
log_info "${DEVICE} 파티션 마운트 해제 중..."
for PART in $(lsblk -ln -o NAME "${DEVICE}" | grep -v "^$(basename ${DEVICE})$" || true); do
    if mountpoint -q "/dev/${PART}" 2>/dev/null; then
        sudo umount "/dev/${PART}" && log_info "  /dev/${PART} 해제됨"
    fi
done

# ── 플래시 ───────────────────────────────────────────────────────────────────
log_info "SD 카드에 이미지 플래시 중... (시간이 걸릴 수 있습니다)"
echo ""

if command -v bmaptool &>/dev/null; then
    # bmaptool 사용 (빠름, bmap 파일 있으면 자동 사용)
    BMAP_FILE="${IMAGE_FILE%.*}.bmap"
    [ -f "${BMAP_FILE}" ] || BMAP_FILE=""

    log_info "bmaptool 사용 (빠른 플래시)"
    if [ "${IMAGE_TYPE}" = "wic.bz2" ]; then
        sudo bmaptool copy "${IMAGE_FILE}" "${DEVICE}"
    elif [ "${IMAGE_TYPE}" = "wic.zst" ]; then
        sudo bmaptool copy "${IMAGE_FILE}" "${DEVICE}"
    else
        sudo bmaptool copy "${IMAGE_FILE}" "${DEVICE}"
    fi
else
    # bmaptool 없으면 dd 사용
    log_warn "bmaptool이 없습니다. dd로 플래시합니다. (느림)"
    log_info "bmaptool 설치: sudo apt install bmap-tools"
    echo ""

    if [ "${IMAGE_TYPE}" = "wic.bz2" ]; then
        log_info "bzcat | dd 실행 중..."
        bzcat "${IMAGE_FILE}" | sudo dd of="${DEVICE}" bs=4M status=progress conv=fsync
    elif [ "${IMAGE_TYPE}" = "wic.zst" ]; then
        log_info "zstdcat | dd 실행 중..."
        zstdcat "${IMAGE_FILE}" | sudo dd of="${DEVICE}" bs=4M status=progress conv=fsync
    else
        log_info "dd 실행 중..."
        sudo dd if="${IMAGE_FILE}" of="${DEVICE}" bs=4M status=progress conv=fsync
    fi
fi

# ── 완료 ─────────────────────────────────────────────────────────────────────
sync
echo ""
log_ok "플래시 완료!"
log_ok "SD 카드를 안전하게 제거하고 라즈베리파이4에 삽입하세요."
echo ""
log_info "라즈베리파이4 부팅 후 SSH 접속:"
log_info "  ssh root@<IP주소>    (비밀번호 없음, debug-tweaks 이미지)"
log_info ""
log_info "systemd 서비스 상태 확인:"
log_info "  systemctl status hu-shell.service"
log_info "  journalctl -u hu-shell.service -f"
