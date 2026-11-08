#!/usr/bin/env bash
# Build the actual SukiSU loader payload from a pinned source plus our patch.
set -euo pipefail
PATCH_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SOURCE_REV=e565c93ff6d0b992d9dfcbd43533a49d744a5f23
WORK="${1:?source work directory required}"
OUT="${2:?output directory required}"
NDK="${ANDROID_NDK_HOME:?Android NDK required}"
TOOLS="$NDK/toolchains/llvm/prebuilt/linux-x86_64/bin"
mkdir -p "$WORK" "$OUT"
git -C "$WORK" init
git -C "$WORK" remote add origin https://github.com/SukiSU-Ultra/SukiSU_KernelPatch_patch.git
git -C "$WORK" fetch --depth=1 origin "$SOURCE_REV"
git -C "$WORK" checkout --detach FETCH_HEAD
test "$(git -C "$WORK" rev-parse HEAD)" = "$SOURCE_REV"
git -C "$WORK" apply --check "$PATCH_DIR/0001-fix-unload-lifetime.patch"
git -C "$WORK" apply "$PATCH_DIR/0001-fix-unload-lifetime.patch"
make -C "$WORK/kernel" -j"$(nproc)" TARGET_COMPILE="$TOOLS" \
  CC="$TOOLS/clang --target=aarch64-linux-android35"
cmake -S "$WORK/tools" -B "$WORK/host-build" -DCMAKE_BUILD_TYPE=Release
cmake --build "$WORK/host-build" --parallel "$(nproc)"
cp "$WORK/kernel/kpimg" "$WORK/kernel/kpimg.elf" "$WORK/host-build/kptools" "$OUT/"
chmod +x "$OUT/kptools"
{
  printf 'source_revision=%s\n' "$SOURCE_REV"
  "$TOOLS/clang" --version
  sha256sum "$PATCH_DIR/0001-fix-unload-lifetime.patch"
} > "$OUT/build-info.txt"
(cd "$OUT" && sha256sum kpimg kpimg.elf kptools > SHA256SUMS)
