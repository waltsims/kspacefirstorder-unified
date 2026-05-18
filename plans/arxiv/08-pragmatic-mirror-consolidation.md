# Pragmatic mirror consolidation (release-cost reduction)

> **Companion to [`07-unification-roadmap.md`](07-unification-roadmap.md).**
> That document targets the long-horizon end state — a single shared core
> library with platform/accelerator adapters. This document targets the
> *next concrete step*: the smallest structural change that captures most
> of the operational payoff, justified by what shipping a release in this
> codebase actually costs today.

## Problem

The k-wave binary distribution lives across **five mirror repos**, each
mirroring an upstream MATLAB k-Wave per-platform tarball:

| Repo                                | What it ships     | Build system today |
|-------------------------------------|-------------------|--------------------|
| `kspaceFirstOrder-CUDA-linux`       | Linux CUDA binary | Makefile + (now) CMakeLists.txt |
| `kspaceFirstOrder-CUDA-windows`     | Windows CUDA binary | `.vcxproj` + (now) CMakeLists.txt |
| `kspaceFirstOrder-OMP-linux`        | Linux OMP binary  | CMakeLists.txt |
| `kspaceFirstOrder-OMP-windows`      | Windows OMP binary| CMakeLists.txt + vcpkg.json |
| `k-wave-omp-darwin`                 | macOS OMP binary  | CMakeLists.txt (per-subdir) |

`kspacefirstorder-unified` coordinates them via git submodules + CI.

The C++ source within each **CUDA pair** is 95%+ identical (same upstream
Brno SC@FIT code, just different build wrappers + per-platform vcpkg /
HDF5 / cuFFT lookups). Same for the **OMP triple**. The only material
per-platform differences are in build glue: include paths, vcpkg vs
Homebrew, MSBuild vs make, fast-math flag dialect (apple-clang vs gcc).

## Evidence — what shipping v1.4.0 actually cost

Shipping **one logical feature** (NVIDIA Blackwell / sm_120 support)
on 2026-05-16 took roughly **12 PRs**:

| # | Repo | PR | What |
|---|---|---|---|
| 1 | CUDA-linux | #5 | Makefile: add sm_120 with CUDA-version gate |
| 2 | CUDA-windows | #1 | Makefile: add sm_120 |
| 3 | CUDA-windows | #2 | vcxproj: CUDA 12.2/13.0 fallback + ResolvedVcpkgRoot |
| 4 | CUDA-linux | #4 | CUFFT `#ifdef CUDART_VERSION < 13000` |
| 5 | CUDA-windows | (in #2) | Same CUFFT `#ifdef` — duplicated by hand |
| 6 | CUDA-linux | #6 | Add CMakeLists.txt + cmake/SetupCUDA.cmake |
| 7 | CUDA-windows | #3 | Same CMakeLists.txt + SetupCUDA.cmake — duplicated by hand |
| 8 | CUDA-linux | #7 | Pre-`enable_language(CUDA)` arch pre-filter |
| 9 | CUDA-windows | #4 | Same pre-filter — duplicated by hand |
| 10 | unified | #5 | Submodule SHA bumps |
| 11 | unified | #7 | CI dedupe (delete duplicate `ci-windows.yml`) |
| 12 | unified | #9, #10, #8 | Smoke-test fixes, cache cleanup, README ref repair |

**~6 of those 12 PRs are pure mirror-duplication tax**: the same diff
applied twice because we have separate CUDA-linux and CUDA-windows
repos. Plus the manual submodule-SHA dance in unified after every
binary-repo merge (re-bumped 4 times during this release).

Plus we hit a real bug nobody would have hit with a single repo: the
`enable_language(CUDA)` test compile blew up on CUDA 12.2 because the
CMakeLists.txt's default arch list (which we'd duplicated, identically,
across both repos) included `sm_100` and `sm_120`. One repo, one CMake
file, one fix. Two repos, two CMake files, two fixes — easy to forget
the second.

## Proposal — pragmatic consolidation in two steps

### Step 1: collapse the CUDA pair into a single `kspace-cuda` repo

The two repos currently differ in:
- `Makefile` (Linux only) vs `kspaceFirstOrder-CUDA.vcxproj` (Windows only)
- `CufftComplexMatrix.cpp` indentation/whitespace (cosmetic)
- A handful of `#ifdef _WIN32` / `__linux__` blocks that exist in both
  already

