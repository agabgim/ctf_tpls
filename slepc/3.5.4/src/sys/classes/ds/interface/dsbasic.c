/*
   Basic DS routines

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

#include <slepc-private/dsimpl.h>      /*I "slepcds.h" I*/

PetscFunctionList DSList = 0;
PetscBool         DSRegisterAllCalled = PETSC_FALSE;
PetscClassId      DS_CLASSID = 0;
PetscLogEvent     DS_Solve = 0,DS_Function = 0,DS_Vectors = 0,DS_Other = 0;
static PetscBool  DSPackageInitialized = PETSC_FALSE;
const char        *DSMatName[DS_NUM_MAT] = {"A","B","C","T","D","F","Q","Z","X","Y","U","VT","W","E0","E1","E2","E3","E4","E5","E6","E7","E8","E9"};
DSMatType         DSMatExtra[DS_NUM_EXTRA] = {DS_MAT_E0,DS_MAT_E1,DS_MAT_E2,DS_MAT_E3,DS_MAT_E4,DS_MAT_E5,DS_MAT_E6,DS_MAT_E7,DS_MAT_E8,DS_MAT_E9};

#undef __FUNCT__
#define __FUNCT__ "DSFinalizePackage"
/*@C
   DSFinalizePackage - This function destroys everything in the SLEPc interface
   to the DS package. It is called from SlepcFinalize().

   Level: developer

.seealso: SlepcFinalize()
@*/
PetscErrorCode DSFinalizePackage(void)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscFunctionListDestroy(&DSList);CHKERRQ(ierr);
  DSPackageInitialized = PETSC_FALSE;
  DSRegisterAllCalled  = PETSC_FALSE;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DSInitializePackage"
