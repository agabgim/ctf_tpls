#include "petscsys.h"
#include "petscfix.h"
#include "petsc/private/fortranimpl.h"
/* traj.c */
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
#define tstrajectorydestroy_ TSTRAJECTORYDESTROY
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE) && !defined(FORTRANDOUBLEUNDERSCORE)
#define tstrajectorydestroy_ tstrajectorydestroy
#endif
#ifdef PETSC_HAVE_FORTRAN_CAPS
#define tstrajectorysetfromoptions_ TSTRAJECTORYSETFROMOPTIONS
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE) && !defined(FORTRANDOUBLEUNDERSCORE)
#define tstrajectorysetfromoptions_ tstrajectorysetfromoptions
#endif


/* Definitions of Fortran Wrapper routines */
#if defined(__cplusplus)
extern "C" {
#endif
PETSC_EXTERN void PETSC_STDCALL  tstrajectorydestroy_(TSTrajectory *ts, int *__ierr ){
*__ierr = TSTrajectoryDestroy(ts);
}
PETSC_EXTERN void PETSC_STDCALL  tstrajectorysetfromoptions_(TSTrajectory ts, int *__ierr ){
*__ierr = TSTrajectorySetFromOptions(
	(TSTrajectory)PetscToPointer((ts) ));
}
#if defined(__cplusplus)
}
#endif
