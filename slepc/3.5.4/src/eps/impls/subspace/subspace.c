/*

   SLEPc eigensolver: "subspace"

   Method: Subspace Iteration

   Algorithm:

       Subspace iteration with Rayleigh-Ritz projection and locking,
       based on the SRRIT implementation.

   References:

       [1] "Subspace Iteration in SLEPc", SLEPc Technical Report STR-3,
           available at http://slepc.upv.es.

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

#include <slepc-private/epsimpl.h>

#undef __FUNCT__
#define __FUNCT__ "EPSSetUp_Subspace"
PetscErrorCode EPSSetUp_Subspace(EPS eps)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = EPSSetDimensions_Default(eps,eps->nev,&eps->ncv,&eps->mpd);CHKERRQ(ierr);
  if (!eps->max_it) eps->max_it = PetscMax(100,2*eps->n/eps->ncv);
  if (!eps->which) { ierr = EPSSetWhichEigenpairs_Default(eps);CHKERRQ(ierr); }
  if (eps->which!=EPS_LARGEST_MAGNITUDE && eps->which!=EPS_TARGET_MAGNITUDE) SETERRQ(PetscObjectComm((PetscObject)eps),1,"Wrong value of eps->which");
  if (!eps->extraction) {
    ierr = EPSSetExtraction(eps,EPS_RITZ);CHKERRQ(ierr);
  } else if (eps->extraction!=EPS_RITZ) SETERRQ(PetscObjectComm((PetscObject)eps),PETSC_ERR_SUP,"Unsupported extraction type");
  if (eps->arbitrary) SETERRQ(PetscObjectComm((PetscObject)eps),PETSC_ERR_SUP,"Arbitrary selection of eigenpairs not supported in this solver");

  ierr = EPSAllocateSolution(eps,0);CHKERRQ(ierr);
  ierr = EPS_SetInnerProduct(eps);CHKERRQ(ierr);
  if (eps->ishermitian) {
    ierr = DSSetType(eps->ds,DSHEP);CHKERRQ(ierr);
  } else {
    ierr = DSSetType(eps->ds,DSNHEP);CHKERRQ(ierr);
  }
  ierr = DSAllocate(eps->ds,eps->ncv);CHKERRQ(ierr);
  ierr = EPSSetWorkVecs(eps,1);CHKERRQ(ierr);

  if (eps->isgeneralized && eps->ishermitian && !eps->ispositive) SETERRQ(PetscObjectComm((PetscObject)eps),PETSC_ERR_SUP,"Requested method does not work for indefinite problems");
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "EPSSubspaceFindGroup"
/*
   EPSSubspaceFindGroup - Find a group of nearly equimodular eigenvalues, provided
   in arrays wr and wi, according to the tolerance grptol. Also the 2-norms
   of the residuals must be passed in (rsd). Arrays are processed from index
   l to index m only. The output information is:

   ngrp - number of entries of the group
   ctr  - (w(l)+w(l+ngrp-1))/2
   ae   - average of wr(l),...,wr(l+ngrp-1)
   arsd - average of rsd(l),...,rsd(l+ngrp-1)
*/
static PetscErrorCode EPSSubspaceFindGroup(PetscInt l,PetscInt m,PetscScalar *wr,PetscScalar *wi,PetscReal *rsd,PetscReal grptol,PetscInt *ngrp,PetscReal *ctr,PetscReal *ae,PetscReal *arsd)
{
  PetscInt  i;
  PetscReal rmod,rmod1;

  PetscFunctionBegin;
  *ngrp = 0;
  *ctr = 0;
  rmod = SlepcAbsEigenvalue(wr[l],wi[l]);

  for (i=l;i<m;) {
    rmod1 = SlepcAbsEigenvalue(wr[i],wi[i]);
    if (PetscAbsReal(rmod-rmod1) > grptol*(rmod+rmod1)) break;
    *ctr = (rmod+rmod1)/2.0;
    if (wi[i] != 0.0) {
      (*ngrp)+=2;
      i+=2;
    } else {
      (*ngrp)++;
      i++;
    }
  }

  *ae = 0;
  *arsd = 0;
  if (*ngrp) {
    for (i=l;i<l+*ngrp;i++) {
      (*ae) += PetscRealPart(wr[i]);
      (*arsd) += rsd[i]*rsd[i];
    }
    *ae = *ae / *ngrp;
    *arsd = PetscSqrtScalar(*arsd / *ngrp);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "EPSSubspaceResidualNorms"
/*
   EPSSubspaceResidualNorms - Computes the column norms of residual vectors
   OP*V(1:n,l:m) - V*T(1:m,l:m), where, on entry, OP*V has been computed and
   stored in AV. ldt is the leading dimension of T. On exit, rsd(l) to
   rsd(m) contain the computed norms.
*/
static PetscErrorCode EPSSubspaceResidualNorms(BV V,BV AV,PetscScalar *T,PetscInt l,PetscInt m,PetscInt ldt,Vec w,PetscReal *rsd)
{
  PetscErrorCode ierr;
  PetscInt       i,k;
  PetscScalar    t;

  PetscFunctionBegin;
  for (i=l;i<m;i++) {
    if (i==m-1 || T[i+1+ldt*i]==0.0) k=i+1;
    else k=i+2;
    ierr = BVSetActiveColumns(V,0,k);CHKERRQ(ierr);
    ierr = BVCopyVec(AV,i,w);CHKERRQ(ierr);
    ierr = BVMultVec(V,-1.0,1.0,w,T+ldt*i);CHKERRQ(ierr);
    ierr = VecDot(w,w,&t);CHKERRQ(ierr);
    rsd[i] = PetscRealPart(t);
  }
  for (i=l;i<m;i++) {
    if (i == m-1) {
      rsd[i] = PetscSqrtReal(rsd[i]);
    } else if (T[i+1+(ldt*i)]==0.0) {
      rsd[i] = PetscSqrtReal(rsd[i]);
    } else {
      rsd[i] = PetscSqrtReal((rsd[i]+rsd[i+1])/2.0);
      rsd[i+1] = rsd[i];
      i++;
    }
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "EPSSolve_Subspace"
PetscErrorCode EPSSolve_Subspace(EPS eps)
{
  PetscErrorCode ierr;
  Vec            v,av,w=eps->work[0];
  Mat            H,Q;
  BV             AV;
  PetscInt       i,k,ld,ngrp,nogrp,*itrsd,*itrsdold;
  PetscInt       nxtsrr,idsrr,idort,nxtort,nv,ncv = eps->ncv,its;
  PetscScalar    *T;
  PetscReal      arsd,oarsd,ctr,octr,ae,oae,*rsd,norm,tcond=1.0;
  PetscBool      breakdown;
  /* Parameters */
  PetscInt       init = 5;        /* Number of initial iterations */
  PetscReal      stpfac = 1.5;    /* Max num of iter before next SRR step */
  PetscReal      alpha = 1.0;     /* Used to predict convergence of next residual */
  PetscReal      beta = 1.1;      /* Used to predict convergence of next residual */
  PetscReal      grptol = 1e-8;   /* Tolerance for EPSSubspaceFindGroup */
  PetscReal      cnvtol = 1e-6;   /* Convergence criterion for cnv */
  PetscInt       orttol = 2;      /* Number of decimal digits whose loss
                                     can be tolerated in orthogonalization */

  PetscFunctionBegin;
  its = 0;
  ierr = PetscMalloc3(ncv,&rsd,ncv,&itrsd,ncv,&itrsdold);CHKERRQ(ierr);
  ierr = DSGetLeadingDimension(eps->ds,&ld);CHKERRQ(ierr);
  ierr = BVDuplicate(eps->V,&AV);CHKERRQ(ierr);

  for (i=0;i<ncv;i++) {
    rsd[i] = 0.0;
    itrsd[i] = -1;
  }

  /* Complete the initial basis with random vectors and orthonormalize them */
  k = eps->nini;
  while (k<ncv) {
    ierr = BVSetRandomColumn(eps->V,k,eps->rand);CHKERRQ(ierr);
    ierr = BVOrthogonalizeColumn(eps->V,k,NULL,&norm,&breakdown);CHKERRQ(ierr);
    if (norm>0.0 && !breakdown) {
      ierr = BVScaleColumn(eps->V,k,1.0/norm);CHKERRQ(ierr);
      k++;
    }
  }

  while (eps->its<eps->max_it) {
    eps->its++;
    nv = PetscMin(eps->nconv+eps->mpd,ncv);
    ierr = DSSetDimensions(eps->ds,nv,0,eps->nconv,0);CHKERRQ(ierr);

    /* Find group in previously computed eigenvalues */
    ierr = EPSSubspaceFindGroup(eps->nconv,nv,eps->eigr,eps->eigi,rsd,grptol,&nogrp,&octr,&oae,&oarsd);CHKERRQ(ierr);

    /* AV(:,idx) = OP * V(:,idx) */
    for (i=eps->nconv;i<nv;i++) {
      ierr = BVGetColumn(eps->V,i,&v);CHKERRQ(ierr);
      ierr = BVGetColumn(AV,i,&av);CHKERRQ(ierr);
      ierr = STApply(eps->st,v,av);CHKERRQ(ierr);
      ierr = BVRestoreColumn(eps->V,i,&v);CHKERRQ(ierr);
      ierr = BVRestoreColumn(AV,i,&av);CHKERRQ(ierr);
    }

    /* T(:,idx) = V' * AV(:,idx) */
    ierr = BVSetActiveColumns(eps->V,0,nv);CHKERRQ(ierr);
    ierr = BVSetActiveColumns(AV,eps->nconv,nv);CHKERRQ(ierr);
    ierr = DSGetMat(eps->ds,DS_MAT_A,&H);CHKERRQ(ierr);
    ierr = BVDot(AV,eps->V,H);CHKERRQ(ierr);
    ierr = DSRestoreMat(eps->ds,DS_MAT_A,&H);CHKERRQ(ierr);
    ierr = DSSetState(eps->ds,DS_STATE_RAW);CHKERRQ(ierr);

    /* Solve projected problem */
    ierr = DSSolve(eps->ds,eps->eigr,eps->eigi);CHKERRQ(ierr);
    ierr = DSSort(eps->ds,eps->eigr,eps->eigi,NULL,NULL,NULL);CHKERRQ(ierr);

    /* Update vectors V(:,idx) = V * U(:,idx) */
    ierr = DSGetMat(eps->ds,DS_MAT_Q,&Q);CHKERRQ(ierr);
    ierr = BVSetActiveColumns(AV,0,nv);CHKERRQ(ierr);
    ierr = BVMultInPlace(eps->V,Q,eps->nconv,nv);CHKERRQ(ierr);
    ierr = BVMultInPlace(AV,Q,eps->nconv,nv);CHKERRQ(ierr);
    ierr = MatDestroy(&Q);CHKERRQ(ierr);

    /* Convergence check */
    ierr = DSGetArray(eps->ds,DS_MAT_A,&T);CHKERRQ(ierr);
    ierr = EPSSubspaceResidualNorms(eps->V,AV,T,eps->nconv,nv,ld,w,rsd);CHKERRQ(ierr);
    ierr = DSRestoreArray(eps->ds,DS_MAT_A,&T);CHKERRQ(ierr);

    for (i=eps->nconv;i<nv;i++) {
      itrsdold[i] = itrsd[i];
      itrsd[i] = its;
      eps->errest[i] = rsd[i];
    }

    for (;;) {
      /* Find group in currently computed eigenvalues */
      ierr = EPSSubspaceFindGroup(eps->nconv,nv,eps->eigr,eps->eigi,eps->errest,grptol,&ngrp,&ctr,&ae,&arsd);CHKERRQ(ierr);
      if (ngrp!=nogrp) break;
      if (ngrp==0) break;
      if (PetscAbsScalar(ae-oae)>ctr*cnvtol*(itrsd[eps->nconv]-itrsdold[eps->nconv])) break;
      if (arsd>ctr*eps->tol) break;
      eps->nconv = eps->nconv + ngrp;
      if (eps->nconv>=nv) break;
    }

    ierr = EPSMonitor(eps,eps->its,eps->nconv,eps->eigr,eps->eigi,eps->errest,nv);CHKERRQ(ierr);
    if (eps->nconv>=eps->nev) break;

    /* Compute nxtsrr (iteration of next projection step) */
    nxtsrr = PetscMin(eps->max_it,PetscMax((PetscInt)PetscFloorReal(stpfac*its),init));

    if (ngrp!=nogrp || ngrp==0 || arsd>=oarsd) {
      idsrr = nxtsrr - its;
    } else {
      idsrr = (PetscInt)PetscFloorReal(alpha+beta*(itrsdold[eps->nconv]-itrsd[eps->nconv])*PetscLogReal(arsd/eps->tol)/PetscLogReal(arsd/oarsd));
      idsrr = PetscMax(1,idsrr);
    }
    nxtsrr = PetscMin(nxtsrr,its+idsrr);

    /* Compute nxtort (iteration of next orthogonalization step) */
    ierr = DSCond(eps->ds,&tcond);CHKERRQ(ierr);
    idort = PetscMax(1,(PetscInt)PetscFloorReal(orttol/PetscMax(1,PetscLog10Real(tcond))));
    nxtort = PetscMin(its+idort,nxtsrr);

    /* V(:,idx) = AV(:,idx) */
    ierr = BVSetActiveColumns(eps->V,eps->nconv,nv);CHKERRQ(ierr);
    ierr = BVSetActiveColumns(AV,eps->nconv,nv);CHKERRQ(ierr);
    ierr = BVCopy(AV,eps->V);CHKERRQ(ierr);
    its++;

    /* Orthogonalization loop */
    do {
      while (its<nxtort) {

        /* A(:,idx) = OP*V(:,idx) with normalization */
        for (i=eps->nconv;i<nv;i++) {
          ierr = BVGetColumn(eps->V,i,&v);CHKERRQ(ierr);
          ierr = STApply(eps->st,v,w);CHKERRQ(ierr);
          ierr = VecCopy(w,v);CHKERRQ(ierr);
          ierr = BVRestoreColumn(eps->V,i,&v);CHKERRQ(ierr);
          ierr = BVNormColumn(eps->V,i,NORM_INFINITY,&norm);CHKERRQ(ierr);
          ierr = BVScaleColumn(eps->V,i,1/norm);CHKERRQ(ierr);
        }
        its++;
      }
      /* Orthonormalize vectors */
      for (i=eps->nconv;i<nv;i++) {
        ierr = BVOrthogonalizeColumn(eps->V,i,NULL,&norm,&breakdown);CHKERRQ(ierr);
        if (breakdown) {
          ierr = BVSetRandomColumn(eps->V,i,eps->rand);CHKERRQ(ierr);
          ierr = BVOrthogonalizeColumn(eps->V,i,NULL,&norm,&breakdown);CHKERRQ(ierr);
        }
        ierr = BVScaleColumn(eps->V,i,1/norm);CHKERRQ(ierr);
      }
      nxtort = PetscMin(its+idort,nxtsrr);
    } while (its<nxtsrr);
  }

  ierr = PetscFree3(rsd,itrsd,itrsdold);CHKERRQ(ierr);

  if (eps->nconv == eps->nev) eps->reason = EPS_CONVERGED_TOL;
  else eps->reason = EPS_DIVERGED_ITS;
  ierr = BVDestroy(&AV);CHKERRQ(ierr);
  /* truncate Schur decomposition and change the state to raw so that
     PSVectors() computes eigenvectors from scratch */
  ierr = DSSetDimensions(eps->ds,eps->nconv,0,0,0);CHKERRQ(ierr);
  ierr = DSSetState(eps->ds,DS_STATE_RAW);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "EPSDestroy_Subspace"
PetscErrorCode EPSDestroy_Subspace(EPS eps)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscFree(eps->data);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "EPSCreate_Subspace"
PETSC_EXTERN PetscErrorCode EPSCreate_Subspace(EPS eps)
{
  PetscFunctionBegin;
  eps->ops->setup                = EPSSetUp_Subspace;
  eps->ops->solve                = EPSSolve_Subspace;
  eps->ops->destroy              = EPSDestroy_Subspace;
  eps->ops->backtransform        = EPSBackTransform_Default;
  eps->ops->computevectors       = EPSComputeVectors_Schur;
  PetscFunctionReturn(0);
}

