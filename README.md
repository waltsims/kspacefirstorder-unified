# kspaceFirstOrder - Unified C++ Implementation

[![CI](https://github.com/waltsims/kspaceFirstOrder-unified/actions/workflows/ci-multi-platform.yml/badge.svg)](https://github.com/waltsims/kspaceFirstOrder-unified/actions/workflows/ci-multi-platform.yml)
[![License: LGPL v3](https://img.shields.io/badge/License-LGPL%20v3-blue.svg)](https://www.gnu.org/licenses/lgpl-3.0.en.html)

## Overview

k-Wave is an open source toolbox originally written MATLAB designed for the time-domain simulation of propagating acoustic waves in 1D, 2D, or 3D. The toolbox has a wide range of functionality, but at its heart is an numerical model that can account for both linear or nonlinear wave propagation, an arbitrary distribution of heterogeneous material parameters, and power law acoustic absorption. See the [k-Wave website](http://www.k-wave.org) for further details.

This project builds on the great work of the original k-Wave authors: B. E. Treeby, J. Jaros, A. P. Rendell, and B. T. Cox, and the SC@FIT Research Group at Brno University of Technology who developed the original C++ implementations.

This project is a unified C++ implementation of the k-Wave toolbox that accelerates 2D/3D simulations using optimized implementations.

## Repository layout

All six platform/backend variants live in-tree under `repos/`:

- `repos/kspaceFirstOrder-cuda-linux/` — Linux CUDA
- `repos/kspaceFirstOrder-cuda-windows/` — Windows CUDA
- `repos/kspaceFirstOrder-openmp-linux/` — Linux OpenMP
- `repos/kspaceFirstOrder-openmp-windows/` — Windows OpenMP
- `repos/kspaceFirstOrder-openmp-darwin/` — macOS OpenMP
- `repos/kspaceFirstOrder-mlx-darwin/` — macOS Apple Silicon GPU (the CUDA solver ported to [MLX](https://github.com/ml-explore/mlx) / Metal)

The first five used to be separate mirror repos, consolidated here as `git subtree`s (with history preserved). See [`plans/ROADMAP.md`](plans/ROADMAP.md) for what's next.

## Building

All six variants build via [`.github/workflows/ci-multi-platform.yml`](.github/workflows/ci-multi-platform.yml) — that workflow is the canonical reference for the exact CMake invocations, dependency setup, and platform pinning per build.

### Windows (OpenMP via CMake + vcpkg)

Worked example. Reproduces the CI `windows-openmp` job locally:

1. Install Visual Studio 2022 (Desktop development with C++ workload), [Ninja](https://ninja-build.org/), and bootstrap [vcpkg](https://github.com/microsoft/vcpkg) (for example into `C:\vcpkg`).

2. Configure the build, pointing CMake at the Windows OpenMP tree and the vcpkg toolchain:

   ```powershell
   cmake -S repos/kspaceFirstOrder-openmp-windows -B build/openmp-windows `
     -G "Ninja" `
     -DCMAKE_BUILD_TYPE=Release `
     -DCMAKE_TOOLCHAIN_FILE="C:/vcpkg/scripts/buildsystems/vcpkg.cmake" `
     -DUSE_MKL=OFF `
     -DENABLE_OPENMP=ON `
     -DARCH=avx2
   ```

   The manifest automatically pulls FFTW, HDF5 (with zlib + szip), and zlib through vcpkg—no manual `vcpkg install` commands are required.

3. Build the executable:

   ```powershell
   cmake --build build/openmp-windows --config Release --parallel
   ```

The resulting binary (`kspaceFirstOrder-OMP.exe`) is emitted in `build/openmp-windows/`.

### macOS Apple Silicon GPU (MLX)

Reproduces the CI `macos-mlx` job locally (needs Xcode or the Command Line Tools with the Metal compiler):

```bash
brew install cmake hdf5 libomp
cmake -S repos/kspaceFirstOrder-mlx-darwin -B build/mlx-darwin \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_DEPLOYMENT_TARGET=14.0 -DKWAVE_BUNDLE_DEPS=ON
cmake --build build/mlx-darwin --parallel
```

`KWAVE_BUNDLE_DEPS=ON` builds MLX from source and links it, HDF5 and libomp statically, so `kspaceFirstOrder-MLX`
only depends on macOS system libraries; it loads `mlx.metallib` (emitted next to it, ~3.5 MB) from its own folder, so
the two files must be shipped together. Released as `kspaceFirstOrder-MLX-darwin` + `mlx.metallib`. Without
`KWAVE_BUNDLE_DEPS`, the build links Homebrew's `mlx` (`brew install mlx`) instead. See
[`repos/kspaceFirstOrder-mlx-darwin/Readme.md`](repos/kspaceFirstOrder-mlx-darwin/Readme.md) for details, validation
and benchmarks.

## License

This project is licensed under the GNU Lesser General Public License v3.0. See the [LICENSE.md](LICENSE.md) file for details.
