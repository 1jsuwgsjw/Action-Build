#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

if ! ndk-build -s -v &>/dev/null; then
    echo "[-] Have you added the root directory of ndk-build to your PATH envoironment variable?"
    exit 1
fi

set -x

cd "${SCRIPT_DIR}/ksu_susfs"
rm -rf libs obj 2>/dev/null
ndk-build
cp libs/arm64-v8a/ksu_susfs ../ksu_module_susfs/tools/ksu_susfs_arm64
echo "[+] Built ksu_susfs and copied arm64 binary to ksu_module_susfs/tools/ksu_susfs_arm64"
