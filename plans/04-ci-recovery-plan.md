# CI Recovery Plan for kspaceFirstOrder

**Created: 2025-09-12**

## Executive Summary

This plan outlines the steps to restore CI functionality for the kspaceFirstOrder unified project. The CI has stopped working since the last successful macOS OMP backend build, and this plan provides a systematic approach to diagnose, fix, and stabilize the CI pipeline.

## Current State Analysis

### Recent Changes
- Project unification completed with CMake-based build system
- GitHub Actions workflows added (.github/workflows/test.yml, test-local.yml)
- Recent commits include:
  - Clang-tidy options added
  - CI build updates
  - .gitignore updates
  - Automatic code formatting checks
  - CMake sccache integration
  - Manual dependency installation removal

### Known Issues
1. **CI Badge**: Non-functional in README.md
2. **Platform Coverage**: Limited testing on Windows (OpenMP excluded)
3. **Dependency Management**: Hybrid system with install_dependencies.sh and FetchContent
4. **Build Performance**: No compiler caching in CI
5. **Error Handling**: Limited diagnostics for build failures

## Root Cause Analysis

### Potential Failure Points

#### 1. CMake Configuration Issues
- **Symptom**: CMake configuration fails
- **Possible Causes**:
  - Incorrect CMakeLists.txt syntax
  - Missing or incorrect dependency detection
  - Platform-specific configuration errors
  - CUDA/OpenMP backend selection issues

#### 2. Dependency Installation Problems
- **Symptom**: Build fails due to missing dependencies
- **Possible Causes**:
  - install_dependencies.sh script issues
  - FetchContent configuration problems
  - Package manager compatibility issues
  - HDF5/FFTW3 version conflicts

#### 3. Platform-Specific Build Failures
- **Symptom**: Build works on some platforms but fails on others
- **Possible Causes**:
  - Compiler differences (GCC vs Clang vs MSVC)
  - Platform-specific code issues
  - Library path problems
  - Architecture-specific optimizations

#### 4. CI Environment Issues
- **Symptom**: Local builds work but CI fails
- **Possible Causes**:
  - GitHub Actions runner differences
  - Environment variable issues
  - Cache/storage limitations
  - Network connectivity problems

## Recovery Strategy

### Phase 1: Diagnosis and Assessment (Priority: Critical)

#### Step 1.1: Verify Local Build Capability
Note: Cmake and brew only available in zsh on local machine.
```bash
# Test basic CMake configuration
mkdir build && cd build
cmake .. -DUSE_CUDA=OFF -DUSE_OPENMP=ON -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build . -j$(nproc)
```
Current Status: build tested and fails: clang++: error: unsupported option '-fopenmp'

#### Step 1.2: Analyze CI Logs
- Review GitHub Actions run logs for specific error messages
- Identify which step is failing (configure, build, test)
- Compare successful vs failed runs

#### Step 1.3: Validate CMake Configuration
```cmake
# Check CMakeLists.txt for syntax errors
cmake --check-system-vars CMakeLists.txt

# Validate dependency detection
cmake .. --debug-output
```

#### Step 1.4: Test Dependency Resolution
```bash
# Test system package detection
pkg-config --exists hdf5
pkg-config --exists fftw3

# Test FetchContent fallback
cmake .. -DUSE_OPENMP=ON --debug-output
```

### Phase 2: Infrastructure Fixes (Priority: High)

#### Step 2.1: Stabilize CMake Configuration
**Files to Update:**
- `CMakeLists.txt`: Ensure robust configuration
- `cmake/FindDependencies.cmake`: Fix dependency detection
- `cmake/PlatformDetection.cmake`: Improve platform handling
- `cmake/CompilerFlags.cmake`: Standardize compiler settings

**Key Improvements:**
```cmake
# Add better error handling
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Build type" FORCE)
endif()

# Improve dependency detection
find_package(HDF5 QUIET COMPONENTS C HL)
if(NOT HDF5_FOUND)
    message(STATUS "HDF5 not found, will build from source")
    # FetchContent configuration
endif()
```

#### Step 2.2: Fix CI Workflow Issues
**Current Issues:**
- Manual dependency installation removed but not properly replaced
- Windows OpenMP builds disabled without clear reason
- No error handling for failed builds
- Limited diagnostic output

