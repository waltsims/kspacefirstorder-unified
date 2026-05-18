# Submodule Inventory (Step 0)

Collected from the embedded project directories and their Git metadata.

- kspaceFirstOrder-openmp-linux
  - URL: https://github.com/waltsims/kspaceFirstOrder-OMP-linux
  - Default branch: main
  - Baseline SHA (pin): 0e95d009b8716d3d932196aa58a5fd4a0e796f31
  - License: LGPL-3.0-or-later (per License.md)
  - Build system: Makefile (GCC/Clang, -fopenmp), deps: FFTW3, HDF5, OpenMP
  - Smoke: binary `--help`

- kspaceFirstOrder-openmp-windows
  - URL: https://github.com/waltsims/kspaceFirstOrder-OMP-windows.git
  - Default branch: main
  - Baseline SHA (pin): ba6f3a03b73cd303934691a9baf00c36f1723926
  - License: LGPL-3.0-or-later (per License.md)
  - Build system: MSVC .vcxproj (OpenMP), deps: FFTW3, HDF5, OpenMP
  - Smoke: binary `--help`

- kspaceFirstOrder-openmp-darwin
  - URL: https://github.com/waltsims/k-wave-omp-darwin.git
  - Default branch: main
  - Baseline SHA (pin): 452bd4f10b109e60c03c26575be3d9807252202c
  - License: LGPL-3.0-or-later (per License.md)
  - Build system: Makefile (AppleClang; OpenMP via Homebrew llvm), deps: FFTW3, HDF5, OpenMP
  - Smoke: binary `--help`

- kspaceFirstOrder-cuda-linux
  - URL: https://github.com/waltsims/kspaceFirstOrder-CUDA-linux
  - Default branch: main
  - Baseline SHA (pin): c9431c3a66bc7f7fa378284bf30b6fc606b993cd
  - License: LGPL-3.0-or-later (per License.md)
  - Build system: Makefile (nvcc + GCC), deps: CUDA Toolkit, FFTW3 (or cuFFT), HDF5
  - Smoke: compile-only in CI; no GPU runtime

- kspaceFirstOrder-cuda-windows
  - URL: https://github.com/waltsims/kspaceFirstOrder-CUDA-windows.git
  - Default branch: main
  - Baseline SHA (pin): b9785bd3c8c503696c175c473b1b2794889c5d2b
  - License: LGPL-3.0-or-later (per License.md)
  - Build system: MSVC + nvcc (Makefile/.vcxproj), deps: CUDA Toolkit, FFTW3 (or cuFFT), HDF5
  - Smoke: compile-only in CI; no GPU runtime

Notes
- These SHAs are the current HEADs of the embedded repos in this workspace.
- If you want to pin to a different known-good point, replace the SHA here and in `scripts/submodules/add_submodules.sh`.
- For macOS OpenMP, prefer Homebrew `llvm` and set `OpenMP_C_FLAGS`/`OpenMP_CXX_FLAGS` in wrappers.

Ready-to-run commands (after confirming values)

```
git submodule add --branch main https://github.com/waltsims/kspaceFirstOrder-OMP-linux repos/kspaceFirstOrder-openmp-linux
git -C repos/kspaceFirstOrder-openmp-linux checkout 0e95d009b8716d3d932196aa58a5fd4a0e796f31
git config -f .gitmodules submodule.repos/kspaceFirstOrder-openmp-linux.shallow true

git submodule add --branch main https://github.com/waltsims/kspaceFirstOrder-OMP-windows.git repos/kspaceFirstOrder-openmp-windows
git -C repos/kspaceFirstOrder-openmp-windows checkout ba6f3a03b73cd303934691a9baf00c36f1723926
git config -f .gitmodules submodule.repos/kspaceFirstOrder-openmp-windows.shallow true

git submodule add --branch main https://github.com/waltsims/k-wave-omp-darwin.git repos/kspaceFirstOrder-openmp-darwin
git -C repos/kspaceFirstOrder-openmp-darwin checkout 452bd4f10b109e60c03c26575be3d9807252202c
git config -f .gitmodules submodule.repos/kspaceFirstOrder-openmp-darwin.shallow true

git submodule add --branch main https://github.com/waltsims/kspaceFirstOrder-CUDA-linux repos/kspaceFirstOrder-cuda-linux
git -C repos/kspaceFirstOrder-cuda-linux checkout c9431c3a66bc7f7fa378284bf30b6fc606b993cd
git config -f .gitmodules submodule.repos/kspaceFirstOrder-cuda-linux.shallow true

git submodule add --branch main https://github.com/waltsims/kspaceFirstOrder-CUDA-windows.git repos/kspaceFirstOrder-cuda-windows
git -C repos/kspaceFirstOrder-cuda-windows checkout b9785bd3c8c503696c175c473b1b2794889c5d2b
git config -f .gitmodules submodule.repos/kspaceFirstOrder-cuda-windows.shallow true
```
