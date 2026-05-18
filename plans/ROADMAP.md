# Roadmap

The live tracker of what's pending on `kspacefirstorder-unified` after the mirror consolidation work. Historical planning documents (the eleven that drove the work to date) are preserved under [`arxiv/`](arxiv/) for context — refer to them when you need the *why* behind decisions already made; do not edit them.

## Where we are (2026-05)

- Five per-platform mirror repos have been collapsed into this repo as in-tree subtrees under `repos/<name>/`. CI builds from these subdirs (`.github/workflows/ci-multi-platform.yml`).
- v1.4.1 binaries shipped from the per-mirror releases. v1.4.2 will be the first release tagged from this repo, carrying the same binary bytes.
- k-wave-python v0.6.2 consumes the per-mirror v1.4.1 release URLs. The flip to unified release URLs is tracked as a consumer-side issue.

## In flight

### Release-flow rewrite — [#19](https://github.com/waltsims/kspacefirstorder-unified/pull/19)

`.github/workflows/release-on-tag.yml` flipped from publishing five per-mirror releases to publishing one unified release on this repo with platform-suffixed asset names (`kspaceFirstOrder-CUDA-linux`, `kspaceFirstOrder-OMP-windows.exe`, etc.). Drops the `BINARY_RELEASE_TOKEN` PAT — default `GITHUB_TOKEN` with job-scoped `contents: write` is enough.

After this lands: workflow_dispatch with `version=v1.4.2`, `dry_run=true` first, then re-run with `dry_run=false` to cut the release.

### Stale workflow cleanup — [#21](https://github.com/waltsims/kspacefirstorder-unified/pull/21)

