/**
 * @file      SolverMlxKernels.cpp
 *
 * @author    Jiri Jaros \n
 *            Faculty of Information Technology \n
 *            Brno University of Technology \n
 *            jarosjir@fit.vutbr.cz
 *
 * @brief     The implementation file for all MLX "kernels" used in the GPU implementation of the k-space solver
 *            (replaces SolverCudaKernels.cu).
 *
 * @version   kspaceFirstOrder 3.6 (MLX port)
 *
 * @date      11 March     2013, 13:10 (created) \n
 *            11 February  2020, 16:14 (revised) \n
 *            05 November  2026          (MLX port)
 *
 * @copyright Copyright (C) 2013 - 2020 SC\@FIT Research Group, Brno University of Technology, Brno, CZ.
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

#include <KSpaceSolver/SolverMlxKernels.h>
#include <Logger/Logger.h>

//--------------------------------------------------------------------------------------------------------------------//
//----------------------------------------------------- Constants ----------------------------------------------------//
//--------------------------------------------------------------------------------------------------------------------//

/// Shortcut for Simulation dimensions.
using SD = Parameters::SimulationDimension;
/// Shortcut for Matrix id in the container.
using MI = MatrixContainer::MatrixIdx;
/// Shortcut for the source mode.
using SourceMode = Parameters::SourceMode;
/// Shortcut for a list of arrays (input/output of compiled functions).
using Arrays = std::vector<mx::array>;
/// Compiled MLX function.
using CompiledFunction = std::function<Arrays(const Arrays&)>;
/// Shortcut for the constants.
using C = MlxDeviceConstants;

//--------------------------------------------------------------------------------------------------------------------//
//---------------------------------------------------- Variables -----------------------------------------------------//
//--------------------------------------------------------------------------------------------------------------------//

MatrixContainer* MlxDeviceConstants::container = nullptr;

float MlxDeviceConstants::fftDivider  = 1.0f;
float MlxDeviceConstants::fftDividerX = 1.0f;
float MlxDeviceConstants::fftDividerY = 1.0f;
float MlxDeviceConstants::fftDividerZ = 1.0f;

float MlxDeviceConstants::dt        = 0.0f;
float MlxDeviceConstants::c2        = 0.0f;
float MlxDeviceConstants::rho0      = 0.0f;
float MlxDeviceConstants::dtRho0    = 0.0f;
float MlxDeviceConstants::dtRho0Sgx = 0.0f;
float MlxDeviceConstants::dtRho0Sgy = 0.0f;
float MlxDeviceConstants::dtRho0Sgz = 0.0f;
float MlxDeviceConstants::bOnA      = 0.0f;
float MlxDeviceConstants::absorbTau = 0.0f;
float MlxDeviceConstants::absorbEta = 0.0f;

Parameters::SourceMode MlxDeviceConstants::velocitySourceMode = Parameters::SourceMode::kDirichlet;
size_t                 MlxDeviceConstants::velocitySourceMany = 0;
Parameters::SourceMode MlxDeviceConstants::presureSourceMode  = Parameters::SourceMode::kDirichlet;
size_t                 MlxDeviceConstants::presureSourceMany  = 0;

//--------------------------------------------------------------------------------------------------------------------//
//------------------------------------------------- Global routines --------------------------------------------------//
//--------------------------------------------------------------------------------------------------------------------//

namespace
{
  /// Get a real matrix from the container.
  inline RealMatrix& realMatrix(const MI matrixIdx)
  {
    return C::container->getMatrix<RealMatrix>(matrixIdx);
  }

  /// Get an FFT matrix from the container.
  inline FftComplexMatrix& fftMatrix(const MI matrixIdx)
  {
    return C::container->getMatrix<FftComplexMatrix>(matrixIdx);
  }

  /// Get the device array of a real/complex matrix from the container.
  inline const mx::array& arr(const MI matrixIdx)
  {
    return C::container->getMatrix<BaseFloatMatrix>(matrixIdx).getArray();
  }

  /// Get the device array of an index matrix from the container.
  inline const mx::array& indexArr(const MI matrixIdx)
  {
    return C::container->getMatrix<BaseIndexMatrix>(matrixIdx).getArray();
  }

  /// Scalar as a 0-D MLX array.
  inline mx::array scalar(const float value)
  {
    return mx::array(value);
  }

  /// Return a scalar (homogeneous medium) or the matrix (heterogeneous medium).
  inline mx::array scalarOrMatrix(const bool scalarFlag, const float value, const MI matrixIdx)
  {
    return (scalarFlag) ? scalar(value) : arr(matrixIdx);
  }

  /// Flatten an array to 1D.
  inline mx::array flat(const mx::array& a)
  {
    return mx::reshape(a, {-1});
  }

  /**
   * Get the source values for a given time step.
   * @param [in] sourceInput - Source input matrix, either a single series (1 x 1 x nt) or many series (1 x n x nt).
   * @param [in] sourceSize  - Number of source points n.
   * @param [in] manyFlag    - Is there a series per source point?
   * @param [in] timeIndex   - Time index.
   * @return Values for all source points, shape (n, 1) for use with scatter.
   */
  mx::array getSourceValues(const mx::array& sourceInput,
                            const int        sourceSize,
                            const bool       manyFlag,
                            const size_t     timeIndex)
  {
    const mx::array input = flat(sourceInput);
    if (manyFlag)
    {
      const int start = static_cast<int>(timeIndex) * sourceSize;
      return mx::reshape(mx::slice(input, {start}, {start + sourceSize}), {sourceSize, 1});
    }
    else
    {
      const int t = static_cast<int>(timeIndex);
      return mx::broadcast_to(mx::reshape(mx::slice(input, {t}, {t + 1}), {1, 1}), {sourceSize, 1});
    }
  }// end of getSourceValues

  /**
   * Insert values into a matrix at given (linear) indices, either replacing them (Dirichlet) or adding them.
   * @param [in] matrix  - Matrix array (any shape).
   * @param [in] indices - Linear indices (uint32).
   * @param [in] values  - Values of shape (n, 1).
   * @param [in] additive - Add instead of replace.
   * @return New matrix array of the same shape.
   */
  mx::array insertValues(const mx::array& matrix,
                         const mx::array& indices,
                         const mx::array& values,
                         const bool       additive)
  {
    const mx::array idx = flat(indices);
    const mx::array out = (additive) ? mx::scatter_add(flat(matrix), idx, values, 0)
                                     : mx::scatter(flat(matrix), idx, values, 0);
    return mx::reshape(out, matrix.shape());
  }// end of insertValues

  //----------------------------------------------- Compiled functions -----------------------------------------------//
  // Each function is fused by mx::compile into a single kernel. Scalars are passed as 0-D arrays, so the same
  // function serves both homogeneous and heterogeneous media (the trace is specialized by the input shapes).

  /// in: {spectrum, kappa, ddx, ddy[, ddz]} -> {spectrum * kappa * dd for each dd}.
  Arrays fnPressureGradient(const Arrays& in)
  {
    const mx::array eKappa = in[0] * in[1];
    Arrays out;
    for (size_t i = 2; i < in.size(); i++)
    {
      out.push_back(eKappa * in[i]);
    }
    return out;
  }

  /// in: {u, ifft, pml, dtRho0, divider} -> {(u * pml - divider * ifft * dtRho0) * pml}.
  Arrays fnVelocityUniform(const Arrays& in)
  {
    return {(in[0] * in[2] - in[4] * in[1] * in[3]) * in[2]};
  }

  /// in: {u, ifft, pml, dxudxn, divider} -> {(u * pml - divider * dxudxn * ifft) * pml}.
  Arrays fnVelocityNonuniform(const Arrays& in)
  {
    return {(in[0] * in[2] - in[4] * in[3] * in[1]) * in[2]};
  }

  /// in: {spectrum, shift, divider} -> {spectrum * shift * divider}.
  Arrays fnVelocityShift(const Arrays& in)
  {
    return {in[0] * in[1] * in[2]};
  }

  /// in: {kappa, divider, fftX, ddxNeg, fftY, ddyNeg[, fftZ, ddzNeg]} -> {fft * (kappa * divider) * dd}.
  Arrays fnVelocityGradient(const Arrays& in)
  {
    const mx::array eKappa = in[0] * in[1];
    Arrays out;
    for (size_t i = 2; i < in.size(); i += 2)
    {
      out.push_back(in[i] * eKappa * in[i + 1]);
    }
    return out;
  }

  /// in: {du, dudn} -> {du * dudn}.
  Arrays fnMultiply(const Arrays& in)
  {
    return {in[0] * in[1]};
  }

  /// in: {rho0, dt, rhoX, pmlX, duxdx, rhoY, pmlY, duydy[, rhoZ, pmlZ, duzdz]} - nonlinear density.
  Arrays fnDensityNonlinear(const Arrays& in)
  {
    mx::array sumRhos = in[2] + in[5];
    if (in.size() > 8)
    {
      sumRhos = sumRhos + in[8];
    }
    const mx::array sumRhosDt = (2.0f * sumRhos + in[0]) * in[1];

    Arrays out;
    for (size_t i = 2; i < in.size(); i += 3)
    {
      const mx::array& rho = in[i];
      const mx::array& pml = in[i + 1];
      const mx::array& du  = in[i + 2];
      out.push_back(pml * ((pml * rho) - sumRhosDt * du));
    }
    return out;
  }

  /// in: {dtRho0, rho, pml, du} -> {pml * (pml * rho - dtRho0 * du)} - linear density.
  Arrays fnDensityLinear(const Arrays& in)
  {
    return {in[2] * (in[2] * in[1] - in[0] * in[3])};
  }

  /// Sum of 2 or 3 arrays starting at offset.
  inline mx::array sumOf(const Arrays& in, size_t offset, size_t count)
  {
    mx::array sum = in[offset] + in[offset + 1];
    if (count == 3)
    {
      sum = sum + in[offset + 2];
    }
    return sum;
  }

  /// in: {c2, bOnA, rho0, rhoX, rhoY[, rhoZ]} -> p = c2 * (rhoSum + bOnA * rhoSum^2 / (2 * rho0)).
  Arrays fnPressureNonlinearLossless(const Arrays& in)
  {
    const mx::array rhoSum = sumOf(in, 3, in.size() - 3);
    return {in[0] * (rhoSum + (in[1] * (rhoSum * rhoSum) / (2.0f * in[2])))};
  }

  /// in: {bOnA, rho0, rhoX..., duxdx...} (2*dims) -> {densitySum, nonlinearTerm, velocityGradientSum}.
  Arrays fnPressureTermsNonlinearPowerLaw(const Arrays& in)
  {
    const size_t dims   = (in.size() - 2) / 2;
    const mx::array rhoSum = sumOf(in, 2, dims);
    const mx::array duSum  = sumOf(in, 2 + dims, dims);
    return {rhoSum,
            ((in[0] * rhoSum * rhoSum) / (2.0f * in[1])) + rhoSum,
            in[1] * duSum};
  }

  /// in: {c2, absorbTau, absorbEta, divider, baseTerm, absorbTauTerm, absorbEtaTerm} -> pressure with power law.
  Arrays fnSumPressurePowerLaw(const Arrays& in)
  {
    return {in[0] * (in[4] + (in[3] * ((in[5] * in[1]) - (in[6] * in[2]))))};
  }

  /// in: {c2, absorbTau, rho0, bOnA, rhoX..., duxdx...} -> nonlinear Stokes pressure.
  Arrays fnPressureNonlinearStokes(const Arrays& in)
  {
    const size_t dims   = (in.size() - 4) / 2;
    const mx::array rhoSum = sumOf(in, 4, dims);
    const mx::array duSum  = sumOf(in, 4 + dims, dims);
    return {in[0] * (rhoSum + in[1] * in[2] * duSum + ((in[3] * rhoSum * rhoSum) / (2.0f * in[2])))};
  }

  /// in: {c2, rhoX, rhoY[, rhoZ]} -> p = c2 * rhoSum.
  Arrays fnPressureLinearLossless(const Arrays& in)
  {
    return {in[0] * sumOf(in, 1, in.size() - 1)};
  }

  /// in: {rho0, rhoX..., duxdx...} -> {densitySum, velocityGradientSum}.
  Arrays fnPressureTermsLinearPowerLaw(const Arrays& in)
  {
    const size_t dims = (in.size() - 1) / 2;
    return {sumOf(in, 1, dims), in[0] * sumOf(in, 1 + dims, dims)};
  }

  /// in: {c2, absorbTau, rho0, rhoX..., duxdx...} -> linear Stokes pressure.
  Arrays fnPressureLinearStokes(const Arrays& in)
  {
    const size_t dims   = (in.size() - 3) / 2;
    const mx::array rhoSum = sumOf(in, 3, dims);
    const mx::array duSum  = sumOf(in, 3 + dims, dims);
    return {in[0] * (rhoSum + in[1] * in[2] * duSum)};
  }

  /**
   * Compiled version of a function (created on the first use).
   * @tparam function - Function to compile.
   * @return Compiled function.
   */
  template<Arrays (*function)(const Arrays&)>
  const CompiledFunction& compiled()
  {
    static const CompiledFunction compiledFunction = mx::compile(function);
    return compiledFunction;
  }
}// end of anonymous namespace

