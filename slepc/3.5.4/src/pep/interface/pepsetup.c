/*
      PEP routines related to problem setup.

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

#include <slepc-private/pepimpl.h>       /*I "slepcpep.h" I*/

#undef __FUNCT__
#define __FUNCT__ "PEPSetUp"
/*@
   PEPSetUp - Sets up all the internal data structures necessary for the
   execution of the PEP solver.

   Collective on PEP

   Input Parameter:
.  pep   - solver context

   Notes:
   This function need not be called explicitly in most cases, since PEPSolve()
   calls it. It can be useful when one wants to measure the set-up time
   separately from the solve time.

   Level: advanced

.seealso: PEPCreate(), PEPSolve(), PEPDestroy()
@*/
PetscErrorCode PEPSetUp(PEP pep)
{
  PetscErrorCode ierr;
  SlepcSC        sc;
  PetscBool      islinear,istrivial,flg;
  PetscInt       i,k;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(pep,PEP_CLASSID,1);
  if (pep->state) PetscFunctionReturn(0);
  ierr = PetscLogEventBegin(PEP_SetUp,pep,0,0,0);CHKERRQ(ierr);

  /* reset the convergence flag from the previous solves */
  pep->reason = PEP_CONVERGED_ITERATING;

  /* set default solver type (PEPSetFromOptions was not called) */
  if (!((PetscObject)pep)->type_name) {
    ierr = PEPSetType(pep,PEPTOAR);CHKERRQ(ierr);
  }
  if (!pep->st) { ierr = PEPGetST(pep,&pep->st);CHKERRQ(ierr); }
  ierr = PetscObjectTypeCompare((PetscObject)pep,PEPLINEAR,&islinear);CHKERRQ(ierr);
  if (!islinear) {
    if (!((PetscObject)pep->st)->type_name) {
      ierr = STSetType(pep->st,STSHIFT);CHKERRQ(ierr);
    }
  }
  if (!pep->ds) { ierr = PEPGetDS(pep,&pep->ds);CHKERRQ(ierr); }
  ierr = DSReset(pep->ds);CHKERRQ(ierr);
  if (!pep->rg) { ierr = PEPGetRG(pep,&pep->rg);CHKERRQ(ierr); }
  if (!((PetscObject)pep->rg)->type_name) {
    ierr = RGSetType(pep->rg,RGINTERVAL);CHKERRQ(ierr);
  }
  if (!((PetscObject)pep->rand)->type_name) {
    ierr = PetscRandomSetFromOptions(pep->rand);CHKERRQ(ierr);
  }

  /* check matrices, transfer them to ST */
  if (!pep->A) SETERRQ(PetscObjectComm((PetscObject)pep),PETSC_ERR_ARG_WRONGSTATE,"PEPSetOperators must be called first");
  ierr = STSetOperators(pep->st,pep->nmat,pep->A);CHKERRQ(ierr);

  /* set problem dimensions */
  ierr = MatGetSize(pep->A[0],&pep->n,NULL);CHKERRQ(ierr);
  ierr = MatGetLocalSize(pep->A[0],&pep->nloc,NULL);CHKERRQ(ierr);

  /* set default problem type */
  if (!pep->problem_type) {
    ierr = PEPSetProblemType(pep,PEP_GENERAL);CHKERRQ(ierr);
  }

  /* initialization of matrix norms */
  if (pep->conv==PEP_CONV_NORM) {
    for (i=0;i<pep->nmat;i++) {
      if (!pep->nrma[i]) {
        ierr = MatNorm(pep->A[i],NORM_INFINITY,&pep->nrma[i]);CHKERRQ(ierr);
      }
    }
  }

  /* call specific solver setup */
  ierr = (*pep->ops->setup)(pep);CHKERRQ(ierr);

  /* set tolerance if not yet set */
  if (pep->tol==PETSC_DEFAULT) pep->tol = SLEPC_DEFAULT_TOL;
  if (pep->refine) {
    if (pep->rtol==PETSC_DEFAULT) pep->rtol = SLEPC_DEFAULT_TOL;
    if (pep->rits==PETSC_DEFAULT) pep->rits = (pep->refine==PEP_REFINE_SIMPLE)? 10: 1;
  }

  /* fill sorting criterion context */
  switch (pep->which) {
    case PEP_LARGEST_MAGNITUDE:
      pep->sc->comparison    = SlepcCompareLargestMagnitude;
      pep->sc->comparisonctx = NULL;
      break;
    case PEP_SMALLEST_MAGNITUDE:
      pep->sc->comparison    = SlepcCompareSmallestMagnitude;
      pep->sc->comparisonctx = NULL;
      break;
    case PEP_LARGEST_REAL:
      pep->sc->comparison    = SlepcCompareLargestReal;
      pep->sc->comparisonctx = NULL;
      break;
    case PEP_SMALLEST_REAL:
      pep->sc->comparison    = SlepcCompareSmallestReal;
      pep->sc->comparisonctx = NULL;
      break;
    case PEP_LARGEST_IMAGINARY:
      pep->sc->comparison    = SlepcCompareLargestImaginary;
      pep->sc->comparisonctx = NULL;
      break;
    case PEP_SMALLEST_IMAGINARY:
      pep->sc->comparison    = SlepcCompareSmallestImaginary;
      pep->sc->comparisonctx = NULL;
      break;
    case PEP_TARGET_MAGNITUDE:
      pep->sc->comparison    = SlepcCompareTargetMagnitude;
      pep->sc->comparisonctx = &pep->target;
      break;
    case PEP_TARGET_REAL:
      pep->sc->comparison    = SlepcCompareTargetReal;
      pep->sc->comparisonctx = &pep->target;
      break;
    case PEP_TARGET_IMAGINARY:
      pep->sc->comparison    = SlepcCompareTargetImaginary;
      pep->sc->comparisonctx = &pep->target;
      break;
  }
  pep->sc->map    = NULL;
  pep->sc->mapobj = NULL;

  /* fill sorting criterion for DS */
  ierr = DSGetSlepcSC(pep->ds,&sc);CHKERRQ(ierr);
  ierr = RGIsTrivial(pep->rg,&istrivial);CHKERRQ(ierr);
  sc->rg            = istrivial? NULL: pep->rg;
  sc->comparison    = pep->sc->comparison;
  sc->comparisonctx = pep->sc->comparisonctx;
  sc->map           = SlepcMap_ST;
  sc->mapobj        = (PetscObject)pep->st;

  /* setup ST */
  if (!islinear) {
    ierr = PetscObjectTypeCompareAny((PetscObject)pep->st,&flg,STSHIFT,STSINVERT,"");CHKERRQ(ierr);
    if (!flg) SETERRQ(PetscObjectComm((PetscObject)pep),PETSC_ERR_SUP,"Only STSHIFT and STSINVERT spectral transformations can be used in PEP");
    ierr = STSetUp(pep->st);CHKERRQ(ierr);
    /* compute matrix coefficients */
    ierr = STGetTransform(pep->st,&flg);CHKERRQ(ierr);
    if (!flg) {
      ierr = STMatSetUp(pep->st,1.0,pep->solvematcoeffs);CHKERRQ(ierr);
    } else {
      if (pep->basis!=PEP_BASIS_MONOMIAL) SETERRQ(PetscObjectComm((PetscObject)pep),PETSC_ERR_SUP,"Cannot use ST-transform with non-monomial basis in PEP");
    }
  }

  /* compute scale factor if no set by user */
  ierr = PEPComputeScaleFactor(pep);CHKERRQ(ierr);

  /* build balancing matrix if required */
  if (pep->scale==PEP_SCALE_DIAGONAL || pep->scale==PEP_SCALE_BOTH) {
    if (!pep->Dl) {
      ierr = BVGetVec(pep->V,&pep->Dl);CHKERRQ(ierr);
      ierr = PetscLogObjectParent((PetscObject)pep,(PetscObject)pep->Dl);CHKERRQ(ierr);
    }
    if (!pep->Dr) {
      ierr = BVGetVec(pep->V,&pep->Dr);CHKERRQ(ierr);
      ierr = PetscLogObjectParent((PetscObject)pep,(PetscObject)pep->Dr);CHKERRQ(ierr);
    }
    ierr = PEPBuildDiagonalScaling(pep);CHKERRQ(ierr);
  }

  /* process initial vectors */
  if (pep->nini<0) {
    k = -pep->nini;
    if (k>pep->ncv) SETERRQ(PetscObjectComm((PetscObject)pep),1,"The number of initial vectors is larger than ncv");
    ierr = BVInsertVecs(pep->V,0,&k,pep->IS,PETSC_TRUE);CHKERRQ(ierr);
    ierr = SlepcBasisDestroy_Private(&pep->nini,&pep->IS);CHKERRQ(ierr);
    pep->nini = k;
  }
  ierr = PetscLogEventEnd(PEP_SetUp,pep,0,0,0);CHKERRQ(ierr);
  pep->state = PEP_STATE_SETUP;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PEPSetOperators"
/*@
   PEPSetOperators - Sets the coefficient matrices associated with the polynomial
   eigenvalue problem.

   Collective on PEP and Mat

   Input Parameters:
+  pep  - the eigenproblem solver context
.  nmat - number of matrices in array A
-  A    - the array of matrices associated with the eigenproblem

   Notes:
   The polynomial eigenproblem is defined as P(l)*x=0, where l is
   the eigenvalue, x is the eigenvector, and P(l) is defined as
   P(l) = A_0 + l*A_1 + ... + l^d*A_d, with d=nmat-1 (the degree of P).
   For non-monomial bases, this expression is different.

   Level: beginner

.seealso: PEPSolve(), PEPGetOperators(), PEPGetNumMatrices(), PEPSetBasis()
@*/
PetscErrorCode PEPSetOperators(PEP pep,PetscInt nmat,Mat A[])
{
  PetscErrorCode ierr;
  PetscInt       i,n,m,m0=0;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(pep,PEP_CLASSID,1);
  PetscValidLogicalCollectiveInt(pep,nmat,2);
  if (nmat <= 0) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"Must have one or more matrices, you have %D",nmat);
  PetscValidPointer(A,3);

  if (pep->state) { ierr = PEPReset(pep);CHKERRQ(ierr); }
  ierr = PetscMalloc1(nmat,&pep->A);CHKERRQ(ierr);
  ierr = PetscCalloc3(3*nmat,&pep->pbc,nmat,&pep->solvematcoeffs,nmat,&pep->nrma);CHKERRQ(ierr);
  for (i=0;i<nmat;i++) pep->pbc[i] = 1.0;  /* default to monomial basis */
  ierr = PetscLogObjectMemory((PetscObject)pep,nmat*sizeof(Mat)+4*nmat*sizeof(PetscReal)+nmat*sizeof(PetscScalar));CHKERRQ(ierr);
  for (i=0;i<nmat;i++) {
    PetscValidHeaderSpecific(A[i],MAT_CLASSID,3);
    PetscCheckSameComm(pep,1,A[i],3);
    ierr = MatGetSize(A[i],&m,&n);CHKERRQ(ierr);
    if (m!=n) SETERRQ1(PetscObjectComm((PetscObject)pep),PETSC_ERR_ARG_WRONG,"A[%D] is a non-square matrix",i);
    if (!i) m0 = m;
    if (m!=m0) SETERRQ(PetscObjectComm((PetscObject)pep),PETSC_ERR_ARG_INCOMP,"Dimensions of matrices do not match with each other");
    ierr = PetscObjectReference((PetscObject)A[i]);CHKERRQ(ierr);
    pep->A[i] = A[i];
  }
  pep->nmat = nmat;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PEPGetOperators"
/*@
   PEPGetOperators - Gets the matrices associated with the polynomial eigensystem.

   Not collective, though parallel Mats are returned if the PEP is parallel

   Input Parameters:
+  pep - the PEP context
-  k   - the index of the requested matrix (starting in 0)

   Output Parameter:
.  A - the requested matrix

   Level: intermediate

.seealso: PEPSolve(), PEPSetOperators(), PEPGetNumMatrices()
@*/
PetscErrorCode PEPGetOperators(PEP pep,PetscInt k,Mat *A)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(pep,PEP_CLASSID,1);
  PetscValidPointer(A,3);
  if (k<0 || k>=pep->nmat) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"k must be between 0 and %d",pep->nmat-1);
  *A = pep->A[k];
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PEPGetNumMatrices"
/*@
   PEPGetNumMatrices - Returns the number of matrices stored in the PEP.

   Not collective

   Input Parameter:
.  pep - the PEP context

   Output Parameters:
.  nmat - the number of matrices passed in PEPSetOperators()

   Level: intermediate

.seealso: PEPSetOperators()
@*/
PetscErrorCode PEPGetNumMatrices(PEP pep,PetscInt *nmat)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(pep,PEP_CLASSID,1);
  PetscValidPointer(nmat,2);
  *nmat = pep->nmat;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PEPSetInitialSpace"
