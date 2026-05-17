#!/usr/bin/env bash
# Verify a freshly-built Linux binary is statically linked for the libraries
# we ship — and that its glibc baseline is no newer than what we promise.
#
# Usage: check-linux-binary-deps.sh <binary-path> <flavor>
#   flavor ∈ {omp, cuda}
#
# Catches the exact regression that shipped in v1.4.0:
#   - dynamic libcudart/libcufft/libfftw3f/libhdf5/libstdc++ deps
#   - GLIBC_2.38+ required version (build runner upgrade)
# Either failure here means the binary won't run on Ubuntu 22.04 LTS / Colab
# / RHEL 9 — i.e. the majority of the install base.

set -euo pipefail

BIN="${1:?usage: $0 <binary-path> <flavor>}"
FLAVOR="${2:?usage: $0 <binary-path> <flavor>}"

# Maximum glibc symbol version we accept in the binary's dynamic-symbol table.
# Bumping this consciously is fine (e.g. when we move to a newer LTS floor);
# tripping it accidentally is the bug we're trying to prevent.
MAX_GLIBC="2.35"

# Library substrings that must NOT appear as dynamic deps. If any does, the
# build is dynamically linking something we explicitly ship statically.
# HDF5 (+ its szip/zlib transitive deps) is intentionally NOT on this list:
# distros ship a stable libhdf5_serial.so.103 ABI, and static HDF5 drags in
# transitive symbols that need explicit link-order treatment. The CUDA
# runtime and FFTW are the actual sources of the v1.4.0 regression.
BANNED_DEPS_COMMON=(
  "libstdc++"
)
BANNED_DEPS_CUDA=(
  "libcudart"
  "libcufft"
  "libculibos"
  "libnvJitLink"
)
BANNED_DEPS_OMP=(
  "libfftw3f"
)

if [[ ! -x "$BIN" ]]; then
  echo "FAIL: $BIN is not an executable file" >&2
  exit 2
fi

echo "=== Binary: $BIN ==="
file "$BIN"

echo
echo "=== ldd output ==="
LDD_OUT="$(ldd "$BIN" 2>&1 || true)"
echo "$LDD_OUT"

fail=0

# 1) No unresolved deps.
if grep -qE "not found" <<<"$LDD_OUT"; then
  echo
  echo "FAIL: ldd reports unresolved dynamic dependencies." >&2
  grep -E "not found" <<<"$LDD_OUT" >&2
  fail=1
fi

# 2) None of the banned substrings appear as dynamic deps.
banned=("${BANNED_DEPS_COMMON[@]}")
case "$FLAVOR" in
  cuda) banned+=("${BANNED_DEPS_CUDA[@]}") ;;
  omp)  banned+=("${BANNED_DEPS_OMP[@]}") ;;
  *) echo "FAIL: unknown flavor '$FLAVOR' (expected omp|cuda)" >&2; exit 2 ;;
esac

for needle in "${banned[@]}"; do
  if grep -qE "^[[:space:]]*${needle}" <<<"$LDD_OUT"; then
    echo
    echo "FAIL: '$needle' appears as a dynamic dependency — must be statically linked." >&2
    grep -E "^[[:space:]]*${needle}" <<<"$LDD_OUT" >&2
    fail=1
  fi
done

# 3) glibc symbol version floor. We parse the required versions from the
# dynamic symbol table and reject any GLIBC_x.y > MAX_GLIBC.
echo
echo "=== glibc symbol versions required (max allowed: $MAX_GLIBC) ==="
GLIBC_VERS="$(objdump -T "$BIN" 2>/dev/null | awk '
  match($0, /GLIBC_[0-9]+\.[0-9]+(\.[0-9]+)?/) {
    print substr($0, RSTART+6, RLENGTH-6)
  }' | sort -uV)"
echo "$GLIBC_VERS"

# Compare each version against MAX_GLIBC using sort -V; a version is "too new"
# if it sorts after MAX_GLIBC.
while read -r v; do
  [[ -z "$v" ]] && continue
  if [[ "$(printf '%s\n%s\n' "$MAX_GLIBC" "$v" | sort -V | tail -n1)" != "$MAX_GLIBC" ]]; then
    echo
    echo "FAIL: binary requires GLIBC_$v which is newer than the project's GLIBC_$MAX_GLIBC baseline." >&2
    echo "      Either the build runner was upgraded (pin it back), or bump MAX_GLIBC in this script." >&2
    fail=1
    break
  fi
done <<<"$GLIBC_VERS"

if (( fail )); then
  echo
  echo "=== check-linux-binary-deps: FAIL ==="
  exit 1
fi

echo
echo "=== check-linux-binary-deps: OK ==="
