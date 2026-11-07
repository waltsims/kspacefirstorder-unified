/**
 * @file      BaseIndexMatrix.cpp
 *
 * @author    Jiri Jaros \n
 *            Faculty of Information Technology \n
 *            Brno University of Technology \n
 *            jarosjir@fit.vutbr.cz
 *
 * @brief     The implementation file containing the base class for index matrices (based on the size_t datatype).
 *
 * @version   kspaceFirstOrder 3.6 (MLX port)
 *
 * @date      26 July      2011, 14:17 (created) \n
 *            11 February  2020, 16:17 (revised)
 *
 * @copyright Copyright (C) 2011 - 2020 SC\@FIT Research Group, Brno University of Technology, Brno, CZ.
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

#include <cstdint>
#include <limits>
#include <stdexcept>
#include <vector>

#include <MatrixClasses/BaseIndexMatrix.h>
#include <Utils/DimensionSizes.h>
#include <Logger/Logger.h>

//--------------------------------------------------------------------------------------------------------------------//
//------------------------------------------------- Public methods ---------------------------------------------------//
//--------------------------------------------------------------------------------------------------------------------//

/**
 * Default constructor.
 */
BaseIndexMatrix::BaseIndexMatrix()
  : BaseMatrix(),
    mDimensionSizes(),
    mSize(0),
    mCapacity(0),
    mHostData(nullptr),
    mArray(0u)
{

}// end of BaseIndexMatrix
//----------------------------------------------------------------------------------------------------------------------

/**
 * Zero all allocated elements.
 */
void BaseIndexMatrix::zeroMatrix()
{
  #pragma omp parallel for schedule(static)
  for (size_t i = 0; i < mCapacity; i++)
  {
    mHostData[i] = size_t(0);
  }
}// end of zeroMatrix
//----------------------------------------------------------------------------------------------------------------------

/**
 * Copy data from host -> device (CPU -> MLX array). Indices are stored as uint32 on the device.
 */
void BaseIndexMatrix::copyToDevice()
{
  std::vector<uint32_t> indices(mCapacity);
  for (size_t i = 0; i < mCapacity; i++)
  {
    if (mHostData[i] > std::numeric_limits<uint32_t>::max())
    {
      throw std::runtime_error("Index matrix value exceeds the 32-bit range supported by the MLX version.");
    }
    indices[i] = static_cast<uint32_t>(mHostData[i]);
  }

  mArray = mx::array(indices.begin(), toMlxShape(mDimensionSizes), mx::uint32);
  mx::eval(mArray);
}// end of copyToDevice
//----------------------------------------------------------------------------------------------------------------------

/**
 * Copy data from device -> host. Index matrices are never modified on the device, the host copy is always valid.
 */
void BaseIndexMatrix::copyFromDevice()
{

}// end of copyFromDevice
//----------------------------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------------------------//
//------------------------------------------------ Protected methods -------------------------------------------------//
//--------------------------------------------------------------------------------------------------------------------//

/**
 * Memory allocation based on the capacity.
 */
void BaseIndexMatrix::allocateMemory()
{
  mHostData = static_cast<size_t*>(alignedMalloc(mCapacity * sizeof(size_t)));
  zeroMatrix();
}// end of allocateMemory
//----------------------------------------------------------------------------------------------------------------------

/**
 * Free memory.
 */
void BaseIndexMatrix::freeMemory()
{
  if (mHostData)
  {
    alignedFree(mHostData);
  }
  mHostData = nullptr;
  mArray    = mx::array(0u);
}// end of freeMemory
//----------------------------------------------------------------------------------------------------------------------
