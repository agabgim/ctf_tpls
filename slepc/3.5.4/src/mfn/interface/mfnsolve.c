/*
      MFN routines related to the solution process.

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

#include <slepc-private/mfnimpl.h>   /*I "slepcmfn.h" I*/

#undef __FUNCT__
#define __FUNCT__ "MFNSolve"
/*@
   MFNSolve - Solves the matrix function problem. Given a vector b, the
   vector x = f(alpha*A)*b is returned.

   Collective on MFN

   Input Parameters:
+  mfn - matrix function context obtained from MFNCreate()
-  b   - the right hand side vector

   Output Parameter:
.  x   - the solution (this may be the same vector as b, then b will be
         overwritten with the answer)

   Options Database Keys:
+  -mfn_view - print information about the solver used
.  -mfn_view_mat binary - save the matrix to the default binary viewer
.  -mfn_view_rhs binary - save right hand side vector to the default binary viewer
-  -mfn_view_solution binary - save computed solution vector to the default binary viewer

   Notes:
   The matrix A is specified with MFNSetOperator().
   The function f is specified with MFNSetFunction().
   The scalar alpha is specified with MFNSetScaleFactor().

   Level: beginner

.seealso: MFNCreate(), MFNSetUp(), MFNDestroy(), MFNSetTolerances(),
          MFNSetOperator(), MFNSetFunction(), MFNSetScaleFactor()
@*/
PetscErrorCode MFNSolve(MFN mfn,Vec b,Vec x)
{
  PetscErrorCode    ierr;
  PetscBool         flg;
  PetscViewer       viewer;
  PetscViewerFormat format;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(mfn,MFN_CLASSID,1);
  PetscValidHeaderSpecific(b,VEC_CLASSID,2);
  PetscCheckSameComm(mfn,1,b,2);
  if (b!=x) PetscValidHeaderSpecific(x,VEC_CLASSID,3);
  if (b!=x) PetscCheckSameComm(mfn,1,x,3);

  /* call setup */
  ierr = MFNSetUp(mfn);CHKERRQ(ierr);
  mfn->its = 0;

  ierr = MFNMonitor(mfn,mfn->its,0);CHKERRQ(ierr);

  /* call solver */
  ierr = PetscLogEventBegin(MFN_Solve,mfn,b,x,0);CHKERRQ(ierr);
  ierr = (*mfn->ops->solve)(mfn,b,x);CHKERRQ(ierr);
  ierr = PetscLogEventEnd(MFN_Solve,mfn,b,x,0);CHKERRQ(ierr);

  if (!mfn->reason) SETERRQ(PetscObjectComm((PetscObject)mfn),PETSC_ERR_PLIB,"Internal error, solver returned without setting converged reason");

  if (mfn->errorifnotconverged && mfn->reason < 0) SETERRQ(PetscObjectComm((PetscObject)mfn),PETSC_ERR_NOT_CONVERGED,"MFNSolve has not converged");

  /* various viewers */
  ierr = MatViewFromOptions(mfn->A,((PetscObject)mfn)->prefix,"-mfn_view_mat");CHKERRQ(ierr);
  ierr = VecViewFromOptions(b,((PetscObject)mfn)->prefix,"-mfn_view_rhs");CHKERRQ(ierr);
  ierr = VecViewFromOptions(x,((PetscObject)mfn)->prefix,"-mfn_view_solution");CHKERRQ(ierr);

  ierr = PetscOptionsGetViewer(PetscObjectComm((PetscObject)mfn),((PetscObject)mfn)->prefix,"-mfn_view",&viewer,&format,&flg);CHKERRQ(ierr);
  if (flg && !PetscPreLoadingOn) {
    ierr = PetscViewerPushFormat(viewer,format);CHKERRQ(ierr);
    ierr = MFNView(mfn,viewer);CHKERRQ(ierr);
    ierr = PetscViewerPopFormat(viewer);CHKERRQ(ierr);
    ierr = PetscViewerDestroy(&viewer);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MFNGetIterationNumber"
/*@
   MFNGetIterationNumber - Gets the current iteration number. If the
   call to MFNSolve() is complete, then it returns the number of iterations
   carried out by the solution method.

   Not Collective

   Input Parameter:
.  mfn - the matrix function context

   Output Parameter:
.  its - number of iterations

   Level: intermediate

   Note:
   During the i-th iteration this call returns i-1. If MFNSolve() is
   complete, then parameter "its" contains either the iteration number at
   which convergence was successfully reached, or failure was detected.
   Call MFNGetConvergedReason() to determine if the solver converged or
   failed and why.

.seealso: MFNGetConvergedReason(), MFNSetTolerances()
@*/
PetscErrorCode MFNGetIterationNumber(MFN mfn,PetscInt *its)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(mfn,MFN_CLASSID,1);
  PetscValidIntPointer(its,2);
  *its = mfn->its;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MFNGetConvergedReason"
/*@C
   MFNGetConvergedReason - Gets the reason why the MFNSolve() iteration was
   stopped.

   Not Collective

   Input Parameter:
.  mfn - the matrix function context

   Output Parameter:
.  reason - negative value indicates diverged, positive value converged

   Possible values for reason:
+  MFN_CONVERGED_TOL - converged up to tolerance
.  MFN_DIVERGED_ITS - required more than its to reach convergence
-  MFN_DIVERGED_BREAKDOWN - generic breakdown in method

   Note:
   Can only be called after the call to MFNSolve() is complete.

   Level: intermediate

.seealso: MFNSetTolerances(), MFNSolve(), MFNConvergedReason, MFNSetErrorIfNotConverged()
@*/
PetscErrorCode MFNGetConvergedReason(MFN mfn,MFNConvergedReason *reason)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(mfn,MFN_CLASSID,1);
  PetscValidIntPointer(reason,2);
  *reason = mfn->reason;
  PetscFunctionReturn(0);
}

