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

static char help[] = "Test DSGHIEP.\n\n";

#include <slepcds.h>

#undef __FUNCT__
#define __FUNCT__ "main"
int main(int argc,char **argv)
{
  PetscErrorCode ierr;
  DS             ds;
  SlepcSC        sc;
  PetscReal      re,im;
  PetscScalar    *A,*B,*eigr,*eigi;
  PetscInt       i,j,n=10,ld;
  PetscViewer    viewer;
  PetscBool      verbose;

  SlepcInitialize(&argc,&argv,(char*)0,help);
  ierr = PetscOptionsGetInt(NULL,"-n",&n,NULL);CHKERRQ(ierr);
  ierr = PetscPrintf(PETSC_COMM_WORLD,"Solve a Dense System of type GHIEP - dimension %D.\n",n);CHKERRQ(ierr);
  ierr = PetscOptionsHasName(NULL,"-verbose",&verbose);CHKERRQ(ierr);

  /* Create DS object */
  ierr = DSCreate(PETSC_COMM_WORLD,&ds);CHKERRQ(ierr);
  ierr = DSSetType(ds,DSGHIEP);CHKERRQ(ierr);
  ierr = DSSetFromOptions(ds);CHKERRQ(ierr);
  ld = n+2;  /* test leading dimension larger than n */
  ierr = DSAllocate(ds,ld);CHKERRQ(ierr);
  ierr = DSSetDimensions(ds,n,0,0,0);CHKERRQ(ierr);

  /* Set up viewer */
  ierr = PetscViewerASCIIGetStdout(PETSC_COMM_WORLD,&viewer);CHKERRQ(ierr);
  ierr = PetscViewerPushFormat(viewer,PETSC_VIEWER_ASCII_INFO_DETAIL);CHKERRQ(ierr);
  ierr = DSView(ds,viewer);CHKERRQ(ierr);
  ierr = PetscViewerPopFormat(viewer);CHKERRQ(ierr);
  if (verbose) {
    ierr = PetscViewerPushFormat(viewer,PETSC_VIEWER_ASCII_MATLAB);CHKERRQ(ierr);
  }

  /* Fill with a symmetric Toeplitz matrix */
  ierr = DSGetArray(ds,DS_MAT_A,&A);CHKERRQ(ierr);
  ierr = DSGetArray(ds,DS_MAT_B,&B);CHKERRQ(ierr);
  for (i=0;i<n;i++) A[i+i*ld]=2.0;
  for (j=1;j<3;j++) {
    for (i=0;i<n-j;i++) { A[i+(i+j)*ld]=1.0; A[(i+j)+i*ld]=1.0; }
  }
  for (j=1;j<3;j++) { A[0+j*ld]=-1.0*(j+2); A[j+0*ld]=-1.0*(j+2); }
  /* Signature matrix */
  for (i=0;i<n;i++) B[i+i*ld]=1.0;
  B[0] = -1.0;
  B[n-1+(n-1)*ld] = -1.0;
  ierr = DSRestoreArray(ds,DS_MAT_A,&A);CHKERRQ(ierr);
  ierr = DSRestoreArray(ds,DS_MAT_B,&B);CHKERRQ(ierr);
  ierr = DSSetState(ds,DS_STATE_RAW);CHKERRQ(ierr);
  if (verbose) {
    ierr = PetscPrintf(PETSC_COMM_WORLD,"Initial - - - - - - - - -\n");CHKERRQ(ierr);
    ierr = DSView(ds,viewer);CHKERRQ(ierr);
  }

  /* Solve */
  ierr = PetscCalloc2(n,&eigr,n,&eigi);CHKERRQ(ierr);
  ierr = DSGetSlepcSC(ds,&sc);CHKERRQ(ierr);
  sc->comparison    = SlepcCompareLargestMagnitude;
  sc->comparisonctx = NULL;
  sc->map           = NULL;
  sc->mapobj        = NULL;
  ierr = DSSolve(ds,eigr,eigi);CHKERRQ(ierr);
  ierr = DSSort(ds,eigr,eigi,NULL,NULL,NULL);CHKERRQ(ierr);
  if (verbose) {
    ierr = PetscPrintf(PETSC_COMM_WORLD,"After solve - - - - - - - - -\n");CHKERRQ(ierr);
    ierr = DSView(ds,viewer);CHKERRQ(ierr);
  }

  /* Print eigenvalues */
  ierr = PetscPrintf(PETSC_COMM_WORLD,"Computed eigenvalues =\n",n);CHKERRQ(ierr);
  for (i=0;i<n;i++) {
#if defined(PETSC_USE_COMPLEX)
    re = PetscRealPart(eigr[i]);
    im = PetscImaginaryPart(eigr[i]);
#else
    re = eigr[i];
    im = eigi[i];
#endif
    if (PetscAbs(im)<1e-10) {
      ierr = PetscViewerASCIIPrintf(viewer,"  %.5f\n",(double)re);CHKERRQ(ierr);
    } else {
      ierr = PetscViewerASCIIPrintf(viewer,"  %.5f%+.5fi\n",(double)re,(double)im);CHKERRQ(ierr);
    }
  }
  ierr = PetscFree2(eigr,eigi);CHKERRQ(ierr);
  ierr = DSDestroy(&ds);CHKERRQ(ierr);
  ierr = SlepcFinalize();
  return 0;
}