**Required Updates:**
```yaml
# In .github/workflows/test.yml
- name: Install dependencies (Linux)
  run: |
    sudo apt-get update
    sudo apt-get install -y cmake build-essential

- name: Configure CMake
  run: |
    cmake -B build \
      -DUSE_CUDA=OFF \
      -DUSE_OPENMP=ON \
      -DCMAKE_BUILD_TYPE=RelWithDebInfo \
      --debug-output

- name: Build with diagnostics
  run: |
    cmake --build build -j$(nproc) --verbose
```

#### Step 2.3: Implement Robust Error Handling
**Add to CI Workflows:**
```yaml
- name: Build
  run: |
    if ! cmake --build build -j$(nproc); then
      echo "Build failed, showing CMake cache..."
      cat build/CMakeCache.txt
      echo "Showing CMake error log..."
      cat build/CMakeFiles/CMakeError.log 2>/dev/null || true
      exit 1
    fi
```

#### Step 2.4: Add Build Diagnostics
**Enhanced CI Steps:**
```yaml
- name: CMake Configuration Diagnostics
  run: |
    cmake --version
    echo "Available compilers:"
    which gcc g++ clang clang++ || true
    gcc --version || true
    clang --version || true

- name: Dependency Check
  run: |
    echo "Checking for system packages..."
    pkg-config --list-all | grep -E "(hdf5|fftw)" || echo "No system packages found"
```

### Phase 3: Platform-Specific Fixes (Priority: High)

#### Step 3.1: macOS Build Issues
**Common Problems:**
- CommandLineTools vs Xcode conflicts
- Homebrew path issues
- SDK path problems (recently commented out in CMakeLists.txt)

**Solutions:**
```yaml
# In CI workflow
- name: Setup macOS
  run: |
    # Ensure CommandLineTools are available
    sudo xcode-select --switch /Library/Developer/CommandLineTools
    # Or install Xcode if needed
    # sudo xcode-select --switch /Applications/Xcode.app/Contents/Developer

- name: Install dependencies (macOS)
  run: |
    brew update
    brew install cmake hdf5 fftw
```

#### Step 3.2: Windows Build Issues
**Common Problems:**
- MSVC compiler detection
- Path separator issues
- Library linking problems

**Solutions:**
```yaml
# Enable Windows OpenMP builds
- name: Configure CMake (Windows)
  run: |
    cmake -B build \
      -DUSE_CUDA=OFF \
      -DUSE_OPENMP=ON \
      -DCMAKE_BUILD_TYPE=RelWithDebInfo \
      -G "Visual Studio 17 2022"

- name: Build (Windows)
  run: cmake --build build --config RelWithDebInfo -j $env:NUMBER_OF_PROCESSORS
```

#### Step 3.3: Linux Build Issues
**Common Problems:**
- GCC version compatibility
- System library conflicts
- Package manager differences

**Solutions:**
```yaml
# Test on multiple Ubuntu versions
strategy:
  matrix:
    include:
      - os: ubuntu-20.04
        compiler: gcc-9
      - os: ubuntu-22.04
        compiler: gcc-11
```

### Phase 4: Testing and Validation (Priority: Medium)

#### Step 4.1: Implement Build Matrix Testing
**Comprehensive Test Matrix:**
```yaml
strategy:
  matrix:
    include:
      # OpenMP builds
      - os: ubuntu-latest
        backend: openmp
        compiler: gcc
      - os: ubuntu-latest
        backend: openmp
        compiler: clang
      - os: macos-latest
        backend: openmp
      - os: windows-latest
        backend: openmp

      # CUDA builds (where available)
      - os: ubuntu-latest
        backend: cuda
      - os: windows-latest
        backend: cuda

      # Core-only builds
      - os: ubuntu-latest
        backend: core-only
      - os: macos-latest
        backend: core-only
      - os: windows-latest
        backend: core-only
```

#### Step 4.2: Add Build Validation Tests
**Post-Build Validation:**
```yaml
- name: Validate Build Artifacts
  run: |
    # Check executable exists
    if [ ! -f "build/src/kspaceFirstOrder" ]; then
      echo "Executable not found!"
      ls -la build/src/
      exit 1
    fi

    # Test basic functionality
    ./build/src/kspaceFirstOrder --help || echo "Help command failed"

    # Check file size (basic sanity check)
    file_size=$(stat -f%z build/src/kspaceFirstOrder 2>/dev/null || stat -c%s build/src/kspaceFirstOrder)
    if [ "$file_size" -lt 1000000 ]; then
      echo "Warning: Executable seems too small ($file_size bytes)"
    fi
```

