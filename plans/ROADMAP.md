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

### Long-horizon: shared core library + adapters

The endpoint described in archived [`07-unification-roadmap.md`](arxiv/07-unification-roadmap.md): a single core C++ library housing common simulation logic (Containers, KSpaceSolver, MatrixClasses, Logger, OutputStreams, Parameters), with thin platform/accelerator adapters (OpenMP, CUDA) plugging into it. Today the duplication across `repos/kspaceFirstOrder-cuda-{linux,windows}/` is ~95% and across the OpenMP triple is similar — almost all of it ready to deduplicate once the path refactor above creates the directory shape.

Tasks at that point: CMakePresets.json, clang-tidy/clang-format targets, CPack packaging, GoogleTest/Catch2 + CTest. None blocking.

## Quality-of-life nits (open if you care)

- **Single-source `cuda-version: 13.0.0`** — currently duplicated between the CI matrix in `ci-multi-platform.yml` and the asset paths in `release-on-tag.yml`. A version bump in one place silently breaks the other. Cheap to hoist into a top-level `env:`.
- **Skip the build matrix on `dry_run=true`** — saves ~30 CI minutes per dry-run. Trade-off: dry-run no longer validates "build still works at this SHA". Worth it if you mostly use dry-run to inspect the asset manifest before a real publish.
- **`softprops/action-gh-release@v2`** would collapse the `gh release create` + `gh release upload` plumbing to ~8 lines with native idempotency and a `prerelease:` boolean. Adds a third-party action dependency; deferred for that reason.
