/*
   BV implemented as an array of independent Vecs

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

#include <slepc-private/bvimpl.h>

typedef struct {
  Vec *V;
} BV_VECS;

#undef __FUNCT__
#define __FUNCT__ "BVMult_Vecs"
PetscErrorCode BVMult_Vecs(BV Y,PetscScalar alpha,PetscScalar beta,BV X,Mat Q)
{
  PetscErrorCode ierr;
  BV_VECS        *y = (BV_VECS*)Y->data,*x = (BV_VECS*)X->data;
  PetscScalar    *q,*s=NULL;
  PetscInt       i,j,ldq;

  PetscFunctionBegin;
  ierr = MatGetSize(Q,&ldq,NULL);CHKERRQ(ierr);
  if (alpha!=1.0) {
    ierr = BVAllocateWork_Private(Y,X->k-X->l);CHKERRQ(ierr);
    s = Y->work;
  }
  ierr = MatDenseGetArray(Q,&q);CHKERRQ(ierr);
  for (j=Y->l;j<Y->k;j++) {
    ierr = VecScale(y->V[Y->nc+j],beta);CHKERRQ(ierr);
    if (alpha!=1.0) {
      for (i=X->l;i<X->k;i++) s[i-X->l] = alpha*q[i+j*ldq];
    } else s = q+j*ldq+X->l;
    ierr = VecMAXPY(y->V[Y->nc+j],X->k-X->l,s,x->V+X->nc+X->l);CHKERRQ(ierr);
  }
  ierr = MatDenseRestoreArray(Q,&q);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "BVMultVec_Vecs"
PetscErrorCode BVMultVec_Vecs(BV X,PetscScalar alpha,PetscScalar beta,Vec y,PetscScalar *q)
{
  PetscErrorCode ierr;
  BV_VECS        *x = (BV_VECS*)X->data;
  PetscScalar    *s=NULL;
  PetscInt       i;

  PetscFunctionBegin;
  if (alpha!=1.0) {
    ierr = BVAllocateWork_Private(X,X->k-X->l);CHKERRQ(ierr);
    s = X->work;
  }
  ierr = VecScale(y,beta);CHKERRQ(ierr);
  if (alpha!=1.0) {
    for (i=0;i<X->k-X->l;i++) s[i] = alpha*q[i];
  } else s = q;
  ierr = VecMAXPY(y,X->k-X->l,s,x->V+X->nc+X->l);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "BVMultInPlace_Vecs"
/*
   BVMultInPlace_Vecs - V(:,s:e-1) = V*Q(:,s:e-1) for regular vectors.

   Writing V = [ V1 V2 V3 ] and Q(:,s:e-1) = [ Q1 Q2 Q3 ]', where V2
   corresponds to the columns s:e-1, the computation is done as
                  V2 := V2*Q2 + V1*Q1 + V3*Q3
*/
PetscErrorCode BVMultInPlace_Vecs(BV V,Mat Q,PetscInt s,PetscInt e)
{
  PetscErrorCode ierr;
  BV_VECS        *ctx = (BV_VECS*)V->data;
  PetscScalar    *q;
  PetscInt       i,ldq;

  PetscFunctionBegin;
  ierr = MatGetSize(Q,&ldq,NULL);CHKERRQ(ierr);
  ierr = MatDenseGetArray(Q,&q);CHKERRQ(ierr);
  /* V2 := V2*Q2 */
  ierr = BVMultInPlace_Vecs_Private(V,V->n,e-s,V->k,ctx->V+V->nc+s,q+s*ldq+s,PETSC_FALSE);CHKERRQ(ierr);
  /* V2 += V1*Q1 + V3*Q3 */
  for (i=s;i<e;i++) {
    if (s>V->l) {
      ierr = VecMAXPY(ctx->V[V->nc+i],s-V->l,q+i*ldq+V->l,ctx->V+V->nc+V->l);CHKERRQ(ierr);
    }
    if (V->k>e) {
      ierr = VecMAXPY(ctx->V[V->nc+i],V->k-e,q+i*ldq+e,ctx->V+V->nc+e);CHKERRQ(ierr);
    }
  }
  ierr = MatDenseRestoreArray(Q,&q);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "BVMultInPlaceTranspose_Vecs"
/*
   BVMultInPlaceTranspose_Vecs - V(:,s:e-1) = V*Q'(:,s:e-1) for regular vectors.
*/
PetscErrorCode BVMultInPlaceTranspose_Vecs(BV V,Mat Q,PetscInt s,PetscInt e)
{
  PetscErrorCode ierr;
  BV_VECS        *ctx = (BV_VECS*)V->data;
  PetscScalar    *q;
  PetscInt       i,j,ldq,n;

  PetscFunctionBegin;
  ierr = MatGetSize(Q,&ldq,&n);CHKERRQ(ierr);
  ierr = MatDenseGetArray(Q,&q);CHKERRQ(ierr);
  /* V2 := V2*Q2' */
  ierr = BVMultInPlace_Vecs_Private(V,V->n,e-s,ldq,ctx->V+V->nc+s,q+s*ldq+s,PETSC_TRUE);CHKERRQ(ierr);
  /* V2 += V1*Q1' + V3*Q3' */
  for (i=s;i<e;i++) {
    for (j=V->l;j<s;j++) {
      ierr = VecAXPY(ctx->V[V->nc+i],q[i+j*ldq],ctx->V[V->nc+j]);CHKERRQ(ierr);
    }
    for (j=e;j<n;j++) {
      ierr = VecAXPY(ctx->V[V->nc+i],q[i+j*ldq],ctx->V[V->nc+j]);CHKERRQ(ierr);
    }
  }
  ierr = MatDenseRestoreArray(Q,&q);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "BVAXPY_Vecs"
PetscErrorCode BVAXPY_Vecs(BV Y,PetscScalar alpha,BV X)
{
  PetscErrorCode ierr;
  BV_VECS        *y = (BV_VECS*)Y->data,*x = (BV_VECS*)X->data;
  PetscInt       j;

  PetscFunctionBegin;
  for (j=0;j<Y->k-Y->l;j++) {
    ierr = VecAXPY(y->V[Y->nc+Y->l+j],alpha,x->V[X->nc+X->l+j]);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "BVDot_Vecs"
PetscErrorCode BVDot_Vecs(BV X,BV Y,Mat M)
{
  PetscErrorCode ierr;
  BV_VECS        *x = (BV_VECS*)X->data,*y = (BV_VECS*)Y->data;
  PetscScalar    *m;
  PetscInt       j,ldm;

  PetscFunctionBegin;
  ierr = MatGetSize(M,&ldm,NULL);CHKERRQ(ierr);
  ierr = MatDenseGetArray(M,&m);CHKERRQ(ierr);
  for (j=X->l;j<X->k;j++) {
    ierr = VecMDot(x->V[X->nc+j],Y->k-Y->l,y->V+Y->nc+Y->l,m+j*ldm+Y->l);CHKERRQ(ierr);
  }
  ierr = MatDenseRestoreArray(M,&m);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "BVDotVec_Vecs"
PetscErrorCode BVDotVec_Vecs(BV X,Vec y,PetscScalar *m)
{
  PetscErrorCode ierr;
  BV_VECS        *x = (BV_VECS*)X->data;
  Vec            z = y;

  PetscFunctionBegin;
  if (X->matrix) {
    ierr = BV_IPMatMult(X,y);CHKERRQ(ierr);
    z = X->Bx;
  }
  ierr = VecMDot(z,X->k-X->l,x->V+X->nc+X->l,m);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "BVScale_Vecs"
PetscErrorCode BVScale_Vecs(BV bv,PetscInt j,PetscScalar alpha)
{
  PetscErrorCode ierr;
  PetscInt       i;
  BV_VECS        *ctx = (BV_VECS*)bv->data;

  PetscFunctionBegin;
  if (j<0) {
    for (i=bv->l;i<bv->k;i++) {
      ierr = VecScale(ctx->V[bv->nc+i],alpha);CHKERRQ(ierr);
    }
  } else {
    ierr = VecScale(ctx->V[bv->nc+j],alpha);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "BVNorm_Vecs"
PetscErrorCode BVNorm_Vecs(BV bv,PetscInt j,NormType type,PetscReal *val)
{
  PetscErrorCode ierr;
  PetscInt       i;
  PetscReal      nrm;
  BV_VECS        *ctx = (BV_VECS*)bv->data;

  PetscFunctionBegin;
  if (j<0) {
    switch (type) {
    case NORM_FROBENIUS:
      *val = 0.0;
      for (i=bv->l;i<bv->k;i++) {
        ierr = VecNorm(ctx->V[bv->nc+i],NORM_2,&nrm);CHKERRQ(ierr);
        *val += nrm*nrm;
      }
      *val = PetscSqrtReal(*val);
      break;
    default:
      SETERRQ(PETSC_COMM_SELF,PETSC_ERR_SUP,"Requested norm not implemented in BVVECS");
    }
  } else {
    ierr = VecNorm(ctx->V[bv->nc+j],type,val);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "BVMatMult_Vecs"
PetscErrorCode BVMatMult_Vecs(BV V,Mat A,BV W)
{
  PetscErrorCode ierr;
  BV_VECS        *v = (BV_VECS*)V->data,*w = (BV_VECS*)W->data;
  PetscInt       j;

  PetscFunctionBegin;
  for (j=0;j<V->k-V->l;j++) {
    ierr = MatMult(A,v->V[V->nc+V->l+j],w->V[W->nc+W->l+j]);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "BVCopy_Vecs"
PetscErrorCode BVCopy_Vecs(BV V,BV W)
{
  PetscErrorCode ierr;
  BV_VECS        *v = (BV_VECS*)V->data,*w = (BV_VECS*)W->data;
  PetscInt       j;

  PetscFunctionBegin;
  for (j=0;j<V->k-V->l;j++) {
    ierr = VecCopy(v->V[V->nc+V->l+j],w->V[W->nc+W->l+j]);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "BVResize_Vecs"
PetscErrorCode BVResize_Vecs(BV bv,PetscInt m,PetscBool copy)
{
  PetscErrorCode ierr;
  BV_VECS        *ctx = (BV_VECS*)bv->data;
  Vec            *newV;
  PetscInt       j;
  char           str[50];

  PetscFunctionBegin;
  ierr = VecDuplicateVecs(bv->t,m,&newV);CHKERRQ(ierr);
  ierr = PetscLogObjectParents(bv,m,newV);CHKERRQ(ierr);
  if (((PetscObject)bv)->name) {
    for (j=0;j<m;j++) {
      ierr = PetscSNPrintf(str,50,"%s_%D",((PetscObject)bv)->name,j);CHKERRQ(ierr);
      ierr = PetscObjectSetName((PetscObject)newV[j],str);CHKERRQ(ierr);
    }
  }
  if (copy) {
    for (j=0;j<PetscMin(m,bv->m);j++) {
      ierr = VecCopy(ctx->V[j],newV[j]);CHKERRQ(ierr);
    }
  }
  ierr = VecDestroyVecs(bv->m,&ctx->V);CHKERRQ(ierr);
  ctx->V = newV;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "BVGetColumn_Vecs"
PetscErrorCode BVGetColumn_Vecs(BV bv,PetscInt j,Vec *v)
{
  BV_VECS  *ctx = (BV_VECS*)bv->data;
  PetscInt l;

  PetscFunctionBegin;
  l = BVAvailableVec;
  bv->cv[l] = ctx->V[bv->nc+j];
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "BVGetArray_Vecs"
PetscErrorCode BVGetArray_Vecs(BV bv,PetscScalar **a)
{
  PetscErrorCode ierr;
  BV_VECS        *ctx = (BV_VECS*)bv->data;
  PetscInt       j;
  PetscScalar    *p;

  PetscFunctionBegin;
  ierr = PetscMalloc((bv->nc+bv->m)*bv->n*sizeof(PetscScalar),a);CHKERRQ(ierr);
  for (j=0;j<bv->nc+bv->m;j++) {
    ierr = VecGetArray(ctx->V[j],&p);CHKERRQ(ierr);
    ierr = PetscMemcpy(*a+j*bv->n,p,bv->n*sizeof(PetscScalar));CHKERRQ(ierr);
    ierr = VecRestoreArray(ctx->V[j],&p);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "BVRestoreArray_Vecs"
PetscErrorCode BVRestoreArray_Vecs(BV bv,PetscScalar **a)
{
  PetscErrorCode ierr;
  BV_VECS        *ctx = (BV_VECS*)bv->data;
  PetscInt       j;
  PetscScalar    *p;

  PetscFunctionBegin;
  for (j=0;j<bv->nc+bv->m;j++) {
    ierr = VecGetArray(ctx->V[j],&p);CHKERRQ(ierr);
    ierr = PetscMemcpy(p,*a+j*bv->n,bv->n*sizeof(PetscScalar));CHKERRQ(ierr);
    ierr = VecRestoreArray(ctx->V[j],&p);CHKERRQ(ierr);
  }
  ierr = PetscFree(*a);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "BVView_Vecs"
PetscErrorCode BVView_Vecs(BV bv,PetscViewer viewer)
{
  PetscErrorCode    ierr;
  BV_VECS           *ctx = (BV_VECS*)bv->data;
  PetscInt          j;
  PetscViewerFormat format;
  PetscBool         isascii,ismatlab=PETSC_FALSE;

  PetscFunctionBegin;
  ierr = PetscObjectTypeCompare((PetscObject)viewer,PETSCVIEWERASCII,&isascii);CHKERRQ(ierr);
  if (isascii) {
    ierr = PetscViewerGetFormat(viewer,&format);CHKERRQ(ierr);
    if (format == PETSC_VIEWER_ASCII_MATLAB) ismatlab = PETSC_TRUE;
  }
  if (ismatlab) {
    ierr = PetscViewerASCIIPrintf(viewer,"%s=[];\n",((PetscObject)bv)->name);CHKERRQ(ierr);
  }
  for (j=bv->nc;j<bv->nc+bv->m;j++) {
    ierr = VecView(ctx->V[j],viewer);CHKERRQ(ierr);
    if (ismatlab) {
      ierr = PetscViewerASCIIPrintf(viewer,"%s=[%s,%s];clear %s\n",((PetscObject)bv)->name,((PetscObject)bv)->name,((PetscObject)ctx->V[j])->name,((PetscObject)ctx->V[j])->name);CHKERRQ(ierr);
    }
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "BVDestroy_Vecs"
PetscErrorCode BVDestroy_Vecs(BV bv)
{
  PetscErrorCode ierr;
  BV_VECS        *ctx = (BV_VECS*)bv->data;

  PetscFunctionBegin;
  ierr = VecDestroyVecs(bv->nc+bv->m,&ctx->V);CHKERRQ(ierr);
  ierr = PetscFree(bv->data);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "BVCreate_Vecs"
PETSC_EXTERN PetscErrorCode BVCreate_Vecs(BV bv)
{
  PetscErrorCode ierr;
  BV_VECS        *ctx;
  PetscInt       j;
  char           str[50];

  PetscFunctionBegin;
  ierr = PetscNewLog(bv,&ctx);CHKERRQ(ierr);
  bv->data = (void*)ctx;

  ierr = VecDuplicateVecs(bv->t,bv->m,&ctx->V);CHKERRQ(ierr);
  ierr = PetscLogObjectParents(bv,bv->m,ctx->V);CHKERRQ(ierr);
  if (((PetscObject)bv)->name) {
    for (j=0;j<bv->m;j++) {
      ierr = PetscSNPrintf(str,50,"%s_%D",((PetscObject)bv)->name,j);CHKERRQ(ierr);
      ierr = PetscObjectSetName((PetscObject)ctx->V[j],str);CHKERRQ(ierr);
    }
  }

  bv->ops->mult             = BVMult_Vecs;
  bv->ops->multvec          = BVMultVec_Vecs;
  bv->ops->multinplace      = BVMultInPlace_Vecs;
  bv->ops->multinplacetrans = BVMultInPlaceTranspose_Vecs;
  bv->ops->axpy             = BVAXPY_Vecs;
  bv->ops->dot              = BVDot_Vecs;
  bv->ops->dotvec           = BVDotVec_Vecs;
  bv->ops->scale            = BVScale_Vecs;
  bv->ops->norm             = BVNorm_Vecs;
  bv->ops->matmult          = BVMatMult_Vecs;
  bv->ops->copy             = BVCopy_Vecs;
  bv->ops->resize           = BVResize_Vecs;
  bv->ops->getcolumn        = BVGetColumn_Vecs;
  bv->ops->getarray         = BVGetArray_Vecs;
  bv->ops->restorearray     = BVRestoreArray_Vecs;
  bv->ops->destroy          = BVDestroy_Vecs;
  bv->ops->view             = BVView_Vecs;
  PetscFunctionReturn(0);
}

