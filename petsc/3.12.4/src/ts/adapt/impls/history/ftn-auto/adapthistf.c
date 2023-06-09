#include "petscsys.h"
#include "petscfix.h"
#include "petsc/private/fortranimpl.h"
/* adapthist.c */
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

#include "petscts.h"
#ifdef PETSC_HAVE_FORTRAN_CAPS
#define tsadapthistorygetstep_ TSADAPTHISTORYGETSTEP
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE) && !defined(FORTRANDOUBLEUNDERSCORE)
#define tsadapthistorygetstep_ tsadapthistorygetstep
#endif
#ifdef PETSC_HAVE_FORTRAN_CAPS
#define tsadapthistorysethistory_ TSADAPTHISTORYSETHISTORY
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE) && !defined(FORTRANDOUBLEUNDERSCORE)
#define tsadapthistorysethistory_ tsadapthistorysethistory
#endif
#ifdef PETSC_HAVE_FORTRAN_CAPS
#define tsadapthistorysettrajectory_ TSADAPTHISTORYSETTRAJECTORY
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE) && !defined(FORTRANDOUBLEUNDERSCORE)
#define tsadapthistorysettrajectory_ tsadapthistorysettrajectory
#endif


/* Definitions of Fortran Wrapper routines */
#if defined(__cplusplus)
extern "C" {
#endif
PETSC_EXTERN void PETSC_STDCALL  tsadapthistorygetstep_(TSAdapt adapt,PetscInt *step,PetscReal *t,PetscReal *dt, int *__ierr){
*__ierr = TSAdaptHistoryGetStep(
	(TSAdapt)PetscToPointer((adapt) ),*step,t,dt);
}
PETSC_EXTERN void PETSC_STDCALL  tsadapthistorysethistory_(TSAdapt adapt,PetscInt *n,PetscReal hist[],PetscBool *backward, int *__ierr){
*__ierr = TSAdaptHistorySetHistory(
	(TSAdapt)PetscToPointer((adapt) ),*n,hist,*backward);
}
PETSC_EXTERN void PETSC_STDCALL  tsadapthistorysettrajectory_(TSAdapt adapt,TSTrajectory tj,PetscBool *backward, int *__ierr){
*__ierr = TSAdaptHistorySetTrajectory(
	(TSAdapt)PetscToPointer((adapt) ),
	(TSTrajectory)PetscToPointer((tj) ),*backward);
}
#if defined(__cplusplus)
}
#endif
