!  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
!  SLEPc - Scalable Library for Eigenvalue Problem Computations
!  Copyright (c) 2002-2014, Universitat Politecnica de Valencia, Spain
!
!  This file is part of SLEPc.
!     
!  SLEPc is free software: you can redistribute it and/or modify it under  the
!  terms of version 3 of the GNU Lesser General Public License as published by
!  the Free Software Foundation.
!
!  SLEPc  is  distributed in the hope that it will be useful, but WITHOUT  ANY 
!  WARRANTY;  without even the implied warranty of MERCHANTABILITY or  FITNESS 
!  FOR  A  PARTICULAR PURPOSE. See the GNU Lesser General Public  License  for 
!  more details.
!
!  You  should have received a copy of the GNU Lesser General  Public  License
!  along with SLEPc. If not, see <http://www.gnu.org/licenses/>.
!  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
!
!  Program usage: mpirun -np n ex15f [-help] [-n <n>] [-mu <mu>] [all SLEPc options] 
!
!  Description: Singular value decomposition of the Lauchli matrix. 
!
!  The command line options are:
!    -n <n>, where <n> = matrix dimension.
!    -mu <mu>, where <mu> = subdiagonal value.
!
! ---------------------------------------------------------------------- 
!
      program main
      implicit none

#include <finclude/petscsys.h>
#include <finclude/petscvec.h>
#include <finclude/petscmat.h>
#include <finclude/slepcsys.h>
#include <finclude/slepcsvd.h>

! - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
!     Declarations
! - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
!
!  Variables:
!     A     operator matrix
!     svd   singular value solver context

      Mat            A
      SVD            svd
      SVDType        tname
      PetscReal      tol, error, sigma, mu
      PetscInt       n, i, j, Istart, Iend
      PetscInt       nsv, maxit, its, nconv
      PetscMPIInt    rank
      PetscErrorCode ierr
      PetscBool      flg
      PetscScalar    one

! - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
!     Beginning of program
! - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 

      call SlepcInitialize(PETSC_NULL_CHARACTER,ierr)
      call MPI_Comm_rank(PETSC_COMM_WORLD,rank,ierr)
      n = 100
      call PetscOptionsGetInt(PETSC_NULL_CHARACTER,'-n',n,flg,ierr)
      mu = PETSC_SQRT_MACHINE_EPSILON
      call PetscOptionsGetReal(PETSC_NULL_CHARACTER,'-mu',mu,flg,ierr)

      if (rank .eq. 0) then
        write(*,100) n, mu
      endif
 100  format (/'Lauchli SVD, n =',I3,', mu=',E12.4,' (Fortran)')

! - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
!     Build the Lauchli matrix
! - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 

      call MatCreate(PETSC_COMM_WORLD,A,ierr)
      call MatSetSizes(A,PETSC_DECIDE,PETSC_DECIDE,n+1,n,ierr)
      call MatSetFromOptions(A,ierr)
      call MatSetUp(A,ierr)

      call MatGetOwnershipRange(A,Istart,Iend,ierr)
      one = 1.0
      do i=Istart,Iend-1
        if (i .eq. 0) then
          do j=0,n-1
            call MatSetValue(A,i,j,one,INSERT_VALUES,ierr)
          end do
        else
          call MatSetValue(A,i,i-1,mu,INSERT_VALUES,ierr)
        end if
      enddo

      call MatAssemblyBegin(A,MAT_FINAL_ASSEMBLY,ierr)
      call MatAssemblyEnd(A,MAT_FINAL_ASSEMBLY,ierr)

! - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
!     Create the singular value solver and display info
! - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 

!     ** Create singular value solver context
      call SVDCreate(PETSC_COMM_WORLD,svd,ierr)

!     ** Set operator
      call SVDSetOperator(svd,A,ierr)

!     ** Use thick-restart Lanczos as default solver
      call SVDSetType(svd,SVDTRLANCZOS,ierr)

!     ** Set solver parameters at runtime
      call SVDSetFromOptions(svd,ierr)

! - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
!     Solve the singular value system
! - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 

      call SVDSolve(svd,ierr) 
      call SVDGetIterationNumber(svd,its,ierr)
      if (rank .eq. 0) then
        write(*,110) its
      endif
 110  format (/' Number of iterations of the method:',I4)

!     ** Optional: Get some information from the solver and display it
      call SVDGetType(svd,tname,ierr)
      if (rank .eq. 0) then
        write(*,120) tname
      endif
 120  format (' Solution method: ',A)
      call SVDGetDimensions(svd,nsv,PETSC_NULL_INTEGER,                 &
     &                      PETSC_NULL_INTEGER,ierr)
      if (rank .eq. 0) then
        write(*,130) nsv
      endif
 130  format (' Number of requested singular values:',I2)
      call SVDGetTolerances(svd,tol,maxit,ierr)
      if (rank .eq. 0) then
        write(*,140) tol, maxit
      endif
 140  format (' Stopping condition: tol=',1P,E10.4,', maxit=',I4)

! - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
!     Display solution and clean up
! - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 

!     ** Get number of converged singular triplets
      call SVDGetConverged(svd,nconv,ierr)
      if (rank .eq. 0) then
        write(*,150) nconv
      endif
 150  format (' Number of converged approximate singular triplets:',I2/)

!     ** Display singular values and relative errors
      if (nconv.gt.0) then
        if (rank .eq. 0) then
          write(*,*) '       sigma          relative error'
          write(*,*) ' ----------------- ------------------'
        endif
        do i=0,nconv-1
!         ** Get converged singular triplet: i-th singular value is stored in sigma
          call SVDGetSingularTriplet(svd,i,sigma,PETSC_NULL_OBJECT,     &
     &         PETSC_NULL_OBJECT,ierr)

!         ** Compute the relative error associated to each eigenpair
          call SVDComputeRelativeError(svd,i,error,ierr)
          if (rank .eq. 0) then
            write(*,160) sigma, error
          endif
 160      format (1P,'   ',E12.4,'       ',E12.4)

        enddo
        if (rank .eq. 0) then
          write(*,*)
        endif
      endif

!     ** Free work space
      call SVDDestroy(svd,ierr)
      call MatDestroy(A,ierr)

      call SlepcFinalize(ierr)
      end