Deletes the dead `repos/kspaceFirstOrder-openmp-windows/.github/workflows/windows.yml` that came in with the subtree-add. Closes [#20](https://github.com/waltsims/kspacefirstorder-unified/issues/20).

## Next, in order

1. **Tag and ship v1.4.2** under the new release flow. Same binary contents as v1.4.1, new publishing location. Validates the unified release URLs.
2. **k-wave-python URL flip** ([k-wave-python#738](https://github.com/waltsims/k-wave-python/issues/738)) — collapse `URL_DICT` in `kwave/__init__.py` from five per-mirror URLs to one base pointing at the unified release. Drops `WINDOWS_OMP_VERSION` / `WINDOWS_CUDA_VERSION` overrides and `get_windows_release_urls()` indirection. Ships in k-wave-python v0.6.3 or v0.7.0.
3. **Windows CUDA DLL packaging** ([#17](https://github.com/waltsims/kspacefirstorder-unified/issues/17)) — `v1.4.x` windows-CUDA release ships no runtime DLLs. Mirror the bundling pattern already in `windows-openmp` (vcpkg DLLs + MSVC redist) and add CUDA runtime DLLs (`cudart64_*.dll`, `cufft64_*.dll`) from `$env:CUDA_PATH\bin`. Unblocks dropping the Windows v1.3.0 pin in k-wave-python.
4. **Archive the 5 mirror repos** (read-only) once k-wave-python has shipped one release with the unified URL pins and baked for a cycle. Their existing releases stay reachable forever via `releases/download/v1.4.1/...` so older k-wave-python installs keep downloading. See [unified#13](https://github.com/waltsims/kspacefirstorder-unified/issues/13).

## Architectural follow-ups (not blocking releases)

### Subtree path refactor — `repos/<name>/` → unified layout

After the consolidation lands, `repos/<five names>/` is a working but ugly layout. Plan 08 (archived) proposes collapsing the CUDA pair into `kspace-cuda/` with one `CMakeLists.txt` (branched on `if(WIN32)`) and the OpenMP triple into `kspace-openmp/` similarly. The CMake migration that already happened on each mirror makes this mostly a directory move + CMake unification.

Defer until release flow is proven and the Windows DLL story is resolved — both touch `cmake/SetupCUDA.cmake` and per-platform configure paths, so churning the directory layout in parallel would conflict.

### Long-horizon: shared core, dual interface (CLI + nanobind), wheel distribution

End state: a single C++ core library (Containers, KSpaceSolver, MatrixClasses, Logger, OutputStreams, Parameters — refactored to be I/O-agnostic) with two adapters plugging in:

- **`kspaceFirstOrder-CUDA` adapter** — cuFFT, CUDA kernels
- **`kspaceFirstOrder-OMP` adapter** — FFTW, OpenMP loops

And two consumers of that core:

- **`cli/`** — `main()` + HDF5 I/O. Existing standalone binary for MATLAB / k-wave-cupy / HPC batch users. The legacy contract stays unchanged.
- **`python/`** — nanobind module. Pass NumPy views in, get NumPy arrays back. No HDF5 round-trip, no subprocess fork, no per-platform binary download.

Building the adapters along the **backend axis** (CUDA pair → `kspace-cuda/`, OMP triple → `kspace-openmp/`, then both → core+adapter split) is plan-08's direction — same as the path refactor above. Collapsing along the platform axis (Linux pair → "linux", etc.) is the wrong axis: within a backend the C++ source is ~95% shared across platforms (only build-glue differs), but across backends within a platform it's ~30% at best (different solvers, cuFFT vs FFTW, different memory model).

#### Strategic payoff of the nanobind path

k-wave-python ships as Python wheels with the C++ adapter baked in (`manylinux2014` × `macosx_arm64` × `win_amd64`, ~6 wheels total counting `[cuda]` extras). Three pain points collapse in one move:

1. **Binary-download dance disappears.** `kwave/__init__.py` (today ~250 lines of platform-specific URL/hash/exec-bit logic) shrinks to ~10 lines. No `BINARY_VERSION` pin, no `URL_DICT`, no `WINDOWS_DLLS`, no `_ensure_executable` self-heal.
2. **HDF5 ABI saga stops being a thing.** The macOS `libhdf5.310 → .320` break that bit v0.6.2 only mattered because the binary linked against system HDF5 and users had a different version. Wheel builds link statically or via the manylinux toolchain (audited with `auditwheel repair`). End of that bug class.
3. **F-order ↔ C-order conversion** moves from `cpp_simulation._write_hdf5()` to the FFI boundary — same logic, cleaner location, fewer copies.

#### Transition for `save_only=True`

That mode (writes HDF5 input without running the simulation, so HPC users can submit to a queue) needs to stay. Expose an optional `dump_to_hdf5(path)` on the nanobind module, backed by the same HDF5 writer the CLI uses. `h5py` as a runtime Python dep of k-wave-python still goes away — the writing happens in C++.

#### Sequencing

Two execution orders are plausible:

- **(A) Path refactor first** (plan-08 directory collapse), then core+adapters, then nanobind. Conservative — each step is independently shippable.
- **(B) Core+adapters refactor and nanobind together**, since the prerequisite for nanobind is exactly the I/O-agnostic core. The user-facing payoff (wheel distribution, no HDF5 dep) is bigger and lands sooner.

(B) is the right call if you can fund a multi-week refactor; (A) is right if you're shipping in slices. Either way, k-wave-python should hold off on cosmetic refactors of `kwave/__init__.py` / `cpp_simulation.py` until the nanobind story is committed — otherwise that work is throwaway.

Open infrastructure tasks once the refactor lands: CMakePresets.json, clang-tidy/clang-format targets, CPack packaging (for the standalone CLI binary), GoogleTest/Catch2 + CTest, `cibuildwheel` configuration for the Python wheels.

## Quality-of-life nits (open if you care)

- **Single-source `cuda-version: 13.0.0`** — currently duplicated between the CI matrix in `ci-multi-platform.yml` and the asset paths in `release-on-tag.yml`. A version bump in one place silently breaks the other. Cheap to hoist into a top-level `env:`.
- **Skip the build matrix on `dry_run=true`** — saves ~30 CI minutes per dry-run. Trade-off: dry-run no longer validates "build still works at this SHA". Worth it if you mostly use dry-run to inspect the asset manifest before a real publish.
- **`softprops/action-gh-release@v2`** would collapse the `gh release create` + `gh release upload` plumbing to ~8 lines with native idempotency and a `prerelease:` boolean. Adds a third-party action dependency; deferred for that reason.
