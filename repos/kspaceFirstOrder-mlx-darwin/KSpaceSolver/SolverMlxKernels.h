/**
 * @file      SolverMlxKernels.h
 *
 * @author    Jiri Jaros \n
 *            Faculty of Information Technology \n
 *            Brno University of Technology \n
 *            jarosjir@fit.vutbr.cz
 *
 * @brief     The header file for all MLX "kernels" used in the GPU implementation of the k-space solver
 *            (replaces SolverCudaKernels.cuh).
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

#ifndef SOLVER_MLX_KERNELS_H
#define SOLVER_MLX_KERNELS_H

#include <MatrixClasses/RealMatrix.h>
#include <MatrixClasses/ComplexMatrix.h>
#include <MatrixClasses/IndexMatrix.h>
#include <MatrixClasses/FftComplexMatrix.h>
#include <Containers/MatrixContainer.h>
#include <Utils/DimensionSizes.h>
#include <Utils/MlxUtils.h>
#include <Parameters/Parameters.h>

/**
 * @struct  MlxDeviceConstants
 * @brief   Simulation constants used by the MLX kernels (replaces CudaDeviceConstants).
 * @details In the CUDA version, these constants were stored in the GPU constant memory together with a table of
 *          device pointers to all matrices. In the MLX version, the scalars are kept as 0-D MLX arrays (so they can be
 *          passed into compiled functions as inputs) and the kernels access the matrices through the container.
 */
struct MlxDeviceConstants
{
  /**
   * @brief Set up the constants from the simulation parameters. Has to be called after preprocessing.
   * @param [in] container - Matrix container with all simulation matrices.
   */
  static void setUp(MatrixContainer& container);

  /// Matrix container.
  static MatrixContainer* container;

  /// Normalization constant for 3D FFT.
  static float fftDivider;
  /// Normalization constant for 1D FFT over x.
  static float fftDividerX;
  /// Normalization constant for 1D FFT over y.
  static float fftDividerY;
  /// Normalization constant for 1D FFT over z.
  static float fftDividerZ;

  /// dt.
  static float dt;
  /// c^2 in homogeneous case.
  static float c2;
  /// rho0 in homogeneous case.
  static float rho0;
  /// dt * rho0 in homogeneous case.
  static float dtRho0;
  /// dt / rho0Sgx in homogeneous case.
  static float dtRho0Sgx;
  /// dt / rho0Sgy in homogeneous case.
  static float dtRho0Sgy;
  /// dt / rho0Sgz in homogeneous case.
  static float dtRho0Sgz;
  /// B/A value for homogeneous case.
  static float bOnA;
  /// AbsorbTau value for homogeneous case.
  static float absorbTau;
  /// AbsorbEta value for homogeneous case.
  static float absorbEta;

  /// Velocity source mode.
  static Parameters::SourceMode velocitySourceMode;
  /// Velocity source many.
  static size_t velocitySourceMany;
  /// Pressure source mode.
  static Parameters::SourceMode presureSourceMode;
  /// Pressure source many.
  static size_t presureSourceMany;
};// end of MlxDeviceConstants
//----------------------------------------------------------------------------------------------------------------------

/**
 * @class   SolverMlxKernels
 * @brief   Static class with all MLX kernels used by the k-space solver.
 * @details The interface mirrors the original SolverCudaKernels class so the solver structure stays the same. Each
 *          kernel is expressed as a sequence of MLX array operations. Element-wise kernels are wrapped in
 *          mx::compile, which fuses them into a single GPU kernel (equivalent to the hand written CUDA kernels).
 *          The template parameters are kept for compatibility, the MLX graphs are specialized by the shapes of the
 *          inputs (scalar vs. matrix) automatically.
 *
 * @tparam simulationDimension - Dimensionality of the simulation.
 * @tparam rho0ScalarFlag      - Is density homogeneous?
 * @tparam bOnAScalarFlag      - Is nonlinearity homogeneous?
 * @tparam c0ScalarFlag        - Is sound speed homogeneous?
 * @tparam alphaCoefScalarFlag - Is absorption homogeneous?
 */
