/**
 * @file This file is part of EDGE.
 *
 * @author Alexander Breuer (anbreuer AT ucsd.edu)
 *
 * @section LICENSE
 * Copyright (c) 2019, Alexander Breuer
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
 * Mesh functions and storage.
 **/
#include "Mesh.h"

#include "../io/Moab.h"
#include "Geom.h"
#include "io/logging.h"

void edge_v::mesh::Mesh::getEnVeCrds( t_entityType          i_enTy,
                                      std::size_t  const  * i_enVe,
                                      double       const (* i_veCrds)[3],
                                      double             (* o_enVeCrds)[3] ) {
  // number of vertices for the entity type
  unsigned short l_nEnVes = CE_N_VES( i_enTy );

  // gather coordinates
  for( unsigned short l_ve = 0; l_ve < l_nEnVes; l_ve++ ) {
    std::size_t l_veId = i_enVe[l_ve];

    for( unsigned short l_di = 0; l_di < 3; l_di++ ) {
      o_enVeCrds[l_ve][l_di] = i_veCrds[l_veId][l_di];
    }
  }
}

void edge_v::mesh::Mesh::setInDiameter( t_entityType          i_enTy,
                                        std::size_t           i_nEns,
                                        std::size_t  const  * i_enVe,
                                        double       const (* i_veCrds)[3],
                                        double              * o_inDia ) {
  // get the number of vertices
  unsigned short l_nEnVes = CE_N_VES( i_enTy );

  // check the size of the stack-memory below
  EDGE_V_CHECK_LE( l_nEnVes, 8 );

#ifdef PP_USE_OMP
#pragma omp parallel for
#endif
  for( std::size_t l_en = 0; l_en < i_nEns; l_en++ ) {
    // get vertex coordinates
    double l_veCrds[8][3] = {};
    getEnVeCrds( i_enTy,
                 i_enVe + (l_nEnVes * l_en),
                 i_veCrds,
                 l_veCrds );

    // compute diameter
    o_inDia[l_en] = Geom::inDiameter( i_enTy,
                                      l_veCrds );
  }
}

void edge_v::mesh::Mesh::setPeriodicBnds( t_entityType         i_elTy,
                                          std::size_t          i_nFas,
                                          std::size_t const  * i_faVe,
                                          double      const (* i_veCrds)[3],
                                          std::size_t        * io_faEl,
                                          std::size_t        * io_elFaEl ) {
  // get boundary faces
  std::vector< std::size_t > l_bndFas;
  for( std::size_t l_fa = 0; l_fa < i_nFas; l_fa++ ) {
    if( io_faEl[l_fa*2+1] == std::numeric_limits< std::size_t >::max() ) {
      l_bndFas.push_back( l_fa );
    }
  }
  EDGE_V_CHECK_EQ( l_bndFas.size()%2, 0 );

  unsigned short l_nDis = CE_N_DIS( i_elTy );
  unsigned short l_nFaVes = CE_N_VES( CE_T_FA( i_elTy ) );

  // lambda which derives the dimension,
  // where the number of matching vertex coordinates matches the requested limit
  auto l_eqDi = [ l_nDis, l_nFaVes, i_faVe, i_veCrds ]( unsigned short i_limit,
                                                        std::size_t    i_f0,
                                                        std::size_t    i_f1,
                                                        unsigned short l_ignDim = std::numeric_limits< unsigned short >::max() ) {
    for( unsigned short l_di = 0; l_di < l_nDis; l_di++) {
      if( l_di == l_ignDim ) continue;
      unsigned short l_nEq = 0;

      for( unsigned short l_ve0 = 0; l_ve0 < l_nFaVes; l_ve0++ ) {
        for( unsigned short l_ve1 = 0; l_ve1 < l_nFaVes; l_ve1++ ) {
          std::size_t l_ve0Id = i_faVe[i_f0*l_nFaVes + l_ve0];
          std::size_t l_ve1Id = i_faVe[i_f1*l_nFaVes + l_ve1];

          double l_diff = i_veCrds[l_ve0Id][l_di] - i_veCrds[l_ve1Id][l_di];
          if( std::abs(l_diff) < 1E-5 ) l_nEq++;
        }
      }

      // if the number of vertices sharing coordinates in a dimension matches the request, we are done
      if( l_nEq == i_limit ) return l_di;
    }
    return std::numeric_limits< unsigned short >::max();
  };

  // 1) iterate over all face-pairs,
  // 2) for each face, determine the constant dimension (assumption for our periodic boundaries)
  // 3) if the two faces have the same constant dim, check if the faces' vertices share coordinates in another dim
  std::vector< std::size_t > l_faPairs;
  for( std::size_t l_f0 = 0; l_f0 < l_bndFas.size(); l_f0++ ) {
    unsigned short l_constDim0 = l_eqDi( l_nFaVes*l_nFaVes, l_bndFas[l_f0], l_bndFas[l_f0] );
    for( std::size_t l_f1 = 0; l_f1 < l_bndFas.size(); l_f1++ ) {
      unsigned short l_constDim1 = l_eqDi( l_nFaVes*l_nFaVes, l_bndFas[l_f1], l_bndFas[l_f1] );

      if( l_constDim0 == l_constDim1 ) {
        unsigned short l_sharedDim = l_eqDi( l_nFaVes, l_bndFas[l_f0], l_bndFas[l_f1], l_constDim0 );

        if(    l_f0 != l_f1
            && l_sharedDim < std::numeric_limits< unsigned short >::max() ) l_faPairs.push_back( l_f1 );
      }
    }
  }

  // check that we got a partner for every face
  EDGE_V_CHECK_EQ( l_faPairs.size(), l_bndFas.size() );
  for( std::size_t l_fa = 0; l_fa < l_faPairs.size(); l_fa++ ) {
    EDGE_V_CHECK_EQ( l_faPairs[ l_faPairs[l_fa] ], l_fa );
  }

  // insert the missing elements
  unsigned short l_nElFas = CE_N_FAS( i_elTy );
  for( std::size_t l_f0 = 0; l_f0 < l_faPairs.size(); l_f0++ ) {
    std::size_t l_f1 = l_faPairs[l_f0];
    std::size_t l_el0 = io_faEl[l_f0*2 + 0];
    std::size_t l_el1 = io_faEl[l_f1*2 + 0];
    io_faEl[l_f0*2 + 1] = l_el1;

    for( unsigned short l_ad = 0; l_ad < l_nElFas; l_ad++ ) {
      if( io_elFaEl[l_el0*l_nElFas + l_ad] == std::numeric_limits< std::size_t >::max() ) {
        io_elFaEl[l_el0*l_nElFas + l_ad] = l_el1;
      }
    }
  }
}