/*@C
  DSInitializePackage - This function initializes everything in the DS package.
  It is called from PetscDLLibraryRegister() when using dynamic libraries, and
  on the first call to DSCreate() when using static libraries.

  Level: developer

.seealso: SlepcInitialize()
@*/
PetscErrorCode DSInitializePackage()
{
  char             logList[256];
  char             *className;
  PetscBool        opt;
  PetscErrorCode   ierr;

  PetscFunctionBegin;
  if (DSPackageInitialized) PetscFunctionReturn(0);
  DSPackageInitialized = PETSC_TRUE;
  /* Register Classes */
  ierr = PetscClassIdRegister("Direct solver",&DS_CLASSID);CHKERRQ(ierr);
  /* Register Constructors */
  ierr = DSRegisterAll();CHKERRQ(ierr);
  /* Register Events */
  ierr = PetscLogEventRegister("DSSolve",DS_CLASSID,&DS_Solve);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("DSFunction",DS_CLASSID,&DS_Function);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("DSVectors",DS_CLASSID,&DS_Vectors);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("DSOther",DS_CLASSID,&DS_Other);CHKERRQ(ierr);
  /* Process info exclusions */
  ierr = PetscOptionsGetString(NULL,"-info_exclude",logList,256,&opt);CHKERRQ(ierr);
  if (opt) {
    ierr = PetscStrstr(logList,"ds",&className);CHKERRQ(ierr);
    if (className) {
      ierr = PetscInfoDeactivateClass(DS_CLASSID);CHKERRQ(ierr);
    }
  }
  /* Process summary exclusions */
  ierr = PetscOptionsGetString(NULL,"-log_summary_exclude",logList,256,&opt);CHKERRQ(ierr);
  if (opt) {
    ierr = PetscStrstr(logList,"ds",&className);CHKERRQ(ierr);
    if (className) {
      ierr = PetscLogEventDeactivateClass(DS_CLASSID);CHKERRQ(ierr);
    }
  }
  ierr = PetscRegisterFinalize(DSFinalizePackage);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DSCreate"
/*@C
   DSCreate - Creates a DS context.

   Collective on MPI_Comm

   Input Parameter:
.  comm - MPI communicator

   Output Parameter:
.  newds - location to put the DS context

   Level: beginner

   Note:
   DS objects are not intended for normal users but only for
   advanced user that for instance implement their own solvers.

.seealso: DSDestroy(), DS
@*/
PetscErrorCode DSCreate(MPI_Comm comm,DS *newds)
{
  DS             ds;
  PetscInt       i;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidPointer(newds,2);
  *newds = 0;
  ierr = DSInitializePackage();CHKERRQ(ierr);
  ierr = SlepcHeaderCreate(ds,_p_DS,struct _DSOps,DS_CLASSID,"DS","Direct Solver (or Dense System)","DS",comm,DSDestroy,DSView);CHKERRQ(ierr);

  ds->state         = DS_STATE_RAW;
  ds->method        = 0;
  ds->funmethod     = 0;
  ds->compact       = PETSC_FALSE;
  ds->refined       = PETSC_FALSE;
  ds->extrarow      = PETSC_FALSE;
  ds->ld            = 0;
  ds->l             = 0;
  ds->n             = 0;
  ds->m             = 0;
  ds->k             = 0;
  ds->t             = 0;
  ds->bs            = 1;
  ds->nf            = 0;
  for (i=0;i<DS_NUM_EXTRA;i++) ds->f[i] = NULL;
  ds->sc            = NULL;

  for (i=0;i<DS_NUM_MAT;i++) {
    ds->mat[i]      = NULL;
    ds->rmat[i]     = NULL;
    ds->omat[i]     = NULL;
  }
  ds->perm          = NULL;
  ds->work          = NULL;
  ds->rwork         = NULL;
  ds->iwork         = NULL;
  ds->lwork         = 0;
  ds->lrwork        = 0;
  ds->liwork        = 0;

  *newds = ds;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DSSetOptionsPrefix"
/*@C
   DSSetOptionsPrefix - Sets the prefix used for searching for all
   DS options in the database.

   Logically Collective on DS

   Input Parameters:
+  ds - the direct solver context
-  prefix - the prefix string to prepend to all DS option requests

   Notes:
   A hyphen (-) must NOT be given at the beginning of the prefix name.
   The first character of all runtime options is AUTOMATICALLY the
   hyphen.

   Level: advanced

.seealso: DSAppendOptionsPrefix()
@*/
PetscErrorCode DSSetOptionsPrefix(DS ds,const char *prefix)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(ds,DS_CLASSID,1);
  ierr = PetscObjectSetOptionsPrefix((PetscObject)ds,prefix);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DSAppendOptionsPrefix"
/*@C
   DSAppendOptionsPrefix - Appends to the prefix used for searching for all
   DS options in the database.

   Logically Collective on DS

   Input Parameters:
+  ds - the direct solver context
-  prefix - the prefix string to prepend to all DS option requests

   Notes:
   A hyphen (-) must NOT be given at the beginning of the prefix name.
   The first character of all runtime options is AUTOMATICALLY the hyphen.

   Level: advanced

.seealso: DSSetOptionsPrefix()
@*/
PetscErrorCode DSAppendOptionsPrefix(DS ds,const char *prefix)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(ds,DS_CLASSID,1);
  ierr = PetscObjectAppendOptionsPrefix((PetscObject)ds,prefix);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DSGetOptionsPrefix"
/*@C
   DSGetOptionsPrefix - Gets the prefix used for searching for all
   DS options in the database.

   Not Collective

   Input Parameters:
.  ds - the direct solver context

   Output Parameters:
.  prefix - pointer to the prefix string used is returned

   Notes: On the fortran side, the user should pass in a string 'prefix' of
   sufficient length to hold the prefix.

   Level: advanced

.seealso: DSSetOptionsPrefix(), DSAppendOptionsPrefix()
@*/
PetscErrorCode DSGetOptionsPrefix(DS ds,const char *prefix[])
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(ds,DS_CLASSID,1);
  PetscValidPointer(prefix,2);
  ierr = PetscObjectGetOptionsPrefix((PetscObject)ds,prefix);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DSSetType"
/*@C
   DSSetType - Selects the type for the DS object.

   Logically Collective on DS

   Input Parameter:
+  ds   - the direct solver context
-  type - a known type

   Level: intermediate

.seealso: DSGetType()
@*/
PetscErrorCode DSSetType(DS ds,DSType type)
{
  PetscErrorCode ierr,(*r)(DS);
  PetscBool      match;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(ds,DS_CLASSID,1);
  PetscValidCharPointer(type,2);

  ierr = PetscObjectTypeCompare((PetscObject)ds,type,&match);CHKERRQ(ierr);
  if (match) PetscFunctionReturn(0);

  ierr =  PetscFunctionListFind(DSList,type,&r);CHKERRQ(ierr);
  if (!r) SETERRQ1(PetscObjectComm((PetscObject)ds),PETSC_ERR_ARG_UNKNOWN_TYPE,"Unable to find requested DS type %s",type);

  ierr = PetscMemzero(ds->ops,sizeof(struct _DSOps));CHKERRQ(ierr);

  ierr = PetscObjectChangeTypeName((PetscObject)ds,type);CHKERRQ(ierr);
  ierr = (*r)(ds);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DSGetType"
/*@C
   DSGetType - Gets the DS type name (as a string) from the DS context.

   Not Collective

   Input Parameter:
.  ds - the direct solver context

   Output Parameter:
.  name - name of the direct solver

   Level: intermediate

.seealso: DSSetType()
@*/
PetscErrorCode DSGetType(DS ds,DSType *type)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ds,DS_CLASSID,1);
  PetscValidPointer(type,2);
  *type = ((PetscObject)ds)->type_name;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DSSetMethod"
/*@
   DSSetMethod - Selects the method to be used to solve the problem.

   Logically Collective on DS

   Input Parameter:
+  ds   - the direct solver context
-  meth - an index indentifying the method

   Level: intermediate

.seealso: DSGetMethod()
@*/
PetscErrorCode DSSetMethod(DS ds,PetscInt meth)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ds,DS_CLASSID,1);
  PetscValidLogicalCollectiveInt(ds,meth,2);
  if (meth<0) SETERRQ(PetscObjectComm((PetscObject)ds),PETSC_ERR_ARG_OUTOFRANGE,"The method must be a non-negative integer");
  if (meth>DS_MAX_SOLVE) SETERRQ(PetscObjectComm((PetscObject)ds),PETSC_ERR_ARG_OUTOFRANGE,"Too large value for the method");
  ds->method = meth;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DSGetMethod"
/*@
   DSGetMethod - Gets the method currently used in the DS.

   Not Collective

   Input Parameter:
.  ds - the direct solver context

   Output Parameter:
.  meth - identifier of the method

   Level: intermediate

.seealso: DSSetMethod()
@*/
PetscErrorCode DSGetMethod(DS ds,PetscInt *meth)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ds,DS_CLASSID,1);
  PetscValidPointer(meth,2);
  *meth = ds->method;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DSSetFunctionMethod"
