# Strategic Improvement Plan for kspaceFirstOrder

**Status as of: 2025-09-12**

## 1. Executive Summary

This document presents a strategic plan to enhance the `kspaceFirstOrder` project by addressing architectural, procedural, and technical debt. The current project structure, while functional, presents potential challenges to developer onboarding due to the presence of legacy reference codebases alongside the active, unified source tree.

The core proposal is to formally establish the `src` directory as the single source of truth, clarifying this through documentation and leveraging the existing CMake build system for all compilation and dependency management. This initiative, combined with modernizing the C++ standard, standardizing code style, and improving documentation, will establish a robust foundation for the project's future development.

The following plan outlines key areas for improvement and a proposed roadmap for implementation. The expected outcomes include a significant reduction in maintenance overhead, improved developer velocity, and a more stable, performant, and extensible codebase.

## 2. Key Improvement Pillars

### Pillar 1: Codebase Modernization and Clarity

**Objective:** Establish the `src` directory as the definitive source, modernize the C++ implementation to improve performance and maintainability, and clarify the project structure.

**Current State:**
*   **Structural Ambiguity:** The repository contains several untracked directories (`k-wave-omp-darwin`, etc.) that were used as a reference during the initial unification effort. While not part of the build, their presence may cause confusion for new contributors regarding the authoritative source code.
*   **Technical Stagnation:** The codebase adheres to C++11. While a solid standard, it prevents the use of modern C++ features (C++17/20) that can enhance performance, readability, and safety.

**Recommendations:**
1.  **Clarify Codebase Structure:** Update the project documentation (`README.md`) to explicitly state that the `src` directory is the single source of truth for the unified codebase. The documentation should also explain that the other `kspaceFirstOrder-*` directories are untracked reference material from a previous project phase and are not part of the active build.
2.  **C++ Standard Evolution:** Plan a migration to a newer C++ standard (e.g., C++17). This will allow the use of modern language features such as `std::filesystem` for path management, parallel algorithms for performance enhancements, and structured bindings for cleaner code.
3.  **Code Quality Automation:**
    *   **Static Analysis:** Integrate static analysis tools (e.g., Clang-Tidy) into the CI pipeline to proactively identify potential bugs, performance issues, and style violations.
    *   **Code Formatting:** Adopt a consistent code style by introducing a `.clang-format` configuration. Enforce formatting automatically in the CI pipeline to eliminate style debates and improve code consistency.

### Pillar 2: Build System and CI/CD Pipeline Streamlining

**Objective:** Create a seamless, high-performance, cross-platform build experience and a robust CI/CD pipeline.

**Current State:**
*   **Build Performance Concerns:** The current hybrid model relies on `install_dependencies.sh` to pre-install system libraries, correctly identifying that building dependencies from source with `FetchContent` can be time-consuming.
*   **Platform Disparity:** The performance solution (`install_dependencies.sh`) is not available for Windows, creating an inconsistent developer experience.
*   **CI Pipeline Gaps:** The CI badge in the `README.md` is non-functional.

**Recommendations:**
1.  **Implement a Compiler Cache for Build Acceleration:** Integrate a compiler cache (e.g., `sccache`) into the development workflow. This is the most effective and cross-platform strategy for dramatically speeding up builds. Once cached, dependency builds become nearly instantaneous, even in clean build directories. This approach provides the convenience of `FetchContent` without the performance penalty.
2.  **Adopt Ninja as the Recommended CMake Generator:** Introduce Ninja as the preferred build system generator for its superior performance, especially for large-scale C++ projects. Update documentation to guide developers on installing and using Ninja with CMake.
3.  **Update Build Process and Documentation:** The primary recommended development workflow should be based on `FetchContent` augmented by a compiler cache. The `README.md` and a new `CONTRIBUTING.md` must be updated with clear instructions on how to install and configure `sccache`. The `install_dependencies.sh` script can be retained as an optional convenience, particularly for CI environments, but the compiler cache should be presented as the superior solution for local development.
4.  **CI Pipeline Hardening:**
    *   Correct the CI badge in the `README.md`.
    *   Expand CI coverage to include static analysis and code formatting checks mentioned in Pillar 1.
    *   Consider adding automated performance regression testing to the CI pipeline to catch performance degradation before it merges to the main branch.

### Pillar 3: Enhancing Documentation and Contributor Experience

**Objective:** Provide clear, accurate, and comprehensive documentation for both users and developers.

**Current State:**
*   **Outdated Information:** The `README.md` fails to address the project's structure concerning the reference directories, which is a major source of potential confusion for new contributors.
*   **Implicit Knowledge:** Contributor guidelines regarding code style and development practices are minimal ("follow the existing code style").

**Recommendations:**
1.  **Documentation Overhaul:**
    *   Update the `README.md` to reflect the unified codebase structure and the streamlined, CMake-centric build process, as detailed in Pillar 1.
    *   Clearly document the project's architecture, backend implementations, and performance considerations.
2.  **Formalize Contribution Guidelines:** Create a `CONTRIBUTING.md` file that specifies:
    *   The development workflow (branching, PRs, etc.).
    *   Code style guidelines (referencing the `.clang-format` file).
    *   Instructions for building and running tests.
    *   Guidelines for adding new features or backends.

## 3. Proposed Roadmap

This is a high-level roadmap to guide the implementation of these improvements.

*   **Phase 1 (Foundation):**
    *   ~~Update `README.md` to clarify project structure, introduce the compiler cache strategy, and fix the CI badge.~~ (Completed 2025-09-12)
    *   ~~Provide instructions for setting up `sccache`.~~ (Completed 2025-09-12)
    *   ~~Introduce `.clang-format` and format the entire codebase.~~ (Completed 2025-09-12)
    *   Adopt Ninja as the recommended CMake generator and update documentation.

*   **Phase 2 (Automation & Quality):**
    *   ~~Integrate `clang-format` checks into the CI pipeline.~~ (Completed locally via pre-commit hooks 2025-09-12; CI integration pending)
    *   Integrate static analysis (Clang-Tidy) into the CI pipeline.
    *   ~~Create `CONTRIBUTING.md`.~~ (Completed 2025-09-12)

*   **Phase 3 (Modernization):**
    *   Begin migration to C++17.
    *   Refactor core classes to improve modularity, reduce complexity, and shorten file lengths.
    *   Investigate and integrate a performance profiling framework.
    *   Explore adding performance regression testing to CI.

## 4. Expected Outcomes

*   **Reduced Maintenance Costs:** A single codebase will dramatically reduce the time and effort required for updates and bug fixes.
*   **Improved Developer Velocity:** A streamlined build process and clear documentation will enable new and existing contributors to become productive more quickly.
*   **Enhanced Code Quality & Stability:** Automated checks for style and static analysis will lead to a more robust and maintainable codebase.
*   **Future-Ready Architecture:** A modernized, unified codebase will be easier to extend with new features, backends, and performance optimizations.