//--------------------------------------------------------------------------------------------------------------------//
//------------------------------------------------- Public methods ---------------------------------------------------//
//--------------------------------------------------------------------------------------------------------------------//

/**
 * Set up the constants.
 */
void MlxDeviceConstants::setUp(MatrixContainer& matrixContainer)
{
  Parameters& params = Parameters::getInstance();
  DimensionSizes fullDimSizes = params.getFullDimensionSizes();

  container = &matrixContainer;

  fftDivider  = 1.0f / fullDimSizes.nElements();
  fftDividerX = 1.0f / fullDimSizes.nx;
  fftDividerY = 1.0f / fullDimSizes.ny;
  fftDividerZ = 1.0f / fullDimSizes.nz;

  dt        = params.getDt();
  c2        = params.getC2Scalar();

  rho0      = params.getRho0Scalar();
  dtRho0    = params.getRho0Scalar() * params.getDt();
  dtRho0Sgx = params.getDtRho0SgxScalar();
  dtRho0Sgy = params.getDtRho0SgyScalar();
  dtRho0Sgz = params.getDtRho0SgzScalar();

  bOnA      = params.getBOnAScalar();
  absorbTau = params.getAbsorbTauScalar();
  absorbEta = params.getAbsorbEtaScalar();

  presureSourceMode  = params.getPressureSourceMode();
  presureSourceMany  = params.getPressureSourceMany();
  velocitySourceMode = params.getVelocitySourceMode();
  velocitySourceMany = params.getVelocitySourceMany();
}// end of setUp
//----------------------------------------------------------------------------------------------------------------------