/*@
   DSSetFunctionMethod - Selects the method to be used to compute a matrix function.

   Logically Collective on DS

   Input Parameter:
+  ds   - the direct solver context
-  meth - an index indentifying the function method

   Level: intermediate

.seealso: DSGetFunctionMethod()
@*/
PetscErrorCode DSSetFunctionMethod(DS ds,PetscInt meth)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ds,DS_CLASSID,1);
  PetscValidLogicalCollectiveInt(ds,meth,2);
  if (meth<0) SETERRQ(PetscObjectComm((PetscObject)ds),PETSC_ERR_ARG_OUTOFRANGE,"The method must be a non-negative integer");
  if (meth>DS_MAX_FUN) SETERRQ(PetscObjectComm((PetscObject)ds),PETSC_ERR_ARG_OUTOFRANGE,"Too large value for the method");
  ds->funmethod = meth;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DSGetFunctionMethod"
/*@
   DSGetFunctionMethod - Gets the method currently used to compute a matrix function.

   Not Collective

   Input Parameter:
.  ds - the direct solver context

   Output Parameter:
.  meth - identifier of the function method

   Level: intermediate

.seealso: DSSetFunctionMethod()
@*/
PetscErrorCode DSGetFunctionMethod(DS ds,PetscInt *meth)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ds,DS_CLASSID,1);
  PetscValidPointer(meth,2);
  *meth = ds->funmethod;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DSSetCompact"
