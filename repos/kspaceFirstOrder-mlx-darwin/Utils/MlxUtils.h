/**
 * @file      MlxUtils.h
 *
 * @brief     Small helpers shared by the MLX port of kspaceFirstOrder-CUDA: MLX namespace alias, conversion of
 *            k-Wave dimension sizes into MLX shapes and portable aligned host memory allocation.
 *
 * @details   k-Wave stores all matrices in row-major order with x being the fastest running index, i.e.
 *            (nz, ny, nx). The same order is used for the MLX arrays, so a 1D operator stored as
 *            DimensionSizes(nx, 1, 1) becomes an MLX array of shape {1, 1, nx} and broadcasts naturally against
 *            the full {nz, ny, nx} grid. 2D simulations use nz = 1.
 *
 * This file is part of the C++ extension of the [k-Wave Toolbox](http://www.k-wave.org).
 *
 * k-Wave is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * k-Wave is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License along with k-Wave.
 * If not, see [http://www.gnu.org/licenses/](http://www.gnu.org/licenses/).
 */

#ifndef MLX_UTILS_H
#define MLX_UTILS_H

#include <cstdlib>
#include <new>

#ifdef _WIN64
  #include <malloc.h>
#endif

#include <mlx/mlx.h>

#include <Utils/DimensionSizes.h>

/// Short alias for the MLX core namespace.
namespace mx = mlx::core;

/**
 * @brief  Convert k-Wave dimension sizes into an MLX shape (nz, ny, nx).
 * @param  [in] dims - Dimension sizes.
 * @return MLX shape.
 */
inline mx::Shape toMlxShape(const DimensionSizes& dims)
{
  return mx::Shape{static_cast<int>(dims.nz), static_cast<int>(dims.ny), static_cast<int>(dims.nx)};
}// end of toMlxShape
//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief   Should every FFT be evaluated immediately?
 * @details MLX returns the scratch buffers of an FFT to its allocator only when the GPU command buffer completes, so
 *          queuing many 3D FFTs keeps tens of grid-sized buffers alive. Evaluating each FFT keeps the peak memory
 *          close to the CUDA code (~20 grids) at a cost of a few percent of runtime. Setting the environment variable
 *          KWAVE_MLX_PIPELINE=1 disables this (faster, but about 2.5x more device memory).
 * @return  true if each FFT is evaluated immediately (default).
 */
inline bool isLowMemoryMode()
{
  static const bool lowMemoryMode = (std::getenv("KWAVE_MLX_PIPELINE") == nullptr);
  return lowMemoryMode;
}// end of isLowMemoryMode
//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief  Allocate aligned host memory (portable replacement of _mm_malloc).
 * @param  [in] sizeInBytes - Size of the block.
 * @return Pointer to the memory block.
 * @throw  std::bad_alloc - If there's not enough memory.
 */
inline void* alignedMalloc(size_t sizeInBytes)
{
  void* ptr = nullptr;
  #ifdef _WIN64
    ptr = _aligned_malloc(sizeInBytes, kDataAlignment);
  #else
    if (posix_memalign(&ptr, kDataAlignment, (sizeInBytes > 0) ? sizeInBytes : kDataAlignment) != 0)
    {
      ptr = nullptr;
    }
  #endif
  if (!ptr)
  {
    throw std::bad_alloc();
  }
  return ptr;
}// end of alignedMalloc
//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Free aligned host memory.
 * @param [in] ptr - Pointer to the memory block.
 */
inline void alignedFree(void* ptr)
{
  #ifdef _WIN64
    _aligned_free(ptr);
  #else
    free(ptr);
  #endif
}// end of alignedFree
//----------------------------------------------------------------------------------------------------------------------

#endif /* MLX_UTILS_H */
