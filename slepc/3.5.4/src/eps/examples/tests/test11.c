/*
   - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
   SLEPc - Scalable Library for Eigenvalue Problem Computations
   Copyright (c) 2002-2014, Universitat Politecnica de Valencia, Spain

   This file is part of SLEPc.

   SLEPc is free software: you can redistribute it and/or modify it under  the
   terms of version 3 of the GNU Lesser General Public License as published by
   the Free Software Foundation.

   SLEPc  is  distributed in the hope that it will be useful, but WITHOUT  ANY
   WARRANTY;  without even the implied warranty of MERCHANTABILITY or  FITNESS
   FOR  A  PARTICULAR PURPOSE. See the GNU Lesser General Public  License  for
   more details.

   You  should have received a copy of the GNU Lesser General  Public  License
   along with SLEPc. If not, see <http://www.gnu.org/licenses/>.
   - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
*/

static char help[] = "Solves the same problem as in ex5, but with a user-defined sorting criterion."
  "It is a standard nonsymmetric eigenproblem with real eigenvalues and the rightmost eigenvalue is known to be 1.\n"
  "This example illustrates how the user can set a custom spectrum selection.\n\n"
  "The command line options are:\n"
  "  -m <m>, where <m> = number of grid subdivisions in each dimension.\n\n";

#include <slepceps.h>

/*
   User-defined routines
*/

PetscErrorCode MyEigenSort(PetscScalar ar,PetscScalar ai,PetscScalar br,PetscScalar bi,PetscInt *r,void *ctx);
PetscErrorCode MatMarkovModel(PetscInt m,Mat A);

#undef __FUNCT__
#define __FUNCT__ "main"
int main(int argc,char **argv)
{
  Vec            v0;              /* initial vector */
  Mat            A;               /* operator matrix */
  EPS            eps;             /* eigenproblem solver context */
  ST             st;              /* spectral transformation associated */
  EPSType        type;
  PetscReal      tol=1000*PETSC_MACHINE_EPSILON;
  PetscScalar    target=0.5;
  PetscInt       N,m=15,nev;
  PetscErrorCode ierr;
  char           str[50];

  SlepcInitialize(&argc,&argv,(char*)0,help);

  ierr = PetscOptionsGetInt(NULL,"-m",&m,NULL);CHKERRQ(ierr);
  N = m*(m+1)/2;
  ierr = PetscPrintf(PETSC_COMM_WORLD,"\nMarkov Model, N=%D (m=%D)\n",N,m);CHKERRQ(ierr);
  ierr = PetscOptionsGetScalar(NULL,"-target",&target,NULL);CHKERRQ(ierr);
  ierr = SlepcSNPrintfScalar(str,50,target,PETSC_FALSE);CHKERRQ(ierr);
  ierr = PetscPrintf(PETSC_COMM_WORLD,"Searching closest eigenvalues to the right of %s.\n\n",str);CHKERRQ(ierr);

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     Compute the operator matrix that defines the eigensystem, Ax=kx
     - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

  ierr = MatCreate(PETSC_COMM_WORLD,&A);CHKERRQ(ierr);
  ierr = MatSetSizes(A,PETSC_DECIDE,PETSC_DECIDE,N,N);CHKERRQ(ierr);
  ierr = MatSetFromOptions(A);CHKERRQ(ierr);
  ierr = MatSetUp(A);CHKERRQ(ierr);
  ierr = MatMarkovModel(m,A);CHKERRQ(ierr);

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                Create the eigensolver and set various options
     - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

  /*
     Create eigensolver context
  */
  ierr = EPSCreate(PETSC_COMM_WORLD,&eps);CHKERRQ(ierr);

  /*
     Set operators. In this case, it is a standard eigenvalue problem
  */
  ierr = EPSSetOperators(eps,A,NULL);CHKERRQ(ierr);
  ierr = EPSSetProblemType(eps,EPS_NHEP);CHKERRQ(ierr);
  ierr = EPSSetTolerances(eps,tol,PETSC_DEFAULT);CHKERRQ(ierr);

  /*
     Set the custom comparing routine in order to obtain the eigenvalues
     closest to the target on the right only
  */
  ierr = EPSSetEigenvalueComparison(eps,MyEigenSort,&target);CHKERRQ(ierr);

  /*
     Set solver parameters at runtime
  */
  ierr = EPSSetFromOptions(eps);CHKERRQ(ierr);

  /*
     Set the preconditioner based on A - target * I
  */
  ierr = EPSGetST(eps,&st);CHKERRQ(ierr);
  ierr = STSetShift(st,target);CHKERRQ(ierr);

  /*
     Set the initial vector. This is optional, if not done the initial
     vector is set to random values
  */
  ierr = MatGetVecs(A,&v0,NULL);CHKERRQ(ierr);
  ierr = VecSet(v0,1.0);CHKERRQ(ierr);
  ierr = EPSSetInitialSpace(eps,1,&v0);CHKERRQ(ierr);

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                      Solve the eigensystem
     - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

  ierr = EPSSolve(eps);CHKERRQ(ierr);

  /*
     Optional: Get some information from the solver and display it
  */
  ierr = EPSGetType(eps,&type);CHKERRQ(ierr);
  ierr = PetscPrintf(PETSC_COMM_WORLD," Solution method: %s\n\n",type);CHKERRQ(ierr);
  ierr = EPSGetDimensions(eps,&nev,NULL,NULL);CHKERRQ(ierr);
  ierr = PetscPrintf(PETSC_COMM_WORLD," Number of requested eigenvalues: %D\n",nev);CHKERRQ(ierr);

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                    Display solution and clean up
     - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

  ierr = EPSPrintSolution(eps,NULL);CHKERRQ(ierr);
  ierr = EPSDestroy(&eps);CHKERRQ(ierr);
  ierr = MatDestroy(&A);CHKERRQ(ierr);
  ierr = VecDestroy(&v0);CHKERRQ(ierr);
  ierr = SlepcFinalize();
  return 0;
}