void edge_v::mesh::Mesh::normOrder( t_entityType         i_elTy,
                                    std::size_t          i_nFas,
                                    std::size_t          i_nEls,
                                    double      const (* i_veCrds)[3],
                                    std::size_t        * io_faVe,
                                    std::size_t        * io_faEl,
                                    std::size_t        * io_elVe,
                                    std::size_t        * io_elFa,
                                    std::size_t        * io_elFaEl ) {
  unsigned short l_nFaVes = CE_N_VES( CE_T_FA( i_elTy ) );
  unsigned short l_nElVes = CE_N_VES( i_elTy );
  unsigned short l_nElFas = CE_N_FAS( i_elTy );

#ifdef PP_USE_OMP
#pragma omp parallel for
#endif
  for( std::size_t l_fa = 0; l_fa < i_nFas; l_fa++ ) {
    std::sort( io_faVe+l_fa*l_nFaVes, io_faVe+(l_fa+1)*l_nFaVes );
    std::sort( io_faEl+l_fa*2,        io_faEl+(l_fa+1)*2 );
  }

#ifdef PP_USE_OMP
#pragma omp parallel for
#endif
  for( std::size_t l_el = 0; l_el < i_nEls; l_el++ ) {
    std::sort( io_elVe+l_el*l_nElVes, io_elVe+(l_el+1)*l_nElVes );

    // lambda, which compares two faces lexicographically
    auto l_faLess = [ io_faVe, l_nFaVes ]( std::size_t i_faId0,
                                           std::size_t i_faId1 ) {
      // get the vertex ids of the faces
      std::size_t const * l_faVe0 = io_faVe + i_faId0*l_nFaVes;
      std::size_t const * l_faVe1 = io_faVe + i_faId1*l_nFaVes;

      // iterate over the vertices and compare their ids
      for( unsigned short l_ve = 0; l_ve < l_nFaVes; l_ve++ ) {
        // abort only for non-equal
        if( l_faVe0[l_ve] < l_faVe1[l_ve] )
          return true;
        else if( l_faVe0[l_ve] > l_faVe1[l_ve] )
          return false;
      }

      // equal would be valid but not for the use-case below
      EDGE_V_LOG_FATAL;
      return false;
    };

    // sort the faces
    for( unsigned short l_f0 = 0; l_f0 < l_nElFas; l_f0++ ) {
      for( unsigned short l_f1 = l_f0+1; l_f1 < l_nElFas; l_f1++ ) {
        // get the ids of the two faces
        std::size_t l_faId0 = io_elFa[l_el*l_nElFas + l_f0];
        std::size_t l_faId1 = io_elFa[l_el*l_nElFas + l_f1];

        // check if the id of face at position f1 is lexicographically less than the one at f0
        bool l_less = l_faLess( l_faId1, l_faId0 );

        // move faces around if not in order
        if( l_less ) {
          io_elFa[l_el*l_nElFas + l_f0] = l_faId1;
          io_elFa[l_el*l_nElFas + l_f1] = l_faId0;

          std::size_t l_elId0 = io_elFaEl[l_el*l_nElFas + l_f0];
          std::size_t l_elId1 = io_elFaEl[l_el*l_nElFas + l_f1];

          io_elFaEl[l_el*l_nElFas + l_f0] = l_elId1;
          io_elFaEl[l_el*l_nElFas + l_f1] = l_elId0;
        }
      }
    }
  }

  for( std::size_t l_el = 0; l_el < i_nEls; l_el++ ) {
    // get vertex coordinates
    double l_veCrds[8][3] = {};
    getEnVeCrds( i_elTy,
                 io_elVe + (l_nElVes * l_el),
                 i_veCrds,
                 l_veCrds );

    Geom::normVesFas( i_elTy,
                      l_veCrds,
                      io_elVe+l_el*l_nElVes,
                      io_elFa+l_el*l_nElFas,
                      io_elFaEl+l_el*l_nElFas );
  }
}

