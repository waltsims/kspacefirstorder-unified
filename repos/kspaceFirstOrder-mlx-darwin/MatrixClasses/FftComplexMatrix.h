/**
 * @file      FftComplexMatrix.h
 *
 * @author    Jiri Jaros \n
 *            Faculty of Information Technology \n
 *            Brno University of Technology \n
 *            jarosjir@fit.vutbr.cz
 *
 * @brief     The header file containing the class that implements 3D/2D and 1D FFTs using MLX (replaces the
 *            FftComplexMatrix class of the CUDA version).
 *
 * @version   kspaceFirstOrder 3.6 (MLX port)
 *
 * @date      09 August    2011, 13:10 (created) \n
 *            11 February  2020, 16:14 (revised) \n
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

#ifndef FFT_COMPLEX_MATRIX_H
#define FFT_COMPLEX_MATRIX_H

#include <MatrixClasses/ComplexMatrix.h>

/**
 * @class   FftComplexMatrix
 * @brief   Class implementing ND and 1D Real-To-Complex and Complex-To-Real transforms using MLX.
 * @details The class keeps the interface of the CUDA FftComplexMatrix so the solver code stays the same:
 *          \li The ND transform is 3D for 3D simulations (axes z, y, x) and 2D for 2D simulations (axes y, x).
 *          \li The 1D transforms run over a single axis for all other indices (batch).
 *
 *          Differences from the cuFFT version:
 *          \li No plans are needed (MLX caches its own kernels/plans).
 *          \li The Y and Z 1D transforms do not need explicit transposes; MLX handles the strided axis. The spectrum
 *              is therefore kept in the natural (z, y, x) layout, not in the transposed one.
 *          \li All transforms are unnormalized (like cuFFT); the normalization is applied in the solver kernels.
 *          \li The spectrum is stored as an MLX array whose shape depends on the last transform (e.g.
 *              (nz, ny, nx/2+1) after an ND or 1DX transform), not in a preallocated buffer.
 */
class FftComplexMatrix : public ComplexMatrix
{
  public:
    /// Default constructor not allowed.
    FftComplexMatrix() = delete;
    /**
     * @brief Constructor, inherited from ComplexMatrix.
     * @param [in] dimensionSizes - Dimension sizes of the matrix.
     */
    FftComplexMatrix(const DimensionSizes& dimensionSizes) : ComplexMatrix(dimensionSizes) { };
    /// Copy constructor not allowed.
    FftComplexMatrix(const FftComplexMatrix&) = delete;
    /// Destructor (Inherited from ComplexMatrix).
    virtual ~FftComplexMatrix() = default;

    /// Operator = is not allowed.
    FftComplexMatrix& operator=(const FftComplexMatrix&) = delete;

    /**
     * @brief Replace the spectrum stored in the matrix (any shape is accepted).
     * @param [in] spectrum - New spectrum.
     */
    void setSpectrum(const mx::array& spectrum);

    /**
     * @brief Compute forward out-of-place ND (2D/3D) Real-to-Complex transform.
     * @param [in] inMatrix - Input data for the forward FFT.
     */
    void computeR2CFftND(const RealMatrix& inMatrix);
    /**
     * @brief Compute inverse out-of-place ND (2D/3D) Complex-to-Real transform.
     * @param [out] outMatrix - Output of the inverse FFT.
     */
    void computeC2RFftND(RealMatrix& outMatrix) const;

    /**
     * @brief Compute forward 1D Real-to-Complex transform in the x dimension.
     * @param [in] inMatrix - Input data for the forward FFT.
     */
    void computeR2CFft1DX(const RealMatrix& inMatrix);
    /**
     * @brief Compute forward 1D Real-to-Complex transform in the y dimension.
     * @param [in] inMatrix - Input data for the forward FFT.
     */
    void computeR2CFft1DY(const RealMatrix& inMatrix);
    /**
     * @brief Compute forward 1D Real-to-Complex transform in the z dimension (3D simulations only).
     * @param [in] inMatrix - Input data for the forward FFT.
     */
    void computeR2CFft1DZ(const RealMatrix& inMatrix);

    /**
     * @brief Compute inverse 1D Complex-to-Real transform in the x dimension.
     * @param [out] outMatrix - Output data for the inverse FFT.
     */
    void computeC2RFft1DX(RealMatrix& outMatrix) const;
    /**
     * @brief Compute inverse 1D Complex-to-Real transform in the y dimension.
     * @param [out] outMatrix - Output data for the inverse FFT.
     */
    void computeC2RFft1DY(RealMatrix& outMatrix) const;
    /**
     * @brief Compute inverse 1D Complex-to-Real transform in the z dimension (3D simulations only).
     * @param [out] outMatrix - Output data for the inverse FFT.
     */
    void computeC2RFft1DZ(RealMatrix& outMatrix) const;

  protected:

  private:
    /// Axes used by the ND transform (3D: {0, 1, 2}, 2D: {1, 2}).
    static std::vector<int> getNDAxes();
    /// Sizes of the ND transform in the order of the axes.
    static mx::Shape getNDSizes(const DimensionSizes& dims);
};// FftComplexMatrix
//----------------------------------------------------------------------------------------------------------------------

#endif /* FFT_COMPLEX_MATRIX_H */