#undef __FUNCT__
#define __FUNCT__ "MatMarkovModel"
/*
    Matrix generator for a Markov model of a random walk on a triangular grid.

    This subroutine generates a test matrix that models a random walk on a
    triangular grid. This test example was used by G. W. Stewart ["{SRRIT} - a
    FORTRAN subroutine to calculate the dominant invariant subspaces of a real
    matrix", Tech. report. TR-514, University of Maryland (1978).] and in a few
    papers on eigenvalue problems by Y. Saad [see e.g. LAA, vol. 34, pp. 269-295
    (1980) ]. These matrices provide reasonably easy test problems for eigenvalue
    algorithms. The transpose of the matrix  is stochastic and so it is known
    that one is an exact eigenvalue. One seeks the eigenvector of the transpose
    associated with the eigenvalue unity. The problem is to calculate the steady
    state probability distribution of the system, which is the eigevector
    associated with the eigenvalue one and scaled in such a way that the sum all
    the components is equal to one.

    Note: the code will actually compute the transpose of the stochastic matrix
    that contains the transition probabilities.
*/
PetscErrorCode MatMarkovModel(PetscInt m,Mat A)
{
  const PetscReal cst = 0.5/(PetscReal)(m-1);
  PetscReal       pd,pu;
  PetscInt        Istart,Iend,i,j,jmax,ix=0;
  PetscErrorCode  ierr;

  PetscFunctionBeginUser;
  ierr = MatGetOwnershipRange(A,&Istart,&Iend);CHKERRQ(ierr);
  for (i=1;i<=m;i++) {
    jmax = m-i+1;
    for (j=1;j<=jmax;j++) {
      ix = ix + 1;
      if (ix-1<Istart || ix>Iend) continue;  /* compute only owned rows */
      if (j!=jmax) {
        pd = cst*(PetscReal)(i+j-1);
        /* north */
        if (i==1) {
          ierr = MatSetValue(A,ix-1,ix,2*pd,INSERT_VALUES);CHKERRQ(ierr);
        } else {
          ierr = MatSetValue(A,ix-1,ix,pd,INSERT_VALUES);CHKERRQ(ierr);
        }
        /* east */
        if (j==1) {
          ierr = MatSetValue(A,ix-1,ix+jmax-1,2*pd,INSERT_VALUES);CHKERRQ(ierr);
        } else {
          ierr = MatSetValue(A,ix-1,ix+jmax-1,pd,INSERT_VALUES);CHKERRQ(ierr);
        }
      }
      /* south */
      pu = 0.5 - cst*(PetscReal)(i+j-3);
      if (j>1) {
        ierr = MatSetValue(A,ix-1,ix-2,pu,INSERT_VALUES);CHKERRQ(ierr);
      }
      /* west */
      if (i>1) {
        ierr = MatSetValue(A,ix-1,ix-jmax-2,pu,INSERT_VALUES);CHKERRQ(ierr);
      }
    }
  }
  ierr = MatAssemblyBegin(A,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(A,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MyEigenSort"
/*
    Function for user-defined eigenvalue ordering criterion.

    Given two eigenvalues ar+i*ai and br+i*bi, the subroutine must choose
    one of them as the preferred one according to the criterion.
    In this example, the preferred value is the one closest to the target,
    but on the right side.
*/
PetscErrorCode MyEigenSort(PetscScalar ar,PetscScalar ai,PetscScalar br,PetscScalar bi,PetscInt *r,void *ctx)
{
  PetscScalar target = *(PetscScalar*)ctx;
  PetscReal   da,db;
  PetscBool   aisright,bisright;

  PetscFunctionBeginUser;
  if (PetscRealPart(target) < PetscRealPart(ar)) aisright = PETSC_TRUE;
  else aisright = PETSC_FALSE;
  if (PetscRealPart(target) < PetscRealPart(br)) bisright = PETSC_TRUE;
  else bisright = PETSC_FALSE;
  if (aisright == bisright) {
    /* both are on the same side of the target */
    da = SlepcAbsEigenvalue(ar-target,ai);
    db = SlepcAbsEigenvalue(br-target,bi);
    if (da < db) *r = -1;
    else if (da > db) *r = 1;
    else *r = 0;
  } else if (aisright && !bisright) *r = -1; /* 'a' is on the right */
  else *r = 1;  /* 'b' is on the right */
  PetscFunctionReturn(0);
}
