# kspaceFirstOrder - Unified C++ Implementation

[![CI](https://github.com/waltsims/kspaceFirstOrder-unified/actions/workflows/ci-multi-platform.yml/badge.svg)](https://github.com/waltsims/kspaceFirstOrder-unified/actions/workflows/ci-multi-platform.yml)
[![License: LGPL v3](https://img.shields.io/badge/License-LGPL%20v3-blue.svg)](https://www.gnu.org/licenses/lgpl-3.0.en.html)

## Overview

k-Wave is an open source toolbox originally written MATLAB designed for the time-domain simulation of propagating acoustic waves in 1D, 2D, or 3D. The toolbox has a wide range of functionality, but at its heart is an numerical model that can account for both linear or nonlinear wave propagation, an arbitrary distribution of heterogeneous material parameters, and power law acoustic absorption. See the [k-Wave website](http://www.k-wave.org) for further details.

This project builds on the great work of the original k-Wave authors: B. E. Treeby, J. Jaros, A. P. Rendell, and B. T. Cox, and the SC@FIT Research Group at Brno University of Technology who developed the original C++ implementations.

This project is a unified C++ implementation of the k-Wave toolbox that accelerates 2D/3D simulations using optimized implementations.

## Building

### Windows (OpenMP via CMake + vcpkg)

The Windows OpenMP implementation now ships with a CMake build that mirrors the upstream `kspaceFirstOrder-OMP-windows` repository and is exercised in `.github/workflows/ci-windows.yml`. To reproduce the build locally:

1. Install Visual Studio 2022 (Desktop development with C++ workload), [Ninja](https://ninja-build.org/), and bootstrap [vcpkg](https://github.com/microsoft/vcpkg) (for example into `C:\vcpkg`).
2. Update submodules so the Windows sources and manifest (`vcpkg.json`) are present:

   ```powershell
   git submodule update --init --recursive
   ```

3. Configure the build, pointing CMake at the Windows OpenMP tree and the vcpkg toolchain:

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

4. Build the executable:

   ```powershell
   cmake --build build/openmp-windows --config Release --parallel
   ```

The resulting binary (`kspaceFirstOrder-OMP.exe`) is emitted in `build/openmp-windows/`.

## License

This project is licensed under the GNU Lesser General Public License v3.0. See the [LICENSE.md](LICENSE.md) file for details.
