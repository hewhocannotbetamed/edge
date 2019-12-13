/**
 * @file This file is part of EDGE.
 *
 * @author Alexander Breuer (anbreuer AT ucsd.edu)
 *         Alexander Heinecke (alexander.heinecke AT intel.com)
 *
 * @section LICENSE
 * Copyright (c) 2019, Alexander Breuer
 * Copyright (c) 2016-2018, Regents of the University of California
 * Copyright (c) 2016, Intel Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * @section DESCRIPTION
 * Quadrature-free ADER-DG surface integration for seismic wave propagation using vanilla kernels.
 **/
#ifndef EDGE_SEISMIC_KERNELS_SURF_INT_VANILLA_HPP
#define EDGE_SEISMIC_KERNELS_SURF_INT_VANILLA_HPP

#include "SurfInt.hpp"
#include "dg/Basis.h"
#include "data/MmVanilla.hpp"

namespace edge {
  namespace seismic {
    namespace kernels { 
      template< typename       TL_T_REAL,
                unsigned short TL_N_RMS,
                t_entityType   TL_T_EL,
                unsigned short TL_O_SP,
                unsigned short TL_N_CRS >
      class SurfIntVanilla;
    }
  }
}

/**
 * Quadrature-free ADER-DG surface integration for seismic wave propagation using vanilla kernels.
 *
 * @paramt TL_T_REAL floating point precision.
 * @paramt TL_N_RMS number of relaxation mechanisms.
 * @paramt TL_T_EL element type.
 * @paramt TL_O_SP spatial order.
 * @paramt TL_N_CRS number of fused simulations.
 **/
template< typename       TL_T_REAL,
          unsigned short TL_N_RMS,
          t_entityType   TL_T_EL,
          unsigned short TL_O_SP,
          unsigned short TL_N_CRS >
