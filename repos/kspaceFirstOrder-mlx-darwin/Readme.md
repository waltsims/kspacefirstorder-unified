# kspaceFirstOrder-MLX

A port of the k-Wave GPU solver **kspaceFirstOrder-CUDA v1.3** (Jiri Jaros, Bradley Treeby et al.) from CUDA to
[MLX](https://github.com/ml-explore/mlx). MLX provides GPU FFTs and fused element-wise kernels on Apple Silicon
(Metal) and on NVIDIA GPUs (CUDA backend), with a CPU fallback, so the same code base can run k-Wave simulations
on a Mac GPU and, potentially, keep running on NVIDIA hardware.

The solver is a drop-in replacement for the CUDA binary: it reads the same HDF5 input files produced by MATLAB k-Wave
(`'SaveToDisk'`), accepts the same command-line options and writes the same output files.

It is a port of the same sources as [`../kspaceFirstOrder-cuda-linux/`](../kspaceFirstOrder-cuda-linux/) (only the
GPU-specific parts were rewritten); the CUDA README is kept as [`Readme-CUDA-original.md`](Readme-CUDA-original.md).
The CPU version for macOS is in [`../kspaceFirstOrder-openmp-darwin/`](../kspaceFirstOrder-openmp-darwin/).

Prebuilt binaries (`kspaceFirstOrder-MLX-darwin` + `mlx.metallib`, to be kept in the same folder) are attached to the
releases of this repository; k-wave-python downloads them automatically on Apple Silicon Macs.

## Requirements (macOS)

* A Mac with Apple Silicon (M1 or newer) and the Xcode Command Line Tools (`xcode-select --install`), which provide
  the C++ compiler.
* [Homebrew](https://brew.sh) with the following packages:

| Homebrew package | Used for                                                                  | Needed at        |
|------------------|---------------------------------------------------------------------------|------------------|
| `mlx`            | GPU arrays, FFTs and kernels (Metal backend)                              | build and run    |
| `hdf5`           | Reading the k-Wave input files and writing the output files (C + HL APIs) | build and run    |
| `libomp`         | OpenMP for host-side preprocessing (optional, see `KWAVE_USE_OPENMP`)     | build and run    |
| `cmake`          | Build system                                                              | build only       |

```bash
brew install mlx hdf5 libomp cmake
```

The binary links to these libraries in the Homebrew prefix (`/opt/homebrew`), so the packages must stay installed
to run `kspaceFirstOrder-MLX`, also when the binary is copied elsewhere (e.g. into the k-Wave `binaries` folder).
After `brew upgrade` of `mlx` or `hdf5` to a new major version, rebuild the binary. To get a binary that does not
need Homebrew at run time, see [Self-contained build](#self-contained-build-for-redistribution).

## Building (macOS, Apple Silicon)

From this folder (`repos/kspaceFirstOrder-mlx-darwin`):

```bash
cmake -S . -B build
cmake --build build -j
./build/kspaceFirstOrder-MLX --version
```

Options:

| CMake option         | Default   | Meaning                                                                 |
|----------------------|-----------|-------------------------------------------------------------------------|
| `KWAVE_FETCH_MLX`    | `OFF`     | Download and build MLX from source instead of using an installed one   |
| `KWAVE_MLX_VERSION`  | `v0.32.4` | MLX tag used with `KWAVE_FETCH_MLX=ON`                                  |
| `KWAVE_USE_OPENMP`   | `ON`      | Use OpenMP for host-side preprocessing (optional)                       |
| `KWAVE_BUNDLE_DEPS`  | `OFF`     | Self-contained binary: static MLX, HDF5 and libomp (see below)         |
| `KWAVE_MLX_METAL_JIT`| = `KWAVE_BUNDLE_DEPS` | Build MLX with run-time compiled Metal kernels (small `mlx.metallib`) |

### Self-contained build (for redistribution)

```bash
brew install cmake hdf5 libomp
cmake -S . -B build-release -DKWAVE_BUNDLE_DEPS=ON -DCMAKE_OSX_DEPLOYMENT_TARGET=14.0
cmake --build build-release -j
otool -L build-release/kspaceFirstOrder-MLX   # only /usr/lib and /System/Library entries
```

MLX (`KWAVE_MLX_VERSION`) is downloaded and built from source as a static library, and HDF5 (+ szip/libaec) and
libomp are linked statically from Homebrew, so the binary only depends on macOS system libraries. MLX loads its
Metal kernel library from the directory of the executable: **ship `build-release/mlx.metallib` together with
`kspaceFirstOrder-MLX`** (same folder). With `KWAVE_MLX_METAL_JIT` (on by default here) most Metal kernels are
compiled at run time, which shrinks `mlx.metallib` from ~150 MB to ~3.5 MB and costs well under a second at
start-up (benchmarks on a 256³ grid are unchanged). This is how the binary distributed through
[kspacefirstorder-unified](https://github.com/waltsims/kspacefirstorder-unified) releases (used by k-wave-python) is
built.

### Linux / NVIDIA (CUDA backend of MLX) - untested

```bash
cmake -S . -B build -DKWAVE_FETCH_MLX=ON -DMLX_BUILD_CUDA=ON
cmake --build build -j
```

## Usage

Exactly as `kspaceFirstOrder-CUDA`:

```bash
./build/kspaceFirstOrder-MLX -i input.h5 -o output.h5 --p_raw --p_max
```

From MATLAB k-Wave, use the GPU wrappers and point them to this binary:

```matlab
sensor_data = kspaceFirstOrder3DG(kgrid, medium, source, sensor, ...
    'BinaryPath', '/path/to/kspaceFirstOrder-mlx-darwin/build', 'BinaryName', 'kspaceFirstOrder-MLX');
```

`kspaceFirstOrder2DG` works the same way. Alternatively, copy `build/kspaceFirstOrder-MLX` into the `binaries`
folder of the k-Wave toolbox and only pass `'BinaryName', 'kspaceFirstOrder-MLX'`. Set the environment variable `KWAVE_MLX_DEVICE=cpu` to force the MLX CPU
backend (useful for testing on machines without a supported GPU).

## How the port works

| CUDA version                                         | MLX version                                                        |
|------------------------------------------------------|--------------------------------------------------------------------|
| Host + device buffers (`cudaMalloc`, pinned memory)  | Host buffer (lazy, released after upload) + `mx::array` (nz, ny, nx) |
| `CufftComplexMatrix` - 8 cuFFT plans                 | `FftComplexMatrix` - `mx::fft::rfftn/irfftn/rfft/irfft`, no plans   |
| Transpose kernels for 1D FFTs over y and z           | Not needed (MLX transforms any axis)                               |
| 30 solver CUDA kernels (`SolverCudaKernels.cu`)      | `SolverMlxKernels.cpp` - array expressions fused with `mx::compile` |
| 4 sampling kernels (`OutputStreamsCudaKernels.cu`)   | `mx::take` + `maximum/minimum/sum of squares` on the device         |
| Source injection kernels                             | `mx::scatter` / `mx::scatter_add`                                   |
| `__constant__` memory (constants + matrix pointers)  | `MlxDeviceConstants` (host scalars + pointer to the container)      |
| CUDA streams/events to overlap sampling and disk I/O | `mx::async_eval` per time step; the CPU runs at most one step ahead |
| `CudaParameters` (device selection)                  | `MlxParameters` (Metal / CUDA / CPU device selection)               |

Notes:

* All FFTs are unnormalized like cuFFT (`FFTNorm::Backward` forward, `FFTNorm::Forward` inverse), so the
  normalization constants (`fftDivider`) are applied in the kernels exactly as in the CUDA code.
* k-Wave stores matrices in (z, y, x) order with x fastest; the MLX arrays use the same shape, so the 1D operators
  (`ddx_k_shift_pos`, PML profiles, ...) broadcast without any reshaping.
* macOS-specific fixes: `__APPLE__` treated like `__linux__` (getopt, gethostname, getrusage), `ru_maxrss` in bytes,
  processor name via `sysctl`, `_mm_malloc` replaced by `posix_memalign`.
* Index matrices are uploaded as `uint32`, so grids are limited to 2^32 elements (e.g. 1600^3).

## Validation

Six input files generated with MATLAB k-Wave (`'SaveToDisk'`) were run with the original CPU code
(`kspaceFirstOrder-OMP` v1.3 built on macOS, FFTW) and with this port, with all sampling
flags enabled (`--p_raw --p_rms --p_max --p_min --p_max_all --p_min_all --p_final --u_raw --u_non_staggered_raw
--u_rms --u_max --u_min --u_max_all --u_min_all --u_final --copy_sensor_mask`). All output datasets agree within
single-precision round-off (max relative error ~1e-6 for time series, < 3e-4 relative to the field maximum for
nearly-vanished final fields; index datasets are identical):

| Case | Features covered | Worst max rel. error |
|------|------------------|----------------------|
| 3D homogeneous, p0, index sensor | lossless, initial pressure, u_non_staggered | 2.6e-4 (p_final residual) |
| 3D heterogeneous, 60x54x48 | nonlinear (B/A), power-law absorption, additive pressure source (many) | 2.3e-6 |
| 2D heterogeneous, cuboid sensor | Dirichlet velocity sources (ux, uy), corner sensor mask | 1.0e-5 |
| 3D Stokes absorption, cuboid sensor | Stokes, additive-no-correction pressure source | 2.7e-6 |
| 3D power-law, additive velocity source | additive (k-space corrected) velocity source (many) | 3.5e-5 |
| 3D transducer | `kWaveTransducer` source (delay mask) | 1.8e-4 (uz, 3e-6 rel. to ux) |

## Performance (Apple M3 Max, 128 GB)

Time per step with `--p_raw` on a plane sensor, compared with `kspaceFirstOrder-OMP` (FFTW, 12 performance
cores). Grids are the computational sizes (input 256^3 / 512^3 with the PML outside).

| Grid  | Medium                               | OMP (CPU) | MLX (GPU) | Speed-up | MLX peak device memory |
|-------|--------------------------------------|-----------|-----------|----------|------------------------|
| 288^3 | lossless, homogeneous                | 127 ms    | 54 ms     | 2.4x     | 2.1 GB                 |
| 288^3 | heterogeneous, nonlinear, power law  | 206 ms    | 91 ms     | 2.3x     | 3.3 GB                 |
| 576^3 | lossless, homogeneous                | 999 ms    | 402 ms    | 2.5x     | 16.4 GB                |
| 576^3 | heterogeneous, nonlinear, power law  | 1367 ms   | 628 ms    | 2.2x     | 26.6 GB                |

The FFTs dominate the run time (each lossless 3D step needs 10 real-to-complex/complex-to-real 3D FFTs). MLX
computes an N-D FFT as a sequence of 1D passes with intermediate copies, so there is room for improvement on the
MLX side.

### Environment variables

| Variable                    | Effect                                                                                   |
|-----------------------------|------------------------------------------------------------------------------------------|
| `KWAVE_MLX_DEVICE=cpu`      | Run on the MLX CPU backend instead of the GPU                                             |
| `KWAVE_MLX_PIPELINE=1`      | Do not evaluate every FFT immediately: up to ~15% faster on mid-size grids, but ~2-2.5x more device memory (MLX releases FFT scratch buffers only when the GPU command buffer completes) |
| `KWAVE_MLX_DEBUG_MEMORY=1`  | Print MLX active/peak/cache memory after the upload and the first time steps              |
| `MLX_DISABLE_COMPILE=1`     | (MLX) Disable kernel fusion, useful for debugging                                         |

## Validation of other paths

* **Checkpoint / restart**: cases 2 and 4 split into two legs (`--checkpoint_timesteps 70`) give the same output
  as a single run (including the rms/max/min aggregates restored from the checkpoint).
* **MATLAB end-to-end**: `kspaceFirstOrder3DG`/`kspaceFirstOrder2DG` calling this binary versus the native MATLAB
  `kspaceFirstOrder3D`/`kspaceFirstOrder2D` (heterogeneous nonlinear absorbing 3D medium with a time-varying source,
  and a 2D initial value problem): max relative error 2.6e-6 (3D) and 8.7e-6 (2D).
* **MLX CPU backend** (`KWAVE_MLX_DEVICE=cpu`): 9e-7 versus the OMP code (slower than the OMP code).

Not yet validated: non-uniform grids (no MATLAB front end generates them routinely) and the MLX CUDA backend on
NVIDIA hardware.

## Known limitations / to do

* Index matrices (sensor and source masks) are stored as `uint32` on the device: grids up to 2^32 points.
* Peak host memory (RSS) is reported as in the original code; on Apple Silicon it includes GPU (unified) memory.
* `Current device memory in use` is printed before the matrices are uploaded and therefore reads 0 MB; the
  `Peak device memory in use` at the end is the meaningful number.

## Reproducing the validation

The scripts in [`validation/`](validation/) were used for the results above. Run the MATLAB scripts from the
`validation` folder with the k-Wave MATLAB toolbox on the path, or with the `KWAVE_MATLAB` environment variable set to
its `k-Wave` folder (`e2e_matlab.m` uses the binary in `../build`):

* `make_inputs.m` / `make_bench.m` - generate the test and benchmark input files with MATLAB k-Wave.
* `compare.py <omp_binary> <mlx_binary> <inputs_dir> <outputs_dir>` - run both solvers with all sampling flags and
  compare every output dataset (needs `h5py`, `numpy`).
* `e2e_matlab.m` - compare `kspaceFirstOrder3DG/2DG` + this binary with the native MATLAB solvers.

The reference `kspaceFirstOrder-OMP` was a macOS (CMake + Homebrew FFTW/HDF5/libomp) build of the k-Wave 1.3 OMP
sources.
