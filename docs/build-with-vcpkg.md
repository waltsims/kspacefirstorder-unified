# Build with vcpkg

This project uses vcpkg manifest mode to provide reproducible, cross‑platform dependency management for HDF5 and FFTW (single precision with threads). Presets are provided to streamline configuration and builds.

## Quick Start

1) Bootstrap vcpkg

- Linux/macOS:

  ```bash
  ./third-party/vcpkg/bootstrap-vcpkg.sh -disableMetrics
  ```

- Windows (PowerShell):

  ```powershell
  .\third-party\vcpkg\bootstrap-vcpkg.bat -disableMetrics
  ```

2) Configure with CMake presets

- Linux CPU (OpenMP):

  ```bash
  cmake --preset linux-openmp
  cmake --build --preset linux-openmp --config Release
  ```

- macOS CPU (OpenMP):

  ```bash
  cmake --preset macos-openmp
  cmake --build --preset macos-openmp --config Release
  ```

- Windows CPU (OpenMP):

  ```powershell
  cmake --preset windows-openmp
  cmake --build --preset windows-openmp --config Release
  ```

The presets automatically:
- Set `CMAKE_TOOLCHAIN_FILE` to `third-party/vcpkg/scripts/buildsystems/vcpkg.cmake`
- Use the correct vcpkg triplet (`x64-linux`, `x64-osx`, `x64-windows`)
- Enable `USE_OPENMP=ON` and `USE_CUDA=OFF` by default

## Dependencies

Declared in `vcpkg.json`:
- `hdf5`
- `fftw3[float,threads]`
- `openmp` (macOS only, to provide libomp for Apple Clang)

CUDA is not managed via vcpkg; use the platform’s CUDA Toolkit if you enable the CUDA backend.

## Notes

- If you prefer a different vcpkg location, set `VCPKG_ROOT` before configuring:
  - `export VCPKG_ROOT=/path/to/vcpkg` (bash)
  - `$env:VCPKG_ROOT = 'C:\path\to\vcpkg'` (PowerShell)
- Binary caching is supported by default in CI; locally you can set `VCPKG_DEFAULT_BINARY_CACHE` to a writable folder for speed.