/**
 * Compute spectral part of pressure gradient in between FFTs.
 *
 * <b> Matlab code: </b> \code
 *  bsxfun(@times, ddx_k_shift_pos, kappa .* fftn(p)), for all 3 dims
 * \endcode
 */
template<SD   simulationDimension,
         bool rho0ScalarFlag,
         bool bOnAScalarFlag,
         bool c0ScalarFlag,
         bool alphaCoefScalarFlag>
void SolverMlxKernels<simulationDimension, rho0ScalarFlag, bOnAScalarFlag, c0ScalarFlag, alphaCoefScalarFlag>::
computePressureGradient()
{
  Arrays in{arr(MI::kTempFftX), arr(MI::kKappa), arr(MI::kDdxKShiftPosR), arr(MI::kDdyKShiftPos)};
  if (simulationDimension == SD::k3D)
  {
    in.push_back(arr(MI::kDdzKShiftPos));
  }

  const Arrays out = compiled<fnPressureGradient>()(in);

  fftMatrix(MI::kTempFftX).setSpectrum(out[0]);
  fftMatrix(MI::kTempFftY).setSpectrum(out[1]);
  if (simulationDimension == SD::k3D)
  {
    fftMatrix(MI::kTempFftZ).setSpectrum(out[2]);
  }
}// end of computePressureGradient
//----------------------------------------------------------------------------------------------------------------------

/**
 * Compute acoustic velocity for a uniform grid.
 *
 * <b> Matlab code: </b> \code
 *  ux_sgx = bsxfun(@times, pml_x_sgx, bsxfun(@times, pml_x_sgx, ux_sgx) - dt .* rho0_sgx_inv .* real(ifftX);
 *  uy_sgy = bsxfun(@times, pml_y_sgy, bsxfun(@times, pml_y_sgy, uy_sgy) - dt .* rho0_sgy_inv .* real(ifftY);
 *  uz_sgz = bsxfun(@times, pml_z_sgz, bsxfun(@times, pml_z_sgz, uz_sgz) - dt .* rho0_sgz_inv .* real(ifftZ);
 * \endcode
 */
template<SD   simulationDimension,
         bool rho0ScalarFlag,
         bool bOnAScalarFlag,
         bool c0ScalarFlag,
         bool alphaCoefScalarFlag>
void SolverMlxKernels<simulationDimension, rho0ScalarFlag, bOnAScalarFlag, c0ScalarFlag, alphaCoefScalarFlag>::
computeVelocityUniform()
{
  const mx::array divider = scalar(C::fftDivider);

  realMatrix(MI::kUxSgx).setArray(compiled<fnVelocityUniform>()({arr(MI::kUxSgx), arr(MI::kTemp1RealND), arr(MI::kPmlXSgx),
                                                           scalarOrMatrix(rho0ScalarFlag, C::dtRho0Sgx, MI::kDtRho0Sgx),
                                                           divider})[0]);
  realMatrix(MI::kUySgy).setArray(compiled<fnVelocityUniform>()({arr(MI::kUySgy), arr(MI::kTemp2RealND), arr(MI::kPmlYSgy),
                                                           scalarOrMatrix(rho0ScalarFlag, C::dtRho0Sgy, MI::kDtRho0Sgy),
                                                           divider})[0]);
  if (simulationDimension == SD::k3D)
  {
    realMatrix(MI::kUzSgz).setArray(compiled<fnVelocityUniform>()({arr(MI::kUzSgz), arr(MI::kTemp3RealND),
                                                             arr(MI::kPmlZSgz),
                                                             scalarOrMatrix(rho0ScalarFlag, C::dtRho0Sgz,
                                                                            MI::kDtRho0Sgz),
                                                             divider})[0]);
  }
}// end of computeVelocityUniform
//----------------------------------------------------------------------------------------------------------------------

/**
 * Compute acoustic velocity for homogeneous medium and nonuniform grid.
 *
 * <b> Matlab code: </b> \code
 *  ux_sgx = bsxfun(@times, pml_x_sgx, bsxfun(@times, pml_x_sgx, ux_sgx)  ...
 *                  - dt .* rho0_sgx_inv .* dxudxnSgx.* real(ifftX))
 * \endcode
 */
template<SD   simulationDimension,
         bool rho0ScalarFlag,
         bool bOnAScalarFlag,
         bool c0ScalarFlag,
         bool alphaCoefScalarFlag>
void SolverMlxKernels<simulationDimension, rho0ScalarFlag, bOnAScalarFlag, c0ScalarFlag, alphaCoefScalarFlag>::
computeVelocityHomogeneousNonuniform()
{
  realMatrix(MI::kUxSgx).setArray(compiled<fnVelocityNonuniform>()({arr(MI::kUxSgx), arr(MI::kTemp1RealND),
                                                              arr(MI::kPmlXSgx), arr(MI::kDxudxnSgx),
                                                              scalar(C::dtRho0Sgx * C::fftDivider)})[0]);
  realMatrix(MI::kUySgy).setArray(compiled<fnVelocityNonuniform>()({arr(MI::kUySgy), arr(MI::kTemp2RealND),
                                                              arr(MI::kPmlYSgy), arr(MI::kDyudynSgy),
                                                              scalar(C::dtRho0Sgy * C::fftDivider)})[0]);
  if (simulationDimension == SD::k3D)
  {
    realMatrix(MI::kUzSgz).setArray(compiled<fnVelocityNonuniform>()({arr(MI::kUzSgz), arr(MI::kTemp3RealND),
                                                                arr(MI::kPmlZSgz), arr(MI::kDzudznSgz),
                                                                scalar(C::dtRho0Sgz * C::fftDivider)})[0]);
  }
}// end of computeVelocityHomogeneousNonuniform
//----------------------------------------------------------------------------------------------------------------------

/**
 * Compute the velocity shift in Fourier space over the x axis.
 */
template<SD   simulationDimension,
         bool rho0ScalarFlag,
         bool bOnAScalarFlag,
         bool c0ScalarFlag,
         bool alphaCoefScalarFlag>
void SolverMlxKernels<simulationDimension, rho0ScalarFlag, bOnAScalarFlag, c0ScalarFlag, alphaCoefScalarFlag>::
computeVelocityShiftInX()
{
  FftComplexMatrix& shiftTemp = fftMatrix(MI::kTempFftShift);
  shiftTemp.setSpectrum(compiled<fnVelocityShift>()({shiftTemp.getArray(), arr(MI::kXShiftNegR),
                                               scalar(C::fftDividerX)})[0]);
}// end of computeVelocityShiftInX
//----------------------------------------------------------------------------------------------------------------------

/**
 * Compute the velocity shift in Fourier space over the y axis. Unlike in the CUDA version, the spectrum is not
 * transposed, the y_shift_neg_r operator of shape (1, nyR, 1) broadcasts over the (nz, nyR, nx) spectrum.
 */
