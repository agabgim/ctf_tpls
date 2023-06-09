/*
  SLEPc eigensolver: "davidson"

  Step: test for restarting, updateV, restartV

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
#include <slepc-private/dsimpl.h>

static PetscErrorCode dvd_updateV_start(dvdDashboard *d);
static PetscErrorCode dvd_isrestarting_fullV(dvdDashboard *d,PetscBool *r);
static PetscErrorCode dvd_managementV_basic_d(dvdDashboard *d);
static PetscErrorCode dvd_updateV_extrapol(dvdDashboard *d);
static PetscErrorCode dvd_updateV_conv_gen(dvdDashboard *d);
static PetscErrorCode dvd_updateV_restart_gen(dvdDashboard *d);
static PetscErrorCode dvd_updateV_update_gen(dvdDashboard *d);
static PetscErrorCode dvd_updateV_testConv(dvdDashboard *d,PetscInt s,PetscInt pre,PetscInt e,PetscInt *nConv);

typedef struct {
  PetscInt
    min_size_V,       /* restart with this number of eigenvectors */
    plusk,            /* when restart, save plusk vectors from last iteration */
    mpd;              /* max size of the searching subspace */
  void *old_updateV_data;
                      /* old updateV data */
  isRestarting_type old_isRestarting;
                      /* old isRestarting */
  Mat oldU;           /* previous projected right igenvectors */
  Mat oldV;           /* previous projected left eigenvectors */
  PetscInt size_oldU; /* size of oldU */
  PetscBool allResiduals;
                      /* if computing all the residuals */
} dvdManagV_basic;


