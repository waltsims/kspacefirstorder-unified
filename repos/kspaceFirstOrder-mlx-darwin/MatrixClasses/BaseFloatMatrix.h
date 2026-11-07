/**
 * @file      BaseFloatMatrix.h
 *
 * @author    Jiri Jaros \n
 *            Faculty of Information Technology \n
 *            Brno University of Technology \n
 *            jarosjir@fit.vutbr.cz
 *
 * @brief     The header file containing the base class for single precisions floating point numbers (floats).
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

#ifndef BASE_FLOAT_MATRIX_H
#define BASE_FLOAT_MATRIX_H

#include <MatrixClasses/BaseMatrix.h>
#include <Utils/DimensionSizes.h>
#include <Utils/MlxUtils.h>

/**
 * @class   BaseFloatMatrix
 * @brief   Abstract base class for float based matrices defining basic interface. Higher dimensional
 *          matrices stored as 1D arrays, row-major order.
 *
 * @details In the MLX port, the "device" copy of the matrix is an MLX array (`mx::array`) of shape (nz, ny, nx).
 *          The host copy is only used for I/O and host-side preprocessing. It is allocated lazily (and zeroed) on the
 *          first access and released after the data has been uploaded to the device, so that large simulations do
 *          not keep two copies of every matrix. MLX arrays are immutable, so the "kernels" do not modify the device
 *          data in place but replace the array with a new one using setArray().
 */
class BaseFloatMatrix : public BaseMatrix
{
  public:
    /// Default constructor.
    BaseFloatMatrix();
    /// Copy constructor not allowed.
    BaseFloatMatrix(const BaseFloatMatrix&) = delete;
    /// Destructor.
    virtual ~BaseFloatMatrix() override;

    /// Operator = not allowed.
    BaseFloatMatrix& operator=(const BaseFloatMatrix&) = delete;

    /**
     * @brief  Get dimension sizes of the matrix.
     * @return Dimension sizes of the matrix.
     */
    virtual const DimensionSizes& getDimensionSizes() const override { return mDimensionSizes; };
    /**
     * @brief  Size of the matrix.
     * @return Number of elements.
     */
    virtual size_t size()                             const override { return mSize; };
    /**
     * @brief  The capacity of the matrix (this may differ from size due to padding, etc.).
     * @return Capacity of the currently allocated storage (in floats).
     */
    virtual size_t capacity()                         const override { return mCapacity; };

    /**
     * @brief  Get matrix data stored on the host side. The memory is allocated and zeroed on the first access.
     * @return Pointer to mutable matrix data stored on the host side.
     */
    virtual float*       getHostData();
    /**
     * @brief  Get matrix data stored on the host side, const version.
     * @return Pointer to immutable matrix data stored on the host side.
     */
    virtual const float* getHostData() const;

    /**
     * @brief  Get the device (MLX) array.
     * @return MLX array of shape (nz, ny, nx).
     */
    const mx::array& getArray() const { return mArray; };
    /**
     * @brief Replace the device (MLX) array, typically with the (lazy) result of a kernel.
     * @param [in] array - New array, must have the same shape as the matrix.
     */
    void setArray(const mx::array& array);
    /**
     * @brief  Is the matrix data present on the device?
     * @return true if the matrix has been uploaded or set on the device.
     */
    bool isOnDevice() const { return mOnDevice; };
    /**
     * @brief  Has the device array been replaced since the last clearModifiedFlag() call?
     * @return true if the array holds a (possibly unevaluated) result of a kernel.
     */
    bool isModified() const { return mModified; };
    /// Clear the modified flag (after the array has been scheduled for evaluation).
    void clearModifiedFlag() { mModified = false; };
    /// Release the host data (the host buffer is allocated again on the next access).
    void releaseHostData() { freeHostMemory(); };
    /// Release the device array (used for temporaries, the matrix has to be set again before it is read).
    void releaseDeviceArray() { mArray = mx::array(0.0f); mOnDevice = false; mModified = false; };

    /// Copy data from host -> device (CPU -> MLX array). The host buffer is released afterwards.
    virtual void copyToDevice() override;
    /// Copy data from device -> host (MLX array -> CPU).
    virtual void copyFromDevice() override;

    /// Zero all elements of the matrix on the host.
    virtual void zeroMatrix();
    /// Zero all elements of the matrix on device.
    virtual void zeroDeviceMatrix();

    /**
     * @brief Calculate matrix = scalar / matrix on the host side.
     * @param [in] scalar - Scalar constant
     */
    virtual void scalarDividedBy(const float scalar);

  protected:
    /// MLX data type of the device array.
    virtual mx::Dtype getDeviceDataType() const = 0;
    /// MLX shape of the device array.
    mx::Shape getDeviceShape() const { return toMlxShape(mDimensionSizes); };

    /// Allocate (and zero) host memory if not allocated yet.
    virtual void allocateMemory();
    /// Memory deallocation (both on host and device).
    virtual void freeMemory();
    /// Free host memory only.
    void freeHostMemory();

    /// Dimension sizes.
    DimensionSizes mDimensionSizes;
    /// Total number of used elements.
    size_t mSize;
    /// Total number of allocated elements (in terms of floats).
    size_t mCapacity;

    /// Raw CPU matrix data (allocated lazily).
    float* mHostData;
    /// Device data.
    mx::array mArray;
    /// Is the device array valid?
    bool mOnDevice;
    /// Has the device array been replaced by a kernel since the last evaluation?
    bool mModified;

  private:

};//end of BaseFloatMatrix
//----------------------------------------------------------------------------------------------------------------------

#endif /* BASE_FLOAT_MATRIX_H */
