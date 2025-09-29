#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
VCPKG_ROOT="${VCPKG_ROOT:-$ROOT_DIR/third-party/vcpkg}"

echo "Bootstrapping vcpkg at: $VCPKG_ROOT"
"$VCPKG_ROOT/bootstrap-vcpkg.sh" -disableMetrics

echo "Installing manifest dependencies (triplet autodetected by vcpkg)"
"$VCPKG_ROOT/vcpkg" install || true

echo "Done. Set VCPKG_ROOT=$VCPKG_ROOT when configuring CMake if needed."

