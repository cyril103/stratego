#!/usr/bin/env bash
set -euo pipefail
if [[ ${1:-} == --help ]]; then
    echo "Usage: bash tools/build_linux_mint.sh [build-directory] [--test]"
    echo "Build on Linux Mint 22.x or Ubuntu 24.04, x86_64. --test runs all engine tests."
    exit 0
fi
[[ $(uname -s) == Linux && $(uname -m) == x86_64 ]] || { echo 'Linux x86_64 requis.' >&2; exit 1; }
# A newer build host can silently introduce a glibc dependency unavailable
# on Mint 22. Fail rather than label such a binary as compatible.
. /etc/os-release
if [[ ! ( $ID == ubuntu && $VERSION_ID == 24.04 ) &&
      ! ( $ID == linuxmint && $VERSION_ID == 22* ) ]]; then
    echo 'Construire sur Ubuntu 24.04 ou Linux Mint 22.x pour garantir la base ABI.' >&2
    exit 1
fi
for tool in cmake gcc git dpkg-shlibdeps cpack file; do
    command -v "$tool" >/dev/null || { echo "Outil manquant : $tool (voir docs/LINUX_MINT.md)" >&2; exit 1; }
done
source_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
build_dir=${1:-"$source_dir/build-linux-mint"}
run_tests=${2:-}
[[ -z $run_tests || $run_tests == --test ]] || { echo 'Option inconnue.' >&2; exit 1; }
jobs=${STRATEGO_BUILD_JOBS:-2}
[[ $jobs =~ ^[1-9][0-9]*$ ]] || { echo 'STRATEGO_BUILD_JOBS doit etre positif.' >&2; exit 1; }
cmake -S "$source_dir" -B "$build_dir" -DCMAKE_BUILD_TYPE=Release \
    -DGLFW_BUILD_WAYLAND=OFF -DGLFW_BUILD_X11=ON
if [[ $run_tests == --test ]]; then
    cmake --build "$build_dir" --parallel "$jobs"
    ctest --test-dir "$build_dir" --output-on-failure --parallel 2
else
    cmake --build "$build_dir" --target stratego --parallel "$jobs"
fi
cpack --config "$build_dir/CPackConfig.cmake" -G DEB
sha256sum "$build_dir"/packages/stratego3d_*.deb
