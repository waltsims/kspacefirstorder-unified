# kspaceFirstOrder - Unified C++ Implementation

[![CI](https://github.com/waltsims/kspaceFirstOrder-unified/actions/workflows/ci.yml/badge.svg)](https://github.com/waltsims/kspaceFirstOrder-unified/actions/workflows/ci.yml)
[![License: LGPL v3](https://img.shields.io/badge/License-LGPL%20v3-blue.svg)](https://www.gnu.org/licenses/lgpl-3.0.en.html)

## Overview

k-Wave is an open source toolbox originally written MATLAB designed for the time-domain simulation of propagating acoustic waves in 1D, 2D, or 3D. The toolbox has a wide range of functionality, but at its heart is an numerical model that can account for both linear or nonlinear wave propagation, an arbitrary distribution of heterogeneous material parameters, and power law acoustic absorption. See the [k-Wave website](http://www.k-wave.org) for further details.

This project builds on the great work of the original k-Wave authors: B. E. Treeby, J. Jaros, A. P. Rendell, and B. T. Cox, and the SC@FIT Research Group at Brno University of Technology who developed the original C++ implementations.

This project is a unified C++ implementation of the k-Wave toolbox that accelerates 2D/3D simulations using optimized implementations for different computational backends:

- **OpenMP**: Multi-threaded CPU implementation for shared memory systems
- **CUDA**: GPU-accelerated implementation for NVIDIA GPUs


## Features

- **Unified Build System**: Cross-platform CMake-based build system
- **Multiple Backends**: OpenMP (CPU) and CUDA (GPU) implementations
- **Cross-Platform**: Linux, macOS, and Windows support
- **High Performance**: Optimized for modern CPUs and GPUs
- **Flexible**: Support for linear/nonlinear wave propagation
- **Extensible**: Easy to add new backends or platforms

## Repository Structure

- **`src/`** - The unified source code and the single source of truth for this project.
  - **`core/`** - Shared functionality (containers, HDF5 I/O, logging, parameters, utilities)
  - **`backends/`** - Backend-specific implementations (OpenMP CPU, CUDA GPU)
  - **`platforms/`** - Platform-specific code (Linux/macOS, Windows)
- **`tests/`** - Test suite for validation and regression testing.
- **`cmake/`** - CMake modules for dependency detection, compiler flags, and platform setup.
- **`docs/`** - Documentation and API reference.
- **`plans/`** - Project planning and unification documentation.
- **`CONTRIBUTING.md`** - A comprehensive guide for developers.
- **`.github/`** - GitHub Actions CI/CD workflows.


## Prerequisites

- **C++ Compiler**: C++11 compatible (GCC 6.0+, Clang 5.0+, MSVC 2017+)
- **CMake**: Version 3.18 or higher

### Dependencies

This project uses CMake's `FetchContent` to automatically download and build its core dependencies (HDF5 and FFTW). This provides a consistent, cross-platform build experience.

## Building

1.  **Configure and Build:**
    Simply create a build directory and run CMake.

    ```bash
    # Clone the repository
    git clone https://github.com/waltsims/kspaceFirstOrder-unified.git
    cd kspaceFirstOrder-unified

    # Create build directory
    mkdir build && cd build

    # Configure the project
    cmake .. -DUSE_CUDA=OFF # (or ON if you have CUDA installed)

    # Build
    cmake --build . --config Release --parallel
    ```

### Build Options

| Option             | Default | Description                               |
| ------------------ | ------- | ----------------------------------------- |
| `USE_CUDA`         | `OFF`   | Enable CUDA GPU backend                   |
| `USE_OPENMP`       | `ON`    | Enable OpenMP CPU backend                 |
| `BUILD_TESTS`      | `OFF`   | Build the test suite                      |
| `CMAKE_BUILD_TYPE` | `Release` | Build type (Debug/Release)                |

## Contributing

We welcome contributions! Please see our `CONTRIBUTING.md` file for detailed instructions on our code style, development process, and how to submit pull requests.

## License

This project is licensed under the GNU Lesser General Public License v3.0. See the [LICENSE.md](LICENSE.md) file for details.
