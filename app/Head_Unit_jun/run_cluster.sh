#!/bin/bash
# Instrument Cluster (PiRacer Dashboard) - VSOMEIP 설정 포함
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
export VSOMEIP_CONFIGURATION="${SCRIPT_DIR}/instrument_cluster/config/vsomeip_cluster.json"
export DISPLAY="${DISPLAY:-:0}"

cd "${SCRIPT_DIR}/instrument_cluster/build" || { echo "instrument_cluster/build not found. Build first."; exit 1; }
exec ./PiRacerDashboard "$@"