/*@
   DSSetCompact - Switch to compact storage of matrices.

   Logically Collective on DS

   Input Parameter:
+  ds   - the direct solver context
-  comp - a boolean flag

   Notes:
   Compact storage is used in some DS types such as DSHEP when the matrix
   is tridiagonal. This flag can be used to indicate whether the user
   provides the matrix entries via the compact form (the tridiagonal DS_MAT_T)
   or the non-compact one (DS_MAT_A).

   The default is PETSC_FALSE.

   Level: advanced

.seealso: DSGetCompact()
@*/
PetscErrorCode DSSetCompact(DS ds,PetscBool comp)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ds,DS_CLASSID,1);
  PetscValidLogicalCollectiveBool(ds,comp,2);
  ds->compact = comp;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DSGetCompact"
/*@
   DSGetCompact - Gets the compact storage flag.

   Not Collective

   Input Parameter:
.  ds - the direct solver context

   Output Parameter:
.  comp - the flag

   Level: advanced

.seealso: DSSetCompact()
@*/
PetscErrorCode DSGetCompact(DS ds,PetscBool *comp)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ds,DS_CLASSID,1);
  PetscValidPointer(comp,2);
  *comp = ds->compact;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DSSetExtraRow"
/*@
   DSSetExtraRow - Sets a flag to indicate that the matrix has one extra
   row.

   Logically Collective on DS

   Input Parameter:
+  ds  - the direct solver context
-  ext - a boolean flag

   Notes:
   In Krylov methods it is useful that the matrix representing the direct solver
   has one extra row, i.e., has dimension (n+1) x n. If this flag is activated, all
   transformations applied to the right of the matrix also affect this additional
   row. In that case, (n+1) must be less or equal than the leading dimension.

   The default is PETSC_FALSE.

   Level: advanced

.seealso: DSSolve(), DSAllocate(), DSGetExtraRow()
@*/
PetscErrorCode DSSetExtraRow(DS ds,PetscBool ext)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ds,DS_CLASSID,1);
  PetscValidLogicalCollectiveBool(ds,ext,2);
  if (ds->n>0 && ds->n==ds->ld) SETERRQ(PetscObjectComm((PetscObject)ds),PETSC_ERR_ORDER,"Cannot set extra row after setting n=ld");
  ds->extrarow = ext;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DSGetExtraRow"
/*@
   DSGetExtraRow - Gets the extra row flag.

   Not Collective

   Input Parameter:
.  ds - the direct solver context

   Output Parameter:
.  ext - the flag

   Level: advanced

.seealso: DSSetExtraRow()
@*/
PetscErrorCode DSGetExtraRow(DS ds,PetscBool *ext)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ds,DS_CLASSID,1);
  PetscValidPointer(ext,2);
  *ext = ds->extrarow;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DSSetRefined"
/*@
   DSSetRefined - Sets a flag to indicate that refined vectors must be
   computed.

   Logically Collective on DS

   Input Parameter:
+  ds  - the direct solver context
-  ref - a boolean flag

   Notes:
   Normally the vectors returned in DS_MAT_X are eigenvectors of the
   projected matrix. With this flag activated, DSVectors() will return
   the right singular vector of the smallest singular value of matrix
   \tilde{A}-theta*I, where \tilde{A} is the extended (n+1)xn matrix
   and theta is the Ritz value. This is used in the refined Ritz
   approximation.

   The default is PETSC_FALSE.

   Level: advanced

.seealso: DSVectors(), DSGetRefined()
@*/
PetscErrorCode DSSetRefined(DS ds,PetscBool ref)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ds,DS_CLASSID,1);
  PetscValidLogicalCollectiveBool(ds,ref,2);
  ds->refined = ref;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DSGetRefined"
/*@
   DSGetRefined - Gets the refined vectors flag.

   Not Collective

   Input Parameter:
.  ds - the direct solver context

   Output Parameter:
.  ref - the flag

   Level: advanced

.seealso: DSSetRefined()
@*/
PetscErrorCode DSGetRefined(DS ds,PetscBool *ref)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ds,DS_CLASSID,1);
  PetscValidPointer(ref,2);
  *ref = ds->refined;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DSSetBlockSize"
