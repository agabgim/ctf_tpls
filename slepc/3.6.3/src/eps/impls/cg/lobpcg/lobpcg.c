/*

   SLEPc eigensolver: "lobpcg"

   Method: Locally Optimal Block Preconditioned Conjugate Gradient

   Algorithm:

       LOBPCG with soft and hard locking. Follows the implementation
       in BLOPEX [2].

   References:

       [1] A. V. Knyazev, "Toward the optimal preconditioned eigensolver:
           locally optimal block preconditioned conjugate gradient method",
           SIAM J. Sci. Comput. 23(2):517-541, 2001.

       [2] A. V. Knyazev et al., "Block Locally Optimal Preconditioned
           Eigenvalue Xolvers (BLOPEX) in Hypre and PETSc", SIAM J. Sci.
           Comput. 29(5):2224-2239, 2007.

   - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
   SLEPc - Scalable Library for Eigenvalue Problem Computations
   Copyright (c) 2002-2015, Universitat Politecnica de Valencia, Spain

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

#include <slepc/private/epsimpl.h>                /*I "slepceps.h" I*/
#include <slepc/private/dsimpl.h>                 /*I "slepcds.h" I*/

typedef struct {
  PetscInt  bs;     /* block size */
  PetscBool lock;   /* soft locking active/inactive */
} EPS_LOBPCG;

