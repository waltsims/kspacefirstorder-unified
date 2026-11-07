/**
 * @file      BaseOutputStream.cpp
 *
 * @author    Jiri Jaros \n
 *            Faculty of Information Technology \n
 *            Brno University of Technology \n
 *            jarosjir@fit.vutbr.cz
 *
 * @brief     The implementation file of the class saving RealMatrix data into the output HDF5 file.
 *
 * @version   kspaceFirstOrder 3.6
 *
 * @date      11 July      2012, 10:30 (created) \n
 *            11 February  2020, 16:21 (revised)
 *
 * @copyright Copyright (C) 2012 - 2020 SC\@FIT Research Group, Brno University of Technology, Brno, CZ.
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

#include <cmath>
#include <cstring>
#include <limits>

// Windows build needs to undefine macro MINMAX to support std::limits
#ifdef _WIN64
  #ifndef NOMINMAX
    # define NOMINMAX
  #endif

  #include <windows.h>
#endif

#include <OutputStreams/BaseOutputStream.h>

#include <Logger/Logger.h>
#include <Parameters/Parameters.h>

//--------------------------------------------------------------------------------------------------------------------//
//------------------------------------------------- Public methods ---------------------------------------------------//
//--------------------------------------------------------------------------------------------------------------------//

/**
 * Constructor - there is no sensor mask by default!
 */
BaseOutputStream::BaseOutputStream(Hdf5File&            file,
                                   const MatrixName&    rootObjectName,
                                   const RealMatrix&    sourceMatrix,
                                   const ReduceOperator reduceOp)
  : mFile(file),
    mRootObjectName(rootObjectName),
    mSourceMatrix(sourceMatrix),
    mReduceOp(reduceOp),
    mBufferSize(0),
    mHostBuffer(nullptr),
    mDeviceBuffer(0.0f)
{

}// end of BaseOutputStream
//----------------------------------------------------------------------------------------------------------------------

/**
 * Apply post-processing on the buffer (done on the device).
 */
void BaseOutputStream::postProcess()
{
  switch (mReduceOp)
  {
    case ReduceOperator::kNone:
    {
      // Do nothing
      break;
    }

    case ReduceOperator::kRms:
    {
      const float scalingCoeff = 1.0f / (Parameters::getInstance().getNt() -
                                         Parameters::getInstance().getSamplingStartTimeIndex());

      mDeviceBuffer = mx::sqrt(mDeviceBuffer * scalingCoeff);
      break;
    }

    case ReduceOperator::kMax:
    {
      // Do nothing
      break;
    }

    case ReduceOperator::kMin:
    {
      // Do nothing
      break;
    }
  }// switch
}// end of postProcess
//----------------------------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------------------------//
//------------------------------------------------ Protected methods -------------------------------------------------//
//--------------------------------------------------------------------------------------------------------------------//

/**
 * Allocate memory using a proper memory alignment.
 */
void BaseOutputStream::allocateMemory()
{
  // Allocate memory on the host side
  mHostBuffer = static_cast<float*>(alignedMalloc(mBufferSize * sizeof(float)));

  // Memory allocation done on core 0 - GPU is pinned to the first sockets
  // We need different initialization for different reduction ops.
  switch (mReduceOp)
  {
    case ReduceOperator::kNone:
    {
      // Zero the matrix - on the CPU side and lock on core 0 (gpu pinned to 1st socket)
      for (size_t i = 0; i < mBufferSize; i++)
      {
        mHostBuffer[i] = 0.0f;
      }
      break;
    }// kNone

    case ReduceOperator::kRms:
    {
      // Zero the matrix - on the CPU side and lock on core 0 (gpu pinned to 1st socket)
      for (size_t i = 0; i < mBufferSize; i++)
      {
        mHostBuffer[i] = 0.0f;
      }
      break;
    }// kRms

    case ReduceOperator::kMax:
    {
      // Set the values to the highest negative float value - on the core 0
      for (size_t i = 0; i < mBufferSize; i++)
      {
        mHostBuffer[i] = -1 * std::numeric_limits<float>::max();
      }
      break;
    }// kMax

    case ReduceOperator::kMin:
    {
      // Set the values to the highest float value - on the core 0
      for (size_t i = 0; i < mBufferSize; i++)
      {
        mHostBuffer[i] = std::numeric_limits<float>::max();
      }
      break;
    }//kMin
  }// switch

  // Initialize the device buffer (aggregated quantities are reduced on the device)
  copyToDevice();
}// end of allocateMemory
//----------------------------------------------------------------------------------------------------------------------

/**
 * Free memory.
 */
void BaseOutputStream::freeMemory()
{
  // Free host buffer
  if (mHostBuffer)
  {
    alignedFree(mHostBuffer);
  }
  mHostBuffer = nullptr;

  // Release the device buffer
  mDeviceBuffer = mx::array(0.0f);
}// end of FreeMemory
//----------------------------------------------------------------------------------------------------------------------

/**
 *  Copy data host -> device.
 */
void BaseOutputStream::copyToDevice()
{
  mDeviceBuffer = mx::array(mHostBuffer, mx::Shape{static_cast<int>(mBufferSize)}, mx::float32);
  mx::eval(mDeviceBuffer);
}// end of copyToDevice
//----------------------------------------------------------------------------------------------------------------------

/**
 * Copy data device -> host.
 */
void BaseOutputStream::copyFromDevice()
{
  mx::array contiguousBuffer = mx::contiguous(mDeviceBuffer);
  mx::eval(contiguousBuffer);
  std::memcpy(mHostBuffer, contiguousBuffer.data<float>(), mBufferSize * sizeof(float));
}// end of copyFromDevice
//----------------------------------------------------------------------------------------------------------------------

/**
 * Apply the reduction operator on sampled values.
 */
void BaseOutputStream::reduceSampledValues(const mx::array& values)
{
  switch (mReduceOp)
  {
    case ReduceOperator::kNone:
    {
      mDeviceBuffer = values;
      break;
    }
    case ReduceOperator::kRms:
    {
      mDeviceBuffer = mDeviceBuffer + values * values;
      break;
    }
    case ReduceOperator::kMax:
    {
      mDeviceBuffer = mx::maximum(mDeviceBuffer, values);
      break;
    }
    case ReduceOperator::kMin:
    {
      mDeviceBuffer = mx::minimum(mDeviceBuffer, values);
      break;
    }
  }
  // Schedule the evaluation so that the graph of aggregated quantities does not grow over time steps
  // and raw data is ready (copied) when the buffer is flushed in the next time step.
  mx::async_eval(mDeviceBuffer);
}// end of reduceSampledValues
//----------------------------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------------------------//
//------------------------------------------------- Private methods --------------------------------------------------//
//--------------------------------------------------------------------------------------------------------------------//
