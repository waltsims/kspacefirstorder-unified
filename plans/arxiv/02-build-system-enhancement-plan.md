# k-spaceFirstOrder Build System Enhancement Plan

## Metadata
- **Document Type**: Implementation Plan
- **Target Audience**: AI assistants, developers, maintainers
- **Last Updated**: 2025-01-01
- **Version**: 1.0
- **Status**: Ready for Implementation

## Executive Summary

**Objective**: Transform k-spaceFirstOrder's build system from complex dependency management to a simple, pip-like workflow while maintaining C++ build robustness.

**Key Changes**:
- Simple 2-command workflow: `./install_dependencies.sh` → `./build.sh`
- Smart OS detection and package installation
- System package preference with FetchContent fallback
- Cross-platform support (Ubuntu, Fedora, Arch, macOS)

**Expected Outcomes**:
- 5-10x faster builds when system packages available
- Near-zero learning curve for new users
- 95%+ first-time build success rate
- Reduced maintenance overhead

## Current State Analysis

### Build System Issues
- **Dependency Management**: Manual HDF5/FFTW3 installation required
- **Platform Complexity**: Different instructions for each OS
- **Build Performance**: Always builds from source via FetchContent
- **User Experience**: Steep learning curve, complex error resolution

### Technical Debt
- No system package detection in CMake
- Hardcoded FetchContent usage
- No build optimization flags
- Minimal error handling and user feedback

## Proposed Solution Architecture

### Core Components

#### 1. Enhanced FindDependencies.cmake
**Purpose**: Smart dependency resolution with automatic fallbacks
**Input**: USE_OPENMP, USE_CUDA flags
**Output**: Properly configured HDF5, FFTW3, CUDA, OpenMP libraries

**Algorithm**:
```
FOR each dependency IN [HDF5, FFTW3]:
    TRY find_package(dependency)
    IF dependency_FOUND:
        USE system package
        LOG "Using system {dependency}"
    ELSE:
        USE FetchContent
        LOG "Building {dependency} from source"
        SET optimized build flags
```

#### 2. install_dependencies.sh
**Purpose**: Cross-platform dependency installer
**Input**: None (auto-detects OS)
**Output**: Installed system packages or clear error messages

**Supported Platforms**:
- Ubuntu/Debian: apt-get
- Fedora/RHEL: dnf
- Arch Linux: pacman
- macOS: Homebrew
- Windows: Manual instructions

#### 3. build.sh
**Purpose**: Unified build script
**Input**: None (uses sensible defaults)
**Output**: Compiled binaries in build/ directory

**Features**:
- Automatic CPU core detection
- Release build optimization
- Progress feedback
- Error handling

### Workflow States

#### State 1: Dependency Installation
```bash
USER INPUT: ./install_dependencies.sh
SYSTEM DETECTION: uname, package managers
DEPENDENCY CHECK: pkg-config, version checks
PACKAGE INSTALLATION: OS-specific commands
VERIFICATION: Test installations
OUTPUT: Success/failure with clear messages
```

#### State 2: Build Process
```bash
USER INPUT: ./build.sh
CMAKE CONFIG: Optimized flags, dependency detection
COMPILATION: Parallel build with progress
VERIFICATION: Binary existence checks
OUTPUT: Success with executable locations
```

## Implementation Details

### FindDependencies.cmake Enhancement

#### Current Code Structure
```cmake
# Always fetches from source
FetchContent_Declare(hdf5 ...)
FetchContent_MakeAvailable(hdf5)
```

#### Enhanced Code Structure
```cmake
# Smart resolution with fallbacks
macro(find_or_fetch_package pkg_name find_cmd fetch_cmd set_vars_cmd)
    message(STATUS "Looking for ${pkg_name}...")
    ${find_cmd}
    if(${pkg_name}_FOUND)
        message(STATUS "✓ Using system ${pkg_name}")
    else()
        message(STATUS "⚠ System ${pkg_name} not found, fetching from source...")
        ${fetch_cmd}
        ${set_vars_cmd}
        message(STATUS "✓ Built ${pkg_name} from source")
    endif()
endmacro()

# Usage for HDF5
find_or_fetch_package(
    HDF5
    "find_package(HDF5 QUIET COMPONENTS C HL)"
    "FetchContent_Declare(hdf5 ...); FetchContent_MakeAvailable(hdf5)"
    "set(HDF5_LIBRARIES hdf5-static hdf5_hl-static)"
)
```

