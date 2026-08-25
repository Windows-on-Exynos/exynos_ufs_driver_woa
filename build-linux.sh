#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "$0")" && pwd)"
src="$root/slsiufsstor"
out="${OUT_DIR:-$root/out/ARM64/Release}"
wdk="${WDK_ROOT:?Set WDK_ROOT to the c directory in Microsoft.Windows.WDK.ARM64}"
sdk="${SDK_ROOT:-$wdk}"
clang_root="${CLANG_ROOT:?Set CLANG_ROOT to a directory containing clang and lld}"
kit_version="${KIT_VERSION:-10.0.26100.0}"

mkdir -p "$out"

common=(
  --target=aarch64-pc-windows-msvc
  -fms-extensions
  -fms-compatibility
  -fms-compatibility-version=19.40
  -ffreestanding
  -fno-builtin
  -fno-stack-protector
  -Wall
  -Wextra
  -Werror
  -Wno-invalid-token-paste
  -Wno-microsoft-anon-tag
  -Wno-ignored-attributes
  -D_ARM64_
  -DARM64
  -D_USE_DECLSPECS_FOR_SAL=1
  -DSTD_CALL
  -DNTDDI_VERSION=0x0A000008
  -D_WIN32_WINNT=0x0A00
  -DWINVER=0x0A00
  -D__UFS_CAL_WINDOWS__
  -I"$src"
  -isystem "$sdk/Include/$kit_version/shared"
  -isystem "$wdk/Include/$kit_version/km"
  -isystem "$wdk/Include/$kit_version/km/crt"
)

"$clang_root/clang" "${common[@]}" -O2 -c \
  "$src/SlsiUfsPlatform.c" -o "$out/SlsiUfsPlatform.obj"
"$clang_root/clang" "${common[@]}" -O2 -c \
  "$src/UfsCal9610.c" -o "$out/UfsCal9610.obj"

"$clang_root/lld" -flavor link \
  /driver /entry:DriverEntry /machine:arm64 /subsystem:native,10.0 \
  /osversion:10.0 /nodefaultlib /dynamicbase /nxcompat /release \
  /stack:0x40000,0x2000 /out:"$out/slsiufsstor.sys" \
  "$out/SlsiUfsPlatform.obj" "$out/UfsCal9610.obj" \
  "$wdk/Lib/$kit_version/km/ARM64/ntoskrnl.lib" \
  "$wdk/Lib/$kit_version/km/ARM64/hal.lib" \
  "$wdk/Lib/$kit_version/km/ARM64/libcntpr.lib"

cp "$src/slsiufsstor.inf" "$out/"
file "$out/slsiufsstor.sys"

llvm_size="${LLVM_SIZE:-$clang_root/llvm-size}"
if [[ -x "$llvm_size" ]]; then
  "$llvm_size" "$out/slsiufsstor.sys"
fi
