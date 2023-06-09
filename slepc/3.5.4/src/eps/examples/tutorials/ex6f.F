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
!  Program usage: mpirun -np n ex6f [-help] [-m <m>] [all SLEPc options] 
!
!  Description: This example solves the eigensystem arising in the Ising
!  model for ferromagnetic materials. The file mvmisg.f must be linked
!  together. Information about the model can be found at the following 
!  site http://math.nist.gov/MatrixMarket/data/NEP
!
!  The command line options are:
!    -m <m>, where <m> is the number of 2x2 blocks, i.e. matrix size N=2*m
!
! ---------------------------------------------------------------------- 
!
      program main
      implicit none

#include <finclude/petscsys.h>
#include <finclude/petscvec.h>
#include <finclude/petscmat.h>
#include <finclude/slepcsys.h>
#include <finclude/slepceps.h>

! - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
!     Declarations
! - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
!
!  Variables:
!     A     operator matrix
!     eps   eigenproblem solver context

      Mat            A
      EPS            eps
      EPSType        tname
      PetscReal      tol
      PetscInt       N, m
      PetscInt       nev, maxit, its
      PetscMPIInt    sz, rank
      PetscErrorCode ierr
      PetscBool      flg

!     This is the routine to use for matrix-free approach
!
      external MatIsing_Mult

! - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
!     Beginning of program
! - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 

      call SlepcInitialize(PETSC_NULL_CHARACTER,ierr)
#if defined(PETSC_USE_COMPLEX)
      write(*,*) 'This example requires real numbers.'
      goto 999
#endif
      call MPI_Comm_size(PETSC_COMM_WORLD,sz,ierr)
      call MPI_Comm_rank(PETSC_COMM_WORLD,rank,ierr)
      if (sz .ne. 1) then
         if (rank .eq. 0) then
            write(*,*) 'This is a uniprocessor example only!'
         endif
         SETERRQ(PETSC_COMM_WORLD,1,' ',ierr)
      endif
      m = 30
      call PetscOptionsGetInt(PETSC_NULL_CHARACTER,'-m',m,flg,ierr)
      N = 2*m

      if (rank .eq. 0) then
        write(*,*)
        write(*,'(A,I6,A)') 'Ising Model Eigenproblem, m=',m,', (N=2*m)'
        write(*,*)
      endif

! - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
!     Register the matrix-vector subroutine for the operator that defines
!     the eigensystem, Ax=kx
! - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 

      call MatCreateShell(PETSC_COMM_WORLD,N,N,N,N,PETSC_NULL_OBJECT,   &
     &                    A,ierr)
      call MatShellSetOperation(A,MATOP_MULT,MatIsing_Mult,ierr)

! - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
!     Create the eigensolver and display info
! - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 

!     ** Create eigensolver context
      call EPSCreate(PETSC_COMM_WORLD,eps,ierr)

!     ** Set operators. In this case, it is a standard eigenvalue problem
      call EPSSetOperators(eps,A,PETSC_NULL_OBJECT,ierr)
      call EPSSetProblemType(eps,EPS_NHEP,ierr)

!     ** Set solver parameters at runtime
      call EPSSetFromOptions(eps,ierr)

! - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
!     Solve the eigensystem
! - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 

      call EPSSolve(eps,ierr) 
      call EPSGetIterationNumber(eps,its,ierr)
      if (rank .eq. 0) then
        write(*,'(A,I4)') ' Number of iterations of the method: ', its
      endif

!     ** Optional: Get some information from the solver and display it
      call EPSGetType(eps,tname,ierr)
      if (rank .eq. 0) then
        write(*,'(A,A)') ' Solution method: ', tname
      endif
      call EPSGetDimensions(eps,nev,PETSC_NULL_INTEGER,                 &
     &                      PETSC_NULL_INTEGER,ierr)
      if (rank .eq. 0) then
        write(*,'(A,I2)') ' Number of requested eigenvalues:', nev
      endif
      call EPSGetTolerances(eps,tol,maxit,ierr)
      if (rank .eq. 0) then
        write(*,'(A,1PE10.4,A,I6)') ' Stopping condition: tol=', tol,   &
     &                              ', maxit=', maxit
      endif

! - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
!     Display solution and clean up
! - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 

      call EPSPrintSolution(eps,PETSC_NULL_OBJECT,ierr)
      call EPSDestroy(eps,ierr)
      call MatDestroy(A,ierr)

#if defined(PETSC_USE_COMPLEX)
 999  continue
#endif
      call SlepcFinalize(ierr)
      end

! ------------------------------------------------------------------- 
! 
!   MatIsing_Mult - user provided matrix-vector multiply 
!
!   Input Parameters:
!   A - matrix
!   x - input vector
!
!   Output Parameter:
!   y - output vector
! 
      subroutine MatIsing_Mult(A,x,y,ierr)
      implicit none

#include <finclude/petscsys.h>
#include <finclude/petscvec.h>
#include <finclude/petscmat.h>

      Mat            A
      Vec            x,y
      PetscInt       trans,one,N
      PetscScalar    x_array(1),y_array(1)
      PetscOffset    i_x,i_y
      PetscErrorCode ierr

