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

static char help[] = "Tests multiple calls to EPSSolve with different matrix.\n\n";

#include <slepceps.h>

#undef __FUNCT__
#define __FUNCT__ "main"
int main(int argc,char **argv)
{
  Mat            A1,A2;       /* problem matrices */
  EPS            eps;         /* eigenproblem solver context */
  PetscScalar    value[3];
  PetscReal      tol=1000*PETSC_MACHINE_EPSILON,v;
  Vec            d;
  PetscInt       n=30,i,Istart,Iend,col[3];
  PetscBool      FirstBlock=PETSC_FALSE,LastBlock=PETSC_FALSE;
  PetscRandom    myrand;
  PetscErrorCode ierr;

  SlepcInitialize(&argc,&argv,(char*)0,help);

  ierr = PetscOptionsGetInt(NULL,"-n",&n,NULL);CHKERRQ(ierr);
  ierr = PetscPrintf(PETSC_COMM_WORLD,"\nTridiagonal with random diagonal, n=%D\n\n",n);CHKERRQ(ierr);

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
           Create matrix tridiag([-1 0 -1])
     - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
  ierr = MatCreate(PETSC_COMM_WORLD,&A1);CHKERRQ(ierr);
  ierr = MatSetSizes(A1,PETSC_DECIDE,PETSC_DECIDE,n,n);CHKERRQ(ierr);
  ierr = MatSetFromOptions(A1);CHKERRQ(ierr);
  ierr = MatSetUp(A1);CHKERRQ(ierr);

  ierr = MatGetOwnershipRange(A1,&Istart,&Iend);CHKERRQ(ierr);
  if (Istart==0) FirstBlock=PETSC_TRUE;
  if (Iend==n) LastBlock=PETSC_TRUE;
  value[0]=-1.0; value[1]=0.0; value[2]=-1.0;
  for (i=(FirstBlock? Istart+1: Istart); i<(LastBlock? Iend-1: Iend); i++) {
    col[0]=i-1; col[1]=i; col[2]=i+1;
    ierr = MatSetValues(A1,1,&i,3,col,value,INSERT_VALUES);CHKERRQ(ierr);
  }
  if (LastBlock) {
    i=n-1; col[0]=n-2; col[1]=n-1;
    ierr = MatSetValues(A1,1,&i,2,col,value,INSERT_VALUES);CHKERRQ(ierr);
  }
  if (FirstBlock) {
    i=0; col[0]=0; col[1]=1; value[0]=0.0; value[1]=-1.0;
    ierr = MatSetValues(A1,1,&i,2,col,value,INSERT_VALUES);CHKERRQ(ierr);
  }

  ierr = MatAssemblyBegin(A1,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(A1,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
       Create two matrices by filling the diagonal with rand values
     - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
  ierr = MatDuplicate(A1,MAT_COPY_VALUES,&A2);CHKERRQ(ierr);
  ierr = MatGetVecs(A1,NULL,&d);CHKERRQ(ierr);
  ierr = PetscRandomCreate(PETSC_COMM_WORLD,&myrand);CHKERRQ(ierr);
  ierr = PetscRandomSetFromOptions(myrand);CHKERRQ(ierr);
  ierr = PetscRandomSetInterval(myrand,0.0,1.0);CHKERRQ(ierr);
  for (i=0; i<n; i++) {
    ierr = PetscRandomGetValueReal(myrand,&v);CHKERRQ(ierr);
    ierr = VecSetValue(d,i,v,INSERT_VALUES);CHKERRQ(ierr);
  }
  ierr = VecAssemblyBegin(d);CHKERRQ(ierr);
  ierr = VecAssemblyEnd(d);CHKERRQ(ierr);
  ierr = MatDiagonalSet(A1,d,INSERT_VALUES);CHKERRQ(ierr);
  for (i=0; i<n; i++) {
    ierr = PetscRandomGetValueReal(myrand,&v);CHKERRQ(ierr);
    ierr = VecSetValue(d,i,v,INSERT_VALUES);CHKERRQ(ierr);
  }
  ierr = VecAssemblyBegin(d);CHKERRQ(ierr);
  ierr = VecAssemblyEnd(d);CHKERRQ(ierr);
  ierr = MatDiagonalSet(A2,d,INSERT_VALUES);CHKERRQ(ierr);
  ierr = VecDestroy(&d);CHKERRQ(ierr);
  ierr = PetscRandomDestroy(&myrand);CHKERRQ(ierr);

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                        Create the eigensolver
     - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
  ierr = EPSCreate(PETSC_COMM_WORLD,&eps);CHKERRQ(ierr);
  ierr = EPSSetProblemType(eps,EPS_HEP);CHKERRQ(ierr);
  ierr = EPSSetTolerances(eps,tol,PETSC_DEFAULT);CHKERRQ(ierr);
  ierr = EPSSetOperators(eps,A1,NULL);CHKERRQ(ierr);
  ierr = EPSSetFromOptions(eps);CHKERRQ(ierr);

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                        Solve first eigenproblem
     - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
  ierr = EPSSolve(eps);CHKERRQ(ierr);
  ierr = PetscPrintf(PETSC_COMM_WORLD," - - - First matrix - - -\n");CHKERRQ(ierr);
  ierr = EPSPrintSolution(eps,NULL);CHKERRQ(ierr);

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                        Solve second eigenproblem
     - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
  ierr = EPSSetOperators(eps,A2,NULL);CHKERRQ(ierr);
  ierr = EPSSolve(eps);CHKERRQ(ierr);
  ierr = PetscPrintf(PETSC_COMM_WORLD," - - - Second matrix - - -\n");CHKERRQ(ierr);
  ierr = EPSPrintSolution(eps,NULL);CHKERRQ(ierr);

  ierr = EPSDestroy(&eps);CHKERRQ(ierr);
  ierr = MatDestroy(&A1);CHKERRQ(ierr);
  ierr = MatDestroy(&A2);CHKERRQ(ierr);
  ierr = SlepcFinalize();
  return 0;
}