edge_v::mesh::Mesh::Mesh( edge_v::io::Moab const & i_moab,
                          bool                     i_periodic ) {
  // get the element type of the mesh
  m_elTy = i_moab.getElType();
  t_entityType l_faTy = CE_T_FA( m_elTy );

  // derive element properties
  unsigned short l_nElVes = CE_N_VES( m_elTy );
  unsigned short l_nElFas = CE_N_FAS( m_elTy );

  // query mesh
  m_nVes = i_moab.nEnsByType( POINT  );
  m_nFas = i_moab.nEnsByType( l_faTy );
  m_nEls = i_moab.nEnsByType( m_elTy );

  // allocate memory
  std::size_t l_size  = m_nVes * 3;
  m_veCrds = (double (*)[3]) new double[ l_size ];

  l_size = m_nFas * CE_N_VES( l_faTy );
  m_faVe = new std::size_t[ l_size ];

  l_size = m_nFas * 2;
  m_faEl = new std::size_t[ l_size ];

  l_size = m_nEls * l_nElFas;
  m_elFa = new std::size_t[ l_size ];

  l_size  = m_nEls * l_nElVes;
  m_elVe = new std::size_t[ l_size ];

  l_size  = m_nEls;
  m_inDiaEl = new double[ l_size ];

  l_size = m_nEls * l_nElFas;
  m_elFaEl = new std::size_t[ l_size ];

  // query moab
  i_moab.getVeCrds( m_veCrds );
  i_moab.getEnVe( l_faTy, m_faVe );
  i_moab.getFaEl( m_elTy, m_faEl );
  i_moab.getEnVe( m_elTy, m_elVe );
  i_moab.getElFa( m_elTy, m_elFa );
  i_moab.getElFaEl( m_elTy, m_elFaEl );

  // adjust periodic boundaries
  if( i_periodic ) {
    setPeriodicBnds( m_elTy,
                     m_nFas,
                     m_faVe,
                     m_veCrds,
                     m_faEl,
                     m_elFaEl );
  }

  // compute mesh properties
  setInDiameter( m_elTy,
                 m_nEls,
                 m_elVe,
                 m_veCrds,
                 m_inDiaEl );

  normOrder( m_elTy,
             m_nFas,
             m_nEls,
             m_veCrds,
             m_faVe,
             m_faEl,
             m_elVe,
             m_elFa,
             m_elFaEl );
}

edge_v::mesh::Mesh::~Mesh() {
  delete[] m_veCrds;
  delete[] m_faVe;
  delete[] m_faEl;
  delete[] m_elVe;
  delete[] m_elFa;
  delete[] m_elFaEl;
  delete[] m_inDiaEl;
}

void edge_v::mesh::Mesh::printStats() const {
  EDGE_V_LOG_INFO << "mesh stats:";
  EDGE_V_LOG_INFO << "  #vertices: " << m_nVes;
  EDGE_V_LOG_INFO << "  #faces:    " << m_nFas;
  EDGE_V_LOG_INFO << "  #elements: " << m_nEls;
}