template<SD   simulationDimension,
         bool rho0ScalarFlag,
         bool bOnAScalarFlag,
         bool c0ScalarFlag,
         bool alphaCoefScalarFlag>
void SolverMlxKernels<simulationDimension, rho0ScalarFlag, bOnAScalarFlag, c0ScalarFlag, alphaCoefScalarFlag>::
computeVelocityShiftInY()
{
  FftComplexMatrix& shiftTemp = fftMatrix(MI::kTempFftShift);
  shiftTemp.setSpectrum(compiled<fnVelocityShift>()({shiftTemp.getArray(), arr(MI::kYShiftNegR),
                                               scalar(C::fftDividerY)})[0]);
}// end of computeVelocityShiftInY
//----------------------------------------------------------------------------------------------------------------------

/**
 * Compute the velocity shift in Fourier space over the z axis.
 */
template<SD   simulationDimension,
         bool rho0ScalarFlag,
         bool bOnAScalarFlag,
         bool c0ScalarFlag,
         bool alphaCoefScalarFlag>
void SolverMlxKernels<simulationDimension, rho0ScalarFlag, bOnAScalarFlag, c0ScalarFlag, alphaCoefScalarFlag>::
computeVelocityShiftInZ()
{
  FftComplexMatrix& shiftTemp = fftMatrix(MI::kTempFftShift);
  shiftTemp.setSpectrum(compiled<fnVelocityShift>()({shiftTemp.getArray(), arr(MI::kZShiftNegR),
                                               scalar(C::fftDividerZ)})[0]);
}// end of computeVelocityShiftInZ
//----------------------------------------------------------------------------------------------------------------------

/**
 * Compute spectral part of the velocity gradient in between FFTs on uniform grid.
 *
 * <b> Matlab code: </b> \code
 *  bsxfun(@times, ddx_k_shift_neg, kappa .* fftn(ux_sgx)), for all 3 dims
 * \endcode
 */
template<SD   simulationDimension,
         bool rho0ScalarFlag,
         bool bOnAScalarFlag,
         bool c0ScalarFlag,
         bool alphaCoefScalarFlag>
void SolverMlxKernels<simulationDimension, rho0ScalarFlag, bOnAScalarFlag, c0ScalarFlag, alphaCoefScalarFlag>::
computeVelocityGradient()
{
  Arrays in{arr(MI::kKappa), scalar(C::fftDivider),
            arr(MI::kTempFftX), arr(MI::kDdxKShiftNegR),
            arr(MI::kTempFftY), arr(MI::kDdyKShiftNeg)};
  if (simulationDimension == SD::k3D)
  {
    in.push_back(arr(MI::kTempFftZ));
    in.push_back(arr(MI::kDdzKShiftNeg));
  }

  const Arrays out = compiled<fnVelocityGradient>()(in);

  fftMatrix(MI::kTempFftX).setSpectrum(out[0]);
  fftMatrix(MI::kTempFftY).setSpectrum(out[1]);
  if (simulationDimension == SD::k3D)
  {
    fftMatrix(MI::kTempFftZ).setSpectrum(out[2]);
  }
}// end of computeVelocityGradient
//----------------------------------------------------------------------------------------------------------------------

/**
 * Shift gradient of acoustic velocity on non-uniform grid.
 */
template<SD   simulationDimension,
         bool rho0ScalarFlag,
         bool bOnAScalarFlag,
         bool c0ScalarFlag,
         bool alphaCoefScalarFlag>
void SolverMlxKernels<simulationDimension, rho0ScalarFlag, bOnAScalarFlag, c0ScalarFlag, alphaCoefScalarFlag>::
computeVelocityGradientShiftNonuniform()
{
  realMatrix(MI::kDuxdx).setArray(compiled<fnMultiply>()({arr(MI::kDuxdx), arr(MI::kDxudxn)})[0]);
  realMatrix(MI::kDuydy).setArray(compiled<fnMultiply>()({arr(MI::kDuydy), arr(MI::kDyudyn)})[0]);
  if (simulationDimension == SD::k3D)
  {
    realMatrix(MI::kDuzdz).setArray(compiled<fnMultiply>()({arr(MI::kDuzdz), arr(MI::kDzudzn)})[0]);
  }
}// end of computeVelocityGradientShiftNonuniform
//----------------------------------------------------------------------------------------------------------------------

/**
 * Compute acoustic density for non-linear case.
 *
 * <b>Matlab code:</b> \code
 *  rho0_plus_rho = 2 .* (rhox + rhoy + rhoz) + rho0;
 *  rhox = bsxfun(@times, pml_x, bsxfun(@times, pml_x, rhox) - dt .* rho0_plus_rho .* duxdx);
 *  rhoy = bsxfun(@times, pml_y, bsxfun(@times, pml_y, rhoy) - dt .* rho0_plus_rho .* duydy);
 *  rhoz = bsxfun(@times, pml_z, bsxfun(@times, pml_z, rhoz) - dt .* rho0_plus_rho .* duzdz);
 * \endcode
 */
template<SD   simulationDimension,
         bool rho0ScalarFlag,
         bool bOnAScalarFlag,
         bool c0ScalarFlag,
         bool alphaCoefScalarFlag>
void SolverMlxKernels<simulationDimension, rho0ScalarFlag, bOnAScalarFlag, c0ScalarFlag, alphaCoefScalarFlag>::
computeDensityNonlinear()
{
  Arrays in{scalarOrMatrix(rho0ScalarFlag, C::rho0, MI::kRho0), scalar(C::dt),
            arr(MI::kRhoX), arr(MI::kPmlX), arr(MI::kDuxdx),
            arr(MI::kRhoY), arr(MI::kPmlY), arr(MI::kDuydy)};
  if (simulationDimension == SD::k3D)
  {
    in.insert(in.end(), {arr(MI::kRhoZ), arr(MI::kPmlZ), arr(MI::kDuzdz)});
  }

  const Arrays out = compiled<fnDensityNonlinear>()(in);

  realMatrix(MI::kRhoX).setArray(out[0]);
  realMatrix(MI::kRhoY).setArray(out[1]);
  if (simulationDimension == SD::k3D)
  {
    realMatrix(MI::kRhoZ).setArray(out[2]);
  }
}// end of computeDensityNonlinear
//----------------------------------------------------------------------------------------------------------------------

/**
 * Compute acoustic density for linear case.
 *
 * <b>Matlab code:</b> \code
 *  rhox = bsxfun(@times, pml_x, bsxfun(@times, pml_x, rhox) - dt .* rho0 .* duxdx);
 *  rhoy = bsxfun(@times, pml_y, bsxfun(@times, pml_y, rhoy) - dt .* rho0 .* duydy);
 *  rhoz = bsxfun(@times, pml_z, bsxfun(@times, pml_z, rhoz) - dt .* rho0 .* duzdz);
 * \endcode
 */
template<SD   simulationDimension,
         bool rho0ScalarFlag,
         bool bOnAScalarFlag,
         bool c0ScalarFlag,
         bool alphaCoefScalarFlag>
