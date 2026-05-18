/**
 * @file      OmpHelpers.h
 *
 * @brief     Common helpers for OpenMP loops that require signed indices.
 */

#ifndef OMP_HELPERS_H
#define OMP_HELPERS_H

#include <cstddef>

namespace omp_helpers
{
  /**
   * Alias for the signed integer type mandated by the OpenMP spec for loop counters.
   * Keeping the definition in one place prevents subtle LLP64/LP64 mismatches.
   */
  using SignedIndex = std::ptrdiff_t;

  /**
   * Explicit conversion helper used when loop bounds originate from STL containers
   * (which report sizes as size_t). Centralizing the cast keeps the OpenMP code
   * uncluttered and documents that the signed conversion is intentional.
   */
  constexpr SignedIndex toSigned(size_t value) noexcept
  {
    return static_cast<SignedIndex>(value);
  }
}

#endif // OMP_HELPERS_H