template<Parameters::SimulationDimension simulationDimension = Parameters::SimulationDimension::k2D,
         bool                            rho0ScalarFlag      = true,
         bool                            bOnAScalarFlag      = true,
         bool                            c0ScalarFlag        = true,
         bool                            alphaCoefScalarFlag = true>
class SolverMlxKernels
{
  public:
    /// Default constructor not allowed (static class).
    SolverMlxKernels() = delete;
    /// Copy constructor not allowed (static class).
    SolverMlxKernels(SolverMlxKernels&) = delete;
    /// Destructor not allowed (static class).
    ~SolverMlxKernels() = delete;

    //----------------------------------------- Compute pressure gradient --------------------------------------------//
    /// Compute spectral part of pressure gradient in between FFTs.
    static void computePressureGradient();

    //---------------------------------------------- Compute velocity ------------------------------------------------//
    /// Compute acoustic velocity for a uniform grid.
    static void computeVelocityUniform();
    /// Compute acoustic velocity for homogeneous medium and nonuniform grid.
    static void computeVelocityHomogeneousNonuniform();

    /// Compute shifted velocity for --u_non_staggered flag, x direction (spectral part).
    static void computeVelocityShiftInX();
    /// Compute shifted velocity for --u_non_staggered flag, y direction (spectral part).
    static void computeVelocityShiftInY();
    /// Compute shifted velocity for --u_non_staggered flag, z direction (spectral part).
    static void computeVelocityShiftInZ();

    //----------------------------------------- Compute velocity gradient --------------------------------------------//
    /// Compute spectral part of the velocity gradient in between FFTs on uniform grid.
    static void computeVelocityGradient();
    /// Shift gradient of acoustic velocity on non-uniform grid.
    static void computeVelocityGradientShiftNonuniform();

    //---------------------------------------------- Compute density -------------------------------------------------//
    /// Compute acoustic density for non-linear case.
    static void computeDensityNonlinear();
    /// Compute acoustic density for linear case.
    static void computeDensityLinear();

    //----------------------------------------- Compute nonlinear pressure -------------------------------------------//
    /// Sum sub-terms to calculate new pressure in nonlinear lossless case.
    static void sumPressureNonlinearLossless();
    /**
     * @brief Compute three temporary sums in the new pressure formula in the nonlinear power law absorption case.
     * @param [out] densitySum          - rhox_sgx + rhoy_sgy + rhoz_sgz.
     * @param [out] nonlinearTerm       - BOnA + densitySum ^2 / 2 * rho0.
     * @param [out] velocityGradientSum - rho0* (duxdx + duydy + duzdz).
     */
    static void computePressureTermsNonlinearPowerLaw(RealMatrix& densitySum,
                                                      RealMatrix& nonlinearTerm,
                                                      RealMatrix& velocityGradientSum);
    /// Compute absorbing term with abosrbNabla1 and absorbNabla2.
    static void computeAbsorbtionTerm();
    /**
     * @brief Sum sub-terms to calculate new pressure, after FFTs, nonlinear power law absorption case.
     * @param [in] nonlinearTerm - Nonlinear term.
     * @param [in] absorbTauTerm - Absorb tau term from the pressure eq.
     * @param [in] absorbEtaTerm - Absorb eta term from the pressure eq.
     */
    static void sumPressureTermsNonlinearPowerLaw(const RealMatrix& nonlinearTerm,
                                                  const RealMatrix& absorbTauTerm,
                                                  const RealMatrix& absorbEtaTerm);
    /// Sum sub-terms to calculate new pressure, nonlinear stokes absorption.
    static void sumPressureNonlinearStokes();

