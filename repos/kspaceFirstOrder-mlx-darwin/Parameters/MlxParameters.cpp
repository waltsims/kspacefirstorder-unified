/**
 * @file      MlxParameters.cpp
 *
 * @author    Jiri Jaros \n
 *            Faculty of Information Technology \n
 *            Brno University of Technology \n
 *            jarosjir@fit.vutbr.cz
 *
 * @brief     The implementation file for the class storing the MLX device setup (replaces CudaParameters).
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

#include <cstdlib>
#include <stdexcept>
#include <string>
#include <variant>

#include <Parameters/MlxParameters.h>
#include <Logger/Logger.h>
#include <Utils/MlxUtils.h>

//--------------------------------------------------------------------------------------------------------------------//
//------------------------------------------------- Public methods ---------------------------------------------------//
//--------------------------------------------------------------------------------------------------------------------//

/**
 * Select the device for execution.
 */
void MlxParameters::selectDevice(const int deviceIdx)
{
  // Allow the user to force the CPU backend (useful for testing).
  const char* envDevice = std::getenv("KWAVE_MLX_DEVICE");
  const bool  forceCpu  = (envDevice != nullptr) && (std::string(envDevice) == "cpu");

  if (!forceCpu && mx::gpu::is_available())
  {
    const int nDevices = mx::gpu::device_count();
    mDeviceIdx = (deviceIdx == kDefaultDeviceIdx) ? 0 : deviceIdx;

    if ((mDeviceIdx < 0) || (mDeviceIdx > nDevices - 1))
    {
      throw std::runtime_error(Logger::formatMessage(kErrFmtBadDeviceIndex, mDeviceIdx, nDevices - 1));
    }

    mx::set_default_device(mx::Device(mx::Device::gpu, mDeviceIdx));
    mIsGpu       = true;
    mBackendName = (mx::metal::is_available()) ? "Metal" : "CUDA";

    const auto& info = mx::gpu::device_info(mDeviceIdx);
    const auto  name = info.find("device_name");
    mDeviceName = ((name != info.end()) && std::holds_alternative<std::string>(name->second))
                     ? std::get<std::string>(name->second) : kOutFmtDeviceNameUnknown;
  }
  else
  {
    mx::set_default_device(mx::Device(mx::Device::cpu));
    mDeviceIdx   = 0;
    mIsGpu       = false;
    mBackendName = "CPU";
    mDeviceName  = "CPU";
  }
}// end of selectDevice
//----------------------------------------------------------------------------------------------------------------------

/**
 * Get device memory currently allocated by MLX.
 */
size_t MlxParameters::getActiveMemoryUsage()
{
  return mx::get_active_memory() >> 20;
}// end of getActiveMemoryUsage
//----------------------------------------------------------------------------------------------------------------------

/**
 * Get peak device memory allocated by MLX.
 */
size_t MlxParameters::getPeakMemoryUsage()
{
  return mx::get_peak_memory() >> 20;
}// end of getPeakMemoryUsage
//----------------------------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------------------------//
//------------------------------------------------ Protected methods -------------------------------------------------//
//--------------------------------------------------------------------------------------------------------------------//

//--------------------------------------------------------------------------------------------------------------------//
//------------------------------------------------- Private methods --------------------------------------------------//
//--------------------------------------------------------------------------------------------------------------------//

/**
 * Constructor.
 */
MlxParameters::MlxParameters()
  : mDeviceIdx(kDefaultDeviceIdx),
    mIsGpu(false),
    mDeviceName(""),
    mBackendName("")
{

}// end of MlxParameters
//----------------------------------------------------------------------------------------------------------------------