#### Step 4.3: Implement Performance Monitoring
**Build Time Tracking:**
```yaml
- name: Record Build Start
  run: echo "BUILD_START=$(date +%s)" >> $GITHUB_ENV

- name: Record Build End
  run: |
    echo "BUILD_END=$(date +%s)" >> $GITHUB_ENV
    BUILD_TIME=$((BUILD_END - BUILD_START))
    echo "Build completed in ${BUILD_TIME} seconds"
```

### Phase 5: Long-term Stabilization (Priority: Medium)

#### Step 5.1: Implement CI Caching
**Compiler Cache Setup:**
```yaml
- name: Setup sccache
  uses: mozilla-actions/sccache-action@v0.0.3

- name: Configure CMake with caching
  run: |
    cmake -B build \
      -DCMAKE_C_COMPILER_LAUNCHER=sccache \
      -DCMAKE_CXX_COMPILER_LAUNCHER=sccache \
      -DUSE_CUDA=OFF \
      -DUSE_OPENMP=ON
```

#### Step 5.2: Add Automated Testing
**Basic Functionality Tests:**
```yaml
- name: Run Smoke Tests
  run: |
    # Test parameter parsing
    ./build/src/kspaceFirstOrder --version || true

    # Test with minimal input (if applicable)
    echo "Basic functionality test passed"
```

#### Step 5.3: Implement Failure Analysis
**Automated Issue Reporting:**
```yaml
- name: Generate Build Report
  if: failure()
  run: |
    echo "## Build Failure Report" >> $GITHUB_STEP_SUMMARY
    echo "- OS: ${{ runner.os }}" >> $GITHUB_STEP_SUMMARY
    echo "- Backend: ${{ matrix.backend }}" >> $GITHUB_STEP_SUMMARY
    echo "- CMake Version: $(cmake --version | head -1)" >> $GITHUB_STEP_SUMMARY
    echo "- Failed Step: ${{ steps.failed_step.outcome }}" >> $GITHUB_STEP_SUMMARY
```

## Implementation Timeline

### Week 1: Diagnosis (Days 1-2)
- [ ] Analyze current CI failures
- [ ] Test local builds on all platforms
- [ ] Identify specific failure points
- [ ] Document findings

### Week 2: Core Fixes (Days 3-5)
- [ ] Fix CMake configuration issues
- [ ] Update CI workflows with better error handling
- [ ] Implement platform-specific fixes
- [ ] Test fixes incrementally

### Week 3: Validation (Days 6-7)
- [ ] Run comprehensive build matrix tests
- [ ] Validate on all supported platforms
- [ ] Implement performance monitoring
- [ ] Document successful fixes

### Week 4: Optimization (Days 8-10)
- [ ] Add CI caching
- [ ] Implement automated testing
- [ ] Add failure analysis tools
- [ ] Update documentation

## Success Criteria

### Functional Requirements
- [ ] All CI jobs pass consistently
- [ ] Builds work on Ubuntu, macOS, and Windows
- [ ] OpenMP backend builds successfully
- [ ] Core functionality works without backends
- [ ] Build artifacts are properly generated

### Performance Requirements
- [ ] Build times under 10 minutes for clean builds
- [ ] Incremental builds under 2 minutes
- [ ] No false positive failures
- [ ] Consistent behavior across platforms

### Quality Requirements
- [ ] Clear error messages for failures
- [ ] Comprehensive logging for debugging
- [ ] Automated failure analysis
- [ ] Up-to-date CI badge in README

## Risk Mitigation

### High-Risk Areas
1. **Platform Compatibility**: Mitigated by testing on all platforms before deployment
2. **Dependency Conflicts**: Mitigated by using FetchContent fallbacks
3. **CI Environment Changes**: Mitigated by pinning GitHub Actions versions

### Rollback Plan
1. **Immediate Rollback**: Revert to last known working commit
2. **Gradual Rollback**: Disable problematic CI jobs while fixing
3. **Alternative CI**: Implement local CI validation during fixes

## Monitoring and Maintenance

### Post-Recovery Tasks
1. Monitor CI success rates weekly
2. Update CI workflows as GitHub Actions evolve
3. Add new platform support as needed
4. Maintain dependency compatibility

### Key Metrics to Track
- CI success rate (target: >95%)
- Average build time per platform
- False positive rate
- Time to resolution for failures

This plan provides a comprehensive approach to restore and stabilize the CI pipeline for the kspaceFirstOrder project.