#undef __FUNCT__
#define __FUNCT__ "dvd_managementV_basic"
PetscErrorCode dvd_managementV_basic(dvdDashboard *d,dvdBlackboard *b,PetscInt bs,PetscInt mpd,PetscInt min_size_V,PetscInt plusk,PetscBool harm,PetscBool allResiduals)
{
  PetscErrorCode  ierr;
  dvdManagV_basic *data;
#if !defined(PETSC_USE_COMPLEX)
  PetscBool       her_probl, std_probl;
#endif

  PetscFunctionBegin;
  /* Setting configuration constrains */
#if !defined(PETSC_USE_COMPLEX)
  /* if the last converged eigenvalue is complex its conjugate pair is also
     converged */
  her_probl = DVD_IS(d->sEP, DVD_EP_HERMITIAN)?PETSC_TRUE:PETSC_FALSE;
  std_probl = DVD_IS(d->sEP, DVD_EP_STD)?PETSC_TRUE:PETSC_FALSE;
  b->max_size_X = PetscMax(b->max_size_X, bs+((her_probl && std_probl)?0:1));
#else
  b->max_size_X = PetscMax(b->max_size_X, bs);
#endif

  b->max_size_V = PetscMax(b->max_size_V, mpd);
  min_size_V = PetscMin(min_size_V, mpd-bs);
  b->size_V = PetscMax(b->size_V, b->max_size_V + b->max_size_P + b->max_nev);
  b->max_size_oldX = plusk;

  /* Setup the step */
  if (b->state >= DVD_STATE_CONF) {
    ierr = PetscMalloc(sizeof(dvdManagV_basic),&data);CHKERRQ(ierr);
    ierr = PetscLogObjectMemory((PetscObject)d->eps,sizeof(dvdManagV_basic));CHKERRQ(ierr);
    data->mpd = b->max_size_V;
    data->min_size_V = min_size_V;
    d->bs = bs;
    data->plusk = plusk;
    data->allResiduals = allResiduals;

    d->eigr = d->eps->eigr;
    d->eigi = d->eps->eigi;
    d->errest = d->eps->errest;
    ierr = PetscMalloc1(d->eps->ncv,&d->real_nR);CHKERRQ(ierr);
    ierr = PetscMalloc1(d->eps->ncv,&d->real_nX);CHKERRQ(ierr);
    if (plusk > 0) {ierr = MatCreateSeqDense(PETSC_COMM_SELF,d->eps->ncv,d->eps->ncv,NULL,&data->oldU);CHKERRQ(ierr);}
    else data->oldU = NULL;
    if (harm && plusk>0) {ierr = MatCreateSeqDense(PETSC_COMM_SELF,d->eps->ncv,d->eps->ncv,NULL,&data->oldV);CHKERRQ(ierr);}
    else data->oldV = NULL;

    data->old_updateV_data = d->updateV_data;
    d->updateV_data = data;
    data->old_isRestarting = d->isRestarting;
    d->isRestarting = dvd_isrestarting_fullV;
    d->updateV = dvd_updateV_extrapol;
    d->preTestConv = dvd_updateV_testConv;
    ierr = EPSDavidsonFLAdd(&d->startList,dvd_updateV_start);CHKERRQ(ierr);
    ierr = EPSDavidsonFLAdd(&d->destroyList,dvd_managementV_basic_d);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "dvd_updateV_start"
static PetscErrorCode dvd_updateV_start(dvdDashboard *d)
{
  dvdManagV_basic *data = (dvdManagV_basic*)d->updateV_data;
  PetscInt        i;

  PetscFunctionBegin;
  for (i=0;i<d->eps->ncv;i++) d->eigi[i] = 0.0;
  d->nR = d->real_nR;
  for (i=0;i<d->eps->ncv;i++) d->nR[i] = PETSC_MAX_REAL;
  d->nX = d->real_nX;
  for (i=0;i<d->eps->ncv;i++) d->errest[i] = PETSC_MAX_REAL;
  data->size_oldU = 0;
  d->nconv = 0;
  d->npreconv = 0;
  d->V_tra_s = d->V_tra_e = d->V_new_s = d->V_new_e = 0;
  d->size_D = 0;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "dvd_isrestarting_fullV"
static PetscErrorCode dvd_isrestarting_fullV(dvdDashboard *d,PetscBool *r)
{
  PetscErrorCode  ierr;
  PetscInt        l,k;
  PetscBool       restart;
  dvdManagV_basic *data = (dvdManagV_basic*)d->updateV_data;

  PetscFunctionBegin;
  ierr = BVGetActiveColumns(d->eps->V,&l,&k);CHKERRQ(ierr);
  restart = (k+2 > d->eps->ncv)?PETSC_TRUE:PETSC_FALSE;

  /* Check old isRestarting function */
  if (!restart && data->old_isRestarting) {
    ierr = data->old_isRestarting(d,&restart);CHKERRQ(ierr);
  }
  *r = restart;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "dvd_managementV_basic_d"
static PetscErrorCode dvd_managementV_basic_d(dvdDashboard *d)
{
  PetscErrorCode  ierr;
  dvdManagV_basic *data = (dvdManagV_basic*)d->updateV_data;

  PetscFunctionBegin;
  /* Restore changes in dvdDashboard */
  d->updateV_data = data->old_updateV_data;

  /* Free local data */
  if (data->oldU) {ierr = MatDestroy(&data->oldU);CHKERRQ(ierr);}
  if (data->oldV) {ierr = MatDestroy(&data->oldV);CHKERRQ(ierr);}
  ierr = PetscFree(d->real_nR);CHKERRQ(ierr);
  ierr = PetscFree(d->real_nX);CHKERRQ(ierr);
  ierr = PetscFree(data);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "dvd_updateV_extrapol"
static PetscErrorCode dvd_updateV_extrapol(dvdDashboard *d)
{
  dvdManagV_basic *data = (dvdManagV_basic*)d->updateV_data;
  PetscInt        i;
  PetscBool       restart;
  PetscErrorCode  ierr;

  PetscFunctionBegin;
  /* TODO: restrict select pairs to each case */
  ierr = d->calcpairs_selectPairs(d, data->min_size_V);CHKERRQ(ierr);

  /* If the subspaces doesn't need restart, add new vector */
  ierr = d->isRestarting(d,&restart);CHKERRQ(ierr);
  if (!restart) {
    d->size_D = 0;
    ierr = dvd_updateV_update_gen(d);CHKERRQ(ierr);

    /* If some vector were add, exit */
    if (d->size_D > 0) PetscFunctionReturn(0);
  }

  /* If some eigenpairs were converged, lock them  */
  if (d->npreconv > 0) {
    i = d->npreconv;
    ierr = dvd_updateV_conv_gen(d);CHKERRQ(ierr);

    /* If some eigenpair was locked, exit */
    if (i > d->npreconv) PetscFunctionReturn(0);
  }

  /* Else, a restarting is performed */
  ierr = dvd_updateV_restart_gen(d);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "dvd_updateV_conv_gen"
static PetscErrorCode dvd_updateV_conv_gen(dvdDashboard *d)
{
  dvdManagV_basic *data = (dvdManagV_basic*)d->updateV_data;
  PetscInt        npreconv,cMT,cMTX,lV,kV,nV;
  PetscErrorCode  ierr;
  Mat             Q,Z;
#if !defined(PETSC_USE_COMPLEX)
  PetscInt        i;
#endif

  PetscFunctionBegin;
  npreconv = d->npreconv;
  /* Constrains the converged pairs to nev */
#if !defined(PETSC_USE_COMPLEX)
  /* Tries to maintain together conjugate eigenpairs */
  for (i=0; (i + (d->eigi[i]!=0.0?1:0) < npreconv) && (d->nconv + i < d->nev); i+= (d->eigi[i]!=0.0?2:1));
  npreconv = i;
#else
  npreconv = PetscMax(PetscMin(d->nev - d->nconv, npreconv), 0);
#endif
  /* Quick exit */
  if (npreconv == 0) PetscFunctionReturn(0);

  ierr = BVGetActiveColumns(d->eps->V,&lV,&kV);CHKERRQ(ierr);
  nV = kV - lV; 
  cMT = nV - npreconv;
  /* Harmonics restarts wiht right eigenvectors, and other with the left ones.
     If the problem is standard or hermitian, left and right vectors are the same */
  if (!(d->W||DVD_IS(d->sEP,DVD_EP_STD)||DVD_IS(d->sEP,DVD_EP_HERMITIAN))) {
    /* ps.Q <- [ps.Q(0:npreconv-1) ps.Z(npreconv:size_H-1)] */
    ierr = DSGetMat(d->eps->ds,DS_MAT_Q,&Q);CHKERRQ(ierr);
    ierr = DSGetMat(d->eps->ds,DS_MAT_Z,&Z);CHKERRQ(ierr);
    ierr = SlepcMatDenseCopy(Z,0,npreconv,Q,0,npreconv,nV,cMT);CHKERRQ(ierr);
    ierr = DSRestoreMat(d->eps->ds,DS_MAT_Q,&Q);CHKERRQ(ierr);
    ierr = DSRestoreMat(d->eps->ds,DS_MAT_Z,&Z);CHKERRQ(ierr);
  }
  if (DVD_IS(d->sEP,DVD_EP_INDEFINITE)) {
    ierr = DSPseudoOrthogonalize(d->eps->ds,DS_MAT_Q,nV,d->nBds,&cMTX,d->nBds);CHKERRQ(ierr);
  } else {
    ierr = DSOrthogonalize(d->eps->ds,DS_MAT_Q,nV,&cMTX);CHKERRQ(ierr);
  }
  cMT = cMTX - npreconv;

  if (d->W) {
    ierr = DSOrthogonalize(d->eps->ds,DS_MAT_Z,nV,&cMTX);CHKERRQ(ierr);
    cMT = PetscMin(cMT,cMTX - npreconv);
  }

  /* Lock the converged pairs */
  d->eigr+= npreconv;
#if !defined(PETSC_USE_COMPLEX)
  if (d->eigi) d->eigi+= npreconv;
#endif
  d->nconv+= npreconv;
  d->errest+= npreconv;
  /* Notify the changes in V and update the other subspaces */
  d->V_tra_s = npreconv;          d->V_tra_e = nV;
  d->V_new_s = cMT;               d->V_new_e = d->V_new_s;
  /* Remove oldU */
  data->size_oldU = 0;

  d->npreconv-= npreconv;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "dvd_updateV_restart_gen"
static PetscErrorCode dvd_updateV_restart_gen(dvdDashboard *d)
{
  dvdManagV_basic *data = (dvdManagV_basic*)d->updateV_data;
  PetscInt        lV,kV,nV,size_plusk,size_X,cMTX,cMTY;
  Mat             Q,Z;
  PetscErrorCode  ierr;

  PetscFunctionBegin;
  /* Select size_X desired pairs from V */
  ierr = BVGetActiveColumns(d->eps->V,&lV,&kV);CHKERRQ(ierr);
  nV = kV - lV;
  size_X = PetscMin(data->min_size_V,nV);

  /* Add plusk eigenvectors from the previous iteration */
  size_plusk = PetscMax(0,PetscMin(PetscMin(data->plusk,data->size_oldU),d->eps->ncv - size_X));

  d->size_MT = nV;
  /* ps.Q <- orth([pX(0:size_X-1) [oldU(0:size_plusk-1); 0] ]) */
  /* Harmonics restarts wiht right eigenvectors, and other with the left ones.
     If the problem is standard or hermitian, left and right vectors are the same */
  ierr = DSGetMat(d->eps->ds,DS_MAT_Q,&Q);CHKERRQ(ierr);
  if (!(d->W||DVD_IS(d->sEP,DVD_EP_STD)||DVD_IS(d->sEP,DVD_EP_HERMITIAN))) {
    ierr = DSGetMat(d->eps->ds,DS_MAT_Z,&Z);CHKERRQ(ierr);
    ierr = SlepcMatDenseCopy(Z,0,0,Q,0,0,nV,size_X);CHKERRQ(ierr);
    ierr = DSRestoreMat(d->eps->ds,DS_MAT_Z,&Z);CHKERRQ(ierr);
  }
  if (size_plusk > 0 && DVD_IS(d->sEP,DVD_EP_INDEFINITE)) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_SUP,"Unsupported plusk>0 in indefinite eigenvalue problems");
  if (size_plusk > 0) {
    ierr = SlepcMatDenseCopy(data->oldU,0,0,Q,0,size_X,nV,size_plusk);CHKERRQ(ierr);
  }
  ierr = DSRestoreMat(d->eps->ds,DS_MAT_Q,&Q);CHKERRQ(ierr);
  if (DVD_IS(d->sEP,DVD_EP_INDEFINITE)) {
    ierr = DSPseudoOrthogonalize(d->eps->ds,DS_MAT_Q,size_X,d->nBds,&cMTX,d->nBds);CHKERRQ(ierr);
  } else {
    ierr = DSOrthogonalize(d->eps->ds,DS_MAT_Q,size_X+size_plusk,&cMTX);CHKERRQ(ierr);
  }

  if (d->W && size_plusk > 0) {
    /* ps.Z <- orth([ps.Z(0:size_X-1) [oldV(0:size_plusk-1); 0] ]) */
    ierr = DSGetMat(d->eps->ds,DS_MAT_Z,&Z);CHKERRQ(ierr);
    ierr = SlepcMatDenseCopy(data->oldV,0,0,Z,0,size_X,nV,size_plusk);CHKERRQ(ierr);
    ierr = DSRestoreMat(d->eps->ds,DS_MAT_Z,&Z);CHKERRQ(ierr);
    ierr = DSOrthogonalize(d->eps->ds,DS_MAT_Z,size_X+size_plusk,&cMTY);CHKERRQ(ierr);
    cMTX = PetscMin(cMTX, cMTY);
  }

  /* Notify the changes in V and update the other subspaces */
  d->V_tra_s = 0;                     d->V_tra_e = cMTX;
  d->V_new_s = d->V_tra_e;            d->V_new_e = d->V_new_s;

  /* Remove oldU */
  data->size_oldU = 0;

  /* Remove npreconv */
  d->npreconv = 0;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "dvd_updateV_update_gen"
static PetscErrorCode dvd_updateV_update_gen(dvdDashboard *d)
{
  dvdManagV_basic *data = (dvdManagV_basic*)d->updateV_data;
  PetscInt        size_D,s,lV,kV,nV;
  Mat             Q,Z;
  PetscErrorCode  ierr;

  PetscFunctionBegin;
  /* Select the desired pairs */
  ierr = BVGetActiveColumns(d->eps->V,&lV,&kV);CHKERRQ(ierr);
  nV = kV - lV;
  size_D = PetscMin(PetscMin(PetscMin(d->bs,nV),d->eps->ncv-nV),nV);
  if (size_D == 0) {
    ierr = d->initV(d);CHKERRQ(ierr);
    ierr = d->calcPairs(d);CHKERRQ(ierr);
  }

  /* Fill V with D */
  ierr = d->improveX(d,0,size_D,&size_D);CHKERRQ(ierr);

  /* If D is empty, exit */
  d->size_D = size_D;
  if (size_D == 0) PetscFunctionReturn(0);

  /* Get the residual of all pairs */
#if !defined(PETSC_USE_COMPLEX)
  s = d->eigi[0]!=0.0?2:1;
#else
  s = 1;
#endif
  ierr = BVGetActiveColumns(d->eps->V,&lV,&kV);CHKERRQ(ierr);
  nV = kV - lV;
  ierr = dvd_updateV_testConv(d,s,s,data->allResiduals?nV:size_D,NULL);CHKERRQ(ierr);

  /* Notify the changes in V */
  d->V_tra_s = 0;                 d->V_tra_e = 0;
  d->V_new_s = nV;                d->V_new_e = nV+size_D;

  /* Save the projected eigenvectors */
  if (data->plusk > 0) {
    ierr = MatZeroEntries(data->oldU);CHKERRQ(ierr);
    data->size_oldU = nV;
    ierr = DSGetMat(d->eps->ds,DS_MAT_Q,&Q);CHKERRQ(ierr);
    ierr = SlepcMatDenseCopy(Q,0,0,data->oldU,0,0,nV,nV);CHKERRQ(ierr);
    ierr = DSRestoreMat(d->eps->ds,DS_MAT_Q,&Q);CHKERRQ(ierr);
    if (d->W) {
      ierr = MatZeroEntries(data->oldV);CHKERRQ(ierr);
      ierr = DSGetMat(d->eps->ds,DS_MAT_Z,&Z);CHKERRQ(ierr);
      ierr = SlepcMatDenseCopy(Z,0,0,data->oldV,0,0,nV,nV);CHKERRQ(ierr);
      ierr = DSRestoreMat(d->eps->ds,DS_MAT_Z,&Z);CHKERRQ(ierr);
    }
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "dvd_updateV_testConv"
static PetscErrorCode dvd_updateV_testConv(dvdDashboard *d,PetscInt s,PetscInt pre,PetscInt e,PetscInt *nConv)
{
  PetscInt        i,j,b;
  PetscReal       norm;
  PetscErrorCode  ierr;
  PetscBool       conv, c;
  dvdManagV_basic *data = (dvdManagV_basic*)d->updateV_data;

  PetscFunctionBegin;
  if (nConv) *nConv = s;
  for (i=s, conv=PETSC_TRUE;
      (conv || data->allResiduals) && (i < e);
      i+=b) {
#if !defined(PETSC_USE_COMPLEX)
    b = d->eigi[i]!=0.0?2:1;
#else
    b = 1;
#endif
    if (i+b-1 >= pre) {
      ierr = d->calcpairs_residual(d,i,i+b);CHKERRQ(ierr);
    }
    /* Test the Schur vector */
    for (j=0,c=PETSC_TRUE; j<b && c; j++) {
      norm = d->nR[i+j]/d->nX[i+j];
      c = d->testConv(d,d->eigr[i+j],d->eigi[i+j],norm,&d->errest[i+j]);
    }
    if (conv && c) { if (nConv) *nConv = i+b; }
    else conv = PETSC_FALSE;
  }
  pre = PetscMax(pre, i);

#if !defined(PETSC_USE_COMPLEX)
  /* Enforce converged conjugate complex eigenpairs */
  if (nConv) {
    for (j=0;j<*nConv;j++) if (d->eigi[j] != 0.0) j++;
    if (j>*nConv) (*nConv)--;
  }
#endif
  for (i=pre;i<e;i++) d->errest[i] = d->nR[i] = PETSC_MAX_REAL;
  PetscFunctionReturn(0);
}
