#
#  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#  SLEPc - Scalable Library for Eigenvalue Problem Computations
#  Copyright (c) 2002-2019, Universitat Politecnica de Valencia, Spain
#
#  This file is part of SLEPc.
#  SLEPc is distributed under a 2-clause BSD license (see LICENSE).
#  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#

ALL: lib

#requiresdefine  'SLEPC_HAVE_PRIMME'

CFLAGS   = ${PRIMME_FLAGS}
FFLAGS   =
SOURCEC  = svdprimme.c
SOURCEF  =
SOURCEH  =
LIBBASE  = libslepcsvd
DIRS     =
MANSEC   = SVD
LOCDIR   = src/svd/impls/external/primme/

include ${SLEPC_DIR}/lib/slepc/conf/slepc_common


