/**
 * @file This file is part of EDGE.
 *
 * @author Alexander Breuer (breuer AT mytum.de)
 *
 * @section LICENSE
 * Copyright (c) 2020, Alexander Breuer
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
 * Velocity-based mesh refinement.
 **/
#ifndef EDGE_V_MESH_REFINEMENT_H
#define EDGE_V_MESH_REFINEMENT_H

#include <string>
#include "models/Model.hpp"

namespace edge_v {
  namespace mesh {
    class Refinement;
  }
}

/**
 * Mesh refinement.
 **/
class edge_v::mesh::Refinement {
  private:
    //! evaluated mesh refinement
    float * m_ref = nullptr;

    /**
     * Frees the memory.
     **/
    void free();

  public:
    /**
     * Destructor.
     **/
    ~Refinement();

    /**
     * Inits the mesh refinement by computing the target refinement at all given points.
     *
     * @param i_nPts number of points.
     * @param i_pts coordinates of the points.
     * @param i_refExpr expression used for the refinement, which defines 'frequency' and 'elements_per_wave_length'.
     * @param i_velMod velocity model.
     **/
    void init( std::size_t            i_nPts,
               double        const (* i_pts)[3],
               std::string   const  & i_refExpr,
               models::Model const  & i_velMod );
};

#endif