    //------------------------------------------- Compute linear pressure --------------------------------------------//
    /// Sum sub-terms to calculate new pressure in linear lossless case.
    static void sumPressureLinearLossless();
    /**
     * @brief Calculate two temporary sums in the new pressure formula, linear power law absorption case.
     * @param [out] densitySum          - rhoxSgx + rhoySgy + rhozSgz.
     * @param [out] velocityGradientSum - rho0 * (duxdx + duydy + duzdz).
     */
    static void computePressureTermsLinearPowerLaw(RealMatrix& densitySum,
                                                   RealMatrix& velocityGradientSum);
    /**
     * @brief Sum sub-terms for new pressure, linear power law absorption case.
     * @param [in] absorbTauTerm - Absorb tau term from the pressure eq.
     * @param [in] absorbEtaTerm - Absorb eta term from the pressure eq.
     * @param [in] densitySum    - Sum of acoustic density.
     */
    static void sumPressureTermsLinearPowerLaw(const RealMatrix& absorbTauTerm,
                                               const RealMatrix& absorbEtaTerm,
                                               const RealMatrix& densitySum);
    /// Sum sub-terms to calculate new pressure, linear stokes absorption.
    static void sumPressureLinearStokes();

    //-------------------------------------------------- Sources -----------------------------------------------------//
    /**
     * @brief Add pressure source to acoustic density (Dirichlet and AdditiveNoCorrection modes).
     * @param [in] container - Container with all matrices.
     */
    static void addPressureSource(const MatrixContainer& container);
    /**
     * @brief Add transducer data source to velocity x component.
     * @param [in] container - Container with all matrices.
     */
    static void addTransducerSource(const MatrixContainer& container);
    /**
     * @brief Add in velocity source terms (Dirichlet and AdditiveNoCorrection modes).
     * @param [in, out] velocity            - Velocity matrix to update.
     * @param [in]      velocitySourceInput - Source input to add.
     * @param [in]      velocitySourceIndex - Index matrix.
     */
    static void addVelocitySource(RealMatrix&        velocity,
                                  const RealMatrix&  velocitySourceInput,
                                  const IndexMatrix& velocitySourceIndex);
    /**
     * @brief Add scaled pressure source to acoustic density.
     * @param [in] scaledSource - Scaled source.
     */
    static void addPressureScaledSource(const RealMatrix& scaledSource);
    /**
     * @brief Add scaled velocity source to acoustic velocity.
     * @param [in, out] velocity     - Velocity matrix to update.
     * @param [in]      scaledSource - Scaled source.
     */
    static void addVelocityScaledSource(RealMatrix&        velocity,
                                        const RealMatrix&  scaledSource);
    /**
     * @brief Insert source signal into scaling matrix.
     * @param [out] scaledSource - Temporary matrix to insert the source into.
     * @param [in]  sourceInput  - Source input signal.
     * @param [in]  sourceIndex  - Source geometry.
     * @param [in]  manyFlag     - Number of time series in the source input.
     */
    static void insertSourceIntoScalingMatrix(RealMatrix&        scaledSource,
                                              const RealMatrix&  sourceInput,
                                              const IndexMatrix& sourceIndex,
                                              const size_t       manyFlag);
    /**
     * @brief Calculate source gradient.
     * @param [in, out] sourceSpectrum - Source spectrum.
     * @param [in]      sourceKappa    - Source kappa.
     */
    static void computeSourceGradient(FftComplexMatrix& sourceSpectrum,
                                      const RealMatrix& sourceKappa);
    /// Add initial pressure source to the pressure matrix and update density matrices.
    static void addInitialPressureSource();
    /// Compute velocity for the initial pressure problem, heterogeneous medium, uniform grid.
    static void computeInitialVelocityUniform();
    /// Compute acoustic velocity for initial pressure problem, homogenous medium, non-uniform grid.
    static void computeInitialVelocityHomogeneousNonuniform();

  private:

};// end of SolverMlxKernels
//----------------------------------------------------------------------------------------------------------------------

#endif /* SOLVER_MLX_KERNELS_H */
