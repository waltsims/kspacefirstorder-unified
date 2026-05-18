# kspaceFirstOrder Project Unification Plan

## Executive Summary

This plan outlines the unification of multiple kspaceFirstOrder project variants into a single, unified codebase with modern build system and CI/CD infrastructure. The current project consists of 5 separate variants supporting different platforms and backends, which creates maintenance overhead and complicates distribution.

## Current State Analysis

### Project Variants
1. **k-wave-omp-darwin** - OpenMP version for macOS
2. **kspaceFirstOrder-CUDA-linux** - CUDA version for Linux
3. **kspaceFirstOrder-CUDA-windows** - CUDA version for Windows
4. **kspaceFirstOrder-OMP-linux** - OpenMP version for Linux
5. **kspaceFirstOrder-OMP-windows** - OpenMP version for Windows

### Key Differences Identified
- **Build Systems**: Makefile (Linux/macOS) vs Visual Studio (Windows)
- **Processing Backends**: OpenMP vs CUDA
- **Platform Dependencies**: Different library paths and compiler flags
- **CUDA-specific Files**: `.cu`/`.cuh` files and additional matrix classes
- **Platform-specific Code**: Error messages, utilities, and system integrations

## Goals and Objectives

### Primary Goals
1. **Unified Codebase**: Single source tree supporting all platforms and backends
2. **Modern Build System**: CMake-based build system with platform detection
3. **Cross-Platform CI/CD**: GitHub Actions testing on all supported platforms
4. **Release Automation**: Automated build and release process
5. **Maintainability**: Reduced duplication and simplified maintenance

### Success Criteria
- All variants build successfully from unified codebase
- CI passes on all platforms (Linux, macOS, Windows)
- CUDA and OpenMP backends both functional
- No regression in performance or functionality
- Simplified contribution workflow

## Step-by-Step Unification Plan

### Phase 1: Project Structure and Analysis (Week 1-2)

#### Step 1.1: Comprehensive Code Analysis
- [ ] Analyze all source files for platform-specific code
- [ ] Identify CUDA-specific vs OpenMP-specific implementations
- [ ] Document all external dependencies and versions
- [ ] Create inventory of all build artifacts and configurations

#### Step 1.2: Unified Directory Structure Design
- [ ] Design new unified directory structure
- [ ] Plan conditional compilation strategy
- [ ] Identify shared vs variant-specific files

#### Step 1.3: Dependency Analysis
- [ ] Document all required libraries (HDF5, FFTW, CUDA, etc.)
- [ ] Identify version compatibility requirements
- [ ] Plan dependency management strategy

### Phase 2: Core Unification (Week 3-6)

#### Step 2.1: Base Code Unification
- [ ] Create unified source tree structure
- [ ] Merge common code from all variants
- [ ] Implement conditional compilation for platform differences
- [ ] Standardize include paths and file organization

#### Step 2.2: Backend-Specific Code Integration
- [ ] Implement CUDA/OpenMP conditional compilation
- [ ] Merge CUDA-specific files with appropriate guards
- [ ] Ensure OpenMP fallback when CUDA unavailable
- [ ] Test backend switching functionality

#### Step 2.3: Platform-Specific Code Handling
- [ ] Abstract platform-specific utilities
- [ ] Implement unified error message system
- [ ] Handle path separators and system calls
- [ ] Abstract library loading and linking

### Phase 3: Build System Implementation (Week 7-8)

#### Step 3.1: CMake Build System Design
- [ ] Design main CMakeLists.txt structure
- [ ] Implement platform detection logic
- [ ] Create backend (CUDA/OpenMP) selection mechanism
- [ ] Configure compiler flags and optimization settings

#### Step 3.2: Dependency Management
- [ ] Implement find modules for all dependencies
- [ ] Handle optional dependencies (CUDA)
- [ ] Configure library linking strategies
- [ ] Implement version checking and compatibility

#### Step 3.3: Build Configuration
- [ ] Create build presets for different configurations
- [ ] Implement debug/release build modes
- [ ] Configure installation targets
- [ ] Test build system on all platforms

### Phase 4: CI/CD and Testing Infrastructure (Week 9-10)

#### Step 4.1: GitHub Actions CI Setup
- [ ] Create CI workflow for Linux builds
- [ ] Add macOS CI configuration
- [ ] Implement Windows CI with Visual Studio
- [ ] Configure CUDA-enabled runners for GPU testing

#### Step 4.2: Build Testing
- [ ] Test builds on all platform combinations
- [ ] Verify CUDA and OpenMP builds
- [ ] Test cross-compilation scenarios
- [ ] Validate build artifacts and dependencies

#### Step 4.3: Release Automation
- [ ] Create release build workflow
- [ ] Implement automatic versioning
- [ ] Configure artifact upload and distribution
- [ ] Set up release notes generation

### Phase 5: Migration and Validation (Week 11-12)

#### Step 5.1: Migration Strategy
- [ ] Create migration scripts for existing builds
- [ ] Update documentation and build instructions
- [ ] Plan deprecation of old variant directories
- [ ] Implement backward compatibility where needed