/*@
   PEPSetInitialSpace - Specify a basis of vectors that constitute the initial
   space, that is, the subspace from which the solver starts to iterate.

   Collective on PEP and Vec

   Input Parameter:
+  pep   - the polynomial eigensolver context
.  n     - number of vectors
-  is    - set of basis vectors of the initial space

   Notes:
   Some solvers start to iterate on a single vector (initial vector). In that case,
   the other vectors are ignored.

   These vectors do not persist from one PEPSolve() call to the other, so the
   initial space should be set every time.

   The vectors do not need to be mutually orthonormal, since they are explicitly
   orthonormalized internally.

   Common usage of this function is when the user can provide a rough approximation
   of the wanted eigenspace. Then, convergence may be faster.

   Level: intermediate
@*/
PetscErrorCode PEPSetInitialSpace(PEP pep,PetscInt n,Vec *is)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(pep,PEP_CLASSID,1);
  PetscValidLogicalCollectiveInt(pep,n,2);
  if (n<0) SETERRQ(PetscObjectComm((PetscObject)pep),PETSC_ERR_ARG_OUTOFRANGE,"Argument n cannot be negative");
  ierr = SlepcBasisReference_Private(n,is,&pep->nini,&pep->IS);CHKERRQ(ierr);
  if (n>0) pep->state = PEP_STATE_INITIAL;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PEPSetDimensions_Default"
