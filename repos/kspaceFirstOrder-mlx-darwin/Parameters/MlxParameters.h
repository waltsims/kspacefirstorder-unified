/**
 * @file      MlxParameters.h
 *
 * @author    Jiri Jaros \n
 *            Faculty of Information Technology \n
 *            Brno University of Technology \n
 *            jarosjir@fit.vutbr.cz
 *
 * @brief     The header file for the class storing the MLX device setup (replaces CudaParameters).
 *
 * @version   kspaceFirstOrder 3.6 (MLX port)
 *
 * @date      12 November  2015, 16:49 (created) \n
 *            11 February  2020, 16:21 (revised)
 *
 * @copyright Copyright (C) 2015 - 2020 SC\@FIT Research Group, Brno University of Technology, Brno, CZ.
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

#ifndef MLX_PARAMETERS_H
#define MLX_PARAMETERS_H

#include <string>

/**
 * @class   MlxParameters
 * @brief   Class responsible for the MLX device selection (replaces CudaParameters).
 * @details MLX selects the backend at build time (Metal on Apple Silicon, CUDA on Linux/NVIDIA, CPU otherwise).
 *          This class picks the device used by the default MLX stream. A GPU is used when available, unless the
 *          environment variable KWAVE_MLX_DEVICE is set to "cpu".
 */
class MlxParameters
{
  public:
    /// Only Parameters can create this class.
    friend class Parameters;

    /// Copy constructor not allowed.
    MlxParameters(const MlxParameters&) = delete;
    /// Destructor.
    ~MlxParameters() = default;
    /// Operator = not allowed.
    MlxParameters& operator=(const MlxParameters&) = delete;

    /**
     * @brief  Get index of the device used.
     * @return Device index (always 0 for the CPU).
     */
    int getDeviceIdx()               const { return mDeviceIdx; };
    /**
     * @brief  Get the name of the device used.
     * @return Device name.
     */
    std::string getDeviceName()      const { return mDeviceName; };
    /**
     * @brief  Get the name of the MLX backend used (Metal, CUDA or CPU).
     * @return Backend name.
     */
    std::string getBackendName()     const { return mBackendName; };
    /**
     * @brief  Is the computation running on a GPU?
     * @return true if a GPU device was selected.
     */
    bool isGpu()                     const { return mIsGpu; };

    /**
     * @brief   Select the device for execution.
     * @param   [in] deviceIdx - Device index, kDefaultDeviceIdx selects the first GPU.
     * @throw   std::runtime_error - If the device index is wrong.
     */
    void selectDevice(const int deviceIdx = kDefaultDeviceIdx);

    /**
     * @brief  Get device memory currently allocated by MLX.
     * @return Memory in MB.
     */
    static size_t getActiveMemoryUsage();
    /**
     * @brief  Get peak device memory allocated by MLX.
     * @return Memory in MB.
     */
    static size_t getPeakMemoryUsage();

    /// Default device index (the first GPU).
    static constexpr int kDefaultDeviceIdx = -1;

  protected:

  private:
    /// Default constructor - only Parameters can call it.
    MlxParameters();

    /// Index of the device used.
    int         mDeviceIdx;
    /// Is a GPU used?
    bool        mIsGpu;
    /// Name of the device.
    std::string mDeviceName;
    /// Name of the MLX backend.
    std::string mBackendName;
};// end of MlxParameters
//----------------------------------------------------------------------------------------------------------------------

#endif /* MLX_PARAMETERS_H */
