# Base image from which to build is taken from Docker official repository for
# Ubuntu. We pick the image for Ubuntu 16.04 Xenial which is a LTS version.
FROM ubuntu:22.04

MAINTAINER AGUSTIN ABARCA <aabarca@ncsu.edu>

# Here we define a variable for the number of cores to use when building this
# image. A default value is provided but you can change it by passing the
# ``--build-arg NPROC=<value>`` flag to the ``docker build`` command.
ARG NPROC=8

# Use the package management system as much as possible.
RUN apt-get update && apt-get install -y \
        vim \
        build-essential \
        gfortran \
        mpich \
        cmake \
        git \
        curl \
        python-dev \
        python-numpy \
        perl \
        libx11-dev \
        libxext-dev \
        m4 \
        zlib1g-dev \
        libcurl4-gnutls-dev \
        && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

# Set CTF development environment.
ENV CTF_TPL_INSTALL_DIR=/tools/ctf/gcc-11.3.0/tpls/opt \
    CTF_SCRATCH_DIR=/scratch \
    CTF_BUILD_DIR=/scratch/ctf_build \
    CTF_DIR=/scratch/ctf-Source.X.Y.Z \
    CTF_INSTALL_DIR=/tools/ctf_installs/YYYY-MM-DD

# Install the CTF TPLs.
RUN mkdir -p ${CTF_SCRATCH_DIR} && \
    cd ${CTF_SCRATCH_DIR} && \
    git clone https://github.com/CASL/ctf_tpls && \
    mkdir -p ${CTF_SCRATCH_DIR}/tpl_build && \
    cd ${CTF_SCRATCH_DIR}/tpl_build && \
    cmake \
        -D CMAKE_INSTALL_PREFIX=${CTF_TPL_INSTALL_DIR} \
        -D CMAKE_BUILD_TYPE=Release \
        -D CMAKE_CXX_COMPILER=mpic++ \
        -D CMAKE_C_COMPILER=mpicc \
        -D CMAKE_Fortran_COMPILER=mpif90 \
        -D FFLAGS="-fPIC -O3" \
        -D CFLAGS="-fPIC -O3" \
        -D CXXFLAGS="-fPIC -O3" \
        -D LDFLAGS="" \
        -D ENABLE_SHARED=ON \
        -D PROCS_INSTALL=${NPROC} \
        ${CTF_SCRATCH_DIR}/ctf_tpls/TPL_build && \
    make && \
    rm -rf ${CTF_SCRATCH_DIR}

# Copy the ``do-configure.sh`` script into the CTF build directory.
COPY do-configure.sh ${CTF_BUILD_DIR}/