void SolverMlxKernels<simulationDimension, rho0ScalarFlag, bOnAScalarFlag, c0ScalarFlag, alphaCoefScalarFlag>::
computeDensityLinear()
{
  const mx::array dtRho0 = (rho0ScalarFlag) ? scalar(C::dtRho0) : scalar(C::dt) * arr(MI::kRho0);

  realMatrix(MI::kRhoX).setArray(compiled<fnDensityLinear>()({dtRho0, arr(MI::kRhoX), arr(MI::kPmlX),
                                                        arr(MI::kDuxdx)})[0]);
  realMatrix(MI::kRhoY).setArray(compiled<fnDensityLinear>()({dtRho0, arr(MI::kRhoY), arr(MI::kPmlY),
                                                        arr(MI::kDuydy)})[0]);
  if (simulationDimension == SD::k3D)
  {
    realMatrix(MI::kRhoZ).setArray(compiled<fnDensityLinear>()({dtRho0, arr(MI::kRhoZ), arr(MI::kPmlZ),
                                                          arr(MI::kDuzdz)})[0]);
  }
}// end of computeDensityLinear
//----------------------------------------------------------------------------------------------------------------------

/**
 * Sum sub-terms to calculate new pressure in nonlinear lossless case.
 *
 * <b>Matlab code:</b> \code
 *  % calculate p using a nonlinear adiabatic equation of state
 *  p = c.^2 .* (rhox + rhoy + rhoz + medium.BonA .* (rhox + rhoy + rhoz).^2 ./ (2 .* rho0));
 * \endcode
 */
template<SD   simulationDimension,
         bool rho0ScalarFlag,
         bool bOnAScalarFlag,
         bool c0ScalarFlag,
         bool alphaCoefScalarFlag>
void SolverMlxKernels<simulationDimension, rho0ScalarFlag, bOnAScalarFlag, c0ScalarFlag, alphaCoefScalarFlag>::
sumPressureNonlinearLossless()
{
  Arrays in{scalarOrMatrix(c0ScalarFlag,   C::c2,   MI::kC2),
            scalarOrMatrix(bOnAScalarFlag, C::bOnA, MI::kBOnA),
            scalarOrMatrix(rho0ScalarFlag, C::rho0, MI::kRho0),
            arr(MI::kRhoX), arr(MI::kRhoY)};
  if (simulationDimension == SD::k3D)
  {
    in.push_back(arr(MI::kRhoZ));
  }

  realMatrix(MI::kP).setArray(compiled<fnPressureNonlinearLossless>()(in)[0]);
}// end of sumPressureNonlinearLossless
//----------------------------------------------------------------------------------------------------------------------

/**
 * Compute three temporary sums in the new pressure formula in the nonlinear power law absorption case.
 */
template<SD   simulationDimension,
         bool rho0ScalarFlag,
         bool bOnAScalarFlag,
         bool c0ScalarFlag,
         bool alphaCoefScalarFlag>
void SolverMlxKernels<simulationDimension, rho0ScalarFlag, bOnAScalarFlag, c0ScalarFlag, alphaCoefScalarFlag>::
computePressureTermsNonlinearPowerLaw(RealMatrix& densitySum,
                                      RealMatrix& nonlinearTerm,
                                      RealMatrix& velocityGradientSum)
{
  Arrays in{scalarOrMatrix(bOnAScalarFlag, C::bOnA, MI::kBOnA),
            scalarOrMatrix(rho0ScalarFlag, C::rho0, MI::kRho0),
            arr(MI::kRhoX), arr(MI::kRhoY)};
  if (simulationDimension == SD::k3D)
  {
    in.push_back(arr(MI::kRhoZ));
  }
  in.insert(in.end(), {arr(MI::kDuxdx), arr(MI::kDuydy)});
  if (simulationDimension == SD::k3D)
  {
    in.push_back(arr(MI::kDuzdz));
  }

  const Arrays out = compiled<fnPressureTermsNonlinearPowerLaw>()(in);

  densitySum.setArray(out[0]);
  nonlinearTerm.setArray(out[1]);
  velocityGradientSum.setArray(out[2]);
}// end of computePressureTermsNonlinearPowerLaw
//----------------------------------------------------------------------------------------------------------------------

/**
 * Compute absorbing term with abosrbNabla1 and absorbNabla2.
 *
 * <b>Matlab code:</b> \code
 *  fftPart1 = absorbNabla1 .* fftPart1;
 *  fftPart2 = absorbNabla2 .* fftPart2;
 * \endcode
 */
template<SD   simulationDimension,
         bool rho0ScalarFlag,
         bool bOnAScalarFlag,
         bool c0ScalarFlag,
         bool alphaCoefScalarFlag>
void SolverMlxKernels<simulationDimension, rho0ScalarFlag, bOnAScalarFlag, c0ScalarFlag, alphaCoefScalarFlag>::
computeAbsorbtionTerm()
{
  fftMatrix(MI::kTempFftX).setSpectrum(compiled<fnMultiply>()({arr(MI::kTempFftX), arr(MI::kAbsorbNabla1)})[0]);
  fftMatrix(MI::kTempFftY).setSpectrum(compiled<fnMultiply>()({arr(MI::kTempFftY), arr(MI::kAbsorbNabla2)})[0]);
}// end of computeAbsorbtionTerm
//----------------------------------------------------------------------------------------------------------------------

/**
 * Sum sub-terms to calculate new pressure, after FFTs, nonlinear power law absorption case.
 */
template<SD   simulationDimension,
         bool rho0ScalarFlag,
         bool bOnAScalarFlag,
         bool c0ScalarFlag,
         bool alphaCoefScalarFlag>
void SolverMlxKernels<simulationDimension, rho0ScalarFlag, bOnAScalarFlag, c0ScalarFlag, alphaCoefScalarFlag>::
sumPressureTermsNonlinearPowerLaw(const RealMatrix& nonlinearTerm,
                                  const RealMatrix& absorbTauTerm,
                                  const RealMatrix& absorbEtaTerm)
{
  realMatrix(MI::kP).setArray(compiled<fnSumPressurePowerLaw>()(
                                {scalarOrMatrix(c0ScalarFlag,        C::c2,        MI::kC2),
                                 scalarOrMatrix(alphaCoefScalarFlag, C::absorbTau, MI::kAbsorbTau),
                                 scalarOrMatrix(alphaCoefScalarFlag, C::absorbEta, MI::kAbsorbEta),
                                 scalar(C::fftDivider),
                                 nonlinearTerm.getArray(), absorbTauTerm.getArray(), absorbEtaTerm.getArray()})[0]);
}// end of sumPressureTermsNonlinearPowerLaw
//----------------------------------------------------------------------------------------------------------------------

/**
 * Sum sub-terms to calculate new pressure, nonlinear stokes absorption.
 */
template<SD   simulationDimension,
         bool rho0ScalarFlag,
         bool bOnAScalarFlag,
         bool c0ScalarFlag,
         bool alphaCoefScalarFlag>