class edge::seismic::kernels::SurfIntVanilla: public edge::seismic::kernels::SurfInt< TL_T_REAL,
                                                                                      TL_N_RMS,
                                                                                      TL_T_EL,
                                                                                      TL_O_SP,
                                                                                      TL_N_CRS > {
  private:
    //! number of dimensions
    static unsigned short const TL_N_DIS = C_ENT[TL_T_EL].N_DIM;

    //! number of faces
    static unsigned short const TL_N_FAS = C_ENT[TL_T_EL].N_FACES;

    //! number of DG face modes
    static unsigned short const TL_N_MDS_FA = CE_N_ELEMENT_MODES( C_ENT[TL_T_EL].TYPE_FACES, TL_O_SP );

    //! number of DG element modes
    static unsigned short const TL_N_MDS_EL = CE_N_ELEMENT_MODES( TL_T_EL, TL_O_SP );

    //! number of neigboring contribution flux matrices
    static unsigned short const TL_N_FMNS = CE_N_FLUXN_MATRICES( TL_T_EL );

    //! number of elastic quantities
    static unsigned short const TL_N_QTS_E = CE_N_QTS_E( TL_N_DIS );

    //! number of quantities per relaxation mechanism
    static unsigned short const TL_N_QTS_M = CE_N_QTS_M( TL_N_DIS );

    //! number of entries in the elastic flux solvers
    static unsigned short const TL_N_ENS_FS_E = CE_N_ENS_FS_E_DE( TL_N_DIS );

    //! number of entries in the anelastic flux solvers
    static unsigned short const TL_N_ENS_FS_A = CE_N_ENS_FS_A_DE( TL_N_DIS );

    //! pointers to the local flux matrices
    TL_T_REAL *m_fIntLN[TL_N_FAS+TL_N_FMNS] = {};

    //! pointers to the transposed flux matrices
    TL_T_REAL *m_fIntT[TL_N_FAS] = {};

    //! matrix kernels
    edge::data::MmVanilla< TL_T_REAL > m_mm;

    /**
     * Generates the matrix kernels for the flux matrices and flux solvers.
     **/
    void generateKernels() {
      // add first flux matrix
      m_mm.add( 0,                           // group
                TL_N_QTS_E,                  // m
                TL_N_MDS_FA,                 // n
                TL_N_MDS_EL,                 // k
                TL_N_MDS_EL,                 // ldA
                TL_N_MDS_FA,                 // ldB
                TL_N_MDS_FA,                 // ldC
                static_cast<real_base>(1.0), // alpha
                static_cast<real_base>(0.0), // beta
                true,                        // fused AC
                false,                       // fused BC
                TL_N_CRS );

      // add elastic flux solver
      m_mm.add( 0,                           // group
                TL_N_QTS_E,                  // m
                TL_N_MDS_FA,                 // n
                TL_N_QTS_E,                  // k
                TL_N_QTS_E,                  // ldA
                TL_N_MDS_FA,                 // ldB
                TL_N_MDS_FA,                 // ldC
                static_cast<real_base>(1.0), // alpha
                static_cast<real_base>(0.0), // beta
                false,                       // fused AC
                true,                        // fused BC
                TL_N_CRS );

      // add second flux matrix for elastic update
      m_mm.add( 0,                           // group
                TL_N_QTS_E,                  // m
                TL_N_MDS_EL,                 // n
                TL_N_MDS_FA,                 // k
                TL_N_MDS_FA,                 // ldA
                TL_N_MDS_EL,                 // ldB
                TL_N_MDS_EL,                 // ldC
                static_cast<real_base>(1.0), // alpha
                static_cast<real_base>(1.0), // beta
                true,                        // fused AC
                false,                       // fused BC
                TL_N_CRS );

      // add anelastic flux solver
      m_mm.add( 1,                           // group
                TL_N_QTS_M,                  // m
                TL_N_MDS_FA,                 // n
                TL_N_QTS_E,                  // k
                TL_N_QTS_E,                  // ldA
                TL_N_MDS_FA,                 // ldB
                TL_N_MDS_FA,                 // ldC
                static_cast<real_base>(1.0), // alpha
                static_cast<real_base>(0.0), // beta
                false,                       // fused AC
                true,                        // fused BC
                TL_N_CRS );

      // add second flux matrix for anelastic update
      m_mm.add( 1,                           // group
                TL_N_QTS_M,                  // m
                TL_N_MDS_EL,                 // n
                TL_N_MDS_FA,                 // k
                TL_N_MDS_FA,                 // ldA
                TL_N_MDS_EL,                 // ldB
                TL_N_MDS_EL,                 // ldC
                static_cast<real_base>(1.0), // alpha
                static_cast<real_base>(1.0), // beta
                true,                        // fused AC
                false,                       // fused BC
                TL_N_CRS );
    }

  public:
    /**
     * Constructor of the vanilla surface integration.
     *
     * @param i_rfs relaxation frequencies, use nullptr if TL_N_RMS==0.
     * @param io_dynMem dynamic memory allocations.
     **/
    SurfIntVanilla( TL_T_REAL     const * i_rfs,
                    data::Dynamic       & io_dynMem ): SurfInt< TL_T_REAL,
                                                                TL_N_RMS,
                                                                TL_T_EL,
                                                                TL_O_SP,
                                                                TL_N_CRS >( i_rfs,
                                                                            io_dynMem ) {
      // store flux matrices dense
      this->storeFluxDense( io_dynMem,
                            m_fIntLN,
                            m_fIntT );

      // generate kernels
      generateKernels();
    }

    /**
     * Element local contribution using vanilla matrix-matrix multiplication kernels.
     *
     * @param i_fsE elastic flux solvers.
     * @param i_fsA anelastic flux solvers, use nullptr if TL_N_RMS==0.
     * @param i_tDofsE elastic time integrated DG-DOFs.
     * @param io_dofsE will be updated with local elastic contribution of the element to the surface integral.
     * @param io_dofsA will be updated with local anelastic contribution of the element to the surface integral, use nullptr for TL_N_RMS==0.
     * @param o_scratch will be used as scratch space for the computations.
     * @param i_dofsP DOFs for prefetching (not used).
     * @param i_tDofsP time integrated DOFs for prefetching (not used).
     **/
    void local( TL_T_REAL const   i_fsE[TL_N_FAS][TL_N_ENS_FS_E],
                TL_T_REAL const (*i_fsA)[TL_N_ENS_FS_A],
                TL_T_REAL const   i_tDofsE[TL_N_QTS_E][TL_N_MDS_EL][TL_N_CRS],
                TL_T_REAL         io_dofsE[TL_N_QTS_E][TL_N_MDS_EL][TL_N_CRS],
                TL_T_REAL       (*io_dofsA)[TL_N_QTS_M][TL_N_MDS_EL][TL_N_CRS],
                TL_T_REAL         o_scratch[2][TL_N_QTS_E][TL_N_MDS_FA][TL_N_CRS],
                TL_T_REAL const   i_dofsP[TL_N_QTS_E][TL_N_MDS_EL][TL_N_CRS] = nullptr,
                TL_T_REAL const   i_tDofsP[TL_N_QTS_E][TL_N_MDS_EL][TL_N_CRS] = nullptr ) const {
      // anelastic update buffer
      TL_T_REAL l_upAn[TL_N_QTS_M][TL_N_MDS_EL][TL_N_CRS];
      for( unsigned short l_qt = 0; l_qt < TL_N_QTS_M; l_qt++ ) {
        for( unsigned short l_md = 0; l_md < TL_N_MDS_EL; l_md++ ) {
          for( unsigned short l_cr = 0; l_cr < TL_N_CRS; l_cr++ ) {
            l_upAn[l_qt][l_md][l_cr] = 0;
          }
        }
      }

      // iterate over faces
      for( unsigned short l_fa = 0; l_fa < TL_N_FAS; l_fa++ ) {
        // local flux matrix
        m_mm.m_kernels[0][0]( i_tDofsE[0][0],
                              m_fIntLN[l_fa],
                              o_scratch[0][0][0] );

        // elastic flux solver
        m_mm.m_kernels[0][1]( i_fsE[l_fa],
                              o_scratch[0][0][0],
                              o_scratch[1][0][0] );

        // transposed flux matrix
        m_mm.m_kernels[0][2]( o_scratch[1][0][0],
                              m_fIntT[l_fa],
                              io_dofsE[0][0] );

        if( TL_N_RMS > 0 ) {
          // anelastic flux solver
          m_mm.m_kernels[1][0]( i_fsA[l_fa],
                                o_scratch[0][0][0],
                                o_scratch[1][0][0] );

          // transposed flux matrix
          m_mm.m_kernels[1][1]( o_scratch[1][0][0],
                                m_fIntT[l_fa],
                                l_upAn[0][0] );
        }
      }

      // scatter to anelastic DOFs
      if( TL_N_RMS > 0) this->scatterUpdateA( l_upAn, io_dofsA );
    }

    /**
     * Applies the first first face-integration matrix to the elastic DOFs.
     *
     * @param i_fa local face.
     * @param i_vId id of the vertex, matching the element's vertex 0, from the perspective of the adjacent element w.r.t. to the reference element.
     * @param i_fId id of the face from the perspective of the adjacent element w.r.t. to the reference element.
     * @param i_tDofsE elastic time integrated DOFs.
     * @param o_tDofsFiE elastic time integrated DOFs after application of the first face-int matrix.
     * @param i_pre DOFs or tDOFs for prefetching.
     **/
    void neighFluxInt( unsigned short       i_fa,
                       unsigned short       i_vId,
                       unsigned short       i_fId,
                       TL_T_REAL      const i_tDofsE[TL_N_QTS_E][TL_N_MDS_EL][TL_N_CRS],
                       TL_T_REAL            o_tDofsFiE[TL_N_QTS_E][TL_N_MDS_FA][TL_N_CRS],
                       TL_T_REAL      const i_pre[TL_N_QTS_E][TL_N_MDS_EL][TL_N_CRS] = nullptr ) const {
      // derive the id of the neighboring flux matrix
      unsigned short l_fMatId = std::numeric_limits< unsigned short >::max();
      if( i_vId != std::numeric_limits< unsigned short >::max() ) {
        l_fMatId = TL_N_FAS + this->fMatId( i_vId, i_fId );
      }
      else {
        l_fMatId = i_fa;
      }

      // multiply with first face integration matrix
      m_mm.m_kernels[0][0]( i_tDofsE[0][0],
                            m_fIntLN[l_fMatId],
                            o_tDofsFiE[0][0] );
    }

    /**
     * Neighboring contribution of a single adjacent element using vanilla kernels.
     *
     * @param i_fa local face.
     * @param i_vId id of the vertex, matching the element's vertex 0, from the perspective of the adjacent element w.r.t. to the reference element.
     * @param i_fId id of the face from the perspective of the adjacent element w.r.t. to the reference element.
     * @param i_fsE elastic flux solver.
     * @param i_fsA anelastic flux solver
     * @param i_tDofsE elastic time integrated DG-DOFs.
     * @param i_tDofsFiE elastic time integrated DG-DOFs multiplied with first flux integration matrix.
     * @param io_dofsE will be updated with the elastic contribution of the adjacent element to the surface integral.
     * @param io_dofsA will be updated with the unscaled (w.r.t. frequencies) anelastic contribution of the adjacent element tot the surface integral, use nullptr for TL_N_RMS==0.
     * @param o_scratch will be used as scratch space for the computations.
     * @param i_pre DOFs or tDOFs for prefetching.
     **/
    void neigh( unsigned short       i_fa,
                unsigned short       i_vId,
                unsigned short       i_fId,
                TL_T_REAL      const i_fsE[TL_N_ENS_FS_E],
                TL_T_REAL      const i_fsA[TL_N_ENS_FS_A],
                TL_T_REAL      const i_tDofsE[TL_N_QTS_E][TL_N_MDS_EL][TL_N_CRS],
                TL_T_REAL      const i_tDofsFiE[TL_N_QTS_E][TL_N_MDS_FA][TL_N_CRS],
                TL_T_REAL            io_dofsE[TL_N_QTS_E][TL_N_MDS_EL][TL_N_CRS],
                TL_T_REAL            io_dofsA[TL_N_QTS_M][TL_N_MDS_EL][TL_N_CRS],
                TL_T_REAL            o_scratch[2][TL_N_QTS_E][TL_N_MDS_FA][TL_N_CRS],
                TL_T_REAL      const i_pre[TL_N_QTS_E][TL_N_MDS_EL][TL_N_CRS] = nullptr ) const {
      // apply first face integration matrix or set pointer to pre-computed data
      TL_T_REAL const * l_tDofsFiE = o_scratch[0][0][0];

      if( i_tDofsFiE == nullptr ) {
        neighFluxInt( i_fa,
                      i_vId,
                      i_fId,
                      i_tDofsE,
                      o_scratch[0],
                      i_pre );
      }
      else {
        l_tDofsFiE = i_tDofsFiE[0][0];
      }

      // elastic flux solver
      m_mm.m_kernels[0][1]( i_fsE,
                            l_tDofsFiE,
                            o_scratch[1][0][0] );

      // transposed face integration matrix
      m_mm.m_kernels[0][2]( o_scratch[1][0][0],
                            m_fIntT[i_fa],
                            io_dofsE[0][0] );

      if( TL_N_RMS > 0 ) {
        // anelastic flux solver
        m_mm.m_kernels[1][0]( i_fsA,
                              o_scratch[0][0][0],
                              o_scratch[1][0][0] );

        // transposed face integration matrix
        m_mm.m_kernels[1][1]( o_scratch[1][0][0],
                              m_fIntT[i_fa],
                              io_dofsA[0][0] );
      }
    }
};

#endif