# k-spaceFirstOrder vcpkg Migration Plan

## Metadata
- **Document Type**: Implementation Plan
- **Target Audience**: Build engineers, CI maintainers, contributors
- **Created**: 2025-09-28
- **Version**: 0.1 (draft)
- **Status**: Planning

## Executive Summary

**Objective**: Consolidate all dependency management across Linux, macOS, and Windows onto vcpkg, delivering reproducible builds and consistent CI behaviour while simplifying contributor onboarding.

**Key Results**:
- One canonical vcpkg manifest powering developer builds and CI
- Drop legacy manual install scripts and bespoke Find modules
- Reduce cold-start setup to `vcpkg bootstrap` + `cmake --preset <target>`
- Parity across OpenMP and CUDA backends where supported

## Current State Snapshot
- Hybrid dependency story: system packages + custom Find modules + bundled sources
- CI installs packages manually on Linux/macOS; Windows downloads zip archives
- Developer onboarding varies per OS and relies on manual documentation
- Third-party directory already contains vcpkg clone but is not the source of truth
- No vcpkg manifests/presets checked in; toolchain usage ad-hoc

## Migration Goals
1. Ship `vcpkg-configuration.json` + `vcpkg.json` defining all libraries, triplets, and registries
2. Adopt CMake presets referencing vcpkg toolchain for all standard build modes
3. Update CI workflows to bootstrap vcpkg, install dependencies via manifest mode, and build/test
4. Provide developer quick-start instructions using vcpkg for every platform
5. Enable cache-friendly binary reuse (CI + local) via artifact caching or vcpkg binary caching

## Scope
- Dependencies: HDF5, FFTW (single precision), OpenMP runtime requirements, CUDA stubs, testing utilities
- Tooling: CMake, vcpkg, CI environments (GitHub Actions), local dev scripts
- Platforms: Ubuntu (22.04+), macOS (latest), Windows (Visual Studio 2022)
- Out of scope: Packaging installers, alternative package managers, non-supported OS ports

## Strategy Overview

### 1. Baseline Inventory
- Catalogue current dependency versions and options in CMake
- Identify platform-specific patches or compile definitions applied today
- Document gaps between vcpkg ports and required features (e.g., FFTW threading, static vs shared)

### 2. Manifest Design
- Create `vcpkg.json` listing direct dependencies with semantic version bounds or port features
- Add custom registries or overlays if upstream ports need patching
- Define triplets (`x64-windows`, `x64-osx`, `x64-linux`) with consistent static/shared selection

### 3. Build System Integration
- Introduce `cmake-presets.json` linking to vcpkg toolchain via `VCPKG_ROOT` or relative path
- Update `CMakeLists.txt` to rely on vcpkg-provided config packages; remove bespoke find logic where redundant
- Ensure optional components (CUDA, tests) gate on manifest features or CMake options

### 4. CI Modernisation
- Extend `.github/workflows/ci.yml` to:
  - Cache vcpkg binary packages per triplet/backend combo
  - Run `vcpkg install --triplet <...>` (manifest mode) prior to CMake configure
  - Export consumed binary cache statistics in build summary
- Validate Windows OpenMP job using vcpkg-provided HDF5/FFTW

### 5. Developer Experience
- Replace `install_dependencies.sh` with vcpkg bootstrap instructions, fallback to manual only for edge cases
- Provide `docs/build-with-vcpkg.md` covering bootstrap, presets, troubleshooting, binary caching tips
- Offer optional convenience wrapper `scripts/bootstrap-vcpkg.sh` and PowerShell equivalent

### 6. Decommission Legacy Paths
- Remove or archive unused third-party sources once vcpkg ports verified
- Simplify `FindDependencies.cmake` to a thin sanity check, or retire it entirely in favour of imported targets
- Update plans/documentation referencing old workflows

## Implementation Phases

### Phase 0 – Preparation (2 days)
- [ ] Confirm ownership of existing `third-party/vcpkg`
- [ ] Update submodule/embedded vcpkg to latest release
- [ ] Decide on manifest vs classic mode (target: manifest)

### Phase 1 – Prototype (1 week)
- [ ] Draft `vcpkg.json` + triplets, test on Ubuntu desktop
- [ ] Build OpenMP backend via presets; confirm runtime
- [ ] Validate CUDA dependencies availability or plan fallback

### Phase 2 – Cross-Platform Enablement (1 week)
- [ ] macOS build using Homebrew-provided LLVM or Apple Clang + vcpkg libs
- [ ] Windows MSVC build with OpenMP and proper runtime libs
- [ ] Resolve port inconsistencies (patches, features) via overlays if needed

### Phase 3 – CI Rollout (1 week)
- [ ] Update GitHub Actions to bootstrap vcpkg and cache binaries
- [ ] Ensure matrix covers all triplets/backends
- [ ] Add diagnostics for manifest diff, cache hit rate, install logs

### Phase 4 – Documentation & Cleanup (3 days)
- [ ] Publish developer guide and update README quickstart
- [ ] Remove superseded scripts and references
- [ ] Announce migration plan and deprecation timeline for legacy flow

## Risks & Mitigations
- **Port gaps or bugs**: Maintain overlay ports; contribute patches upstream
- **Binary cache size/time limits**: Tune `VCPKG_DEFAULT_BINARY_CACHE`, leverage GitHub cache per triplet
- **Contributor friction**: Provide scripts and clear fallback instructions; keep CMake manual path functional temporarily
- **CI downtime during switch**: Roll out behind feature branch, ensure old workflow available until new path stabilized

## Success Criteria
- CI jobs run solely via vcpkg-provisioned dependencies across all platforms
- New contributor can configure and build in under 15 minutes with documented steps
- `FindDependencies.cmake` reduced to compatibility shim or deprecated
- Binary cache hit rate ≥70% in CI for consecutive runs

## Follow-Up Actions
- Simplify CMakeLists.txt to be clean and readable without complicated options and expressions.
- Track upstream contributions to vcpkg ports used by project
- Establish monthly dependency update cadence leveraging vcpkg manifests
- Evaluate enabling `vcpkg x-update-baseline` to lock dependency graphs

---

Prepared by: GPT-5 Codex (Cursor AI assistant)


