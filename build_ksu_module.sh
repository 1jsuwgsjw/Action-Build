#!/bin/bash
set -euo pipefail

OUT_FILE=ksu_module_susfs.zip
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

if ! command -v zip >/dev/null 2>&1; then
    echo "[-] zip command not found. Install zip first, for example: sudo apt-get install zip"
    exit 1
fi

rm -f "${SCRIPT_DIR}/${OUT_FILE}"
cd "${SCRIPT_DIR}/ksu_module_susfs"

# Include dotfiles such as .susfs_no_auto_add_try_umount, but keep archive
# entries clean (module.prop, META-INF/..., tools/...) instead of ./module.prop.
shopt -s dotglob nullglob
entries=(*)
if [ "${#entries[@]}" -eq 0 ]; then
    echo "[-] ksu_module_susfs is empty"
    exit 1
fi

zip -r9 "../${OUT_FILE}" "${entries[@]}"
echo "[+] Built ${SCRIPT_DIR}/${OUT_FILE}"
