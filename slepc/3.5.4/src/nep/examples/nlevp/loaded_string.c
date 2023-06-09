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
/*
   This example implements one of the problems found at
       NLEVP: A Collection of Nonlinear Eigenvalue Problems,
       The University of Manchester.
   The details of the collection can be found at:
       [1] T. Betcke et al., "NLEVP: A Collection of Nonlinear Eigenvalue
           Problems", ACM Trans. Math. Software 39(2), Article 7, 2013.

   The loaded_string problem is a rational eigenvalue problem for the
   finite element model of a loaded vibrating string.
*/

static char help[] = "NLEVP problem: loaded_string.\n\n"
  "The command line options are:\n"
  "  -n <n>, dimension of the matrices.\n"
  "  -kappa <kappa>, stiffness of elastic spring.\n"
  "  -mass <m>, mass of the attached load.\n\n";

#include <slepcnep.h>

#define NMAT 3

#undef __FUNCT__
#define __FUNCT__ "main"
int main(int argc,char **argv)
{
  Mat            A[NMAT];         /* problem matrices */
  FN             f[NMAT];         /* functions to define the nonlinear operator */
  NEP            nep;             /* nonlinear eigensolver context */
  PetscInt       n=20,Istart,Iend,i,nconv;
  PetscReal      kappa=1.0,m=1.0,re,im,norm;
  PetscScalar    kr,ki,sigma,numer[2],denom[2];
  PetscErrorCode ierr;

  SlepcInitialize(&argc,&argv,(char*)0,help);

  ierr = PetscOptionsGetInt(NULL,"-n",&n,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsGetReal(NULL,"-kappa",&kappa,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsGetReal(NULL,"-mass",&m,NULL);CHKERRQ(ierr);
  sigma = kappa/m;
  ierr = PetscPrintf(PETSC_COMM_WORLD,"Loaded vibrating string, n=%D kappa=%g m=%g\n\n",n,(double)kappa,(double)m);CHKERRQ(ierr);

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
                       Build the problem matrices 
     - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

  /* initialize matrices */
  for (i=0;i<NMAT;i++) {
    ierr = MatCreate(PETSC_COMM_WORLD,&A[i]);CHKERRQ(ierr);
    ierr = MatSetSizes(A[i],PETSC_DECIDE,PETSC_DECIDE,n,n);CHKERRQ(ierr);
    ierr = MatSetFromOptions(A[i]);CHKERRQ(ierr);
    ierr = MatSetUp(A[i]);CHKERRQ(ierr);
  }
  ierr = MatGetOwnershipRange(A[0],&Istart,&Iend);CHKERRQ(ierr);

  /* A0 */
  for (i=Istart;i<Iend;i++) {
    ierr = MatSetValue(A[0],i,i,(i==n-1)?1.0*n:2.0*n,INSERT_VALUES);CHKERRQ(ierr);
    if (i>0) { ierr = MatSetValue(A[0],i,i-1,-1.0*n,INSERT_VALUES);CHKERRQ(ierr); }
    if (i<n-1) { ierr = MatSetValue(A[0],i,i+1,-1.0*n,INSERT_VALUES);CHKERRQ(ierr); }
  }

  /* A1 */

  for (i=Istart;i<Iend;i++) {
    ierr = MatSetValue(A[1],i,i,(i==n-1)?2.0/(6.0*n):4.0/(6.0*n),INSERT_VALUES);CHKERRQ(ierr);
    if (i>0) { ierr = MatSetValue(A[1],i,i-1,1.0/(6.0*n),INSERT_VALUES);CHKERRQ(ierr); }
    if (i<n-1) { ierr = MatSetValue(A[1],i,i+1,1.0/(6.0*n),INSERT_VALUES);CHKERRQ(ierr); }
  }

  /* A2 */
  if (Istart<=n-1 && n-1<Iend) {
    ierr = MatSetValue(A[2],n-1,n-1,kappa,INSERT_VALUES); CHKERRQ(ierr);
  }

  /* assemble matrices */
  for (i=0;i<NMAT;i++) {
    ierr = MatAssemblyBegin(A[i],MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  }
  for (i=0;i<NMAT;i++) {
    ierr = MatAssemblyEnd(A[i],MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  }

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
                       Create the problem functions
     - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

  /* f1=1 */
  ierr = FNCreate(PETSC_COMM_WORLD,&f[0]);CHKERRQ(ierr);
  ierr = FNSetType(f[0],FNRATIONAL);CHKERRQ(ierr);
  numer[0] = 1.0;
  ierr = FNSetParameters(f[0],1,numer,0,NULL);CHKERRQ(ierr);

  /* f2=-lambda */
  ierr = FNCreate(PETSC_COMM_WORLD,&f[1]);CHKERRQ(ierr);
  ierr = FNSetType(f[1],FNRATIONAL);CHKERRQ(ierr);
  numer[0] = -1.0; numer[1] = 0.0;
  ierr = FNSetParameters(f[1],2,numer,0,NULL);CHKERRQ(ierr);

  /* f3=lambda/(lambda-sigma) */
  ierr = FNCreate(PETSC_COMM_WORLD,&f[2]);CHKERRQ(ierr);
  ierr = FNSetType(f[2],FNRATIONAL);CHKERRQ(ierr);
  numer[0] = 1.0; numer[1] = 0.0;
  denom[0] = 1.0; denom[1] = -sigma;
  ierr = FNSetParameters(f[2],2,numer,2,denom);CHKERRQ(ierr);

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
                Create the eigensolver and solve the problem
     - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

  ierr = NEPCreate(PETSC_COMM_WORLD,&nep);CHKERRQ(ierr);
  ierr = NEPSetSplitOperator(nep,3,A,f,SUBSET_NONZERO_PATTERN);CHKERRQ(ierr);
  ierr = NEPSetFromOptions(nep);CHKERRQ(ierr);
  ierr = NEPSolve(nep);CHKERRQ(ierr);

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                    Display solution and clean up
     - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
  
  /*
     Get number of converged approximate eigenpairs
  */
  ierr = NEPGetConverged(nep,&nconv);CHKERRQ(ierr);
  ierr = PetscPrintf(PETSC_COMM_WORLD," Number of converged approximate eigenpairs: %D\n\n",nconv);CHKERRQ(ierr);

  if (nconv>0) {
    /*
       Display eigenvalues and relative errors
    */
    ierr = PetscPrintf(PETSC_COMM_WORLD,
         "           k              ||T(k)x||\n"
         "   ----------------- ------------------\n");CHKERRQ(ierr);
    for (i=0;i<nconv;i++) {
      ierr = NEPGetEigenpair(nep,i,&kr,&ki,NULL,NULL);CHKERRQ(ierr);
      ierr = NEPComputeRelativeError(nep,i,&norm);CHKERRQ(ierr);
#if defined(PETSC_USE_COMPLEX)
      re = PetscRealPart(kr);
      im = PetscImaginaryPart(kr);
#else
      re = kr;
      im = ki;
#endif
      if (im!=0.0) {
        ierr = PetscPrintf(PETSC_COMM_WORLD," %9f%+9f j %12g\n",(double)re,(double)im,(double)norm);CHKERRQ(ierr);
      } else {
        ierr = PetscPrintf(PETSC_COMM_WORLD,"   %12f         %12g\n",(double)re,(double)norm);CHKERRQ(ierr);
      }
    }
    ierr = PetscPrintf(PETSC_COMM_WORLD,"\n");CHKERRQ(ierr);
  }

  ierr = NEPDestroy(&nep);CHKERRQ(ierr);
  for (i=0;i<NMAT;i++) {
    ierr = MatDestroy(&A[i]);CHKERRQ(ierr);
    ierr = FNDestroy(&f[i]);CHKERRQ(ierr);
  }
  ierr = SlepcFinalize();CHKERRQ(ierr);
  return 0;
}