void SolverMlxKernels<simulationDimension, rho0ScalarFlag, bOnAScalarFlag, c0ScalarFlag, alphaCoefScalarFlag>::
sumPressureNonlinearStokes()
{
  Arrays in{scalarOrMatrix(c0ScalarFlag,        C::c2,        MI::kC2),
            scalarOrMatrix(alphaCoefScalarFlag, C::absorbTau, MI::kAbsorbTau),
            scalarOrMatrix(rho0ScalarFlag,      C::rho0,      MI::kRho0),
            scalarOrMatrix(bOnAScalarFlag,      C::bOnA,      MI::kBOnA),
            arr(MI::kRhoX), arr(MI::kRhoY)};
  if (simulationDimension == SD::k3D)
  {
    in.push_back(arr(MI::kRhoZ));
  }
  in.insert(in.end(), {arr(MI::kDuxdx), arr(MI::kDuydy)});
  if (simulationDimension == SD::k3D)
  {
    in.push_back(arr(MI::kDuzdz));
  }

  realMatrix(MI::kP).setArray(compiled<fnPressureNonlinearStokes>()(in)[0]);
}// end of sumPressureNonlinearStokes
//----------------------------------------------------------------------------------------------------------------------

/**
 * Sum sub-terms to calculate new pressure in linear lossless case.
 *
 * <b>Matlab code:</b> \code
 *  % calculate p using a linear adiabatic equation of state
 *  p = c.^2 .* (rhox + rhoy + rhoz);
 * \endcode
 */
template<SD   simulationDimension,
         bool rho0ScalarFlag,
         bool bOnAScalarFlag,
         bool c0ScalarFlag,
         bool alphaCoefScalarFlag>
void SolverMlxKernels<simulationDimension, rho0ScalarFlag, bOnAScalarFlag, c0ScalarFlag, alphaCoefScalarFlag>::
sumPressureLinearLossless()
{
  Arrays in{scalarOrMatrix(c0ScalarFlag, C::c2, MI::kC2), arr(MI::kRhoX), arr(MI::kRhoY)};
  if (simulationDimension == SD::k3D)
  {
    in.push_back(arr(MI::kRhoZ));
  }

  realMatrix(MI::kP).setArray(compiled<fnPressureLinearLossless>()(in)[0]);
}// end of sumPressureLinearLossless
//----------------------------------------------------------------------------------------------------------------------

/**
 * Calculate two temporary sums in the new pressure formula, linear power law absorption case.
 */
template<SD   simulationDimension,
         bool rho0ScalarFlag,
         bool bOnAScalarFlag,
         bool c0ScalarFlag,
         bool alphaCoefScalarFlag>
void SolverMlxKernels<simulationDimension, rho0ScalarFlag, bOnAScalarFlag, c0ScalarFlag, alphaCoefScalarFlag>::
computePressureTermsLinearPowerLaw(RealMatrix& densitySum,
                                   RealMatrix& velocityGradientSum)
{
  Arrays in{scalarOrMatrix(rho0ScalarFlag, C::rho0, MI::kRho0), arr(MI::kRhoX), arr(MI::kRhoY)};
  if (simulationDimension == SD::k3D)
  {
    in.push_back(arr(MI::kRhoZ));
  }
  in.insert(in.end(), {arr(MI::kDuxdx), arr(MI::kDuydy)});
  if (simulationDimension == SD::k3D)
  {
    in.push_back(arr(MI::kDuzdz));
  }

  const Arrays out = compiled<fnPressureTermsLinearPowerLaw>()(in);

  densitySum.setArray(out[0]);
  velocityGradientSum.setArray(out[1]);
}// end of computePressureTermsLinearPowerLaw
//----------------------------------------------------------------------------------------------------------------------

/**
 * Sum sub-terms for new pressure, linear power law absorption case.
 */
template<SD   simulationDimension,
         bool rho0ScalarFlag,
         bool bOnAScalarFlag,
         bool c0ScalarFlag,
         bool alphaCoefScalarFlag>
void SolverMlxKernels<simulationDimension, rho0ScalarFlag, bOnAScalarFlag, c0ScalarFlag, alphaCoefScalarFlag>::
sumPressureTermsLinearPowerLaw(const RealMatrix& absorbTauTerm,
                               const RealMatrix& absorbEtaTerm,
                               const RealMatrix& densitySum)
{
  realMatrix(MI::kP).setArray(compiled<fnSumPressurePowerLaw>()(
                                {scalarOrMatrix(c0ScalarFlag,        C::c2,        MI::kC2),
                                 scalarOrMatrix(alphaCoefScalarFlag, C::absorbTau, MI::kAbsorbTau),
                                 scalarOrMatrix(alphaCoefScalarFlag, C::absorbEta, MI::kAbsorbEta),
                                 scalar(C::fftDivider),
                                 densitySum.getArray(), absorbTauTerm.getArray(), absorbEtaTerm.getArray()})[0]);
}// end of sumPressureTermsLinearPowerLaw
//----------------------------------------------------------------------------------------------------------------------

/**
 * Sum sub-terms to calculate new pressure, linear stokes absorption.
 */
template<SD   simulationDimension,
         bool rho0ScalarFlag,
         bool bOnAScalarFlag,
         bool c0ScalarFlag,
         bool alphaCoefScalarFlag>
void SolverMlxKernels<simulationDimension, rho0ScalarFlag, bOnAScalarFlag, c0ScalarFlag, alphaCoefScalarFlag>::
sumPressureLinearStokes()
{
  Arrays in{scalarOrMatrix(c0ScalarFlag,        C::c2,        MI::kC2),
            scalarOrMatrix(alphaCoefScalarFlag, C::absorbTau, MI::kAbsorbTau),
            scalarOrMatrix(rho0ScalarFlag,      C::rho0,      MI::kRho0),
            arr(MI::kRhoX), arr(MI::kRhoY)};
  if (simulationDimension == SD::k3D)
  {
    in.push_back(arr(MI::kRhoZ));
  }
  in.insert(in.end(), {arr(MI::kDuxdx), arr(MI::kDuydy)});
  if (simulationDimension == SD::k3D)
  {
    in.push_back(arr(MI::kDuzdz));
  }

  realMatrix(MI::kP).setArray(compiled<fnPressureLinearStokes>()(in)[0]);
}// end of sumPressureLinearStokes
//----------------------------------------------------------------------------------------------------------------------

/**
 * Add pressure source to acoustic density (Dirichlet and AdditiveNoCorrection modes).
 */
template<SD   simulationDimension,
         bool rho0ScalarFlag,
         bool bOnAScalarFlag,
         bool c0ScalarFlag,
         bool alphaCoefScalarFlag>
void SolverMlxKernels<simulationDimension, rho0ScalarFlag, bOnAScalarFlag, c0ScalarFlag, alphaCoefScalarFlag>::
addPressureSource(const MatrixContainer& container)
{
  if ((C::presureSourceMode != SourceMode::kDirichlet) && (C::presureSourceMode != SourceMode::kAdditiveNoCorrection))
  {
    return;
  }

  const IndexMatrix& sourceIndex = container.getMatrix<IndexMatrix>(MI::kPressureSourceIndex);
  const mx::array values = getSourceValues(arr(MI::kPressureSourceInput),
                                           static_cast<int>(sourceIndex.size()),
                                           C::presureSourceMany != 0,
                                           Parameters::getInstance().getTimeIndex());
  const bool additive = (C::presureSourceMode == SourceMode::kAdditiveNoCorrection);

  realMatrix(MI::kRhoX).setArray(insertValues(arr(MI::kRhoX), sourceIndex.getArray(), values, additive));
  realMatrix(MI::kRhoY).setArray(insertValues(arr(MI::kRhoY), sourceIndex.getArray(), values, additive));
  if (simulationDimension == SD::k3D)
  {
    realMatrix(MI::kRhoZ).setArray(insertValues(arr(MI::kRhoZ), sourceIndex.getArray(), values, additive));
  }
}// end of addPressureSource
//----------------------------------------------------------------------------------------------------------------------

