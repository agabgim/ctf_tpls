#include "petscsys.h"
#include "petscfix.h"
#include "petsc/private/fortranimpl.h"
/* hdf5v.c */
/* Fortran interface file */

/*
* This file was generated automatically by bfort from the C source
* file.  
 */

#ifdef PETSC_USE_POINTER_CONVERSION
#if defined(__cplusplus)
extern "C" { 
#endif 
extern void *PetscToPointer(void*);
extern int PetscFromPointer(void *);
extern void PetscRmPointer(void*);
#if defined(__cplusplus)
} 
#endif 

#else

#define PetscToPointer(a) (*(PetscFortranAddr *)(a))
#define PetscFromPointer(a) (PetscFortranAddr)(a)
#define PetscRmPointer(a)
#endif

#include "petscviewerhdf5.h"
#ifdef PETSC_HAVE_FORTRAN_CAPS
#define petscviewerhdf5setbasedimension2_ PETSCVIEWERHDF5SETBASEDIMENSION2
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE) && !defined(FORTRANDOUBLEUNDERSCORE)
#define petscviewerhdf5setbasedimension2_ petscviewerhdf5setbasedimension2
#endif
#ifdef PETSC_HAVE_FORTRAN_CAPS
#define petscviewerhdf5getbasedimension2_ PETSCVIEWERHDF5GETBASEDIMENSION2
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE) && !defined(FORTRANDOUBLEUNDERSCORE)
#define petscviewerhdf5getbasedimension2_ petscviewerhdf5getbasedimension2
#endif
#ifdef PETSC_HAVE_FORTRAN_CAPS
#define petscviewerhdf5setspoutput_ PETSCVIEWERHDF5SETSPOUTPUT
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE) && !defined(FORTRANDOUBLEUNDERSCORE)
#define petscviewerhdf5setspoutput_ petscviewerhdf5setspoutput
#endif
#ifdef PETSC_HAVE_FORTRAN_CAPS
#define petscviewerhdf5getspoutput_ PETSCVIEWERHDF5GETSPOUTPUT
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE) && !defined(FORTRANDOUBLEUNDERSCORE)
#define petscviewerhdf5getspoutput_ petscviewerhdf5getspoutput
#endif
#ifdef PETSC_HAVE_FORTRAN_CAPS
#define petscviewerhdf5setcollective_ PETSCVIEWERHDF5SETCOLLECTIVE
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE) && !defined(FORTRANDOUBLEUNDERSCORE)
#define petscviewerhdf5setcollective_ petscviewerhdf5setcollective
#endif
#ifdef PETSC_HAVE_FORTRAN_CAPS
#define petscviewerhdf5getcollective_ PETSCVIEWERHDF5GETCOLLECTIVE
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE) && !defined(FORTRANDOUBLEUNDERSCORE)
#define petscviewerhdf5getcollective_ petscviewerhdf5getcollective
#endif
#ifdef PETSC_HAVE_FORTRAN_CAPS
#define petscviewerhdf5popgroup_ PETSCVIEWERHDF5POPGROUP
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE) && !defined(FORTRANDOUBLEUNDERSCORE)
#define petscviewerhdf5popgroup_ petscviewerhdf5popgroup
#endif
#ifdef PETSC_HAVE_FORTRAN_CAPS
#define petscviewerhdf5opengroup_ PETSCVIEWERHDF5OPENGROUP
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE) && !defined(FORTRANDOUBLEUNDERSCORE)
#define petscviewerhdf5opengroup_ petscviewerhdf5opengroup
#endif
#ifdef PETSC_HAVE_FORTRAN_CAPS
#define petscviewerhdf5incrementtimestep_ PETSCVIEWERHDF5INCREMENTTIMESTEP
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE) && !defined(FORTRANDOUBLEUNDERSCORE)
#define petscviewerhdf5incrementtimestep_ petscviewerhdf5incrementtimestep
#endif
#ifdef PETSC_HAVE_FORTRAN_CAPS
#define petscviewerhdf5settimestep_ PETSCVIEWERHDF5SETTIMESTEP
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE) && !defined(FORTRANDOUBLEUNDERSCORE)
#define petscviewerhdf5settimestep_ petscviewerhdf5settimestep
#endif
#ifdef PETSC_HAVE_FORTRAN_CAPS
#define petscviewerhdf5gettimestep_ PETSCVIEWERHDF5GETTIMESTEP
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE) && !defined(FORTRANDOUBLEUNDERSCORE)
#define petscviewerhdf5gettimestep_ petscviewerhdf5gettimestep
#endif
#ifdef PETSC_HAVE_FORTRAN_CAPS
#define petscviewerhdf5hasgroup_ PETSCVIEWERHDF5HASGROUP
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE) && !defined(FORTRANDOUBLEUNDERSCORE)
#define petscviewerhdf5hasgroup_ petscviewerhdf5hasgroup
#endif
#ifdef PETSC_HAVE_FORTRAN_CAPS
#define petscviewerhdf5hasobject_ PETSCVIEWERHDF5HASOBJECT
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE) && !defined(FORTRANDOUBLEUNDERSCORE)
#define petscviewerhdf5hasobject_ petscviewerhdf5hasobject
#endif


