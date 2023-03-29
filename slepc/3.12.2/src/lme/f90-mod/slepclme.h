!
!  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
!  SLEPc - Scalable Library for Eigenvalue Problem Computations
!  Copyright (c) 2002-2019, Universitat Politecnica de Valencia, Spain
!
!  This file is part of SLEPc.
!  SLEPc is distributed under a 2-clause BSD license (see LICENSE).
!  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
!
!  Include file for Fortran use of the LME object in SLEPc
!
#include "slepc/finclude/slepcsys.h"
#include "slepc/finclude/slepclme.h"

      type tLME
        PetscFortranAddr:: v PETSC_FORTRAN_TYPE_INITIALIZE
      end type tLME

      LME, parameter :: SLEPC_NULL_LME = tLME(0)

      PetscEnum LME_CONVERGED_TOL
      PetscEnum LME_DIVERGED_ITS
      PetscEnum LME_DIVERGED_BREAKDOWN
      PetscEnum LME_CONVERGED_ITERATING

      parameter (LME_CONVERGED_TOL          =  1)
      parameter (LME_DIVERGED_ITS           = -1)
      parameter (LME_DIVERGED_BREAKDOWN     = -2)
      parameter (LME_CONVERGED_ITERATING    =  0)

      PetscEnum LME_LYAPUNOV
      PetscEnum LME_SYLVESTER
      PetscEnum LME_GEN_LYAPUNOV
      PetscEnum LME_GEN_SYLVESTER
      PetscEnum LME_DT_LYAPUNOV
      PetscEnum LME_STEIN

      parameter (LME_LYAPUNOV               =  0)
      parameter (LME_SYLVESTER              =  1)
      parameter (LME_GEN_LYAPUNOV           =  2)
      parameter (LME_GEN_SYLVESTER          =  3)
      parameter (LME_DT_LYAPUNOV            =  4)
      parameter (LME_STEIN                  =  5)

!
!   Possible arguments to LMEMonitorSet()
!
      external LMEMONITORDEFAULT
      external LMEMONITORLG

!
!  End of Fortran include file for the LME package in SLEPc
!