/**
 * Add transducer data source to velocity x component.
 */
template<SD   simulationDimension,
         bool rho0ScalarFlag,
         bool bOnAScalarFlag,
         bool c0ScalarFlag,
         bool alphaCoefScalarFlag>
void SolverMlxKernels<simulationDimension, rho0ScalarFlag, bOnAScalarFlag, c0ScalarFlag, alphaCoefScalarFlag>::
addTransducerSource(const MatrixContainer& container)
{
  const IndexMatrix& sourceIndex = container.getMatrix<IndexMatrix>(MI::kVelocitySourceIndex);
  const int          sourceSize  = static_cast<int>(sourceIndex.size());
  const uint32_t     timeIndex   = static_cast<uint32_t>(Parameters::getInstance().getTimeIndex());

  // transducer_source_input[delay_mask[i] + t_index]
  const mx::array positions = flat(indexArr(MI::kDelayMask)) + mx::array(timeIndex, mx::uint32);
  const mx::array values    = mx::reshape(mx::take(flat(arr(MI::kTransducerSourceInput)), positions),
                                          {sourceSize, 1});

  realMatrix(MI::kUxSgx).setArray(insertValues(arr(MI::kUxSgx), sourceIndex.getArray(), values, true));
}// end of addTransducerSource
//----------------------------------------------------------------------------------------------------------------------

/**
 * Add in velocity source terms (Dirichlet and AdditiveNoCorrection modes).
 */
template<SD   simulationDimension,
         bool rho0ScalarFlag,
         bool bOnAScalarFlag,
         bool c0ScalarFlag,
         bool alphaCoefScalarFlag>
void SolverMlxKernels<simulationDimension, rho0ScalarFlag, bOnAScalarFlag, c0ScalarFlag, alphaCoefScalarFlag>::
addVelocitySource(RealMatrix&        velocity,
                  const RealMatrix&  velocitySourceInput,
                  const IndexMatrix& velocitySourceIndex)
{
  if ((C::velocitySourceMode != SourceMode::kDirichlet) &&
      (C::velocitySourceMode != SourceMode::kAdditiveNoCorrection))
  {
    return;
  }

  const mx::array values = getSourceValues(velocitySourceInput.getArray(),
                                           static_cast<int>(velocitySourceIndex.size()),
                                           C::velocitySourceMany != 0,
                                           Parameters::getInstance().getTimeIndex());
  const bool additive = (C::velocitySourceMode == SourceMode::kAdditiveNoCorrection);

  velocity.setArray(insertValues(velocity.getArray(), velocitySourceIndex.getArray(), values, additive));
}// end of addVelocitySource
//----------------------------------------------------------------------------------------------------------------------

/**
 * Add scaled pressure source to acoustic density.
 */
template<SD   simulationDimension,
         bool rho0ScalarFlag,
         bool bOnAScalarFlag,
         bool c0ScalarFlag,
         bool alphaCoefScalarFlag>
void SolverMlxKernels<simulationDimension, rho0ScalarFlag, bOnAScalarFlag, c0ScalarFlag, alphaCoefScalarFlag>::
addPressureScaledSource(const RealMatrix& scaledSource)
{
  realMatrix(MI::kRhoX).setArray(arr(MI::kRhoX) + scaledSource.getArray());
  realMatrix(MI::kRhoY).setArray(arr(MI::kRhoY) + scaledSource.getArray());
  if (simulationDimension == SD::k3D)
  {
    realMatrix(MI::kRhoZ).setArray(arr(MI::kRhoZ) + scaledSource.getArray());
  }
}// end of addPressureScaledSource
//----------------------------------------------------------------------------------------------------------------------

/**
 * Add scaled velocity source to acoustic velocity.
 */
template<SD   simulationDimension,
         bool rho0ScalarFlag,
         bool bOnAScalarFlag,
         bool c0ScalarFlag,
         bool alphaCoefScalarFlag>
void SolverMlxKernels<simulationDimension, rho0ScalarFlag, bOnAScalarFlag, c0ScalarFlag, alphaCoefScalarFlag>::
addVelocityScaledSource(RealMatrix&        velocity,
                        const RealMatrix&  scaledSource)
{
  velocity.setArray(velocity.getArray() + scaledSource.getArray());
}// end of addVelocityScaledSource
//----------------------------------------------------------------------------------------------------------------------

/**
 * Insert source signal into scaling matrix.
 */
template<SD   simulationDimension,
         bool rho0ScalarFlag,
         bool bOnAScalarFlag,
         bool c0ScalarFlag,
         bool alphaCoefScalarFlag>
void SolverMlxKernels<simulationDimension, rho0ScalarFlag, bOnAScalarFlag, c0ScalarFlag, alphaCoefScalarFlag>::
insertSourceIntoScalingMatrix(RealMatrix&        scaledSource,
                              const RealMatrix&  sourceInput,
                              const IndexMatrix& sourceIndex,
                              const size_t       manyFlag)
{
  const mx::array values = getSourceValues(sourceInput.getArray(),
                                           static_cast<int>(sourceIndex.size()),
                                           manyFlag != 0,
                                           Parameters::getInstance().getTimeIndex());

  scaledSource.setArray(insertValues(scaledSource.getArray(), sourceIndex.getArray(), values, false));
}// end of insertSourceIntoScalingMatrix
//----------------------------------------------------------------------------------------------------------------------

/**
 * Calculate source gradient.
 */
template<SD   simulationDimension,
         bool rho0ScalarFlag,
         bool bOnAScalarFlag,
         bool c0ScalarFlag,
         bool alphaCoefScalarFlag>
void SolverMlxKernels<simulationDimension, rho0ScalarFlag, bOnAScalarFlag, c0ScalarFlag, alphaCoefScalarFlag>::
computeSourceGradient(FftComplexMatrix& sourceSpectrum,
                      const RealMatrix& sourceKappa)
{
  sourceSpectrum.setSpectrum(compiled<fnVelocityShift>()({sourceSpectrum.getArray(), sourceKappa.getArray(),
                                                    scalar(C::fftDivider)})[0]);
}// end of computeSourceGradient
//----------------------------------------------------------------------------------------------------------------------

/**
 * Add initial pressure source to the pressure matrix and update density matrices.
 *
 * <b>Matlab code:</b> \code
 *  % add the initial pressure to rho as a mass source (3D code)
 *  p = source.p0;
 *  rhox = source.p0 ./ (3 .* c.^2);
 *  rhoy = source.p0 ./ (3 .* c.^2);
 *  rhoz = source.p0 ./ (3 .* c.^2);
 * \endcode
 */
template<SD   simulationDimension,
         bool rho0ScalarFlag,
         bool bOnAScalarFlag,
         bool c0ScalarFlag,
         bool alphaCoefScalarFlag>