### OS Detection Logic

#### Detection Algorithm
```bash
# Primary detection
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    if command -v apt-get >/dev/null; then
        OS="ubuntu"
    elif command -v dnf >/dev/null; then
        OS="fedora"
    elif command -v pacman >/dev/null; then
        OS="arch"
    fi
elif [[ "$OSTYPE" == "darwin"* ]]; then
    OS="macos"
fi

# Fallback detection
if [ "$OS" = "unknown" ]; then
    if [ -f /etc/os-release ]; then
        OS=$(grep ^ID= /etc/os-release | cut -d= -f2 | tr -d '"')
    fi
fi
```

#### Package Installation Commands

##### Ubuntu/Debian
```bash
REQUIRED_PACKAGES="build-essential cmake libhdf5-dev libfftw3-dev pkg-config"
sudo apt-get update
sudo apt-get install -y $REQUIRED_PACKAGES
```

##### Fedora/RHEL
```bash
REQUIRED_PACKAGES="gcc-c++ cmake hdf5-devel fftw-devel pkgconfig"
sudo dnf install -y $REQUIRED_PACKAGES
```

##### Arch Linux
```bash
REQUIRED_PACKAGES="gcc cmake hdf5 fftw pkgconf"
sudo pacman -S --needed $REQUIRED_PACKAGES
```

##### macOS
```bash
# Check Homebrew
if ! command -v brew >/dev/null; then
    echo "Please install Homebrew first"
    exit 1
fi

REQUIRED_PACKAGES="cmake hdf5 fftw pkg-config"
brew install $REQUIRED_PACKAGES
```

### Build Optimization

#### CMake Configuration
```cmake
# In CMakeLists.txt
set(CMAKE_BUILD_PARALLEL_LEVEL 8)
set(FETCHCONTENT_QUIET OFF)
set(FETCHCONTENT_UPDATES_DISCONNECTED ON)

# Compiler caching
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
endif()
```

#### Build Script
```bash
#!/bin/bash
set -e

# Optimized build configuration
CMAKE_FLAGS=(
    -DCMAKE_BUILD_TYPE=Release
    -DUSE_OPENMP=ON
    -DBUILD_TESTS=OFF
    -DFETCHCONTENT_QUIET=OFF
)

# Use all available cores
JOBS=$(nproc 2>/dev/null || echo 4)

cmake -S . -B build "${CMAKE_FLAGS[@]}"
cmake --build build -- -j$JOBS
```

## Error Handling and User Feedback

### Error Categories

#### 1. Missing Dependencies
**Detection**: Check before installation
**Message**: "Missing dependencies: HDF5, FFTW3. Run ./install_dependencies.sh"
**Recovery**: Automatic installation script

#### 2. Build Failures
**Detection**: CMake/make exit codes
**Message**: "Build failed. Check build/build.log for details"
**Recovery**: Clear error messages with suggested fixes

#### 3. Version Conflicts
**Detection**: pkg-config version checks
**Message**: "System HDF5 version X.X found, requires Y.Y+"
**Recovery**: Fallback to FetchContent or upgrade instructions

### User Feedback System

#### Status Messages
```bash
# Information
log_info "Configuring build system..."

# Success
log_success "Dependencies installed successfully"

# Warning
log_warning "System package not found, building from source"

# Error
log_error "Build failed: missing compiler"
```

#### Progress Indication
```bash
# Show download progress
GIT_PROGRESS=TRUE

# Show build progress
cmake --build build -- -j$JOBS

# Show completion status
echo "✓ Build completed in build/src/kspaceFirstOrder"
```

## Testing Strategy

### Unit Tests
- **OS Detection**: Test on all supported platforms
- **Package Installation**: Verify correct packages installed
- **Dependency Resolution**: Test system vs FetchContent paths
- **Build Process**: Verify successful compilation

### Integration Tests
- **Clean Environment**: Test on fresh systems
- **Partial Installation**: Test fallback mechanisms
- **Cross-Platform**: Verify identical behavior across OSes

### Performance Benchmarks
- **Build Time**: Measure with/without system packages
- **Disk Usage**: Compare system vs FetchContent usage
- **Network Usage**: Track download sizes

## Implementation Phases