/* Definitions of Fortran Wrapper routines */
#if defined(__cplusplus)
extern "C" {
#endif
PETSC_EXTERN void PETSC_STDCALL  petscviewerhdf5setbasedimension2_(PetscViewer viewer,PetscBool *flg, int *__ierr){
*__ierr = PetscViewerHDF5SetBaseDimension2(
	(PetscViewer)PetscToPointer((viewer) ),*flg);
}
PETSC_EXTERN void PETSC_STDCALL  petscviewerhdf5getbasedimension2_(PetscViewer viewer,PetscBool *flg, int *__ierr){
*__ierr = PetscViewerHDF5GetBaseDimension2(
	(PetscViewer)PetscToPointer((viewer) ),flg);
}
PETSC_EXTERN void PETSC_STDCALL  petscviewerhdf5setspoutput_(PetscViewer viewer,PetscBool *flg, int *__ierr){
*__ierr = PetscViewerHDF5SetSPOutput(
	(PetscViewer)PetscToPointer((viewer) ),*flg);
}
PETSC_EXTERN void PETSC_STDCALL  petscviewerhdf5getspoutput_(PetscViewer viewer,PetscBool *flg, int *__ierr){
*__ierr = PetscViewerHDF5GetSPOutput(
	(PetscViewer)PetscToPointer((viewer) ),flg);
}
PETSC_EXTERN void PETSC_STDCALL  petscviewerhdf5setcollective_(PetscViewer viewer,PetscBool *flg, int *__ierr){
*__ierr = PetscViewerHDF5SetCollective(
	(PetscViewer)PetscToPointer((viewer) ),*flg);
}
PETSC_EXTERN void PETSC_STDCALL  petscviewerhdf5getcollective_(PetscViewer viewer,PetscBool *flg, int *__ierr){
*__ierr = PetscViewerHDF5GetCollective(
	(PetscViewer)PetscToPointer((viewer) ),flg);
}
PETSC_EXTERN void PETSC_STDCALL  petscviewerhdf5popgroup_(PetscViewer viewer, int *__ierr){
*__ierr = PetscViewerHDF5PopGroup(
	(PetscViewer)PetscToPointer((viewer) ));
}
PETSC_EXTERN void PETSC_STDCALL  petscviewerhdf5opengroup_(PetscViewer viewer,hid_t *fileId,hid_t *groupId, int *__ierr){
*__ierr = PetscViewerHDF5OpenGroup(
	(PetscViewer)PetscToPointer((viewer) ),
	(hid_t* )PetscToPointer((fileId) ),
	(hid_t* )PetscToPointer((groupId) ));
}
PETSC_EXTERN void PETSC_STDCALL  petscviewerhdf5incrementtimestep_(PetscViewer viewer, int *__ierr){
*__ierr = PetscViewerHDF5IncrementTimestep(
	(PetscViewer)PetscToPointer((viewer) ));
}
PETSC_EXTERN void PETSC_STDCALL  petscviewerhdf5settimestep_(PetscViewer viewer,PetscInt *timestep, int *__ierr){
*__ierr = PetscViewerHDF5SetTimestep(
	(PetscViewer)PetscToPointer((viewer) ),*timestep);
}
PETSC_EXTERN void PETSC_STDCALL  petscviewerhdf5gettimestep_(PetscViewer viewer,PetscInt *timestep, int *__ierr){
*__ierr = PetscViewerHDF5GetTimestep(
	(PetscViewer)PetscToPointer((viewer) ),timestep);
}
PETSC_EXTERN void PETSC_STDCALL  petscviewerhdf5hasgroup_(PetscViewer viewer,PetscBool *has, int *__ierr){
*__ierr = PetscViewerHDF5HasGroup(
	(PetscViewer)PetscToPointer((viewer) ),has);
}
PETSC_EXTERN void PETSC_STDCALL  petscviewerhdf5hasobject_(PetscViewer viewer,PetscObject obj,PetscBool *has, int *__ierr){
*__ierr = PetscViewerHDF5HasObject(
	(PetscViewer)PetscToPointer((viewer) ),
	(PetscObject)PetscToPointer((obj) ),has);
}
#if defined(__cplusplus)
}
#endif