/*@
   DSSetBlockSize - Sets the block size.

   Logically Collective on DS

   Input Parameter:
+  ds - the direct solver context
-  bs - the block size

   Level: intermediate

.seealso: DSGetBlockSize()
@*/
PetscErrorCode DSSetBlockSize(DS ds,PetscInt bs)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ds,DS_CLASSID,1);
  PetscValidLogicalCollectiveInt(ds,bs,2);
  if (bs<1) SETERRQ(PetscObjectComm((PetscObject)ds),PETSC_ERR_ARG_OUTOFRANGE,"The block size must be at least one");
  ds->bs = bs;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DSGetBlockSize"
/*@
   DSGetBlockSize - Gets the block size.

   Not Collective

   Input Parameter:
.  ds - the direct solver context

   Output Parameter:
.  bs - block size

   Level: intermediate

.seealso: DSSetBlockSize()
@*/
PetscErrorCode DSGetBlockSize(DS ds,PetscInt *bs)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ds,DS_CLASSID,1);
  PetscValidPointer(bs,2);
  *bs = ds->bs;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DSSetSlepcSC"
/*@C
   DSSetSlepcSC - Sets the sorting criterion context.

   Not Collective

   Input Parameters:
+  ds - the direct solver context
-  sc - a pointer to the sorting criterion context

   Level: developer

.seealso: DSGetSlepcSC(), DSSort()
@*/
PetscErrorCode DSSetSlepcSC(DS ds,SlepcSC sc)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(ds,DS_CLASSID,1);
  PetscValidPointer(sc,2);
  if (ds->sc) {
    ierr = PetscFree(ds->sc);CHKERRQ(ierr);
  }
  ds->sc = sc;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DSGetSlepcSC"
/*@C
   DSGetSlepcSC - Gets the sorting criterion context.

   Not Collective

   Input Parameter:
.  ds - the direct solver context

   Output Parameters:
.  sc - a pointer to the sorting criterion context

   Level: developer

.seealso: DSSetSlepcSC(), DSSort()
@*/
PetscErrorCode DSGetSlepcSC(DS ds,SlepcSC *sc)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(ds,DS_CLASSID,1);
  PetscValidPointer(sc,2);
  if (!ds->sc) {
    ierr = PetscNewLog(ds,&ds->sc);CHKERRQ(ierr);
  }
  *sc = ds->sc;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DSSetFN"
/*@
   DSSetFN - Sets a number of functions to be used internally by DS.

   Collective on DS and FN

   Input Parameters:
+  ds - the direct solver context
.  n  - number of functions
-  f  - array of functions

   Notes:
   In the basic usage, only one function is used, for instance to
   evaluate a function of the projected matrix. In the context of nonlinear
   eigensolvers, there are as many functions as terms in the split
   nonlinear operator T(lambda) = sum_i A_i*f_i(lambda).

   This function must be called before DSAllocate(). Then DSAllocate()
   will allocate an extra matrix per each function.

   Level: developer

.seealso: DSGetFN(), DSGetFN(), DSAllocate()
 @*/
PetscErrorCode DSSetFN(DS ds,PetscInt n,FN f[])
{
  PetscInt       i;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(ds,DS_CLASSID,1);
  PetscValidLogicalCollectiveInt(ds,n,2);
  if (n<=0) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"Must have one or more functions, you have %D",n);
  if (n>DS_NUM_EXTRA) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"Too many functions, you specified %D but the limit is %D",n,DS_NUM_EXTRA);
  if (ds->ld) { ierr = PetscInfo(ds,"DSSetFN() called after DSAllocate()\n");CHKERRQ(ierr); }
  PetscValidPointer(f,3);
  PetscCheckSameComm(ds,1,*f,3);
  for (i=0;i<ds->nf;i++) {
    ierr = FNDestroy(&ds->f[i]);CHKERRQ(ierr);
  }
  for (i=0;i<n;i++) {
    PetscValidHeaderSpecific(f[i],FN_CLASSID,4);
    ierr = PetscObjectReference((PetscObject)f[i]);CHKERRQ(ierr);
    ds->f[i] = f[i];
  }
  ds->nf = n;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DSGetFN"