#### Step 5.2: Comprehensive Testing
- [ ] Functional testing of all build variants
- [ ] Performance regression testing
- [ ] Cross-platform compatibility testing
- [ ] Integration testing with k-Wave toolbox

#### Step 5.3: Documentation Update
- [ ] Update README and build documentation
- [ ] Create contributor guidelines for unified project
- [ ] Document CI/CD processes and release procedures
- [ ] Update API documentation for any changes

## Proposed Directory Structure

```
kspaceFirstOrder-unified/
├── CMakeLists.txt                    # Main CMake configuration
├── cmake/                           # CMake modules and helpers
│   ├── FindDependencies.cmake
│   ├── CompilerFlags.cmake
│   └── PlatformDetection.cmake
├── src/                             # Unified source code
│   ├── main.cpp
│   ├── CMakeLists.txt
│   ├── core/                        # Core shared functionality
│   │   ├── Containers/
│   │   ├── HDF5/
│   │   ├── Logger/
│   │   ├── Parameters/
│   │   └── Utils/
│   ├── backends/                    # Backend-specific code
│   │   ├── openmp/                  # OpenMP implementation
│   │   │   ├── MatrixClasses/
│   │   │   ├── KSpaceSolver/
│   │   │   └── OutputStreams/
│   │   └── cuda/                    # CUDA implementation
│   │       ├── MatrixClasses/
│   │       ├── KSpaceSolver/
│   │       └── OutputStreams/
│   └── platform/                    # Platform-specific code
│       ├── unix/                    # Linux/macOS specific
│       └── windows/                 # Windows specific
├── tests/                           # Test suite
├── docs/                            # Documentation
├── scripts/                         # Build and utility scripts
└── .github/                         # GitHub Actions workflows
    └── workflows/
        ├── ci.yml
        ├── release.yml
        └── validation.yml
```

## Build System Design

### CMake Configuration Strategy
- **Platform Detection**: Automatic detection of OS and architecture
- **Backend Selection**: CMake options for CUDA vs OpenMP
- **Dependency Finding**: Robust find modules for all libraries
- **Compiler Configuration**: Platform-specific compiler flags and optimizations

### Key CMake Features
```cmake
# Backend selection
option(USE_CUDA "Enable CUDA backend" ON)
option(USE_OPENMP "Enable OpenMP backend" ON)

# Platform detection
if(WIN32)
    # Windows-specific configuration
elseif(UNIX AND NOT APPLE)
    # Linux-specific configuration
elseif(APPLE)
    # macOS-specific configuration
endif()

# Backend-specific source files
if(USE_CUDA)
    list(APPEND SOURCES src/backends/cuda/...)
endif()
```

## CI/CD Architecture

### GitHub Actions Matrix Strategy
```yaml
strategy:
  matrix:
    include:
      - os: ubuntu-latest
        backend: openmp
      - os: ubuntu-latest
        backend: cuda
      - os: macos-latest
        backend: openmp
      - os: windows-latest
        backend: openmp
      - os: windows-latest
        backend: cuda
```

### Build and Test Stages
1. **Dependencies**: Install platform-specific dependencies
2. **Configure**: Run CMake configuration with appropriate options
3. **Build**: Compile with platform-specific optimizations
4. **Test**: Run unit and integration tests
5. **Package**: Create distribution artifacts

## Risk Assessment and Mitigation

### High-Risk Areas
1. **CUDA Compatibility**: Ensure CUDA code works across versions
2. **Platform Differences**: Handle Windows vs Unix path/file differences
3. **Dependency Management**: Ensure consistent library versions
4. **Performance Regression**: Maintain performance across backends

### Mitigation Strategies
- Comprehensive testing on all platforms before merge
- Gradual migration with fallback to old builds
- Extensive performance benchmarking
- Clear documentation of platform-specific requirements

## Timeline and Milestones

### Phase 1: Analysis (Weeks 1-2)
- [ ] Complete code analysis and documentation
- [ ] Finalize unified directory structure
- [ ] Define CMake architecture

### Phase 2: Unification (Weeks 3-6)
- [ ] Merge all source code into unified structure
- [ ] Implement conditional compilation
- [ ] Test basic builds on all platforms

### Phase 3: Build System (Weeks 7-8)
- [ ] Complete CMake implementation
- [ ] Test all build configurations
- [ ] Validate dependency management

### Phase 4: CI/CD (Weeks 9-10)
- [ ] Implement GitHub Actions workflows
- [ ] Test CI on all platforms
- [ ] Set up release automation

### Phase 5: Validation (Weeks 11-12)
- [ ] Complete migration testing
- [ ] Performance validation
- [ ] Documentation updates

## Success Metrics

- **Code Coverage**: Single unified codebase replacing 5 variants
- **Build Success**: All platforms build successfully via CMake
- **CI Reliability**: >95% success rate on all CI jobs
- **Performance**: No regression in performance benchmarks
- **Maintainability**: Reduced maintenance overhead by 80%

## Next Steps

1. Begin Phase 1 analysis immediately
2. Set up development branch for unification work
3. Create initial CMake skeleton
4. Establish CI testing for current variants as baseline

This plan provides a comprehensive roadmap for unifying the kspaceFirstOrder project while maintaining compatibility and performance across all platforms and backends.
