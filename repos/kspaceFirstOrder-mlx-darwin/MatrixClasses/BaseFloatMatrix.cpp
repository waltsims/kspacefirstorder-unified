/**
 * @file      BaseFloatMatrix.cpp
 *
 * @author    Jiri Jaros \n
 *            Faculty of Information Technology \n
 *            Brno University of Technology \n
 *            jarosjir@fit.vutbr.cz
 *
 * @brief     The implementation file containing the base class for single precisions floating point numbers (floats).
 *
 * @version   kspaceFirstOrder 3.6 (MLX port)
 *
 * @date      11 July      2011, 12:13 (created) \n
 *            11 February  2020, 16:17 (revised) \n
 *            05 November  2026          (MLX port)
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

#include <cstring>
#include <stdexcept>

#include <MatrixClasses/BaseFloatMatrix.h>
#include <Utils/DimensionSizes.h>
#include <Logger/Logger.h>

//--------------------------------------------------------------------------------------------------------------------//
//---------------------------------------------------- Constants -----------------------------------------------------//
//--------------------------------------------------------------------------------------------------------------------//

//--------------------------------------------------------------------------------------------------------------------//
//------------------------------------------------- Public methods ---------------------------------------------------//
//--------------------------------------------------------------------------------------------------------------------//

/**
 * Default constructor.
 */
BaseFloatMatrix::BaseFloatMatrix()
  : BaseMatrix(),
    mDimensionSizes(),
    mSize(0),
    mCapacity(0),
    mHostData(nullptr),
    mArray(0.0f),
    mOnDevice(false),
    mModified(false)
{

}// end of BaseFloatMatrix
//----------------------------------------------------------------------------------------------------------------------

/**
 * Destructor.
 */
BaseFloatMatrix::~BaseFloatMatrix()
{
  freeHostMemory();
}// end of ~BaseFloatMatrix
//----------------------------------------------------------------------------------------------------------------------

/**
 * Get host data, allocate if necessary.
 */
float* BaseFloatMatrix::getHostData()
{
  if (!mHostData)
  {
    allocateMemory();
  }
  return mHostData;
}// end of getHostData
//----------------------------------------------------------------------------------------------------------------------

/**
 * Get host data, allocate if necessary, const version.
 */
const float* BaseFloatMatrix::getHostData() const
{
  return const_cast<BaseFloatMatrix*>(this)->getHostData();
}// end of getHostData
//----------------------------------------------------------------------------------------------------------------------

/**
 * Replace the device array.
 */
void BaseFloatMatrix::setArray(const mx::array& array)
{
  if (array.shape() != getDeviceShape())
  {
    throw std::runtime_error("Internal error: MLX array shape does not match the matrix dimensions.");
  }
  mArray    = array;
  mOnDevice = true;
  mModified = true;
}// end of setArray
//----------------------------------------------------------------------------------------------------------------------

/**
 * Zero all allocated elements on the host.
 */
void BaseFloatMatrix::zeroMatrix()
{
  float* data = getHostData();

  #pragma omp parallel for schedule (static)
  for (size_t i = 0; i < mCapacity; i++)
  {
    data[i] = 0.0f;
  }
}// end of zeroMatrix
//----------------------------------------------------------------------------------------------------------------------

/**
 * Zero all allocated elements on device.
 */
void BaseFloatMatrix::zeroDeviceMatrix()
{
  mArray    = mx::zeros(getDeviceShape(), getDeviceDataType());
  mOnDevice = true;
  mModified = true;
}// end of zeroDeviceMatrix
//----------------------------------------------------------------------------------------------------------------------

/**
 * Calculate matrix = scalar / matrix.
 */
void BaseFloatMatrix::scalarDividedBy(const float scalar)
{
  float* data = getHostData();

  #pragma omp parallel for schedule(static)
  for (size_t i = 0; i < mCapacity; i++)
  {
    data[i] = scalar / data[i];
  }
}// end of scalarDividedBy
//----------------------------------------------------------------------------------------------------------------------

/**
 * Copy data from host -> device (CPU -> MLX array).
 * If the host data has never been touched, the matrix is zero (as in the CUDA version where host memory is zeroed
 * after allocation). The host buffer is released after the upload to save memory.
 */
void BaseFloatMatrix::copyToDevice()
{
  if (mHostData)
  {
    if (getDeviceDataType() == mx::complex64)
    {
      mArray = mx::array(reinterpret_cast<const mx::complex64_t*>(mHostData), getDeviceShape(), mx::complex64);
    }
    else
    {
      mArray = mx::array(mHostData, getDeviceShape(), mx::float32);
    }
    // Materialize the array so that the host buffer can be released.
    mx::eval(mArray);
    freeHostMemory();
  }
  else
  {
    mArray = mx::zeros(getDeviceShape(), getDeviceDataType());
  }

  mOnDevice = true;
}// end of copyToDevice
//----------------------------------------------------------------------------------------------------------------------

/**
 * Copy data from device -> host (MLX array -> CPU).
 */
void BaseFloatMatrix::copyFromDevice()
{
  if (!mOnDevice)
  {
    // Nothing on the device, the host data is the valid copy.
    return;
  }

  mx::array contiguousArray = mx::contiguous(mArray);
  mx::eval(contiguousArray);

  float* data = getHostData();
  std::memcpy(data, contiguousArray.data<void>(), mCapacity * sizeof(float));
}// end of copyFromDevice
//----------------------------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------------------------//
//------------------------------------------------ Protected methods -------------------------------------------------//
//--------------------------------------------------------------------------------------------------------------------//

/**
 * Host memory allocation based on the capacity. The memory is zeroed.
 */
void BaseFloatMatrix::allocateMemory()
{
  if (mHostData)
  {
    return;
  }

  mHostData = static_cast<float*>(alignedMalloc(mCapacity * sizeof(float)));

  #pragma omp parallel for schedule (static)
  for (size_t i = 0; i < mCapacity; i++)
  {
    mHostData[i] = 0.0f;
  }
}// end of allocateMemory
//----------------------------------------------------------------------------------------------------------------------

/**
 * Free memory.
 */
void BaseFloatMatrix::freeMemory()
{
  freeHostMemory();
  mArray    = mx::array(0.0f);
  mOnDevice = false;
  mModified = false;
}// end of freeMemory
//----------------------------------------------------------------------------------------------------------------------

/**
 * Free host memory.
 */
void BaseFloatMatrix::freeHostMemory()
{
  if (mHostData)
  {
    alignedFree(mHostData);
  }
  mHostData = nullptr;
}// end of freeHostMemory
//----------------------------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------------------------//
//------------------------------------------------- Private methods --------------------------------------------------//
//--------------------------------------------------------------------------------------------------------------------//