/*@
   DSGetFN - Gets the functions associated with this DS.

   Not collective, though parallel FNs are returned if the DS is parallel

   Input Parameter:
+  ds - the direct olver context
-  k  - the index of the requested function (starting in 0)

   Output Parameter:
.  f - the function

   Level: developer

.seealso: DSSetFN()
@*/
PetscErrorCode DSGetFN(DS ds,PetscInt k,FN *f)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ds,DS_CLASSID,1);
  if (k<0 || k>=ds->nf) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"k must be between 0 and %d",ds->nf-1);
  PetscValidPointer(f,3);
  *f = ds->f[k];
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DSGetNumFN"
/*@
   DSGetNumFN - Returns the number of functions stored internally by
   the DS.

   Not collective

   Input Parameter:
.  ds - the direct solver context

   Output Parameters:
.  n - the number of functions passed in DSSetFN()

   Level: developer

.seealso: DSSetFN()
@*/
PetscErrorCode DSGetNumFN(DS ds,PetscInt *n)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ds,DS_CLASSID,1);
  PetscValidPointer(n,2);
  *n = ds->nf;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DSSetFromOptions"
/*@
   DSSetFromOptions - Sets DS options from the options database.

   Collective on DS

   Input Parameters:
.  ds - the direct solver context

   Notes:
   To see all options, run your program with the -help option.

   Level: beginner
@*/
PetscErrorCode DSSetFromOptions(DS ds)
{
  PetscErrorCode ierr;
  PetscInt       bs,meth;
  PetscBool      flag;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(ds,DS_CLASSID,1);
  if (!DSRegisterAllCalled) { ierr = DSRegisterAll();CHKERRQ(ierr); }
  /* Set default type (we do not allow changing it with -ds_type) */
  if (!((PetscObject)ds)->type_name) {
    ierr = DSSetType(ds,DSNHEP);CHKERRQ(ierr);
  }
  ierr = PetscObjectOptionsBegin((PetscObject)ds);CHKERRQ(ierr);
    ierr = PetscOptionsInt("-ds_block_size","Block size for the dense system solver","DSSetBlockSize",ds->bs,&bs,&flag);CHKERRQ(ierr);
    if (flag) { ierr = DSSetBlockSize(ds,bs);CHKERRQ(ierr); }
    ierr = PetscOptionsInt("-ds_method","Method to be used for the dense system","DSSetMethod",ds->method,&meth,&flag);CHKERRQ(ierr);
    if (flag) { ierr = DSSetMethod(ds,meth);CHKERRQ(ierr); }
    ierr = PetscOptionsInt("-ds_function_method","Method to be used to compute a matrix function","DSSetFunctionMethod",ds->funmethod,&meth,&flag);CHKERRQ(ierr);
    if (flag) { ierr = DSSetFunctionMethod(ds,meth);CHKERRQ(ierr); }
    ierr = PetscObjectProcessOptionsHandlers((PetscObject)ds);CHKERRQ(ierr);
  ierr = PetscOptionsEnd();CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DSView"
/*@C
   DSView - Prints the DS data structure.

   Collective on DS

   Input Parameters:
+  ds - the direct solver context
-  viewer - optional visualization context

   Note:
   The available visualization contexts include
+     PETSC_VIEWER_STDOUT_SELF - standard output (default)
-     PETSC_VIEWER_STDOUT_WORLD - synchronized standard
         output where only the first processor opens
         the file.  All other processors send their
         data to the first processor to print.

   The user can open an alternative visualization context with
   PetscViewerASCIIOpen() - output to a specified file.

   Level: beginner

.seealso: DSViewMat()
@*/
PetscErrorCode DSView(DS ds,PetscViewer viewer)
{
  PetscBool         isascii,issvd;
  const char        *state;
  PetscViewerFormat format;
  PetscErrorCode    ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(ds,DS_CLASSID,1);
  if (!viewer) viewer = PETSC_VIEWER_STDOUT_(PetscObjectComm((PetscObject)ds));
  PetscValidHeaderSpecific(viewer,PETSC_VIEWER_CLASSID,2);
  PetscCheckSameComm(ds,1,viewer,2);
  ierr = PetscObjectTypeCompare((PetscObject)viewer,PETSCVIEWERASCII,&isascii);CHKERRQ(ierr);
  if (isascii) {
    ierr = PetscViewerGetFormat(viewer,&format);CHKERRQ(ierr);
    ierr = PetscObjectPrintClassNamePrefixType((PetscObject)ds,viewer);CHKERRQ(ierr);
    if (format == PETSC_VIEWER_ASCII_INFO_DETAIL) {
      switch (ds->state) {
        case DS_STATE_RAW:          state = "raw"; break;
        case DS_STATE_INTERMEDIATE: state = "intermediate"; break;
        case DS_STATE_CONDENSED:    state = "condensed"; break;
        case DS_STATE_TRUNCATED:    state = "truncated"; break;
        default: SETERRQ(PetscObjectComm((PetscObject)ds),1,"Wrong value of ds->state");
      }
      ierr = PetscViewerASCIIPrintf(viewer,"  current state: %s\n",state);CHKERRQ(ierr);
      ierr = PetscObjectTypeCompare((PetscObject)ds,DSSVD,&issvd);CHKERRQ(ierr);
      if (issvd) {
        ierr = PetscViewerASCIIPrintf(viewer,"  dimensions: ld=%D, n=%D, m=%D, l=%D, k=%D",ds->ld,ds->n,ds->m,ds->l,ds->k);CHKERRQ(ierr);
      } else {
        ierr = PetscViewerASCIIPrintf(viewer,"  dimensions: ld=%D, n=%D, l=%D, k=%D",ds->ld,ds->n,ds->l,ds->k);CHKERRQ(ierr);
      }
      if (ds->state==DS_STATE_TRUNCATED) {
        ierr = PetscViewerASCIIPrintf(viewer,", t=%D\n",ds->t);CHKERRQ(ierr);
      } else {
        ierr = PetscViewerASCIIPrintf(viewer,"\n");CHKERRQ(ierr);
      }
      ierr = PetscViewerASCIIPrintf(viewer,"  flags:%s%s%s\n",ds->compact?" compact":"",ds->extrarow?" extrarow":"",ds->refined?" refined":"");CHKERRQ(ierr);
      if (ds->nf) {
        ierr = PetscViewerASCIIPrintf(viewer,"  number of functions: %D\n",ds->nf);CHKERRQ(ierr);
      }
    }
    if (ds->ops->view) {
      ierr = PetscViewerASCIIPushTab(viewer);CHKERRQ(ierr);
      ierr = (*ds->ops->view)(ds,viewer);CHKERRQ(ierr);
      ierr = PetscViewerASCIIPopTab(viewer);CHKERRQ(ierr);
    }
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DSAllocate"
/*@
   DSAllocate - Allocates memory for internal storage or matrices in DS.

   Logically Collective on DS

   Input Parameters:
+  ds - the direct solver context
-  ld - leading dimension (maximum allowed dimension for the matrices, including
        the extra row if present)

   Level: intermediate

.seealso: DSGetLeadingDimension(), DSSetDimensions(), DSSetExtraRow()
@*/
PetscErrorCode DSAllocate(DS ds,PetscInt ld)
{
  PetscErrorCode ierr;
  PetscInt       i;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(ds,DS_CLASSID,1);
  PetscValidLogicalCollectiveInt(ds,ld,2);
  if (ld<1) SETERRQ(PetscObjectComm((PetscObject)ds),PETSC_ERR_ARG_OUTOFRANGE,"Leading dimension should be at least one");
  ds->ld = ld;
  ierr = (*ds->ops->allocate)(ds,ld);CHKERRQ(ierr);
  for (i=0;i<ds->nf;i++) {
    ierr = DSAllocateMat_Private(ds,DSMatExtra[i]);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DSReset"
/*@
   DSReset - Resets the DS context to the initial state.

   Collective on DS

   Input Parameter:
.  ds - the direct solver context

   Level: advanced

.seealso: DSDestroy()
@*/
PetscErrorCode DSReset(DS ds)
{
  PetscInt       i;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(ds,DS_CLASSID,1);
  ds->state    = DS_STATE_RAW;
  ds->compact  = PETSC_FALSE;
  ds->refined  = PETSC_FALSE;
  ds->extrarow = PETSC_FALSE;
  ds->ld       = 0;
  ds->l        = 0;
  ds->n        = 0;
  ds->m        = 0;
  ds->k        = 0;
  for (i=0;i<DS_NUM_MAT;i++) {
    ierr = PetscFree(ds->mat[i]);CHKERRQ(ierr);
    ierr = PetscFree(ds->rmat[i]);CHKERRQ(ierr);
    ierr = MatDestroy(&ds->omat[i]);CHKERRQ(ierr);
  }
  for (i=0;i<ds->nf;i++) {
    ierr = FNDestroy(&ds->f[i]);CHKERRQ(ierr);
  }
  ds->nf            = 0;
  ierr = PetscFree(ds->perm);CHKERRQ(ierr);
  ierr = PetscFree(ds->work);CHKERRQ(ierr);
  ierr = PetscFree(ds->rwork);CHKERRQ(ierr);
  ierr = PetscFree(ds->iwork);CHKERRQ(ierr);
  ds->lwork         = 0;
  ds->lrwork        = 0;
  ds->liwork        = 0;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DSDestroy"
/*@C
   DSDestroy - Destroys DS context that was created with DSCreate().

   Collective on DS

   Input Parameter:
.  ds - the direct solver context

   Level: beginner

.seealso: DSCreate()
@*/
PetscErrorCode DSDestroy(DS *ds)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (!*ds) PetscFunctionReturn(0);
  PetscValidHeaderSpecific(*ds,DS_CLASSID,1);
  if (--((PetscObject)(*ds))->refct > 0) { *ds = 0; PetscFunctionReturn(0); }
  ierr = DSReset(*ds);CHKERRQ(ierr);
  ierr = PetscFree((*ds)->sc);CHKERRQ(ierr);
  ierr = PetscHeaderDestroy(ds);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DSRegister"
/*@C
   DSRegister - Adds a direct solver to the DS package.

   Not collective

   Input Parameters:
+  name - name of a new user-defined DS
-  routine_create - routine to create context

   Notes:
   DSRegister() may be called multiple times to add several user-defined
   direct solvers.

   Level: advanced

.seealso: DSRegisterAll()
@*/
PetscErrorCode DSRegister(const char *name,PetscErrorCode (*function)(DS))
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscFunctionListAdd(&DSList,name,function);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

PETSC_EXTERN PetscErrorCode DSCreate_HEP(DS);
PETSC_EXTERN PetscErrorCode DSCreate_NHEP(DS);
PETSC_EXTERN PetscErrorCode DSCreate_GHEP(DS);
PETSC_EXTERN PetscErrorCode DSCreate_GHIEP(DS);
PETSC_EXTERN PetscErrorCode DSCreate_GNHEP(DS);
PETSC_EXTERN PetscErrorCode DSCreate_SVD(DS);
PETSC_EXTERN PetscErrorCode DSCreate_NEP(DS);

#undef __FUNCT__
#define __FUNCT__ "DSRegisterAll"
/*@C
   DSRegisterAll - Registers all of the direct solvers in the DS package.

   Not Collective

   Level: advanced
@*/
PetscErrorCode DSRegisterAll(void)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  DSRegisterAllCalled = PETSC_TRUE;
  ierr = DSRegister(DSHEP,DSCreate_HEP);CHKERRQ(ierr);
  ierr = DSRegister(DSNHEP,DSCreate_NHEP);CHKERRQ(ierr);
  ierr = DSRegister(DSGHEP,DSCreate_GHEP);CHKERRQ(ierr);
  ierr = DSRegister(DSGHIEP,DSCreate_GHIEP);CHKERRQ(ierr);
  ierr = DSRegister(DSGNHEP,DSCreate_GNHEP);CHKERRQ(ierr);
  ierr = DSRegister(DSSVD,DSCreate_SVD);CHKERRQ(ierr);
  ierr = DSRegister(DSNEP,DSCreate_NEP);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

