
#include <petsc/private/viewerimpl.h>  /*I "petscviewer.h" I*/

/*@C
   PetscViewerGetSubViewer - Creates a new PetscViewer (same type as the old)
    that lives on a subcommunicator

    Collective on PetscViewer

   Input Parameter:
.  viewer - the PetscViewer to be reproduced

   Output Parameter:
.  outviewer - new PetscViewer

   Level: advanced

   Notes:
    Call PetscViewerRestoreSubViewer() to return this PetscViewer, NOT PetscViewerDestroy()

     This is most commonly used to view a sequential object that is part of a
    parallel object. For example block Jacobi PC view could use this to obtain a
    PetscViewer that is used with the sequential KSP on one block of the preconditioner.

    Between the calls to PetscViewerGetSubViewer() and PetscViewerRestoreSubViewer() the original
    viewer should not be used

    PETSCVIEWERDRAW and PETSCVIEWERBINARY only support returning a singleton viewer on rank 0,
    all other ranks will return a NULL viewer

  Developer Notes:
    There is currently incomplete error checking that the user does not use the original viewer between the
    the calls to PetscViewerGetSubViewer() and PetscViewerRestoreSubViewer(). If the user does there
    could be errors in the viewing that go undetected or crash the code.

.seealso: PetscViewerSocketOpen(), PetscViewerASCIIOpen(), PetscViewerDrawOpen(), PetscViewerRestoreSubViewer()
@*/
PetscErrorCode  PetscViewerGetSubViewer(PetscViewer viewer,MPI_Comm comm,PetscViewer *outviewer)
{
  PetscErrorCode ierr;
  PetscMPIInt    size;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(viewer,PETSC_VIEWER_CLASSID,1);
  PetscValidPointer(outviewer,2);
  ierr = MPI_Comm_size(PetscObjectComm((PetscObject)viewer),&size);CHKERRQ(ierr);
  if (size == 1) {
    ierr       = PetscObjectReference((PetscObject)viewer);CHKERRQ(ierr);
    *outviewer = viewer;
  } else if (viewer->ops->getsubviewer) {
    ierr = (*viewer->ops->getsubviewer)(viewer,comm,outviewer);CHKERRQ(ierr);
  } else SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_SUP,"Cannot get SubViewer PetscViewer for type %s",((PetscObject)viewer)->type_name);
  PetscFunctionReturn(0);
}

/*@C
   PetscViewerRestoreSubViewer - Restores a new PetscViewer obtained with PetscViewerGetSubViewer().

    Collective on PetscViewer

   Input Parameters:
+  viewer - the PetscViewer that was reproduced
-  outviewer - new PetscViewer

   Level: advanced

   Notes:
    Call PetscViewerGetSubViewer() to get this PetscViewer, NOT PetscViewerCreate()

.seealso: PetscViewerSocketOpen(), PetscViewerASCIIOpen(), PetscViewerDrawOpen(), PetscViewerGetSubViewer()
@*/
PetscErrorCode  PetscViewerRestoreSubViewer(PetscViewer viewer,MPI_Comm comm,PetscViewer *outviewer)
{
  PetscErrorCode ierr;
  PetscMPIInt    size;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(viewer,PETSC_VIEWER_CLASSID,1);

  ierr = MPI_Comm_size(PetscObjectComm((PetscObject)viewer),&size);CHKERRQ(ierr);
  if (size == 1) {
    ierr = PetscObjectDereference((PetscObject)viewer);CHKERRQ(ierr);
    if (outviewer) *outviewer = NULL;
  } else if (viewer->ops->restoresubviewer) {
    ierr = (*viewer->ops->restoresubviewer)(viewer,comm,outviewer);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

