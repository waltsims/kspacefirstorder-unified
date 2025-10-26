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
  using SignedIndex = std::ptrdiff_t;

  constexpr SignedIndex toSigned(size_t value) noexcept
  {
    return static_cast<SignedIndex>(value);
  }
}

#endif // OMP_HELPERS_H
