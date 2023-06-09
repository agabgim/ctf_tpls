/*
      This file contains the subroutines which implement various operations
      of the matrix associated to the shift-and-invert technique for eigenvalue
      problems, and also a subroutine to create it.

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

#include <slepc-private/stimpl.h>

typedef struct {
  PetscScalar alpha;
  PetscScalar *coeffs;
  ST          st;
  Vec         z;
  PetscInt    nmat;
  PetscInt    *matIdx;
} ST_SHELLMAT;

#undef __FUNCT__
#define __FUNCT__ "STMatShellShift"
PetscErrorCode STMatShellShift(Mat A,PetscScalar alpha)
{
  PetscErrorCode ierr;
  ST_SHELLMAT    *ctx;

  PetscFunctionBegin;
  ierr = MatShellGetContext(A,(void**)&ctx);CHKERRQ(ierr);
  ctx->alpha = alpha;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatMult_Shell"
/*
  For i=0:nmat-1 computes y = (sum_i (coeffs[i]*alpha^i*st->A[idx[i]]))x
  If null coeffs computes with coeffs[i]=1.0  
*/
static PetscErrorCode MatMult_Shell(Mat A,Vec x,Vec y)
{
  PetscErrorCode ierr;
  ST_SHELLMAT    *ctx;
  ST             st;
  PetscInt       i;
  PetscScalar    t=1.0,c;

  PetscFunctionBegin;
  ierr = MatShellGetContext(A,(void**)&ctx);CHKERRQ(ierr);
  st = ctx->st;
  ierr = MatMult(st->A[ctx->matIdx[0]],x,y);CHKERRQ(ierr);
  if (ctx->coeffs && ctx->coeffs[0]!=1.0) {
    ierr = VecScale(y,ctx->coeffs[0]);CHKERRQ(ierr);
  }
  if (ctx->alpha!=0.0) {
    for (i=1;i<ctx->nmat;i++) {
      ierr = MatMult(st->A[ctx->matIdx[i]],x,ctx->z);CHKERRQ(ierr);
      t *= ctx->alpha;
      c = (ctx->coeffs)?t*ctx->coeffs[i]:t;
      ierr = VecAXPY(y,c,ctx->z);CHKERRQ(ierr);
    }
    if (ctx->nmat==1) {    /* y = (A + alpha*I) x */
      ierr = VecAXPY(y,ctx->alpha,x);CHKERRQ(ierr);
    }
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatMultTranspose_Shell"
static PetscErrorCode MatMultTranspose_Shell(Mat A,Vec x,Vec y)
{
  PetscErrorCode ierr;
  ST_SHELLMAT    *ctx;
  ST             st;
  PetscInt       i;
  PetscScalar    t=1.0,c;

  PetscFunctionBegin;
  ierr = MatShellGetContext(A,(void**)&ctx);CHKERRQ(ierr);
  st = ctx->st;
  ierr = MatMultTranspose(st->A[ctx->matIdx[0]],x,y);CHKERRQ(ierr);
  if (ctx->coeffs && ctx->coeffs[0]!=1.0) {
    ierr = VecScale(y,ctx->coeffs[0]);CHKERRQ(ierr);
  }
  if (ctx->alpha!=0.0) {
    for (i=1;i<ctx->nmat;i++) {
      ierr = MatMultTranspose(st->A[ctx->matIdx[i]],x,ctx->z);CHKERRQ(ierr);
      t *= ctx->alpha;
      c = (ctx->coeffs)?t*ctx->coeffs[i]:t;
      ierr = VecAXPY(y,c,ctx->z);CHKERRQ(ierr);
    }
    if (ctx->nmat==1) {    /* y = (A + alpha*I) x */
      ierr = VecAXPY(y,ctx->alpha,x);CHKERRQ(ierr);
    }
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatGetDiagonal_Shell"
static PetscErrorCode MatGetDiagonal_Shell(Mat A,Vec diag)
{
  PetscErrorCode ierr;
  ST_SHELLMAT    *ctx;
  ST             st;
  Vec            diagb;
  PetscInt       i;
  PetscScalar    t=1.0,c;

  PetscFunctionBegin;
  ierr = MatShellGetContext(A,(void**)&ctx);CHKERRQ(ierr);
  st = ctx->st;
  ierr = MatGetDiagonal(st->A[ctx->matIdx[0]],diag);CHKERRQ(ierr);
  if (ctx->coeffs && ctx->coeffs[0]!=1.0) {
    ierr = VecScale(diag,ctx->coeffs[0]);CHKERRQ(ierr);
  }
  if (ctx->alpha!=0.0) {
    if (ctx->nmat==1) {    /* y = (A + alpha*I) x */
      ierr = VecShift(diag,ctx->alpha);CHKERRQ(ierr);
    } else {
      ierr = VecDuplicate(diag,&diagb);CHKERRQ(ierr);
      for (i=1;i<ctx->nmat;i++) {
        ierr = MatGetDiagonal(st->A[ctx->matIdx[i]],diagb);CHKERRQ(ierr);
        t *= ctx->alpha;
        c = (ctx->coeffs)?t*ctx->coeffs[i]:t;
        ierr = VecAYPX(diag,c,diagb);CHKERRQ(ierr);
      }
      ierr = VecDestroy(&diagb);CHKERRQ(ierr);
    }
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatDestroy_Shell"
static PetscErrorCode MatDestroy_Shell(Mat A)
{
  PetscErrorCode ierr;
  ST_SHELLMAT    *ctx;

  PetscFunctionBegin;
  ierr = MatShellGetContext(A,(void**)&ctx);CHKERRQ(ierr);
  ierr = VecDestroy(&ctx->z);CHKERRQ(ierr);
  ierr = PetscFree(ctx->matIdx);CHKERRQ(ierr);
  ierr = PetscFree(ctx->coeffs);CHKERRQ(ierr);
  ierr = PetscFree(ctx);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "STMatShellCreate"
PetscErrorCode STMatShellCreate(ST st,PetscScalar alpha,PetscInt nmat,PetscInt *matIdx,PetscScalar *coeffs,Mat *mat)
{
  PetscErrorCode ierr;
  PetscInt       n,m,N,M,i;
  PetscBool      has=PETSC_FALSE,hasA,hasB;
  ST_SHELLMAT    *ctx;

  PetscFunctionBegin;
  ierr = MatGetSize(st->A[0],&M,&N);CHKERRQ(ierr);
  ierr = MatGetLocalSize(st->A[0],&m,&n);CHKERRQ(ierr);
  ierr = PetscNew(&ctx);CHKERRQ(ierr);
  ctx->st = st;
  ctx->alpha = alpha;
  ctx->nmat = matIdx?nmat:st->nmat;
  ierr = PetscMalloc1(ctx->nmat,&ctx->matIdx);CHKERRQ(ierr);
  if (matIdx) {
    for (i=0;i<ctx->nmat;i++) ctx->matIdx[i] = matIdx[i];
  } else {
    ctx->matIdx[0] = 0;
    if (ctx->nmat>1) ctx->matIdx[1] = 1;
  }
  if (coeffs) {
    ierr = PetscMalloc(ctx->nmat*sizeof(PetscScalar),&ctx->coeffs);CHKERRQ(ierr);
    for (i=0;i<ctx->nmat;i++) ctx->coeffs[i] = coeffs[i];
  }
  ierr = MatGetVecs(st->A[0],&ctx->z,NULL);CHKERRQ(ierr);
  ierr = MatCreateShell(PetscObjectComm((PetscObject)st),m,n,M,N,(void*)ctx,mat);CHKERRQ(ierr);
  ierr = MatShellSetOperation(*mat,MATOP_MULT,(void(*)(void))MatMult_Shell);CHKERRQ(ierr);
  ierr = MatShellSetOperation(*mat,MATOP_MULT_TRANSPOSE,(void(*)(void))MatMultTranspose_Shell);CHKERRQ(ierr);
  ierr = MatShellSetOperation(*mat,MATOP_DESTROY,(void(*)(void))MatDestroy_Shell);CHKERRQ(ierr);

  ierr = MatHasOperation(st->A[ctx->matIdx[0]],MATOP_GET_DIAGONAL,&hasA);CHKERRQ(ierr);
  if (st->nmat>1) {
    has = hasA;
    for (i=1;i<ctx->nmat;i++) {
      ierr = MatHasOperation(st->A[ctx->matIdx[i]],MATOP_GET_DIAGONAL,&hasB);CHKERRQ(ierr);
      has = (has && hasB)? PETSC_TRUE: PETSC_FALSE;
    }
  }
  if ((hasA && st->nmat==1) || has) {
    ierr = MatShellSetOperation(*mat,MATOP_GET_DIAGONAL,(void(*)(void))MatGetDiagonal_Shell);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

