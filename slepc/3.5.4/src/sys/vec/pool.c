/*
   Implementation of a pool of Vec using VecDuplicateVecs.

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

#include <slepc-private/vecimplslepc.h>       /*I "slepcvec.h" I*/

#undef __FUNCT__
#define __FUNCT__ "SlepcVecPoolCreate"
/*@C
   SlepcVecPoolCreate - Create a pool of Vec.

   Collective on VecPool

   Input Parameters:
+  v - template vector.
-  init_size - first guess of maximum vectors.

   Output Parameter:
.  pool - the pool context.

   Level: developer

.seealso: SlepcVecPoolGetVecs(), SlepcVecPoolDestroy()
@*/
PetscErrorCode SlepcVecPoolCreate(Vec v,PetscInt init_size,VecPool *p)
{
  PetscErrorCode ierr;
  VecPool_       *pool;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(v,VEC_CLASSID,1);
  PetscValidLogicalCollectiveInt(v,init_size,2);
  PetscValidPointer(p,3);
  if (init_size<0) SETERRQ(PetscObjectComm((PetscObject)v),PETSC_ERR_ARG_WRONG,"init_size should be positive");
  ierr = PetscMalloc(sizeof(VecPool_),&pool);CHKERRQ(ierr);
  ierr = PetscObjectReference((PetscObject)v);CHKERRQ(ierr);
  pool->v     = v;
  pool->vecs  = NULL;
  pool->n     = 0;
  pool->used  = 0;
  pool->guess = init_size;
  pool->next  = NULL;
  *p = pool;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "SlepcVecPoolDestroy"
/*@C
   SlepcVecPoolDestroy - Destroy the pool of Vec.

   Collective on VecPool

   Input Parameters:
.  pool - pool of Vec.

   Level: developer

.seealso: SlepcVecPoolGetVecs(), SlepcVecPoolCreate()
@*/
PetscErrorCode SlepcVecPoolDestroy(VecPool *p)
{
  PetscErrorCode ierr;
  VecPool_       *pool = (VecPool_*)*p;

  PetscFunctionBegin;
  PetscValidPointer(p,1);
  ierr = VecDestroy(&pool->v);CHKERRQ(ierr);
  if (pool->vecs) { ierr = VecDestroyVecs(pool->n,&pool->vecs);CHKERRQ(ierr); }
  pool->n     = 0;
  pool->used  = 0;
  pool->guess = 0;
  if (pool->next) { ierr = SlepcVecPoolDestroy((VecPool*)&pool->next);CHKERRQ(ierr); }
  ierr = PetscFree(pool);CHKERRQ(ierr);
  *p = NULL;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "SlepcVecPoolGetVecs"
/*@C
   SlepcVecPoolGetVecs - Get an array of Vec from the pool.

   Collective on VecPool

   Input Parameters:
+  pool - pool of Vec.
-  n - number of vectors.

   Output Paramter:
.  vecs - vectors

   Level: developer

.seealso: SlepcVecPoolRestoreVecs()
@*/
PetscErrorCode SlepcVecPoolGetVecs(VecPool p,PetscInt n,Vec **vecs)
{
  PetscErrorCode ierr;
  VecPool_       *pool = (VecPool_*)p;

  PetscFunctionBegin;
  PetscValidPointer(p,1);
  PetscValidPointer(vecs,3);
  if (n<0) SETERRQ(PetscObjectComm((PetscObject)pool->v),PETSC_ERR_ARG_OUTOFRANGE,"n should be positive");
  while (pool->next) pool = pool->next;
  if (pool->n-pool->used < n) {
    pool->guess = PetscMax(p->guess,pool->used+n);
    if (pool->vecs && pool->used == 0) {
      ierr = VecDestroyVecs(pool->n,&pool->vecs);CHKERRQ(ierr);
    }
    if (pool->vecs) {
      ierr = SlepcVecPoolCreate(p->v,pool->guess-pool->used,&pool->next);CHKERRQ(ierr);
      pool = pool->next;
    }
    pool->n = pool->guess;
    ierr = VecDuplicateVecs(p->v,pool->n,&pool->vecs);CHKERRQ(ierr);
  }
  *vecs = pool->vecs + pool->used;
  pool->used += n;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "SlepcVecPoolRestoreVecs"
/*@C
   SlepcVecPoolRestoreVecs - Get back an array of Vec previously returned by
   SlepcVecPoolGetVecs().

   Collective on VecPool

   Input Parameters:
+  pool - pool of Vec.
.  n - number of vectors.
-  vecs - vectors

   Level: developer

.seealso: SlepcVecPoolGetVecs()
@*/
PetscErrorCode SlepcVecPoolRestoreVecs(VecPool p,PetscInt n,Vec **vecs)
{
  PetscErrorCode ierr;
  VecPool_       *pool = (VecPool_*)p, *pool0 = pool;

  PetscFunctionBegin;
  while (pool->next) pool = (pool0 = pool)->next;
  if (pool->used == 0 && pool0 != pool) {
    pool0->guess = pool0->used + pool->guess;
    ierr = SlepcVecPoolDestroy((VecPool*)&pool);CHKERRQ(ierr);
    pool = pool0;
    pool->next = NULL;
  }
  pool->used -= n;
  if (pool->used < 0) SETERRQ(PetscObjectComm((PetscObject)pool->v),PETSC_ERR_ARG_OUTOFRANGE,"Unmatched SlepcVecPoolRestoreVecs");
  PetscFunctionReturn(0);
}