!     The actual routine for the matrix-vector product
      external mvmisg

      call MatGetSize(A,N,PETSC_NULL_INTEGER,ierr)
      call VecGetArray(x,x_array,i_x,ierr)
      call VecGetArray(y,y_array,i_y,ierr)

      trans = 0
      one = 1
      call mvmisg(trans,N,one,x_array(i_x+1),N,y_array(i_y+1),N)

      call VecRestoreArray(x,x_array,i_x,ierr)
      call VecRestoreArray(y,y_array,i_y,ierr)

      return
      end

! ------------------------------------------------------------------- 
!     The actual routine for the matrix-vector product
!     See http://math.nist.gov/MatrixMarket/data/NEP/mvmisg/mvmisg.html

      SUBROUTINE MVMISG( TRANS, N, M, X, LDX, Y, LDY )
!     ..
!     .. Scalar Arguments ..
      PetscInt     LDY, LDX, M, N, TRANS
!     ..
!     .. Array Arguments ..
      PetscScalar  Y( LDY, * ), X( LDX, * )
!     ..
!
!  Purpose
!  =======
!
!  Compute
!
!               Y(:,1:M) = op(A)*X(:,1:M)
!
!  where op(A) is A or A' (the transpose of A). The A is the Ising 
!  matrix.
!
!  Arguments
!  =========
!
!  TRANS   (input) INTEGER
!          If TRANS = 0, compute Y(:,1:M) = A*X(:,1:M) 
!          If TRANS = 1, compute Y(:,1:M) = A'*X(:,1:M) 
!           
!  N       (input) INTEGER
!          The order of the matrix A. N has to be an even number.
!
!  M       (input) INTEGER
!          The number of columns of X to multiply.
!
!  X       (input) DOUBLE PRECISION array, dimension ( LDX, M )
!          X contains the matrix (vectors) X.
!
!  LDX     (input) INTEGER
!          The leading dimension of array X, LDX >= max( 1, N )
!
!  Y       (output) DOUBLE PRECISION array, dimension (LDX, M )
!          contains the product of the matrix op(A) with X.
!
!  LDY     (input) INTEGER
!          The leading dimension of array Y, LDY >= max( 1, N )
!
!  ===================================================================
!
!
!     .. PARAMETERS ..
      PetscReal   PI 
      PARAMETER   ( PI = 3.141592653589793D+00 )
      PetscReal   ALPHA, BETA
      PARAMETER   ( ALPHA = PI/4, BETA = PI/4 ) 
!
!     .. Local Variables .. 
      PetscInt    I, K 
      PetscReal   COSA, COSB, SINA
      PetscReal   SINB, TEMP, TEMP1 
!
!     .. Intrinsic functions ..
      INTRINSIC   COS, SIN 
!
      COSA = COS( ALPHA ) 
      SINA = SIN( ALPHA ) 
      COSB = COS( BETA )
      SINB = SIN( BETA ) 
!      
      IF ( TRANS.EQ.0 ) THEN 
!
!     Compute Y(:,1:M) = A*X(:,1:M)

         DO 30 K = 1, M
!
            Y( 1, K ) = COSB*X( 1, K ) - SINB*X( N, K ) 
            DO 10 I = 2, N-1, 2   
               Y( I, K )   =  COSB*X( I, K ) + SINB*X( I+1, K )
               Y( I+1, K ) = -SINB*X( I, K ) + COSB*X( I+1, K )  
   10       CONTINUE
            Y( N, K ) = SINB*X( 1, K ) + COSB*X( N, K ) 
!
            DO 20 I = 1, N, 2
               TEMP        =  COSA*Y( I, K ) + SINA*Y( I+1, K )
               Y( I+1, K ) = -SINA*Y( I, K ) + COSA*Y( I+1, K )  
               Y( I, K )   = TEMP 
   20       CONTINUE  
!
   30    CONTINUE 
!
      ELSE IF ( TRANS.EQ.1 ) THEN 
!
!        Compute Y(:1:M) = A'*X(:,1:M) 
!
         DO 60 K = 1, M 
!
            DO 40 I = 1, N, 2
               Y( I, K )   =  COSA*X( I, K ) - SINA*X( I+1, K )
               Y( I+1, K ) =  SINA*X( I, K ) + COSA*X( I+1, K )  
   40       CONTINUE  
            TEMP  = COSB*Y(1,K) + SINB*Y(N,K) 
            DO 50 I = 2, N-1, 2   
               TEMP1       =  COSB*Y( I, K ) - SINB*Y( I+1, K )
               Y( I+1, K ) =  SINB*Y( I, K ) + COSB*Y( I+1, K )  
               Y( I, K )   =  TEMP1
   50       CONTINUE
            Y( N, K ) = -SINB*Y( 1, K ) + COSB*Y( N, K ) 
            Y( 1, K ) = TEMP 
!
   60    CONTINUE
!
      END IF 
!
      RETURN
!  
!     END OF MVMISG
      END 
