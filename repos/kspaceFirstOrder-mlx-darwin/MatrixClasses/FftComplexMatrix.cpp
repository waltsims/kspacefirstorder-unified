/**
 * @file      FftComplexMatrix.cpp
 *
 * @author    Jiri Jaros \n
 *            Faculty of Information Technology \n
 *            Brno University of Technology \n
 *            jarosjir@fit.vutbr.cz
 *
 * @brief     The implementation file containing the class that implements 3D/2D and 1D FFTs using MLX.
 *
 * @version   kspaceFirstOrder 3.6 (MLX port)
 *
 * @date      09 August    2011, 13:10 (created) \n
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

#include <stdexcept>

#include <MatrixClasses/FftComplexMatrix.h>
#include <MatrixClasses/RealMatrix.h>
#include <Parameters/Parameters.h>
#include <Logger/Logger.h>

using FFTNorm = mx::fft::FFTNorm;

//--------------------------------------------------------------------------------------------------------------------//
//------------------------------------------------- Public methods ---------------------------------------------------//
//--------------------------------------------------------------------------------------------------------------------//

/**
 * Replace the spectrum.
 */
void FftComplexMatrix::setSpectrum(const mx::array& spectrum)
{
  // Evaluate immediately to release the FFT scratch memory (see isLowMemoryMode).
  if (isLowMemoryMode())
  {
    mx::eval(spectrum);
  }
  mArray    = spectrum;
  mOnDevice = true;
  mModified = true;
}// end of setSpectrum
//----------------------------------------------------------------------------------------------------------------------

/**
 * Compute forward out-of-place ND (2D/3D) Real-to-Complex transform (unnormalized).
 */
void FftComplexMatrix::computeR2CFftND(const RealMatrix& inMatrix)
{
  setSpectrum(mx::fft::rfftn(inMatrix.getArray(), getNDAxes(), FFTNorm::Backward));
}// end of computeR2CFftND
//----------------------------------------------------------------------------------------------------------------------

/**
 * Compute inverse out-of-place ND (2D/3D) Complex-to-Real transform (unnormalized, like cuFFT).
 * FFTNorm::Forward means "scale the forward transform", i.e. no scaling of the inverse transform.
 */
void FftComplexMatrix::computeC2RFftND(RealMatrix& outMatrix) const
{
  outMatrix.setArray(mx::fft::irfftn(mArray,
                                     getNDSizes(outMatrix.getDimensionSizes()),
                                     getNDAxes(),
                                     FFTNorm::Forward));
  if (isLowMemoryMode())
  {
    mx::eval(outMatrix.getArray());
  }
}// end of computeC2RFftND
//----------------------------------------------------------------------------------------------------------------------

/**
 * Compute forward 1D Real-to-Complex transform over x.
 */
void FftComplexMatrix::computeR2CFft1DX(const RealMatrix& inMatrix)
{
  setSpectrum(mx::fft::rfft(inMatrix.getArray(), int(inMatrix.getDimensionSizes().nx), 2, FFTNorm::Backward));
}// end of computeR2CFft1DX
//----------------------------------------------------------------------------------------------------------------------

/**
 * Compute forward 1D Real-to-Complex transform over y.
 */
void FftComplexMatrix::computeR2CFft1DY(const RealMatrix& inMatrix)
{
  setSpectrum(mx::fft::rfft(inMatrix.getArray(), int(inMatrix.getDimensionSizes().ny), 1, FFTNorm::Backward));
}// end of computeR2CFft1DY
//----------------------------------------------------------------------------------------------------------------------

/**
 * Compute forward 1D Real-to-Complex transform over z.
 */
void FftComplexMatrix::computeR2CFft1DZ(const RealMatrix& inMatrix)
{
  if (!Parameters::getInstance().isSimulation3D())
  {
    throw std::runtime_error(kErrFmtCannotCallR2CFftPlan1DZfor2D);
  }
  setSpectrum(mx::fft::rfft(inMatrix.getArray(), int(inMatrix.getDimensionSizes().nz), 0, FFTNorm::Backward));
}// end of computeR2CFft1DZ
//----------------------------------------------------------------------------------------------------------------------

/**
 * Compute inverse 1D Complex-to-Real transform over x.
 */
void FftComplexMatrix::computeC2RFft1DX(RealMatrix& outMatrix) const
{
  outMatrix.setArray(mx::fft::irfft(mArray, int(outMatrix.getDimensionSizes().nx), 2, FFTNorm::Forward));
}// end of computeC2RFft1DX
//----------------------------------------------------------------------------------------------------------------------

/**
 * Compute inverse 1D Complex-to-Real transform over y.
 */
void FftComplexMatrix::computeC2RFft1DY(RealMatrix& outMatrix) const
{
  outMatrix.setArray(mx::fft::irfft(mArray, int(outMatrix.getDimensionSizes().ny), 1, FFTNorm::Forward));
}// end of computeC2RFft1DY
//----------------------------------------------------------------------------------------------------------------------

/**
 * Compute inverse 1D Complex-to-Real transform over z.
 */
void FftComplexMatrix::computeC2RFft1DZ(RealMatrix& outMatrix) const
{
  if (!Parameters::getInstance().isSimulation3D())
  {
    throw std::runtime_error(kErrFmtCannotCallR2CFftPlan1DZfor2D);
  }
  outMatrix.setArray(mx::fft::irfft(mArray, int(outMatrix.getDimensionSizes().nz), 0, FFTNorm::Forward));
}// end of computeC2RFft1DZ
//----------------------------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------------------------//
//------------------------------------------------- Private methods --------------------------------------------------//
//--------------------------------------------------------------------------------------------------------------------//

/**
 * Axes of the ND transform.
 */
std::vector<int> FftComplexMatrix::getNDAxes()
{
  return (Parameters::getInstance().isSimulation3D()) ? std::vector<int>{0, 1, 2} : std::vector<int>{1, 2};
}// end of getNDAxes
//----------------------------------------------------------------------------------------------------------------------

/**
 * Sizes of the ND transform.
 */
mx::Shape FftComplexMatrix::getNDSizes(const DimensionSizes& dims)
{
  return (Parameters::getInstance().isSimulation3D())
            ? mx::Shape{int(dims.nz), int(dims.ny), int(dims.nx)}
            : mx::Shape{int(dims.ny), int(dims.nx)};
}// end of getNDSizes
//----------------------------------------------------------------------------------------------------------------------
