!
!  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
!  SLEPc - Scalable Library for Eigenvalue Problem Computations
!  Copyright (c) 2002-2019, Universitat Politecnica de Valencia, Spain
!
!  This file is part of SLEPc.
!  SLEPc is distributed under a 2-clause BSD license (see LICENSE).
!  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
!
!  Include file for Fortran use of the MFN object in SLEPc
!
#include "slepc/finclude/slepcsys.h"
#include "slepc/finclude/slepcmfn.h"

      type tMFN
        PetscFortranAddr:: v PETSC_FORTRAN_TYPE_INITIALIZE
      end type tMFN

      MFN, parameter :: SLEPC_NULL_MFN = tMFN(0)

      PetscEnum MFN_CONVERGED_TOL
      PetscEnum MFN_CONVERGED_ITS
      PetscEnum MFN_DIVERGED_ITS
      PetscEnum MFN_DIVERGED_BREAKDOWN
      PetscEnum MFN_CONVERGED_ITERATING

      parameter (MFN_CONVERGED_TOL          =  1)
      parameter (MFN_CONVERGED_ITS          =  2)
      parameter (MFN_DIVERGED_ITS           = -1)
      parameter (MFN_DIVERGED_BREAKDOWN     = -2)
      parameter (MFN_CONVERGED_ITERATING    =  0)

!
!   Possible arguments to MFNMonitorSet()
!
      external MFNMONITORDEFAULT
      external MFNMONITORLG

!
!  End of Fortran include file for the MFN package in SLEPc
!