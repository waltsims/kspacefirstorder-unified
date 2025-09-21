# Dependencies Installation Guide

This document describes how to install the required dependencies for building kspaceFirstOrder on different platforms.

## Required Dependencies

- CMake 3.18 or newer
- C++17 compliant compiler
- HDF5 (with C++ support)
- FFTW3 (single precision)
- OpenMP (optional, but recommended)
- CUDA Toolkit (optional, for CUDA backend)

## macOS

Using Homebrew:
```bash
# Install Xcode Command Line Tools if not already installed
xcode-select --install

# Install Homebrew if not already installed
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install dependencies
brew install cmake
brew install hdf5
brew install fftw
brew install libomp

```

## Ubuntu/Debian

```bash
# Install build tools and dependencies
sudo apt update
sudo apt install -y \
    build-essential \
    cmake \
    libhdf5-dev \
    libfftw3-dev \
    libfftw3-single3 \
    libomp-dev

# For CUDA support (optional)
# First add NVIDIA repository and install CUDA
wget https://developer.download.nvidia.com/compute/cuda/repos/ubuntu2204/x86_64/cuda-keyring_1.1-1_all.deb
sudo dpkg -i cuda-keyring_1.1-1_all.deb
sudo apt update
sudo apt install -y cuda-toolkit
```

## CentOS/RHEL/Rocky Linux

```bash
# Install EPEL repository
sudo dnf install -y epel-release

# Install development tools
sudo dnf groupinstall -y "Development Tools"

# Install dependencies
sudo dnf install -y \
    cmake \
    hdf5-devel \
    fftw3-devel \
    libomp-devel

# For CUDA support (optional)
# First add NVIDIA repository
sudo dnf config-manager --add-repo https://developer.download.nvidia.com/compute/cuda/repos/rhel8/x86_64/cuda-rhel8.repo
sudo dnf install -y cuda-toolkit
```

## Windows

Using vcpkg (recommended):
```powershell
# Install vcpkg if not already installed
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg integrate install

# Install dependencies
.\vcpkg install hdf5:x64-windows
.\vcpkg install fftw3:x64-windows
```

Alternatively, you can download pre-built binaries:
1. HDF5: https://www.hdfgroup.org/downloads/hdf5/
2. FFTW: http://www.fftw.org/install/windows.html
3. CUDA Toolkit (optional): https://developer.nvidia.com/cuda-downloads

## Verifying Installation

You can verify that the dependencies are properly installed by running:

```bash
# Check CMake version
cmake --version

# Check HDF5
h5cc -showconfig

# Check FFTW3
pkg-config --modversion fftw3

# Check OpenMP (in C++ code)
#include <omp.h>
int main() {
    #pragma omp parallel
    {
        // This will run in parallel
    }
    return 0;
}
```

## Troubleshooting

### Common Issues

1. **HDF5 not found**: Make sure you have both the runtime and development packages installed. On some systems, you might need to install `hdf5-cpp` separately.

2. **FFTW3 not found**: Ensure you have the single-precision version installed. Some systems package it separately as `libfftw3-single3` or similar.

3. **OpenMP not working on macOS**: Apple's Clang doesn't include OpenMP by default. Install `libomp` from Homebrew and make sure your CMake configuration includes the correct paths.

4. **CUDA not found**: Make sure the CUDA Toolkit is installed and `nvcc` is in your PATH. You might need to set `CUDA_PATH` environment variable on Windows.

### Environment Variables

You might need to set these environment variables if the libraries are installed in non-standard locations:

```bash
# HDF5
export HDF5_ROOT=/path/to/hdf5
export HDF5_DIR=/path/to/hdf5

# FFTW3
export FFTW_ROOT=/path/to/fftw3
export FFTW_DIR=/path/to/fftw3

# CUDA
export CUDA_PATH=/path/to/cuda
export CUDA_HOME=/path/to/cuda
```