Target structure (`kspace-cuda/`):
```
kspace-cuda/
├── CMakeLists.txt                 (single source of truth; today's per-platform
│                                   CMakeLists.txt collapses into this with
│                                   `if(WIN32)` branches for vcpkg vs system HDF5)
├── cmake/
│   ├── SetupCUDA.cmake            (already unified; the prefilter fix happened
│                                   once here instead of twice)
│   └── PlatformDeps.cmake         (NEW — picks vcpkg on Windows, system pkg
│                                   on Linux, exposes `kspace::hdf5` target)
├── Containers/   …                (shared C++ source — single copy)
├── KSpaceSolver/ …
├── MatrixClasses/ …               (CufftComplexMatrix.cpp lives here once)
└── Logger/ …
```

Retirements:
- `Makefile` (deleted; CMake covers both Linux and Windows builds)
- `kspaceFirstOrder-CUDA.vcxproj` (deleted; CMake generates one if needed)

The two upstream mirrors become a single repo. Future releases:
- New CUDA arch → one PR, one Makefile-equivalent (CMake), one CUFFT update
- Submodule pin in unified: one entry instead of two

### Step 2: collapse the OMP triple into a single `kspace-openmp` repo

Same pattern, larger payoff because it's three repos collapsing to one.
Per-platform differences:

| Concern | Linux | Windows | macOS |
|---|---|---|---|
| Dep manager | apt / EBROOT | vcpkg | Homebrew |
| OpenMP flag | `-fopenmp` (gcc) | `/openmp` (MSVC) | `-Xpreprocessor -fopenmp -lomp` (apple-clang + libomp) |
| Fast-math | `-ffast-math` | `/fp:fast` | `-fno-fast-math -ffp-contract=fast` (NaN fix from k-wave-omp-darwin#3) |
| HDF5 lib name | `libhdf5_hl` (static or shared) | `hdf5_hl-static.lib` | brew's `libhdf5_hl` |

All of these are routine `if(WIN32)/if(APPLE)` branches in a single
CMakeLists.txt. The k-wave-omp-darwin per-subdirectory CMakeLists
style would carry over unchanged.

### Step 3: webhook-driven submodule auto-bump in unified

A small GHA in unified that runs on `workflow_dispatch` (or on a webhook
from the binary repos via `repository_dispatch`):

```yaml
on:
  repository_dispatch:
    types: [binary-repo-pushed]
jobs:
  bump:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v5
        with:
          submodules: true
          token: ${{ secrets.UNIFIED_BUMP_TOKEN }}
      - run: |
          git submodule update --remote
          git diff --quiet || (
            git config user.email auto-bump@k-wave.org
            git config user.name "Auto-bump bot"
            git commit -am "Auto-bump submodules"
            git push
          )
```

Combined with steps 1 and 2 this becomes a 2-line update (the two
remaining submodules) on any binary-repo push, then CI validates.

## Migration plan (incremental, low-risk)

The trick is doing this **without** breaking the current k-wave-python
download flow.

1. **Phase A — make CMake the only path on each existing repo.**
   Retire `Makefile` and `.vcxproj` per binary repo. Already done for
   `kspaceFirstOrder-OMP-{linux,windows,darwin}`; still pending on the
   CUDA pair (where the legacy Makefile + .vcxproj still ship). Tracking:
   leave the legacy build files alongside CMake for one release, then
   delete in the release after that.

2. **Phase B — create `kspace-cuda` as a new repo.**
   Initial commit is the union of `kspaceFirstOrder-CUDA-{linux,windows}`'s
   source with one shared CMakeLists.txt (CMake-only). Tag `v1.5.0` at
   creation. unified gets a new submodule pin alongside (not replacing)
   the existing two, so we can A/B compare builds for one release.

3. **Phase C — flip k-wave-python URL pins** from the per-platform
   binary repos to the new `kspace-cuda`. Both upload paths produce the
   same artifact bytes; k-wave-python `__init__.py` just changes which
   release URL it downloads from. Validated against the A/B from Phase B.

4. **Phase D — archive the per-platform CUDA mirrors.** Keep them
   read-only for users still pinned to older k-wave-python releases.

5. **Phase E — repeat A-D for the OMP triple.**

## Out of scope for this plan

- Full unification into a single core library with adapters (that's
  `07-unification-roadmap.md`).
- Changes to the k-wave-python Python solver path (independent of
  binary distribution).
- Changes to upstream MATLAB k-Wave's own structure.

## Acceptance criteria

- Shipping a new CUDA arch (e.g. sm_130 when Rubin lands) requires:
  - ≤ 2 PRs (one to `kspace-cuda`, one to k-wave-python `__init__.py`)
  - 0 manual submodule-SHA edits on unified (auto-bumped)
- Per-release operational time approximately ⅕ of the v1.4.0 baseline.