/*
  PEPSetDimensions_Default - Set reasonable values for ncv, mpd if not set
  by the user. This is called at setup.
 */
PetscErrorCode PEPSetDimensions_Default(PEP pep,PetscInt nev,PetscInt *ncv,PetscInt *mpd)
{
  PetscErrorCode ierr;
  PetscBool      krylov;
  PetscInt       dim;

  PetscFunctionBegin;
  ierr = PetscObjectTypeCompareAny((PetscObject)pep,&krylov,PEPTOAR,PEPQARNOLDI,"");CHKERRQ(ierr);
  dim = krylov?(pep->nmat-1)*pep->n:pep->n;
  if (*ncv) { /* ncv set */
    if (krylov) {
      if (*ncv<nev+1 && !(*ncv==nev && *ncv==dim)) SETERRQ(PetscObjectComm((PetscObject)pep),1,"The value of ncv must be at least nev+1");
    } else {
      if (*ncv<nev) SETERRQ(PetscObjectComm((PetscObject)pep),1,"The value of ncv must be at least nev");
    }
  } else if (*mpd) { /* mpd set */
    *ncv = PetscMin(dim,nev+(*mpd));
  } else { /* neither set: defaults depend on nev being small or large */
    if (nev<500) *ncv = PetscMin(dim,PetscMax(2*nev,nev+15));
    else {
      *mpd = 500;
      *ncv = PetscMin(dim,nev+(*mpd));
    }
  }
  if (!*mpd) *mpd = *ncv;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PEPAllocateSolution"
/*@
   PEPAllocateSolution - Allocate memory storage for common variables such
   as eigenvalues and eigenvectors.

   Collective on PEP

   Input Parameters:
+  pep   - eigensolver context
-  extra - number of additional positions, used for methods that require a
           working basis slightly larger than ncv

   Developers Note:
   This is PETSC_EXTERN because it may be required by user plugin PEP
   implementations.

   Level: developer
@*/
PetscErrorCode PEPAllocateSolution(PEP pep,PetscInt extra)
{
  PetscErrorCode ierr;
  PetscInt       oldsize,newc,requested;
  PetscLogDouble cnt;
  Vec            t;

  PetscFunctionBegin;
  requested = pep->ncv + extra;

  /* oldsize is zero if this is the first time setup is called */
  ierr = BVGetSizes(pep->V,NULL,NULL,&oldsize);CHKERRQ(ierr);
  newc = PetscMax(0,requested-oldsize);

  /* allocate space for eigenvalues and friends */
  if (requested != oldsize) {
    if (oldsize) {
      ierr = PetscFree4(pep->eigr,pep->eigi,pep->errest,pep->perm);CHKERRQ(ierr);
    }
    ierr = PetscMalloc4(requested,&pep->eigr,requested,&pep->eigi,requested,&pep->errest,requested,&pep->perm);CHKERRQ(ierr);
    cnt = 2*newc*sizeof(PetscScalar) + newc*sizeof(PetscReal) + newc*sizeof(PetscInt);
    ierr = PetscLogObjectMemory((PetscObject)pep,cnt);CHKERRQ(ierr);
  }

  /* allocate V */
  if (!pep->V) { ierr = PEPGetBV(pep,&pep->V);CHKERRQ(ierr); }
  if (!oldsize) {
    if (!((PetscObject)(pep->V))->type_name) {
      ierr = BVSetType(pep->V,BVSVEC);CHKERRQ(ierr);
    }
    ierr = STMatGetVecs(pep->st,&t,NULL);CHKERRQ(ierr);
    ierr = BVSetSizesFromVec(pep->V,t,requested);CHKERRQ(ierr);
    ierr = VecDestroy(&t);CHKERRQ(ierr);
  } else {
    ierr = BVResize(pep->V,requested,PETSC_FALSE);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

