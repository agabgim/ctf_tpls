#!/usr/bin/env bash

EXTRA_ARGS=$@

cmake \
    -D CMAKE_INSTALL_PREFIX=${CTF_INSTALL_DIR} \
    -D CMAKE_BUILD_TYPE=Release \
    -D CMAKE_CXX_COMPILER=mpicxx \
    -D CMAKE_C_COMPILER=mpicc \
    -D CMAKE_Fortran_COMPILER=mpifort \
    -D BUILD_SHARED_LIBS=ON \
    -D TPL_ENABLE_MPI=ON \
    -D TPL_INSTALL_DIR=${CTF_TPL_INSTALL_DIR} \
    -D CTF_ENABLE_ALL_PACKAGES=ON \
    -D CTF_ENABLE_TESTS=ON \
    -D CTF_CONFIGURE_OPTIONS_FILE=${CTF_DIR}/cmake/std/gcc-8.3.0/ctf-tpls-std.cmake \
    -D MPI_EXEC=/usr/bin/mpiexec.mpich \
    ${EXTRA_ARGS} \
    ${CTF_DIR}