#undef __FUNCT__
#define __FUNCT__ "EPSSetDimensions_LOBPCG"
PetscErrorCode EPSSetDimensions_LOBPCG(EPS eps,PetscInt nev,PetscInt *ncv,PetscInt *mpd)
{
  EPS_LOBPCG *ctx = (EPS_LOBPCG*)eps->data;
  PetscInt   k;

  PetscFunctionBegin;
  k = PetscMax(3*ctx->bs,((eps->nev-1)/ctx->bs+3)*ctx->bs);
  if (*ncv) { /* ncv set */
    if (*ncv<k) SETERRQ(PetscObjectComm((PetscObject)eps),1,"The value of ncv is not sufficiently large");
  } else *ncv = k;
  if (!*mpd) *mpd = 3*ctx->bs;
  else if (*mpd!=3*ctx->bs) SETERRQ(PetscObjectComm((PetscObject)eps),1,"This solver does not allow a value of mpd different from 3*blocksize");
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "EPSSetUp_LOBPCG"
PetscErrorCode EPSSetUp_LOBPCG(EPS eps)
{
  PetscErrorCode ierr;
  EPS_LOBPCG     *ctx = (EPS_LOBPCG*)eps->data;
  PetscBool      precond,istrivial;

  PetscFunctionBegin;
  if (!eps->ishermitian || (eps->isgeneralized && !eps->ispositive)) SETERRQ(PetscObjectComm((PetscObject)eps),PETSC_ERR_SUP,"LOBPCG only works for Hermitian problems");
  if (!ctx->bs) ctx->bs = PetscMin(16,eps->nev);
  if (eps->n-eps->nds<5*ctx->bs) SETERRQ(PetscObjectComm((PetscObject)eps),PETSC_ERR_SUP,"The problem size is too small relative to the block size");
  ierr = EPSSetDimensions_LOBPCG(eps,eps->nev,&eps->ncv,&eps->mpd);CHKERRQ(ierr);
  if (!eps->max_it) eps->max_it = PetscMax(100,2*eps->n/eps->ncv);
  if (!eps->which) eps->which = EPS_SMALLEST_REAL;
  if (eps->which!=EPS_SMALLEST_REAL) SETERRQ(PetscObjectComm((PetscObject)eps),1,"Wrong value of eps->which");
  if (eps->arbitrary) SETERRQ(PetscObjectComm((PetscObject)eps),PETSC_ERR_SUP,"Arbitrary selection of eigenpairs not supported in this solver");
  if (eps->extraction) { ierr = PetscInfo(eps,"Warning: extraction type ignored\n");CHKERRQ(ierr); }
  ierr = RGIsTrivial(eps->rg,&istrivial);CHKERRQ(ierr);
  if (!istrivial) SETERRQ(PetscObjectComm((PetscObject)eps),PETSC_ERR_SUP,"This solver does not support region filtering");

  ierr = STSetUp(eps->st);CHKERRQ(ierr);
  ierr = PetscObjectTypeCompare((PetscObject)eps->st,STPRECOND,&precond);CHKERRQ(ierr);
  if (!precond) SETERRQ(PetscObjectComm((PetscObject)eps),PETSC_ERR_SUP,"LOBPCG only works with precond ST");

  ierr = EPSAllocateSolution(eps,0);CHKERRQ(ierr);
  ierr = EPS_SetInnerProduct(eps);CHKERRQ(ierr);
  ierr = DSSetType(eps->ds,DSGHEP);CHKERRQ(ierr);
  ierr = DSAllocate(eps->ds,eps->mpd);CHKERRQ(ierr);
  ierr = EPSSetWorkVecs(eps,1);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "EPSSolve_LOBPCG"
PetscErrorCode EPSSolve_LOBPCG(EPS eps)
{
  PetscErrorCode ierr;
  EPS_LOBPCG     *ctx = (EPS_LOBPCG*)eps->data;
  PetscInt       i,j,k,ld,nv,ini,kini,nmat,nc,nconv,bdone,its;
  PetscReal      norm;
  PetscBool      breakdown,countc;
  Mat            A,B,M;
  Vec            v,w=eps->work[0];
  BV             X,Y,Z,R,P,AX,BX;

  PetscFunctionBegin;
  ierr = DSGetLeadingDimension(eps->ds,&ld);CHKERRQ(ierr);
  ierr = STGetNumMatrices(eps->st,&nmat);CHKERRQ(ierr);
  ierr = STGetOperators(eps->st,0,&A);CHKERRQ(ierr);
  if (nmat>1) { ierr = STGetOperators(eps->st,1,&B);CHKERRQ(ierr); }
  else B = NULL;

  /* 1. Allocate memory */
  ierr = BVDuplicateResize(eps->V,3*ctx->bs,&Z);CHKERRQ(ierr);
  ierr = BVDuplicateResize(eps->V,ctx->bs,&X);CHKERRQ(ierr);
  ierr = BVDuplicateResize(eps->V,ctx->bs,&R);CHKERRQ(ierr);
  ierr = BVDuplicateResize(eps->V,ctx->bs,&P);CHKERRQ(ierr);
  ierr = BVDuplicateResize(eps->V,ctx->bs,&AX);CHKERRQ(ierr);
  if (B) {
    ierr = BVDuplicateResize(eps->V,ctx->bs,&BX);CHKERRQ(ierr);
  }
  nc = eps->nds;
  if (nc>0 || eps->nev>ctx->bs) {
    ierr = BVDuplicateResize(eps->V,nc+eps->nev,&Y);CHKERRQ(ierr);
  }
  if (nc>0) {
    for (j=0;j<nc;j++) {
      ierr = BVGetColumn(eps->V,-nc+j,&v);CHKERRQ(ierr);
      ierr = BVInsertVec(Y,j,v);CHKERRQ(ierr);
      ierr = BVRestoreColumn(eps->V,-nc+j,&v);CHKERRQ(ierr);
    }
    ierr = BVSetActiveColumns(Y,0,nc);CHKERRQ(ierr);
  }

  /* 2. Apply the constraints to the initial vectors */
  kini = eps->nini;
  while (kini<eps->ncv-2*ctx->bs) { /* Generate more initial vectors if necessary */
    ierr = BVSetRandomColumn(eps->V,kini,eps->rand);CHKERRQ(ierr);
    ierr = BVOrthogonalizeColumn(eps->V,kini,NULL,&norm,&breakdown);CHKERRQ(ierr);
    if (norm>0.0 && !breakdown) {
      ierr = BVScaleColumn(eps->V,kini,1.0/norm);CHKERRQ(ierr);
      kini++;
    }
  }
  nv = ctx->bs;
  ierr = BVSetActiveColumns(eps->V,0,nv);CHKERRQ(ierr);
  ierr = BVSetActiveColumns(Z,0,nv);CHKERRQ(ierr);
  ierr = BVCopy(eps->V,Z);CHKERRQ(ierr);
  ierr = BVCopy(Z,X);CHKERRQ(ierr);

  /* 3. B-orthogonalize initial vectors */
  if (B) {
    ierr = BVMatMult(X,B,BX);CHKERRQ(ierr);
  }

  /* 4. Compute initial Ritz vectors */
  ierr = BVMatMult(X,A,AX);CHKERRQ(ierr);
  ierr = DSSetDimensions(eps->ds,nv,0,0,0);CHKERRQ(ierr);
  ierr = DSGetMat(eps->ds,DS_MAT_A,&M);CHKERRQ(ierr);
  ierr = BVMatProject(AX,NULL,X,M);CHKERRQ(ierr);
  ierr = DSRestoreMat(eps->ds,DS_MAT_A,&M);CHKERRQ(ierr);
  ierr = DSSetIdentity(eps->ds,DS_MAT_B);CHKERRQ(ierr);
  ierr = DSSetState(eps->ds,DS_STATE_RAW);CHKERRQ(ierr);
  ierr = DSSolve(eps->ds,eps->eigr,eps->eigi);CHKERRQ(ierr);
  ierr = DSSort(eps->ds,eps->eigr,eps->eigi,NULL,NULL,NULL);CHKERRQ(ierr);
  ierr = DSVectors(eps->ds,DS_MAT_X,NULL,NULL);CHKERRQ(ierr);
  ierr = DSGetMat(eps->ds,DS_MAT_X,&M);CHKERRQ(ierr);
  ierr = BVMultInPlace(X,M,0,nv);CHKERRQ(ierr);
  ierr = BVMultInPlace(AX,M,0,nv);CHKERRQ(ierr);
  ierr = DSRestoreMat(eps->ds,DS_MAT_X,&M);CHKERRQ(ierr);

  /* 5. Initialize range of active iterates */
  bdone = 0;  /* finished blocks, the leading bdone*bs columns of V are eigenvectors */
  nconv = 0;  /* number of converged eigenvalues in the current block */
  its   = 0;  /* iterations for the current block */

  /* 6. Main loop */
  while (eps->reason == EPS_CONVERGED_ITERATING) {

    if (ctx->lock) {
      ierr = BVSetActiveColumns(R,nconv,ctx->bs);CHKERRQ(ierr);
      ierr = BVSetActiveColumns(AX,nconv,ctx->bs);CHKERRQ(ierr);
      if (B) {
        ierr = BVSetActiveColumns(BX,nconv,ctx->bs);CHKERRQ(ierr);
      }
    }

    /* 7. Compute residuals */
    ierr = DSGetMat(eps->ds,DS_MAT_A,&M);CHKERRQ(ierr);
    ierr = BVCopy(AX,R);CHKERRQ(ierr);
    if (B) {
      ierr = BVMult(R,-1.0,1.0,BX,M);CHKERRQ(ierr);
    } else {
      ierr = BVMult(R,-1.0,1.0,X,M);CHKERRQ(ierr);
    }
    ierr = DSRestoreMat(eps->ds,DS_MAT_A,&M);CHKERRQ(ierr);

    /* 8. Compute residual norms and update index set of active iterates */
    ini = (ctx->lock)? nconv: 0;
    k = ini;
    countc = PETSC_TRUE;
    for (j=ini;j<ctx->bs;j++) {
      i = bdone*ctx->bs+j;
      ierr = BVGetColumn(R,j,&v);CHKERRQ(ierr);
      ierr = VecNorm(v,NORM_2,&norm);CHKERRQ(ierr);
      ierr = BVRestoreColumn(R,j,&v);CHKERRQ(ierr);
      ierr = (*eps->converged)(eps,eps->eigr[i],eps->eigi[i],norm,&eps->errest[i],eps->convergedctx);CHKERRQ(ierr);
      if (countc) {
        if (eps->errest[i] < eps->tol) k++;
        else countc = PETSC_FALSE;
      }
      if (!countc && !eps->trackall) break;
    }
    nconv = k;
    eps->nconv = bdone*ctx->bs + nconv;
    if (eps->its+its) {
      ierr = EPSMonitor(eps,eps->its+its,eps->nconv,eps->eigr,eps->eigi,eps->errest,(bdone+1)*ctx->bs);CHKERRQ(ierr);
    }
    if (eps->nconv >= eps->nev) eps->reason = EPS_CONVERGED_TOL;
    else if (eps->its+its >= eps->max_it) {
      eps->its += its;
      eps->reason = EPS_DIVERGED_ITS;
    }
    if (eps->reason != EPS_CONVERGED_ITERATING || nconv == ctx->bs) {
      ierr = BVSetActiveColumns(eps->V,bdone*ctx->bs,eps->nconv);CHKERRQ(ierr);
      ierr = BVSetActiveColumns(Z,0,nconv);CHKERRQ(ierr);
      ierr = BVSetActiveColumns(X,0,nconv);CHKERRQ(ierr);
      ierr = BVCopy(X,eps->V);CHKERRQ(ierr);
      ierr = BVCopy(X,Z);CHKERRQ(ierr);
    }
    if (eps->reason != EPS_CONVERGED_ITERATING) {
      eps->its += its;
      break;
    } else if (nconv == ctx->bs) {
      eps->its += its;
      its = 0;
    }
    its++;

    if (nconv == ctx->bs) {  /* block finished: lock eigenvectors and compute new R */

      /* extend constraints */
      ierr = BVSetActiveColumns(Y,nc+bdone*ctx->bs,nc+(bdone+1)*ctx->bs);CHKERRQ(ierr);
      ierr = BVCopy(Z,Y);CHKERRQ(ierr);
      for (j=0;j<ctx->bs;j++) {
        ierr = BVOrthogonalizeColumn(Y,nc+bdone*ctx->bs+j,NULL,&norm,&breakdown);CHKERRQ(ierr);
        if (norm>0.0 && !breakdown) {
          ierr = BVScaleColumn(Y,nc+bdone*ctx->bs+j,1.0/norm);CHKERRQ(ierr);
        } else SETERRQ(PetscObjectComm((PetscObject)eps),1,"Orthogonalization of constraints failed");
      }
      ierr = BVSetActiveColumns(Y,0,nc+(bdone+1)*ctx->bs);CHKERRQ(ierr);

      bdone++;
      nconv = 0;

      /* set new initial vectors */
      ierr = BVSetActiveColumns(eps->V,bdone*ctx->bs,(bdone+1)*ctx->bs);CHKERRQ(ierr);
      ierr = BVCopy(eps->V,Z);CHKERRQ(ierr);
      for (j=0;j<ctx->bs;j++) {
        ierr = BVGetColumn(Z,j,&v);CHKERRQ(ierr);
        ierr = BVOrthogonalizeVec(Y,v,NULL,NULL,NULL);CHKERRQ(ierr);
        ierr = VecNormalize(v,NULL);CHKERRQ(ierr);
        ierr = BVRestoreColumn(Z,j,&v);CHKERRQ(ierr);
      }
      ierr = BVSetActiveColumns(X,nconv,ctx->bs);CHKERRQ(ierr);
      ierr = BVSetActiveColumns(AX,nconv,ctx->bs);CHKERRQ(ierr);
      ierr = BVCopy(Z,X);CHKERRQ(ierr);

      /* B-orthogonalize initial vectors */
      if (B) {
        ierr = BVOrthogonalize(X,NULL);CHKERRQ(ierr);
      }

      /* Compute initial Ritz vectors */
      nv = ctx->bs;
      ierr = BVMatMult(X,A,AX);CHKERRQ(ierr);
      ierr = DSSetDimensions(eps->ds,nv,0,0,0);CHKERRQ(ierr);
      ierr = DSGetMat(eps->ds,DS_MAT_A,&M);CHKERRQ(ierr);
      ierr = BVMatProject(AX,NULL,X,M);CHKERRQ(ierr);
      ierr = DSRestoreMat(eps->ds,DS_MAT_A,&M);CHKERRQ(ierr);
      ierr = DSSetIdentity(eps->ds,DS_MAT_B);CHKERRQ(ierr);
      ierr = DSSetState(eps->ds,DS_STATE_RAW);CHKERRQ(ierr);
      ierr = DSSolve(eps->ds,eps->eigr+bdone*ctx->bs,eps->eigi);CHKERRQ(ierr);
      ierr = DSSort(eps->ds,eps->eigr+bdone*ctx->bs,eps->eigi,NULL,NULL,NULL);CHKERRQ(ierr);
      ierr = DSVectors(eps->ds,DS_MAT_X,NULL,NULL);CHKERRQ(ierr);
      ierr = DSGetMat(eps->ds,DS_MAT_X,&M);CHKERRQ(ierr);
      ierr = BVMultInPlace(X,M,0,nv);CHKERRQ(ierr);
      ierr = BVMultInPlace(AX,M,0,nv);CHKERRQ(ierr);
      ierr = DSRestoreMat(eps->ds,DS_MAT_X,&M);CHKERRQ(ierr);

      ierr = EPSMonitor(eps,eps->its+its-1,eps->nconv,eps->eigr,eps->eigi,eps->errest,(bdone+1)*ctx->bs);CHKERRQ(ierr);

      if (ctx->lock) {
        ierr = BVSetActiveColumns(R,nconv,ctx->bs);CHKERRQ(ierr);
        ierr = BVSetActiveColumns(AX,nconv,ctx->bs);CHKERRQ(ierr);
        if (B) {
          ierr = BVSetActiveColumns(BX,nconv,ctx->bs);CHKERRQ(ierr);
        }
      }

      /* compute residuals */
      ierr = DSGetMat(eps->ds,DS_MAT_A,&M);CHKERRQ(ierr);
      ierr = BVCopy(AX,R);CHKERRQ(ierr);
      if (B) {
        ierr = BVMatMult(X,B,BX);CHKERRQ(ierr);
        ierr = BVMult(R,-1.0,1.0,BX,M);CHKERRQ(ierr);
      } else {
        ierr = BVMult(R,-1.0,1.0,X,M);CHKERRQ(ierr);
      }
      ierr = DSRestoreMat(eps->ds,DS_MAT_A,&M);CHKERRQ(ierr);
    }

    ini = (ctx->lock)? nconv: 0;
    if (ctx->lock) {
      ierr = BVSetActiveColumns(R,nconv,ctx->bs);CHKERRQ(ierr);
      ierr = BVSetActiveColumns(P,nconv,ctx->bs);CHKERRQ(ierr);
      ierr = BVSetActiveColumns(AX,nconv,ctx->bs);CHKERRQ(ierr);
      if (B) {
        ierr = BVSetActiveColumns(BX,nconv,ctx->bs);CHKERRQ(ierr);
      }
    }

    /* 9. Apply preconditioner to the residuals */
    for (j=ini;j<ctx->bs;j++) {
      ierr = BVGetColumn(R,j,&v);CHKERRQ(ierr);
      ierr = STMatSolve(eps->st,v,w);CHKERRQ(ierr);
      if (nc+bdone*ctx->bs>0) {
        ierr = BVOrthogonalizeVec(Y,w,NULL,NULL,NULL);CHKERRQ(ierr);
        ierr = VecNormalize(w,NULL);CHKERRQ(ierr);
      }
      ierr = VecCopy(w,v);CHKERRQ(ierr);
      ierr = BVRestoreColumn(R,j,&v);CHKERRQ(ierr);
    }

    /* 11. B-orthonormalize preconditioned residuals */
    ierr = BVOrthogonalize(R,NULL);CHKERRQ(ierr);

    /* 13-16. B-orthonormalize conjugate directions */
    if (its>1) {
      ierr = BVOrthogonalize(P,NULL);CHKERRQ(ierr);
    }

    /* 17-23. Compute symmetric Gram matrices */
    ierr = BVSetActiveColumns(Z,0,ctx->bs);CHKERRQ(ierr);
    ierr = BVSetActiveColumns(X,0,ctx->bs);CHKERRQ(ierr);
    ierr = BVCopy(X,Z);CHKERRQ(ierr);
    ierr = BVSetActiveColumns(Z,ctx->bs,2*ctx->bs-ini);CHKERRQ(ierr);
    ierr = BVCopy(R,Z);CHKERRQ(ierr);
    if (its>1) {
      ierr = BVSetActiveColumns(Z,2*ctx->bs-ini,3*ctx->bs-2*ini);CHKERRQ(ierr);
      ierr = BVCopy(P,Z);CHKERRQ(ierr);
    }

    if (its>1) nv = 3*ctx->bs-2*ini;
    else nv = 2*ctx->bs-ini;

    ierr = BVSetActiveColumns(Z,0,nv);CHKERRQ(ierr);
    ierr = DSSetDimensions(eps->ds,nv,0,0,0);CHKERRQ(ierr);
    ierr = DSGetMat(eps->ds,DS_MAT_A,&M);CHKERRQ(ierr);
    ierr = BVMatProject(Z,A,Z,M);CHKERRQ(ierr);
    ierr = DSRestoreMat(eps->ds,DS_MAT_A,&M);CHKERRQ(ierr);
    ierr = DSGetMat(eps->ds,DS_MAT_B,&M);CHKERRQ(ierr);
    if (B) {
      ierr = BVMatProject(Z,B,Z,M);CHKERRQ(ierr);
    } else {
      ierr = BVDot(Z,Z,M);CHKERRQ(ierr);
    }
    ierr = DSRestoreMat(eps->ds,DS_MAT_B,&M);CHKERRQ(ierr);
    
    /* 24. Solve the generalized eigenvalue problem */
    ierr = DSSetState(eps->ds,DS_STATE_RAW);CHKERRQ(ierr);
    ierr = DSSolve(eps->ds,eps->eigr+bdone*ctx->bs,eps->eigi);CHKERRQ(ierr);
    ierr = DSSort(eps->ds,eps->eigr+bdone*ctx->bs,eps->eigi,NULL,NULL,NULL);CHKERRQ(ierr);
    ierr = DSVectors(eps->ds,DS_MAT_X,NULL,NULL);CHKERRQ(ierr);
    
    /* 25-33. Compute Ritz vectors */
    ierr = DSGetMat(eps->ds,DS_MAT_X,&M);CHKERRQ(ierr);
    ierr = BVSetActiveColumns(Z,ctx->bs,nv);CHKERRQ(ierr);
    if (ctx->lock) {
      ierr = BVSetActiveColumns(P,0,ctx->bs);CHKERRQ(ierr);
    }
    ierr = BVMult(P,1.0,0.0,Z,M);CHKERRQ(ierr);
    ierr = BVCopy(P,X);CHKERRQ(ierr);
    if (ctx->lock) {
      ierr = BVSetActiveColumns(P,nconv,ctx->bs);CHKERRQ(ierr);
    }
    ierr = BVSetActiveColumns(Z,0,ctx->bs);CHKERRQ(ierr);
    ierr = BVMult(X,1.0,1.0,Z,M);CHKERRQ(ierr);
    if (ctx->lock) {
      ierr = BVSetActiveColumns(X,nconv,ctx->bs);CHKERRQ(ierr);
    }
    ierr = BVMatMult(X,A,AX);CHKERRQ(ierr);
    if (B) {
      ierr = BVMatMult(X,B,BX);CHKERRQ(ierr);
    }
    ierr = DSRestoreMat(eps->ds,DS_MAT_X,&M);CHKERRQ(ierr);
  }

  ierr = BVDestroy(&Z);CHKERRQ(ierr);
  ierr = BVDestroy(&X);CHKERRQ(ierr);
  ierr = BVDestroy(&R);CHKERRQ(ierr);
  ierr = BVDestroy(&P);CHKERRQ(ierr);
  ierr = BVDestroy(&AX);CHKERRQ(ierr);
  if (B) {
    ierr = BVDestroy(&BX);CHKERRQ(ierr);
  }
  if (nc>0 || eps->nev>ctx->bs) {
    ierr = BVDestroy(&Y);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "EPSLOBPCGSetBlockSize_LOBPCG"
static PetscErrorCode EPSLOBPCGSetBlockSize_LOBPCG(EPS eps,PetscInt bs)
{
  EPS_LOBPCG *ctx = (EPS_LOBPCG*)eps->data;

  PetscFunctionBegin;
  ctx->bs = bs;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "EPSLOBPCGSetBlockSize"
/*@
   EPSLOBPCGSetBlockSize - Sets the block size of the LOBPCG method.

   Logically Collective on EPS

   Input Parameters:
+  eps - the eigenproblem solver context
-  bs  - the block size

   Options Database Key:
.  -eps_lobpcg_blocksize - Sets the block size

   Level: advanced

.seealso: EPSLOBPCGGetBlockSize()
@*/
PetscErrorCode EPSLOBPCGSetBlockSize(EPS eps,PetscInt bs)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(eps,EPS_CLASSID,1);
  PetscValidLogicalCollectiveInt(eps,bs,2);
  ierr = PetscTryMethod(eps,"EPSLOBPCGSetBlockSize_C",(EPS,PetscInt),(eps,bs));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "EPSLOBPCGGetBlockSize_LOBPCG"
static PetscErrorCode EPSLOBPCGGetBlockSize_LOBPCG(EPS eps,PetscInt *bs)
{
  EPS_LOBPCG *ctx = (EPS_LOBPCG*)eps->data;

  PetscFunctionBegin;
  *bs = ctx->bs;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "EPSLOBPCGGetBlockSize"
/*@
   EPSLOBPCGGetBlockSize - Gets the block size used in the LOBPCG method.

   Not Collective

   Input Parameter:
.  eps - the eigenproblem solver context

   Output Parameter:
.  bs - the block size

   Level: advanced

.seealso: EPSLOBPCGSetBlockSize()
@*/
PetscErrorCode EPSLOBPCGGetBlockSize(EPS eps,PetscInt *bs)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(eps,EPS_CLASSID,1);
  PetscValidPointer(bs,2);
  ierr = PetscTryMethod(eps,"EPSLOBPCGGetBlockSize_C",(EPS,PetscInt*),(eps,bs));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "EPSLOBPCGSetLocking_LOBPCG"
static PetscErrorCode EPSLOBPCGSetLocking_LOBPCG(EPS eps,PetscBool lock)
{
  EPS_LOBPCG *ctx = (EPS_LOBPCG*)eps->data;

  PetscFunctionBegin;
  ctx->lock = lock;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "EPSLOBPCGSetLocking"
/*@
   EPSLOBPCGSetLocking - Choose between locking and non-locking variants of
   the LOBPCG method.

   Logically Collective on EPS

   Input Parameters:
+  eps  - the eigenproblem solver context
-  lock - true if the locking variant must be selected

   Options Database Key:
.  -eps_lobpcg_locking - Sets the locking flag

   Notes:
   This flag refers to soft locking (converged vectors within the current
   block iterate), since hard locking is always used (when nev is larger
   than the block size).

   Level: advanced

.seealso: EPSLOBPCGGetLocking()
@*/
PetscErrorCode EPSLOBPCGSetLocking(EPS eps,PetscBool lock)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(eps,EPS_CLASSID,1);
  PetscValidLogicalCollectiveBool(eps,lock,2);
  ierr = PetscTryMethod(eps,"EPSLOBPCGSetLocking_C",(EPS,PetscBool),(eps,lock));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "EPSLOBPCGGetLocking_LOBPCG"
static PetscErrorCode EPSLOBPCGGetLocking_LOBPCG(EPS eps,PetscBool *lock)
{
  EPS_LOBPCG *ctx = (EPS_LOBPCG*)eps->data;

  PetscFunctionBegin;
  *lock = ctx->lock;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "EPSLOBPCGGetLocking"
/*@
   EPSLOBPCGGetLocking - Gets the locking flag used in the LOBPCG method.

   Not Collective

   Input Parameter:
.  eps - the eigenproblem solver context

   Output Parameter:
.  lock - the locking flag

   Level: advanced

.seealso: EPSLOBPCGSetLocking()
@*/
PetscErrorCode EPSLOBPCGGetLocking(EPS eps,PetscBool *lock)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(eps,EPS_CLASSID,1);
  PetscValidPointer(lock,2);
  ierr = PetscTryMethod(eps,"EPSLOBPCGGetLocking_C",(EPS,PetscBool*),(eps,lock));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "EPSView_LOBPCG"
PetscErrorCode EPSView_LOBPCG(EPS eps,PetscViewer viewer)
{
  PetscErrorCode ierr;
  EPS_LOBPCG     *ctx = (EPS_LOBPCG*)eps->data;
  PetscBool      isascii;

  PetscFunctionBegin;
  ierr = PetscObjectTypeCompare((PetscObject)viewer,PETSCVIEWERASCII,&isascii);CHKERRQ(ierr);
  if (isascii) {
    ierr = PetscViewerASCIIPrintf(viewer,"  LOBPCG: block size %D\n",ctx->bs);CHKERRQ(ierr);
    ierr = PetscViewerASCIIPrintf(viewer,"  LOBPCG: soft locking %sactivated\n",ctx->lock?"":"de");CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "EPSSetFromOptions_LOBPCG"
PetscErrorCode EPSSetFromOptions_LOBPCG(PetscOptions *PetscOptionsObject,EPS eps)
{
  PetscErrorCode ierr;
  PetscBool      lock,flg;
  PetscInt       bs;
  KSP            ksp;

  PetscFunctionBegin;
  ierr = PetscOptionsHead(PetscOptionsObject,"EPS LOBPCG Options");CHKERRQ(ierr);
  ierr = PetscOptionsInt("-eps_lobpcg_blocksize","LOBPCG block size","EPSLOBPCGSetBlockSize",20,&bs,&flg);CHKERRQ(ierr);
  if (flg) {
    ierr = EPSLOBPCGSetBlockSize(eps,bs);CHKERRQ(ierr);
  }
  ierr = PetscOptionsBool("-eps_lobpcg_locking","Choose between locking and non-locking variants","EPSLOBPCGSetLocking",PETSC_TRUE,&lock,&flg);CHKERRQ(ierr);
  if (flg) {
    ierr = EPSLOBPCGSetLocking(eps,lock);CHKERRQ(ierr);
  }

  /* Set STPrecond as the default ST */
  if (!((PetscObject)eps->st)->type_name) {
    ierr = STSetType(eps->st,STPRECOND);CHKERRQ(ierr);
  }

  /* Set the default options of the KSP */
  ierr = STGetKSP(eps->st,&ksp);CHKERRQ(ierr);
  if (!((PetscObject)ksp)->type_name) {
    ierr = KSPSetType(ksp,KSPPREONLY);CHKERRQ(ierr);
  }
  ierr = PetscOptionsTail();CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "EPSDestroy_LOBPCG"
PetscErrorCode EPSDestroy_LOBPCG(EPS eps)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscFree(eps->data);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)eps,"EPSLOBPCGSetBlockSize_C",NULL);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)eps,"EPSLOBPCGGetBlockSize_C",NULL);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)eps,"EPSLOBPCGSetLocking_C",NULL);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)eps,"EPSLOBPCGGetLocking_C",NULL);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "EPSCreate_LOBPCG"
PETSC_EXTERN PetscErrorCode EPSCreate_LOBPCG(EPS eps)
{
  EPS_LOBPCG     *lobpcg;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscNewLog(eps,&lobpcg);CHKERRQ(ierr);
  eps->data = (void*)lobpcg;
  lobpcg->lock = PETSC_TRUE;

  eps->ops->setup          = EPSSetUp_LOBPCG;
  eps->ops->solve          = EPSSolve_LOBPCG;
  eps->ops->setfromoptions = EPSSetFromOptions_LOBPCG;
  eps->ops->destroy        = EPSDestroy_LOBPCG;
  eps->ops->view           = EPSView_LOBPCG;
  eps->ops->backtransform  = EPSBackTransform_Default;
  ierr = STSetType(eps->st,STPRECOND);CHKERRQ(ierr);
  ierr = STPrecondSetKSPHasMat(eps->st,PETSC_TRUE);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)eps,"EPSLOBPCGSetBlockSize_C",EPSLOBPCGSetBlockSize_LOBPCG);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)eps,"EPSLOBPCGGetBlockSize_C",EPSLOBPCGGetBlockSize_LOBPCG);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)eps,"EPSLOBPCGSetLocking_C",EPSLOBPCGSetLocking_LOBPCG);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)eps,"EPSLOBPCGGetLocking_C",EPSLOBPCGGetLocking_LOBPCG);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