### Phase 1: Core Infrastructure (Priority: High)
**Duration**: 1 week
**Deliverables**:
- Enhanced FindDependencies.cmake with smart resolution
- Basic OS detection in install_dependencies.sh
- Unified build.sh script
- Testing on primary development platform

**Success Criteria**:
- System package detection working
- Fallback to FetchContent functional
- Build script produces working binaries

### Phase 2: Cross-Platform Support (Priority: High)
**Duration**: 1 week
**Deliverables**:
- Ubuntu/Debian support
- Fedora/RHEL support
- Arch Linux support
- macOS support (Homebrew)

**Success Criteria**:
- All major Linux distros supported
- macOS builds working
- Consistent behavior across platforms

### Phase 3: Polish and Documentation (Priority: Medium)
**Duration**: 1 week
**Deliverables**:
- Comprehensive error handling
- Updated README with new workflow
- Troubleshooting guide
- Performance optimizations

**Success Criteria**:
- Clear error messages for all failure modes
- Documentation covers all use cases
- Build time optimizations implemented

### Phase 4: Advanced Features (Priority: Low)
**Duration**: 1 week
**Deliverables**:
- Version checking and compatibility
- Build caching improvements
- CI/CD integration scripts
- Windows support (manual)

**Success Criteria**:
- Version conflicts handled gracefully
- CI builds optimized
- Windows users have clear instructions

## Success Metrics

### Quantitative Metrics
- **Build Time Reduction**: 5-10x faster with system packages
- **Success Rate**: >95% first-time build success
- **Platform Coverage**: Ubuntu, Fedora, Arch, macOS
- **Disk Space**: <500MB additional usage

### Qualitative Metrics
- **User Experience**: Simple 2-command workflow
- **Error Clarity**: Actionable error messages
- **Documentation**: Self-service troubleshooting
- **Maintenance**: Minimal ongoing support

## Risk Assessment

### High Risk Issues
- **Package Version Incompatibility**: System packages might be outdated
  - **Mitigation**: Version checking with fallback to FetchContent
  - **Impact**: Low (fallback ensures builds work)

- **macOS Homebrew Dependency**: Users might not have Homebrew
  - **Mitigation**: Clear installation instructions
  - **Impact**: Medium (affects macOS users only)

### Medium Risk Issues
- **Windows Support Gap**: No native package manager
  - **Mitigation**: Detailed manual instructions
  - **Impact**: Low (Windows not primary platform)

- **Package Manager Variations**: Different distros have different tools
  - **Mitigation**: Comprehensive OS detection
  - **Impact**: Low (covered in Phase 2)

## Dependencies and Prerequisites

### External Dependencies
- **CMake 3.18+**: For FetchContent and modern features
- **C++11 Compiler**: GCC, Clang, or MSVC
- **Package Managers**: apt, dnf, pacman, brew (platform-dependent)

### Internal Dependencies
- **Current CMakeLists.txt**: Must be compatible with new FindDependencies
- **Source Code Structure**: No changes required
- **Build Scripts**: Can coexist with existing scripts

## Migration Strategy

### Backward Compatibility
- **Existing Scripts**: Continue to work unchanged
- **Manual Builds**: Still supported via direct CMake
- **Custom Configurations**: Advanced users can override defaults

### Transition Plan
1. **Phase 1**: Implement alongside existing system
2. **Testing**: Verify new system works correctly
3. **Documentation**: Update with new workflow
4. **Announcement**: Communicate changes to users
5. **Sunset**: Deprecate old documentation after grace period

## Future Enhancements

### Short Term (3-6 months)
- Windows support via vcpkg or Chocolatey
- Docker-based builds for reproducible environments
- Build time analytics and optimization

### Long Term (6-12 months)
- GUI installer for non-technical users
- Cloud-based dependency caching
- Automated dependency updates
- Multi-architecture support (ARM, RISC-V)

---

## AI Processing Notes

**Key Decision Points**: This document uses structured markdown with clear headings, allowing AI systems to easily extract and understand:
- Component relationships and dependencies
- Implementation phases and priorities
- Success criteria and metrics
- Risk assessments and mitigations

**Parsing Structure**: Each section follows consistent patterns for AI extraction:
- Headers use `# ## ###` hierarchy
- Code blocks use triple backticks with language specification
- Lists use consistent bullet/dash patterns
- Tables use markdown table syntax where appropriate

**Actionable Content**: All sections include specific commands, file paths, and implementation details that AI can directly use for code generation and implementation.
