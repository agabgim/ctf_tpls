/*
  SLEPc eigensolver: "davidson"

  Step: calc the best eigenpairs in the subspace V.

  For that, performs these steps:
    1) Update W <- A * V
    2) Update H <- V' * W
    3) Obtain eigenpairs of H
    4) Select some eigenpairs
    5) Compute the Ritz pairs of the selected ones

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

#include "davidson.h"
#include <slepcblaslapack.h>

static PetscErrorCode dvd_calcpairs_proj(dvdDashboard *d);
static PetscErrorCode dvd_calcpairs_qz_start(dvdDashboard *d);
static PetscErrorCode dvd_calcpairs_qz_d(dvdDashboard *d);
static PetscErrorCode dvd_calcpairs_projeig_solve(dvdDashboard *d);
static PetscErrorCode dvd_calcpairs_selectPairs(dvdDashboard *d,PetscInt n);
static PetscErrorCode dvd_calcpairs_res_0(dvdDashboard *d,PetscInt r_s,PetscInt r_e);
static PetscErrorCode dvd_calcpairs_proj_res(dvdDashboard *d,PetscInt r_s,PetscInt r_e,Vec *R);
static PetscErrorCode dvd_calcpairs_updateproj(dvdDashboard *d);
static PetscErrorCode dvd_calcpairs_apply_arbitrary(dvdDashboard *d,PetscInt r_s,PetscInt r_e,PetscScalar *rr,PetscScalar *ri);
static PetscErrorCode EPSXDUpdateProj(Mat Q,Mat Z,PetscInt l,Mat A,PetscInt lA,PetscInt kA,Mat aux);
PETSC_STATIC_INLINE PetscErrorCode dvd_calcpairs_updateBV0_gen(dvdDashboard *d,BV bv,DSMatType MT);
static PetscErrorCode EPSXDComputeDSConv(dvdDashboard *d);

/**** Control routines ********************************************************/
#undef __FUNCT__
#define __FUNCT__ "dvd_calcpairs_qz"
PetscErrorCode dvd_calcpairs_qz(dvdDashboard *d,dvdBlackboard *b,PetscBool borth,PetscInt cX_proj,PetscBool harm)
{
  PetscErrorCode ierr;
  PetscBool      std_probl,her_probl,ind_probl,her_ind_probl;
  DSType         dstype;
  Vec            v1;

  PetscFunctionBegin;
  std_probl = DVD_IS(d->sEP, DVD_EP_STD)?PETSC_TRUE:PETSC_FALSE;
  her_probl = DVD_IS(d->sEP, DVD_EP_HERMITIAN)?PETSC_TRUE:PETSC_FALSE;
  ind_probl = DVD_IS(d->sEP, DVD_EP_INDEFINITE)?PETSC_TRUE:PETSC_FALSE;
  her_ind_probl = (her_probl || ind_probl)? PETSC_TRUE:PETSC_FALSE;

  /* Setting configuration constrains */
  b->max_size_proj = PetscMax(b->max_size_proj, b->max_size_V+cX_proj);
  d->W_shift = d->B?PETSC_TRUE:PETSC_FALSE;
  if (d->B && her_ind_probl && !borth) d->BV_shift = PETSC_TRUE;
  else d->BV_shift = PETSC_FALSE;

  /* Setup the step */
  if (b->state >= DVD_STATE_CONF) {
    d->max_cX_in_proj = cX_proj;
    d->max_size_P = b->max_size_P;
    d->max_size_proj = b->max_size_proj;
    /* Create a DS if the method works with Schur decompositions */
    d->calcPairs = dvd_calcpairs_proj;
    d->calcpairs_residual = dvd_calcpairs_res_0;
    d->calcpairs_proj_res = dvd_calcpairs_proj_res;
    d->calcpairs_selectPairs = dvd_calcpairs_selectPairs;
    /* Create and configure a DS for solving the projected problems */
    if (d->W) {    /* If we use harmonics */
      dstype = DSGNHEP;
    } else {
      if (ind_probl) {
        dstype = DSGHIEP;
      } else if (std_probl) {
        dstype = her_probl ? DSHEP : DSNHEP;
      } else {
        dstype = her_probl ? DSGHEP : DSGNHEP;
      }
    }
    ierr = DSSetType(d->eps->ds,dstype);CHKERRQ(ierr);
    ierr = DSAllocate(d->eps->ds,d->eps->ncv);CHKERRQ(ierr);
    /* Create various vector basis */
    if (harm) {
      ierr = BVDuplicateResize(d->eps->V,d->eps->ncv,&d->W);CHKERRQ(ierr);
      ierr = BVSetMatrix(d->W,NULL,PETSC_FALSE);CHKERRQ(ierr);
    } else d->W = NULL;
    ierr = BVDuplicateResize(d->eps->V,d->eps->ncv,&d->AX);CHKERRQ(ierr);
    ierr = BVSetMatrix(d->AX,NULL,PETSC_FALSE);CHKERRQ(ierr);
    ierr = BVDuplicateResize(d->eps->V,d->eps->ncv,&d->auxBV);CHKERRQ(ierr);
    ierr = BVSetMatrix(d->auxBV,NULL,PETSC_FALSE);CHKERRQ(ierr);
    if (d->B) {
      ierr = BVDuplicateResize(d->eps->V,d->eps->ncv,&d->BX);CHKERRQ(ierr);
      ierr = BVSetMatrix(d->BX,NULL,PETSC_FALSE);CHKERRQ(ierr);
    } else d->BX = NULL;
    ierr = MatGetVecs(d->A,&v1,NULL);CHKERRQ(ierr);
    ierr = SlepcVecPoolCreate(v1,0,&d->auxV);CHKERRQ(ierr);
    ierr = VecDestroy(&v1);CHKERRQ(ierr);
    /* Create projected problem matrices */
    ierr = MatCreateSeqDense(PETSC_COMM_SELF,d->eps->ncv,d->eps->ncv,NULL,&d->H);CHKERRQ(ierr);
    if (!std_probl) {
      ierr = MatCreateSeqDense(PETSC_COMM_SELF,d->eps->ncv,d->eps->ncv,NULL,&d->G);CHKERRQ(ierr);
    } else d->G = NULL;
    if (her_probl) {
      ierr = MatSetOption(d->H,MAT_HERMITIAN,PETSC_TRUE);CHKERRQ(ierr);
      if (d->G) {ierr = MatSetOption(d->G,MAT_HERMITIAN,PETSC_TRUE);CHKERRQ(ierr);}
    }

    if (ind_probl) {
      ierr = PetscMalloc1(d->eps->ncv,&d->nBds);CHKERRQ(ierr);
    } else d->nBds = NULL;
    ierr = MatCreateSeqDense(PETSC_COMM_SELF,d->eps->ncv,d->eps->ncv,NULL,&d->auxM);CHKERRQ(ierr);

    ierr = EPSDavidsonFLAdd(&d->startList,dvd_calcpairs_qz_start);CHKERRQ(ierr);
    ierr = EPSDavidsonFLAdd(&d->endList,EPSXDComputeDSConv);CHKERRQ(ierr);
    ierr = EPSDavidsonFLAdd(&d->destroyList,dvd_calcpairs_qz_d);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "dvd_calcpairs_qz_start"
static PetscErrorCode dvd_calcpairs_qz_start(dvdDashboard *d)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = BVSetActiveColumns(d->eps->V,0,0);CHKERRQ(ierr);
  if (d->W) { ierr = BVSetActiveColumns(d->W,0,0);CHKERRQ(ierr); }
  ierr = BVSetActiveColumns(d->AX,0,0);CHKERRQ(ierr);
  if (d->BX) { ierr = BVSetActiveColumns(d->BX,0,0);CHKERRQ(ierr); }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "dvd_calcpairs_qz_d"
static PetscErrorCode dvd_calcpairs_qz_d(dvdDashboard *d)
{
  PetscErrorCode  ierr;

  PetscFunctionBegin;
  ierr = BVDestroy(&d->W);CHKERRQ(ierr);
  ierr = BVDestroy(&d->AX);CHKERRQ(ierr);
  ierr = BVDestroy(&d->BX);CHKERRQ(ierr);
  ierr = BVDestroy(&d->auxBV);CHKERRQ(ierr);
  ierr = MatDestroy(&d->H);CHKERRQ(ierr);
  if (d->G) {ierr = MatDestroy(&d->G);CHKERRQ(ierr);}
  ierr = MatDestroy(&d->auxM);CHKERRQ(ierr);
  ierr = SlepcVecPoolDestroy(&d->auxV);CHKERRQ(ierr);
  ierr = PetscFree(d->nBds);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "dvd_calcpairs_proj"
static PetscErrorCode dvd_calcpairs_proj(dvdDashboard *d)
{
  PetscErrorCode ierr;
  PetscInt       i,l,k;
  Vec            v1,v2;
  PetscScalar    *pv;

  PetscFunctionBegin;
  ierr = BVGetActiveColumns(d->eps->V,&l,&k);CHKERRQ(ierr);
  /* Update AV, BV, W and the projected matrices */
  /* 1. S <- S*MT */
  if (d->V_tra_s != d->V_tra_e) {
    ierr = dvd_calcpairs_updateBV0_gen(d,d->eps->V,DS_MAT_Q);CHKERRQ(ierr);
    if (d->W) {ierr = dvd_calcpairs_updateBV0_gen(d,d->W,DS_MAT_Z);CHKERRQ(ierr);}
    ierr = dvd_calcpairs_updateBV0_gen(d,d->AX,DS_MAT_Q);CHKERRQ(ierr);
    if (d->BX) {ierr = dvd_calcpairs_updateBV0_gen(d,d->BX,DS_MAT_Q);CHKERRQ(ierr);}
    ierr = dvd_calcpairs_updateproj(d);CHKERRQ(ierr);
    /* Update signature */
    if (d->nBds) {
      ierr = VecCreateSeq(PETSC_COMM_SELF,l+d->V_tra_e,&v1);CHKERRQ(ierr);
      ierr = BVSetActiveColumns(d->eps->V,0,l+d->V_tra_e);CHKERRQ(ierr);
      ierr = BVGetSignature(d->eps->V,v1);CHKERRQ(ierr);
      ierr = VecGetArray(v1,&pv);CHKERRQ(ierr);
      for (i=0; i<d->V_tra_e; ++i) pv[l+i] = d->nBds[i];
      ierr = VecRestoreArray(v1,&pv);CHKERRQ(ierr);
      ierr = BVSetSignature(d->eps->V,v1);CHKERRQ(ierr);
      ierr = BVSetActiveColumns(d->eps->V,l,k);CHKERRQ(ierr);
      ierr = VecDestroy(&v1);CHKERRQ(ierr);
    }
    k = l+d->V_tra_e;
    l+= d->V_tra_s;
  } else {
    /* 2. V <- orth(V, V_new) */
    ierr = dvd_orthV(d->eps->V,l+d->V_new_s,l+d->V_new_e,d->eps->rand);CHKERRQ(ierr);
    /* 3. AV <- [AV A * V(V_new_s:V_new_e-1)] */
    /* Check consistency */
    if (k-l != d->V_new_s) SETERRQ(PETSC_COMM_SELF,1, "Consistency broken");
    for (i=l+d->V_new_s; i<l+d->V_new_e; i++) {
      ierr = BVGetColumn(d->eps->V,i,&v1);CHKERRQ(ierr);
      ierr = BVGetColumn(d->AX,i,&v2);CHKERRQ(ierr);
      ierr = MatMult(d->A,v1,v2);CHKERRQ(ierr);
      ierr = BVRestoreColumn(d->eps->V,i,&v1);CHKERRQ(ierr);
      ierr = BVRestoreColumn(d->AX,i,&v2);CHKERRQ(ierr);
    }
    /* 4. BV <- [BV B * V(V_new_s:V_new_e-1)] */
    if (d->BX) {
      /* Check consistency */
      if (k-l != d->V_new_s) SETERRQ(PETSC_COMM_SELF,1, "Consistency broken");
      for (i=l+d->V_new_s; i<l+d->V_new_e; i++) {
        ierr = BVGetColumn(d->eps->V,i,&v1);CHKERRQ(ierr);
        ierr = BVGetColumn(d->BX,i,&v2);CHKERRQ(ierr);
        ierr = MatMult(d->B,v1,v2);CHKERRQ(ierr);
        ierr = BVRestoreColumn(d->eps->V,i,&v1);CHKERRQ(ierr);
        ierr = BVRestoreColumn(d->BX,i,&v2);CHKERRQ(ierr);
      }
    }
    /* 5. W <- [W f(AV,BV)] */
    if (d->W) {
      ierr = d->calcpairs_W(d);CHKERRQ(ierr);
      ierr = dvd_orthV(d->W,l+d->V_new_s,l+d->V_new_e,d->eps->rand);CHKERRQ(ierr);
    }
    /* 6. H <- W' * AX; G <- W' * BX */
    ierr = BVSetActiveColumns(d->eps->V,l+d->V_new_s,l+d->V_new_e);CHKERRQ(ierr);
    ierr = BVSetActiveColumns(d->AX,l+d->V_new_s,l+d->V_new_e);CHKERRQ(ierr);
    if (d->BX) {ierr = BVSetActiveColumns(d->BX,l+d->V_new_s,l+d->V_new_e);CHKERRQ(ierr);}
    if (d->W) {ierr = BVSetActiveColumns(d->W,l+d->V_new_s,l+d->V_new_e);CHKERRQ(ierr);}
    ierr = BVMatProject(d->AX,NULL,d->W?d->W:d->eps->V,d->H);CHKERRQ(ierr);
    if (d->G) {ierr = BVMatProject(d->BX?d->BX:d->eps->V,NULL,d->W?d->W:d->eps->V,d->G);CHKERRQ(ierr);}
    ierr = BVSetActiveColumns(d->eps->V,l,k);CHKERRQ(ierr);
    ierr = BVSetActiveColumns(d->AX,l,k);CHKERRQ(ierr);
    if (d->BX) {ierr = BVSetActiveColumns(d->BX,l,k);CHKERRQ(ierr);}
    if (d->W) {ierr = BVSetActiveColumns(d->W,l,k);CHKERRQ(ierr);}

    /* Perform the transformation on the projected problem */
    if (d->W) {
      ierr = d->calcpairs_proj_trans(d);CHKERRQ(ierr);
    }
    k = l+d->V_new_e;
  }
  ierr = BVSetActiveColumns(d->eps->V,l,k);CHKERRQ(ierr);
  ierr = BVSetActiveColumns(d->AX,l,k);CHKERRQ(ierr);
  if (d->BX) {ierr = BVSetActiveColumns(d->BX,l,k);CHKERRQ(ierr);}
  if (d->W) {ierr = BVSetActiveColumns(d->W,l,k);CHKERRQ(ierr);}


  /* Solve the projected problem */
  ierr = dvd_calcpairs_projeig_solve(d);CHKERRQ(ierr);

  d->V_tra_s = d->V_tra_e = 0;
  d->V_new_s = d->V_new_e;
  PetscFunctionReturn(0);
}

/**** Basic routines **********************************************************/

#undef __FUNCT__
#define __FUNCT__ "dvd_calcpairs_updateproj"
static PetscErrorCode dvd_calcpairs_updateproj(dvdDashboard *d)
{
  PetscErrorCode  ierr;
  Mat             Q,Z;
  PetscInt        lV,kV;
  PetscBool       symm;

  PetscFunctionBegin;
  ierr = DSGetMat(d->eps->ds,DS_MAT_Q,&Q);CHKERRQ(ierr);
  if (d->W) {ierr = DSGetMat(d->eps->ds,DS_MAT_Z,&Z);CHKERRQ(ierr);}
  else Z = Q;
  ierr = BVGetActiveColumns(d->eps->V,&lV,&kV);CHKERRQ(ierr);
  ierr = EPSXDUpdateProj(Q,Z,0,d->H,lV,lV+d->V_tra_e,d->auxM);CHKERRQ(ierr);
  if (d->G) {ierr = EPSXDUpdateProj(Q,Z,0,d->G,lV,lV+d->V_tra_e,d->auxM);CHKERRQ(ierr);}
  ierr = DSRestoreMat(d->eps->ds,DS_MAT_Q,&Q);CHKERRQ(ierr);
  if (d->W) {ierr = DSRestoreMat(d->eps->ds,DS_MAT_Z,&Z);CHKERRQ(ierr);}

  ierr = PetscObjectTypeCompareAny((PetscObject)d->eps->ds,&symm,DSHEP,DSGHIEP,"");CHKERRQ(ierr);
  if (d->V_tra_s==0 || symm) PetscFunctionReturn(0);
  /* Compute upper part of H (and G): H(0:l-1,l:k-1) <- W(0:l-1)' * AV(l:k-1), where
     k=l+d->V_tra_s */
  ierr = BVSetActiveColumns(d->W?d->W:d->eps->V,0,lV);CHKERRQ(ierr);
  ierr = BVSetActiveColumns(d->AX,lV,lV+d->V_tra_s);CHKERRQ(ierr);
  ierr = BVDot(d->AX,d->W?d->W:d->eps->V,d->H);CHKERRQ(ierr);
  if (d->G) {
    ierr = BVSetActiveColumns(d->BX?d->BX:d->eps->V,lV,lV+d->V_tra_s);CHKERRQ(ierr);
    ierr = BVDot(d->BX?d->BX:d->eps->V,d->W?d->W:d->eps->V,d->G);CHKERRQ(ierr);
  }
  ierr = PetscObjectTypeCompareAny((PetscObject)d->eps->ds,&symm,DSGHEP,"");CHKERRQ(ierr);
  if (!symm) {
    ierr = BVSetActiveColumns(d->W?d->W:d->eps->V,lV,lV+d->V_tra_s);CHKERRQ(ierr);
    ierr = BVSetActiveColumns(d->AX,0,lV);CHKERRQ(ierr);
    ierr = BVDot(d->AX,d->W?d->W:d->eps->V,d->H);CHKERRQ(ierr);
    if (d->G) {
      ierr = BVSetActiveColumns(d->BX?d->BX:d->eps->V,0,lV);CHKERRQ(ierr);
      ierr = BVDot(d->BX?d->BX:d->eps->V,d->W?d->W:d->eps->V,d->G);CHKERRQ(ierr);
    }
  }
  ierr = BVSetActiveColumns(d->eps->V,lV,kV);CHKERRQ(ierr);
  ierr = BVSetActiveColumns(d->AX,lV,kV);CHKERRQ(ierr);
  if (d->BX) {ierr = BVSetActiveColumns(d->BX,lV,kV);CHKERRQ(ierr);}
  if (d->W) {ierr = BVSetActiveColumns(d->W,lV,kV);CHKERRQ(ierr);}
  if (d->W) {ierr = dvd_harm_updateproj(d);CHKERRQ(ierr);}
  PetscFunctionReturn(0);
}

/* in complex, d->size_H real auxiliar values are needed */
#undef __FUNCT__
#define __FUNCT__ "dvd_calcpairs_projeig_solve"
static PetscErrorCode dvd_calcpairs_projeig_solve(dvdDashboard *d)
{
  PetscErrorCode  ierr;
  Mat             A;
  Vec             v;
  PetscScalar     *pA,*pv;
  PetscInt        i,lV,kV,n,ld;

  PetscFunctionBegin;
  ierr = BVGetActiveColumns(d->eps->V,&lV,&kV);CHKERRQ(ierr);
  n = kV-lV;
  ierr = DSSetDimensions(d->eps->ds,n,0,0,0);CHKERRQ(ierr);
  ierr = DSGetMat(d->eps->ds,DS_MAT_A,&A);CHKERRQ(ierr);
  ierr = SlepcMatDenseCopy(d->H,lV,lV,A,0,0,n,n);CHKERRQ(ierr);
  ierr = DSRestoreMat(d->eps->ds,DS_MAT_A,&A);CHKERRQ(ierr);
  if (d->G) {
    ierr = DSGetMat(d->eps->ds,DS_MAT_B,&A);CHKERRQ(ierr);
    ierr = SlepcMatDenseCopy(d->G,lV,lV,A,0,0,n,n);CHKERRQ(ierr);
    ierr = DSRestoreMat(d->eps->ds,DS_MAT_B,&A);CHKERRQ(ierr);
  }
  /* Set the signature on projected matrix B */
  if (DVD_IS(d->sEP,DVD_EP_INDEFINITE)) {
    ierr = DSGetLeadingDimension(d->eps->ds,&ld);CHKERRQ(ierr);
    ierr = DSGetArray(d->eps->ds,DS_MAT_B,&pA);CHKERRQ(ierr);
    ierr = PetscMemzero(pA,sizeof(PetscScalar)*n*ld);CHKERRQ(ierr);
    ierr = VecCreateSeq(PETSC_COMM_SELF,kV,&v);CHKERRQ(ierr);
    ierr = BVGetSignature(d->eps->V,v);CHKERRQ(ierr);
    ierr = VecGetArray(v,&pv);CHKERRQ(ierr);
    for (i=0; i<n; i++) {
      pA[i+ld*i] = d->nBds[i] = PetscRealPart(pv[lV+i]);
    }
    ierr = VecRestoreArray(v,&pv);CHKERRQ(ierr);
    ierr = VecDestroy(&v);CHKERRQ(ierr);
    ierr = DSRestoreArray(d->eps->ds,DS_MAT_B,&pA);CHKERRQ(ierr);
  }
  ierr = DSSetState(d->eps->ds,DS_STATE_RAW);CHKERRQ(ierr);
  ierr = DSSolve(d->eps->ds,d->eigr,d->eigi);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "dvd_calcpairs_apply_arbitrary"
static PetscErrorCode dvd_calcpairs_apply_arbitrary(dvdDashboard *d,PetscInt r_s,PetscInt r_e,PetscScalar *rr,PetscScalar *ri)
{
  PetscInt        i,k,ld;
  PetscScalar     *pX;
  Vec             *X,xr,xi;
  PetscErrorCode  ierr;
#if defined(PETSC_USE_COMPLEX)
  PetscInt        N=1;
#else
  PetscInt        N=2,j;
#endif

  PetscFunctionBegin;
  /* Quick exit without neither arbitrary selection nor harmonic extraction */
  if (!d->eps->arbitrary && !d->calcpairs_eig_backtrans) PetscFunctionReturn(0);

  /* Quick exit without arbitrary selection, but with harmonic extraction */
  if (d->calcpairs_eig_backtrans) {
    for (i=r_s; i<r_e; i++) {
      ierr = d->calcpairs_eig_backtrans(d,d->eigr[i],d->eigi[i],&rr[i-r_s],&ri[i-r_s]);CHKERRQ(ierr);
    }
  }
  if (!d->eps->arbitrary) PetscFunctionReturn(0);

  ierr = SlepcVecPoolGetVecs(d->auxV,N,&X);CHKERRQ(ierr);
  ierr = DSGetLeadingDimension(d->eps->ds,&ld);CHKERRQ(ierr);
  for (i=r_s; i<r_e; i++) {
    k = i;
    ierr = DSVectors(d->eps->ds,DS_MAT_X,&k,NULL);CHKERRQ(ierr);
    ierr = DSNormalize(d->eps->ds,DS_MAT_X,i);CHKERRQ(ierr);
    ierr = DSGetArray(d->eps->ds,DS_MAT_X,&pX);CHKERRQ(ierr);
    ierr = dvd_improvex_compute_X(d,i,k+1,X,pX,ld);CHKERRQ(ierr);
    ierr = DSRestoreArray(d->eps->ds,DS_MAT_X,&pX);CHKERRQ(ierr);
#if !defined(PETSC_USE_COMPLEX)
    if (d->nX[i] != 1.0) {
      for (j=i; j<k+1; j++) {
        ierr = VecScale(X[j-i],1.0/d->nX[i]);CHKERRQ(ierr);
      }
    }
    xr = X[0];
    xi = X[1];
    if (i == k) {
      ierr = VecZeroEntries(xi);CHKERRQ(ierr);
    }
#else
    xr = X[0];
    xi = NULL;
    if (d->nX[i] != 1.0) {
      ierr = VecScale(xr,1.0/d->nX[i]);CHKERRQ(ierr);
    }
#endif
    ierr = (d->eps->arbitrary)(rr[i-r_s],ri[i-r_s],xr,xi,&rr[i-r_s],&ri[i-r_s],d->eps->arbitraryctx);CHKERRQ(ierr);
#if !defined(PETSC_USE_COMPLEX)
    if (i != k) {
      rr[i+1-r_s] = rr[i-r_s];
      ri[i+1-r_s] = ri[i-r_s];
      i++;
    }
#endif
  }
  ierr = SlepcVecPoolRestoreVecs(d->auxV,N,&X);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "dvd_calcpairs_selectPairs"
static PetscErrorCode dvd_calcpairs_selectPairs(dvdDashboard *d,PetscInt n)
{
  PetscInt        k,lV,kV,nV;
  PetscScalar     *rr,*ri;
  PetscErrorCode  ierr;

  PetscFunctionBegin;
  ierr = BVGetActiveColumns(d->eps->V,&lV,&kV);CHKERRQ(ierr);
  nV = kV - lV; 
  n = PetscMin(n,nV);
  /* Put the best n pairs at the beginning. Useful for restarting */
  if (d->eps->arbitrary || d->calcpairs_eig_backtrans) {
    ierr = PetscMalloc1(nV,&rr);CHKERRQ(ierr);
    ierr = PetscMalloc1(nV,&ri);CHKERRQ(ierr);
    ierr = dvd_calcpairs_apply_arbitrary(d,0,nV,rr,ri);CHKERRQ(ierr);
  } else {
    rr = d->eigr;
    ri = d->eigi;
  }
  k = n;
  ierr = DSSort(d->eps->ds,d->eigr,d->eigi,rr,ri,&k);CHKERRQ(ierr);
  /* Put the best pair at the beginning. Useful to check its residual */
#if !defined(PETSC_USE_COMPLEX)
  if (n != 1 && (n != 2 || d->eigi[0] == 0.0))
#else
  if (n != 1)
#endif
  {
    ierr = dvd_calcpairs_apply_arbitrary(d,0,nV,rr,ri);CHKERRQ(ierr);
    k = 1;
    ierr = DSSort(d->eps->ds,d->eigr,d->eigi,rr,ri,&k);CHKERRQ(ierr);
  }
  if (d->calcpairs_eigs_trans) {
    ierr = d->calcpairs_eigs_trans(d);CHKERRQ(ierr);
  }
  if (d->eps->arbitrary || d->calcpairs_eig_backtrans) {
    ierr = PetscFree(rr);CHKERRQ(ierr);
    ierr = PetscFree(ri);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "EPSXDComputeDSConv"
static PetscErrorCode EPSXDComputeDSConv(dvdDashboard *d)
{
  PetscErrorCode ierr;
  PetscInt       i,ld;
  Mat            A;
  Vec            v;
  PetscScalar    *pA,*pv;
  PetscBool      symm;

  PetscFunctionBegin;
  ierr = BVSetActiveColumns(d->eps->V,0,d->eps->nconv);CHKERRQ(ierr);
  ierr = PetscObjectTypeCompareAny((PetscObject)d->eps->ds,&symm,DSHEP,"");CHKERRQ(ierr);
  if (symm) PetscFunctionReturn(0);
  ierr = DSSetDimensions(d->eps->ds,d->eps->nconv,0,0,0);CHKERRQ(ierr);
  ierr = DSGetMat(d->eps->ds,DS_MAT_A,&A);CHKERRQ(ierr);
  ierr = SlepcMatDenseCopy(d->H,0,0,A,0,0,d->eps->nconv,d->eps->nconv);CHKERRQ(ierr);
  ierr = DSRestoreMat(d->eps->ds,DS_MAT_A,&A);CHKERRQ(ierr);
  if (d->G) {
    ierr = DSGetMat(d->eps->ds,DS_MAT_B,&A);CHKERRQ(ierr);
    ierr = SlepcMatDenseCopy(d->G,0,0,A,0,0,d->eps->nconv,d->eps->nconv);CHKERRQ(ierr);
    ierr = DSRestoreMat(d->eps->ds,DS_MAT_B,&A);CHKERRQ(ierr);
  }
  /* Set the signature on projected matrix B */
  if (DVD_IS(d->sEP,DVD_EP_INDEFINITE)) {
    ierr = DSGetLeadingDimension(d->eps->ds,&ld);CHKERRQ(ierr);
    ierr = DSGetArray(d->eps->ds,DS_MAT_B,&pA);CHKERRQ(ierr);
    ierr = PetscMemzero(pA,sizeof(PetscScalar)*d->eps->nconv*ld);CHKERRQ(ierr);
    ierr = VecCreateSeq(PETSC_COMM_SELF,d->eps->nconv,&v);CHKERRQ(ierr);
    ierr = BVGetSignature(d->eps->V,v);CHKERRQ(ierr);
    ierr = VecGetArray(v,&pv);CHKERRQ(ierr);
    for (i=0; i<d->eps->nconv; i++) {
      pA[i+ld*i] = pv[i];
    }
    ierr = VecRestoreArray(v,&pv);CHKERRQ(ierr);
    ierr = VecDestroy(&v);CHKERRQ(ierr);
    ierr = DSRestoreArray(d->eps->ds,DS_MAT_B,&pA);CHKERRQ(ierr);
  }
  ierr = DSSetState(d->eps->ds,DS_STATE_RAW);CHKERRQ(ierr);
  ierr = DSSolve(d->eps->ds,d->eps->eigr,d->eps->eigi);CHKERRQ(ierr);
  if (d->W) {
    for (i=0; i<d->eps->nconv; i++) {
      ierr = d->calcpairs_eig_backtrans(d,d->eps->eigr[i],d->eps->eigi[i],&d->eps->eigr[i],&d->eps->eigi[i]);CHKERRQ(ierr);
    }
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "dvd_calcpairs_res_0"
/* Compute the residual vectors R(i) <- (AV - BV*eigr(i))*pX(i), and also
   the norm associated to the Schur pair, where i = r_s..r_e
*/
static PetscErrorCode dvd_calcpairs_res_0(dvdDashboard *d,PetscInt r_s,PetscInt r_e)
{
  PetscInt        i,ldpX;
  PetscScalar     *pX;
  PetscErrorCode  ierr;
  BV              BX = d->BX?d->BX:d->eps->V;
  Vec             *R;

  PetscFunctionBegin;
  ierr = DSGetLeadingDimension(d->eps->ds,&ldpX);CHKERRQ(ierr);
  ierr = DSGetArray(d->eps->ds,DS_MAT_Q,&pX);CHKERRQ(ierr);
  /* nX(i) <- ||X(i)|| */
  ierr = dvd_improvex_compute_X(d,r_s,r_e,NULL,pX,ldpX);CHKERRQ(ierr);
  ierr = SlepcVecPoolGetVecs(d->auxV,r_e-r_s,&R);CHKERRQ(ierr);
  for (i=r_s; i<r_e; i++) {
    /* R(i-r_s) <- AV*pX(i) */
    ierr = BVMultVec(d->AX,1.0,0.0,R[i-r_s],&pX[ldpX*i]);CHKERRQ(ierr);
    /* R(i-r_s) <- R(i-r_s) - eigr(i)*BX*pX(i) */
    ierr = BVMultVec(BX,-d->eigr[i],1.0,R[i-r_s],&pX[ldpX*i]);CHKERRQ(ierr);
  }
  ierr = DSRestoreArray(d->eps->ds,DS_MAT_Q,&pX);CHKERRQ(ierr);
  ierr = d->calcpairs_proj_res(d,r_s,r_e,R);CHKERRQ(ierr);
  ierr = SlepcVecPoolRestoreVecs(d->auxV,r_e-r_s,&R);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "dvd_calcpairs_proj_res"
static PetscErrorCode dvd_calcpairs_proj_res(dvdDashboard *d,PetscInt r_s,PetscInt r_e,Vec *R)
{
  PetscInt        i,l,k;
  PetscErrorCode  ierr;
  PetscBool       lindep;
  BV              cX;

  PetscFunctionBegin;
  /* If exists left subspace, R <- orth(cY, R), nR[i] <- ||R[i]|| */
  if (d->W) cX = d->W;

  /* If not HEP, R <- orth(cX, R), nR[i] <- ||R[i]|| */
  else if (!(DVD_IS(d->sEP, DVD_EP_STD) && DVD_IS(d->sEP, DVD_EP_HERMITIAN))) cX = d->eps->V;

  /* Otherwise, nR[i] <- ||R[i]|| */
  else cX = NULL;

  if (cX) {
    ierr = BVGetActiveColumns(cX,&l,&k);CHKERRQ(ierr);
    ierr = BVSetActiveColumns(cX,0,l);CHKERRQ(ierr);
    for (i=0; i<r_e-r_s; i++) {
      ierr = BVOrthogonalizeVec(cX,R[i],NULL,&d->nR[r_s+i],&lindep);CHKERRQ(ierr);
    }
    ierr = BVSetActiveColumns(cX,l,k);CHKERRQ(ierr);
    if (lindep || (PetscAbs(d->nR[r_s+i]) < PETSC_MACHINE_EPSILON)) {
      ierr = PetscInfo2(d->eps,"The computed eigenvector residual %D is too low, %g!\n",r_s+i,(double)(d->nR[r_s+i]));CHKERRQ(ierr);
    }
  } else {
    for (i=0;i<r_e-r_s;i++) {
      ierr = VecNormBegin(R[i],NORM_2,&d->nR[r_s+i]);CHKERRQ(ierr);
    }
    for (i=0;i<r_e-r_s;i++) {
      ierr = VecNormEnd(R[i],NORM_2,&d->nR[r_s+i]);CHKERRQ(ierr);
    }
  }
  PetscFunctionReturn(0);
}

/**** Pattern routines ********************************************************/

/* BV <- BV*MT */
#undef __FUNCT__
#define __FUNCT__ "dvd_calcpairs_updateBV0_gen"
PETSC_STATIC_INLINE PetscErrorCode dvd_calcpairs_updateBV0_gen(dvdDashboard *d,BV bv,DSMatType mat)
{
  PetscErrorCode  ierr;
  PetscInt        l,k,n;
  Mat             MT,auxM;

  PetscFunctionBegin;
  ierr = BVGetActiveColumns(d->eps->V,&l,&k);CHKERRQ(ierr);
  ierr = DSGetMat(d->eps->ds,mat,&MT);CHKERRQ(ierr);
  ierr = MatCreateSeqDense(PETSC_COMM_SELF,k,k,NULL,&auxM);CHKERRQ(ierr);
  ierr = MatZeroEntries(auxM);CHKERRQ(ierr);
  ierr = MatGetSize(MT,&n,0);CHKERRQ(ierr);
  if (k-l!=n) SETERRQ(PETSC_COMM_SELF,1, "Consistency broken");
  ierr = SlepcMatDenseCopy(MT,0,0,auxM,l,l,n,d->V_tra_e);CHKERRQ(ierr);
  ierr = DSRestoreMat(d->eps->ds,mat,&MT);CHKERRQ(ierr);
  ierr = BVMultInPlace(bv,auxM,l,l+d->V_tra_e);CHKERRQ(ierr);
  ierr = MatDestroy(&auxM);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "EPSXDUpdateProj"
/* A(lA:kA-1,lA:kA-1) <- Z(l:k-1)'*A(l:k-1,l:k-1)*Q(l,k-1), where k=l+kA-lA */
static PetscErrorCode EPSXDUpdateProj(Mat Q,Mat Z,PetscInt l,Mat A,PetscInt lA,PetscInt kA,Mat aux)
{
  PetscErrorCode ierr;
  PetscScalar    one=1.0,zero=0.0;
  PetscInt       dA_=kA-lA,m0,n0,ldA_,ldQ_,ldZ_,nQ_;
  PetscBLASInt   dA,nQ,ldA,ldQ,ldZ;
  PetscScalar    *pA,*pQ,*pZ,*pW;
  PetscBool      symm=PETSC_FALSE,set,flg;

  PetscFunctionBegin;
  ierr = MatGetSize(A,&m0,&n0);CHKERRQ(ierr); ldA_=m0;
  if (m0!=n0) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_WRONG,"A should be square");
  if (lA<0 || lA>m0) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_WRONG,"Invalid initial row, column in A");
  if (kA<0 || kA<lA || kA>m0) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_WRONG,"Invalid final row, column in A");
  ierr = MatIsHermitianKnown(A,&set,&flg);CHKERRQ(ierr);
  symm = set? flg: PETSC_FALSE;
  ierr = MatGetSize(Q,&m0,&n0);CHKERRQ(ierr); ldQ_=nQ_=m0;
  if (l<0 || l>n0 || l+dA_>n0) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_WRONG,"Invalid initial column in Q");
  ierr = MatGetSize(Z,&m0,&n0);CHKERRQ(ierr); ldZ_=m0;
  if (l<0 || l>n0 || l+dA_>n0) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_WRONG,"Invalid initial column in Z");
  ierr = MatGetSize(aux,&m0,&n0);CHKERRQ(ierr);
  if (m0*n0<nQ_*dA_) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_WRONG,"aux should be larger");
  ierr = PetscBLASIntCast(dA_,&dA);CHKERRQ(ierr);
  ierr = PetscBLASIntCast(nQ_,&nQ);CHKERRQ(ierr);
  ierr = PetscBLASIntCast(ldA_,&ldA);CHKERRQ(ierr);
  ierr = PetscBLASIntCast(ldQ_,&ldQ);CHKERRQ(ierr);
  ierr = PetscBLASIntCast(ldZ_,&ldZ);CHKERRQ(ierr);
  ierr = MatDenseGetArray(A,&pA);CHKERRQ(ierr);
  ierr = MatDenseGetArray(Q,&pQ);CHKERRQ(ierr);
  if (Q!=Z) {ierr = MatDenseGetArray(Z,&pZ);CHKERRQ(ierr);}
  else pZ = pQ;
#if PETSC_USE_DEBUG
  /* Avoid valgrind warning in xgemm and xsymm */
  ierr = MatZeroEntries(aux);CHKERRQ(ierr);
#endif
  ierr = MatDenseGetArray(aux,&pW);CHKERRQ(ierr);
  /* W = A*Q */
  if (symm) {
    PetscStackCallBLAS("BLASsymm",BLASsymm_("L","U",&nQ,&dA,&one,&pA[ldA*lA+lA],&ldA,&pQ[ldQ*l+l],&ldQ,&zero,pW,&nQ));
  } else {
    PetscStackCallBLAS("BLASgemm",BLASgemm_("N","N",&nQ,&dA,&nQ,&one,&pA[ldA*lA+lA],&ldA,&pQ[ldQ*l+l],&ldQ,&zero,pW,&nQ));
  }
  /* A = Q'*W */
  PetscStackCallBLAS("BLASgemm",BLASgemm_("C","N",&dA,&dA,&nQ,&one,&pZ[ldZ*l+l],&ldZ,pW,&nQ,&zero,&pA[ldA*lA+lA],&ldA));
  ierr = MatDenseGetArray(A,&pA);CHKERRQ(ierr);
  ierr = MatDenseGetArray(Q,&pQ);CHKERRQ(ierr);
  if (Q!=Z) {ierr = MatDenseGetArray(Z,&pZ);CHKERRQ(ierr);}
  else pZ = pQ;
  ierr = MatDenseGetArray(aux,&pW);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}
