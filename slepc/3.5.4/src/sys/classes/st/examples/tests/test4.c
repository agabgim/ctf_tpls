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

static char help[] = "Test ST with four matrices.\n\n";

#include <slepcst.h>

#undef __FUNCT__
#define __FUNCT__ "main"
int main(int argc,char **argv)
{
  Mat            A,B,C,D,mat[4];
  ST             st;
  Vec            v,w;
  STType         type;
  PetscScalar    value[3],sigma;
  PetscInt       n=10,i,Istart,Iend,col[3];
  PetscBool      FirstBlock=PETSC_FALSE,LastBlock=PETSC_FALSE;
  PetscErrorCode ierr;

  SlepcInitialize(&argc,&argv,(char*)0,help);
  ierr = PetscOptionsGetInt(NULL,"-n",&n,NULL);CHKERRQ(ierr);
  ierr = PetscPrintf(PETSC_COMM_WORLD,"\n1-D Laplacian plus diagonal, n=%D\n\n",n);CHKERRQ(ierr);
  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     Compute the operator matrix for the 1-D Laplacian
     - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

  ierr = MatCreate(PETSC_COMM_WORLD,&A);CHKERRQ(ierr);
  ierr = MatSetSizes(A,PETSC_DECIDE,PETSC_DECIDE,n,n);CHKERRQ(ierr);
  ierr = MatSetFromOptions(A);CHKERRQ(ierr);
  ierr = MatSetUp(A);CHKERRQ(ierr);

  ierr = MatCreate(PETSC_COMM_WORLD,&B);CHKERRQ(ierr);
  ierr = MatSetSizes(B,PETSC_DECIDE,PETSC_DECIDE,n,n);CHKERRQ(ierr);
  ierr = MatSetFromOptions(B);CHKERRQ(ierr);
  ierr = MatSetUp(B);CHKERRQ(ierr);

  ierr = MatCreate(PETSC_COMM_WORLD,&C);CHKERRQ(ierr);
  ierr = MatSetSizes(C,PETSC_DECIDE,PETSC_DECIDE,n,n);CHKERRQ(ierr);
  ierr = MatSetFromOptions(C);CHKERRQ(ierr);
  ierr = MatSetUp(C);CHKERRQ(ierr);

  ierr = MatCreate(PETSC_COMM_WORLD,&D);CHKERRQ(ierr);
  ierr = MatSetSizes(D,PETSC_DECIDE,PETSC_DECIDE,n,n);CHKERRQ(ierr);
  ierr = MatSetFromOptions(D);CHKERRQ(ierr);
  ierr = MatSetUp(D);CHKERRQ(ierr);

  ierr = MatGetOwnershipRange(A,&Istart,&Iend);CHKERRQ(ierr);
  if (Istart==0) FirstBlock=PETSC_TRUE;
  if (Iend==n) LastBlock=PETSC_TRUE;
  value[0]=-1.0; value[1]=2.0; value[2]=-1.0;
  for (i=(FirstBlock? Istart+1: Istart); i<(LastBlock? Iend-1: Iend); i++) {
    col[0]=i-1; col[1]=i; col[2]=i+1;
    ierr = MatSetValues(A,1,&i,3,col,value,INSERT_VALUES);CHKERRQ(ierr);
    ierr = MatSetValue(B,i,i,(PetscScalar)i,INSERT_VALUES);CHKERRQ(ierr);
  }
  if (LastBlock) {
    i=n-1; col[0]=n-2; col[1]=n-1;
    ierr = MatSetValues(A,1,&i,2,col,value,INSERT_VALUES);CHKERRQ(ierr);
    ierr = MatSetValue(B,i,i,(PetscScalar)i,INSERT_VALUES);CHKERRQ(ierr);
  }
  if (FirstBlock) {
    i=0; col[0]=0; col[1]=1; value[0]=2.0; value[1]=-1.0;
    ierr = MatSetValues(A,1,&i,2,col,value,INSERT_VALUES);CHKERRQ(ierr);
    ierr = MatSetValue(B,i,i,-1.0,INSERT_VALUES);CHKERRQ(ierr);
  }
  for (i=Istart;i<Iend;i++) {
    ierr = MatSetValue(C,i,n-i-1,1.0,INSERT_VALUES);CHKERRQ(ierr);
    ierr = MatSetValue(D,i,i,i*.1,INSERT_VALUES);CHKERRQ(ierr);
    if (i==0) {
      ierr = MatSetValue(D,0,n-1,1.0,INSERT_VALUES);CHKERRQ(ierr);
    }
    if (i==n-1) {
      ierr = MatSetValue(D,n-1,0,1.0,INSERT_VALUES);CHKERRQ(ierr);
    }
  }

  ierr = MatAssemblyBegin(A,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(A,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyBegin(B,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(B,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyBegin(C,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(C,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyBegin(D,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(D,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatGetVecs(A,&v,&w);CHKERRQ(ierr);
  ierr = VecSet(v,1.0);CHKERRQ(ierr);

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                Create the spectral transformation object
     - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
  ierr = STCreate(PETSC_COMM_WORLD,&st);CHKERRQ(ierr);
  mat[0] = A;
  mat[1] = B;
  mat[2] = C;
  mat[3] = D;
  ierr = STSetOperators(st,4,mat);CHKERRQ(ierr);
  ierr = STSetFromOptions(st);CHKERRQ(ierr);
  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
              Apply the transformed operator for several ST's
     - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
  /* shift, sigma=0.0 */
  ierr = STSetUp(st);CHKERRQ(ierr);
  ierr = STGetType(st,&type);CHKERRQ(ierr);
  ierr = PetscPrintf(PETSC_COMM_WORLD,"ST type %s\n",type);CHKERRQ(ierr);
  for (i=0;i<4;i++) {
    ierr = STMatMult(st,i,v,w);
    ierr = PetscPrintf(PETSC_COMM_WORLD,"k= %D\n",i);CHKERRQ(ierr);
    ierr = VecView(w,NULL);CHKERRQ(ierr);
  }
  ierr = STMatSolve(st,v,w);
  ierr = PetscPrintf(PETSC_COMM_WORLD,"solve\n");CHKERRQ(ierr);
  ierr = VecView(w,NULL);CHKERRQ(ierr);

  /* shift, sigma=0.1 */
  sigma = 0.1;
  ierr = STSetShift(st,sigma);CHKERRQ(ierr);
  ierr = STGetShift(st,&sigma);CHKERRQ(ierr);
  ierr = PetscPrintf(PETSC_COMM_WORLD,"With shift=%g\n",(double)PetscRealPart(sigma));CHKERRQ(ierr);
  for (i=0;i<4;i++) {
    ierr = STMatMult(st,i,v,w);
    ierr = PetscPrintf(PETSC_COMM_WORLD,"k= %D\n",i);CHKERRQ(ierr);
    ierr = VecView(w,NULL);CHKERRQ(ierr);
  }
  ierr = STMatSolve(st,v,w);
  ierr = PetscPrintf(PETSC_COMM_WORLD,"solve\n");CHKERRQ(ierr);
  ierr = VecView(w,NULL);CHKERRQ(ierr);

  /* sinvert, sigma=0.1 */
  ierr = STPostSolve(st);CHKERRQ(ierr);
  ierr = STSetType(st,STSINVERT);CHKERRQ(ierr);
  ierr = STGetType(st,&type);CHKERRQ(ierr);
  ierr = PetscPrintf(PETSC_COMM_WORLD,"ST type %s\n",type);CHKERRQ(ierr);
  ierr = STGetShift(st,&sigma);CHKERRQ(ierr);
  ierr = PetscPrintf(PETSC_COMM_WORLD,"With shift=%g\n",(double)PetscRealPart(sigma));CHKERRQ(ierr);
  for (i=0;i<4;i++) {
    ierr = STMatMult(st,i,v,w);
    ierr = PetscPrintf(PETSC_COMM_WORLD,"k= %D\n",i);CHKERRQ(ierr);
    ierr = VecView(w,NULL);CHKERRQ(ierr);
  }
  ierr = STMatSolve(st,v,w);
  ierr = PetscPrintf(PETSC_COMM_WORLD,"solve\n");CHKERRQ(ierr);
  ierr = VecView(w,NULL);CHKERRQ(ierr);

  /* sinvert, sigma=-0.5 */
  sigma = -0.5;
  ierr = STSetShift(st,sigma);CHKERRQ(ierr);
  ierr = STGetShift(st,&sigma);CHKERRQ(ierr);
  ierr = PetscPrintf(PETSC_COMM_WORLD,"With shift=%g\n",(double)PetscRealPart(sigma));CHKERRQ(ierr);
  for (i=0;i<4;i++) {
    ierr = STMatMult(st,i,v,w);
    ierr = PetscPrintf(PETSC_COMM_WORLD,"k= %D\n",i);CHKERRQ(ierr);
    ierr = VecView(w,NULL);CHKERRQ(ierr);
  }
  ierr = STMatSolve(st,v,w);
  ierr = PetscPrintf(PETSC_COMM_WORLD,"solve\n");CHKERRQ(ierr);
  ierr = VecView(w,NULL);CHKERRQ(ierr);
  ierr = STDestroy(&st);CHKERRQ(ierr);
  ierr = MatDestroy(&A);CHKERRQ(ierr);
  ierr = MatDestroy(&B);CHKERRQ(ierr);
  ierr = MatDestroy(&C);CHKERRQ(ierr);
  ierr = MatDestroy(&D);CHKERRQ(ierr);
  ierr = VecDestroy(&v);CHKERRQ(ierr);
  ierr = VecDestroy(&w);CHKERRQ(ierr);
  ierr = SlepcFinalize();
  return 0;
}