void SolverMlxKernels<simulationDimension, rho0ScalarFlag, bOnAScalarFlag, c0ScalarFlag, alphaCoefScalarFlag>::
addInitialPressureSource()
{
  constexpr float dimScalingFactor = (simulationDimension == SD::k3D) ? 3.0f : 2.0f;

  const mx::array& p0  = arr(MI::kInitialPressureSourceInput);
  const mx::array  rho = p0 / (dimScalingFactor * scalarOrMatrix(c0ScalarFlag, C::c2, MI::kC2));

  realMatrix(MI::kP).setArray(p0);
  realMatrix(MI::kRhoX).setArray(rho);
  realMatrix(MI::kRhoY).setArray(rho);
  if (simulationDimension == SD::k3D)
  {
    realMatrix(MI::kRhoZ).setArray(rho);
  }
}// end of addInitialPressureSource
//----------------------------------------------------------------------------------------------------------------------

/**
 * Compute velocity for the initial pressure problem, uniform grid.
 *
 * <b> Matlab code: </b> \code
 *  ux_sgx = dt ./ rho0_sgx .* ifft(ux_sgx) / 2;
 *  uy_sgy = dt ./ rho0_sgy .* ifft(uy_sgy) / 2;
 *  uz_sgz = dt ./ rho0_sgz .* ifft(uz_sgz) / 2;
 * \endcode
 */
template<SD   simulationDimension,
         bool rho0ScalarFlag,
         bool bOnAScalarFlag,
         bool c0ScalarFlag,
         bool alphaCoefScalarFlag>
void SolverMlxKernels<simulationDimension, rho0ScalarFlag, bOnAScalarFlag, c0ScalarFlag, alphaCoefScalarFlag>::
computeInitialVelocityUniform()
{
  const mx::array divider = scalar(C::fftDivider * 0.5f);

  realMatrix(MI::kUxSgx).setArray(compiled<fnVelocityShift>()({arr(MI::kUxSgx),
                                                         scalarOrMatrix(rho0ScalarFlag, C::dtRho0Sgx, MI::kDtRho0Sgx),
                                                         divider})[0]);
  realMatrix(MI::kUySgy).setArray(compiled<fnVelocityShift>()({arr(MI::kUySgy),
                                                         scalarOrMatrix(rho0ScalarFlag, C::dtRho0Sgy, MI::kDtRho0Sgy),
                                                         divider})[0]);
  if (simulationDimension == SD::k3D)
  {
    realMatrix(MI::kUzSgz).setArray(compiled<fnVelocityShift>()({arr(MI::kUzSgz),
                                                           scalarOrMatrix(rho0ScalarFlag, C::dtRho0Sgz,
                                                                          MI::kDtRho0Sgz),
                                                           divider})[0]);
  }
}// end of computeInitialVelocityUniform
//----------------------------------------------------------------------------------------------------------------------

/**
 * Compute acoustic velocity for initial pressure problem, homogenous medium, non-uniform grid.
 *
 * <b> Matlab code: </b> \code
 *  ux_sgx = dt ./ rho0_sgx .* dxudxn_sgx .* ifft(ux_sgx) / 2;
 *  uy_sgy = dt ./ rho0_sgy .* dyudyn_sgy .* ifft(uy_sgy) / 2;
 *  uz_sgz = dt ./ rho0_sgz .* dzudzn_sgz .* ifft(uz_sgz) / 2;
 * \endcode
 */
template<SD   simulationDimension,
         bool rho0ScalarFlag,
         bool bOnAScalarFlag,
         bool c0ScalarFlag,
         bool alphaCoefScalarFlag>
void SolverMlxKernels<simulationDimension, rho0ScalarFlag, bOnAScalarFlag, c0ScalarFlag, alphaCoefScalarFlag>::
computeInitialVelocityHomogeneousNonuniform()
{
  realMatrix(MI::kUxSgx).setArray(compiled<fnVelocityShift>()({arr(MI::kUxSgx), arr(MI::kDxudxnSgx),
                                                         scalar(C::fftDivider * 0.5f * C::dtRho0Sgx)})[0]);
  realMatrix(MI::kUySgy).setArray(compiled<fnVelocityShift>()({arr(MI::kUySgy), arr(MI::kDyudynSgy),
                                                         scalar(C::fftDivider * 0.5f * C::dtRho0Sgy)})[0]);
  if (simulationDimension == SD::k3D)
  {
    realMatrix(MI::kUzSgz).setArray(compiled<fnVelocityShift>()({arr(MI::kUzSgz), arr(MI::kDzudznSgz),
                                                           scalar(C::fftDivider * 0.5f * C::dtRho0Sgz)})[0]);
  }
}// end of computeInitialVelocityHomogeneousNonuniform
//----------------------------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------------------------//
//----------------------------------------------- Template instantiation ---------------------------------------------//
//--------------------------------------------------------------------------------------------------------------------//

template class SolverMlxKernels<SD::k3D,  true,  true, true,  true>;
template class SolverMlxKernels<SD::k3D,  true,  true, true,  false>;
template class SolverMlxKernels<SD::k3D,  true,  true, false, true>;
template class SolverMlxKernels<SD::k3D,  true,  true, false, false>;

template class SolverMlxKernels<SD::k3D,  true,  false, true,  true>;
template class SolverMlxKernels<SD::k3D,  true,  false, true,  false>;
template class SolverMlxKernels<SD::k3D,  true,  false, false, true>;
template class SolverMlxKernels<SD::k3D,  true,  false, false, false>;

template class SolverMlxKernels<SD::k3D,  false,  true, true,  true>;
template class SolverMlxKernels<SD::k3D,  false,  true, true,  false>;
template class SolverMlxKernels<SD::k3D,  false,  true, false, true>;
template class SolverMlxKernels<SD::k3D,  false,  true, false, false>;

template class SolverMlxKernels<SD::k3D,  false,  false, true,  true>;
template class SolverMlxKernels<SD::k3D,  false,  false, true,  false>;
template class SolverMlxKernels<SD::k3D,  false,  false, false, true>;
template class SolverMlxKernels<SD::k3D,  false,  false, false, false>;


template class SolverMlxKernels<SD::k2D,  true,  true, true,  true>;
template class SolverMlxKernels<SD::k2D,  true,  true, true,  false>;
template class SolverMlxKernels<SD::k2D,  true,  true, false, true>;
template class SolverMlxKernels<SD::k2D,  true,  true, false, false>;

template class SolverMlxKernels<SD::k2D,  true,  false, true,  true>;
template class SolverMlxKernels<SD::k2D,  true,  false, true,  false>;
template class SolverMlxKernels<SD::k2D,  true,  false, false, true>;
template class SolverMlxKernels<SD::k2D,  true,  false, false, false>;

template class SolverMlxKernels<SD::k2D,  false,  true, true,  true>;
template class SolverMlxKernels<SD::k2D,  false,  true, true,  false>;
template class SolverMlxKernels<SD::k2D,  false,  true, false, true>;
template class SolverMlxKernels<SD::k2D,  false,  true, false, false>;

template class SolverMlxKernels<SD::k2D,  false,  false, true,  true>;
template class SolverMlxKernels<SD::k2D,  false,  false, true,  false>;
template class SolverMlxKernels<SD::k2D,  false,  false, false, true>;
template class SolverMlxKernels<SD::k2D,  false,  false, false, false>;
//----------------------------------------------------------------------------------------------------------------------
