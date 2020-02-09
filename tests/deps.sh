#!/bin/bash
##
# @file This file is part of EDGE.
#
# @author Alexander Breuer (breuer AT mytum.de)
#
# @section LICENSE
# Copyright (c) 2020, Alexander Breuer
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
#
# 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# @section DESCRIPTION
# Conditionally builds EDGE's dependencies.
##
[ -d "$(pwd)/deps" ] && echo "deps-dir exists, exiting.." && exit
echo "--- LIBXSMM"
./tools/build/deps/libxsmm.sh -i $(pwd)/submodules/libxsmm -o $(pwd)/deps -j ${EDGE_N_BUILD_PROCS}
echo "--- zlib"
./tools/build/deps/zlib.sh -o $(pwd)/deps -j ${EDGE_N_BUILD_PROCS}
echo "--- HDF5"
./tools/build/deps/hdf5.sh -z $(pwd)/deps -o $(pwd)/deps -j ${EDGE_N_BUILD_PROCS}
echo "--- MOAB"
./tools/build/deps/moab.sh -d -z $(pwd)/deps -5 $(pwd)/deps -e $(pwd)/submodules/eigen -i $(pwd)/submodules/moab -o $(pwd)/deps -j ${EDGE_N_BUILD_PROCS}
echo "--- METIS"
./tools/build/deps/metis.sh -o $(pwd)/deps -j ${EDGE_N_BUILD_PROCS}

echo "--- EDGE-V"
cd tools/edge_v
scons parallel=none zlib=../../deps hdf5=../../deps moab=../../deps metis=../../deps install_dir=../../deps/edge_v_none -j ${EDGE_N_BUILD_PROCS}
scons parallel=omp zlib=../../deps hdf5=../../deps moab=../../deps metis=../../deps install_dir=../../deps/edge_v_omp -j ${EDGE_N_BUILD_PROCS}
cd ../..