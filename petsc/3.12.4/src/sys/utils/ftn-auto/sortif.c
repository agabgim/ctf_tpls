#include "petscsys.h"
#include "petscfix.h"
#include "petsc/private/fortranimpl.h"
/* sorti.c */
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

#include "petscsys.h"
#ifdef PETSC_HAVE_FORTRAN_CAPS
#define petscsortint_ PETSCSORTINT
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE) && !defined(FORTRANDOUBLEUNDERSCORE)
#define petscsortint_ petscsortint
#endif
#ifdef PETSC_HAVE_FORTRAN_CAPS
#define petscsortedremovedupsint_ PETSCSORTEDREMOVEDUPSINT
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE) && !defined(FORTRANDOUBLEUNDERSCORE)
#define petscsortedremovedupsint_ petscsortedremovedupsint
#endif
#ifdef PETSC_HAVE_FORTRAN_CAPS
#define petscsortremovedupsint_ PETSCSORTREMOVEDUPSINT
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE) && !defined(FORTRANDOUBLEUNDERSCORE)
#define petscsortremovedupsint_ petscsortremovedupsint
#endif
#ifdef PETSC_HAVE_FORTRAN_CAPS
#define petscfindint_ PETSCFINDINT
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE) && !defined(FORTRANDOUBLEUNDERSCORE)
#define petscfindint_ petscfindint
#endif
#ifdef PETSC_HAVE_FORTRAN_CAPS
#define petsccheckdupsint_ PETSCCHECKDUPSINT
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE) && !defined(FORTRANDOUBLEUNDERSCORE)
#define petsccheckdupsint_ petsccheckdupsint
#endif
#ifdef PETSC_HAVE_FORTRAN_CAPS
#define petscfindmpiint_ PETSCFINDMPIINT
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE) && !defined(FORTRANDOUBLEUNDERSCORE)
#define petscfindmpiint_ petscfindmpiint
#endif
#ifdef PETSC_HAVE_FORTRAN_CAPS
#define petscsortintwitharray_ PETSCSORTINTWITHARRAY
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE) && !defined(FORTRANDOUBLEUNDERSCORE)
#define petscsortintwitharray_ petscsortintwitharray
#endif
#ifdef PETSC_HAVE_FORTRAN_CAPS
#define petscsortintwitharraypair_ PETSCSORTINTWITHARRAYPAIR
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE) && !defined(FORTRANDOUBLEUNDERSCORE)
#define petscsortintwitharraypair_ petscsortintwitharraypair
#endif
#ifdef PETSC_HAVE_FORTRAN_CAPS
#define petscsortmpiint_ PETSCSORTMPIINT
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE) && !defined(FORTRANDOUBLEUNDERSCORE)
#define petscsortmpiint_ petscsortmpiint
#endif
#ifdef PETSC_HAVE_FORTRAN_CAPS
#define petscsortremovedupsmpiint_ PETSCSORTREMOVEDUPSMPIINT
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE) && !defined(FORTRANDOUBLEUNDERSCORE)
#define petscsortremovedupsmpiint_ petscsortremovedupsmpiint
#endif
#ifdef PETSC_HAVE_FORTRAN_CAPS
#define petscsortmpiintwitharray_ PETSCSORTMPIINTWITHARRAY
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE) && !defined(FORTRANDOUBLEUNDERSCORE)
#define petscsortmpiintwitharray_ petscsortmpiintwitharray
#endif
#ifdef PETSC_HAVE_FORTRAN_CAPS
#define petscsortmpiintwithintarray_ PETSCSORTMPIINTWITHINTARRAY
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE) && !defined(FORTRANDOUBLEUNDERSCORE)
#define petscsortmpiintwithintarray_ petscsortmpiintwithintarray
#endif
#ifdef PETSC_HAVE_FORTRAN_CAPS
#define petscsortintwithscalararray_ PETSCSORTINTWITHSCALARARRAY
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE) && !defined(FORTRANDOUBLEUNDERSCORE)
#define petscsortintwithscalararray_ petscsortintwithscalararray
#endif
#ifdef PETSC_HAVE_FORTRAN_CAPS
#define petscmergeintarray_ PETSCMERGEINTARRAY
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE) && !defined(FORTRANDOUBLEUNDERSCORE)
#define petscmergeintarray_ petscmergeintarray
#endif
#ifdef PETSC_HAVE_FORTRAN_CAPS
#define petscmergeintarraypair_ PETSCMERGEINTARRAYPAIR
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE) && !defined(FORTRANDOUBLEUNDERSCORE)
#define petscmergeintarraypair_ petscmergeintarraypair
#endif
#ifdef PETSC_HAVE_FORTRAN_CAPS
#define petscmergempiintarray_ PETSCMERGEMPIINTARRAY
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE) && !defined(FORTRANDOUBLEUNDERSCORE)
#define petscmergempiintarray_ petscmergempiintarray
#endif


/* Definitions of Fortran Wrapper routines */
#if defined(__cplusplus)
extern "C" {
#endif
PETSC_EXTERN void PETSC_STDCALL  petscsortint_(PetscInt *n,PetscInt X[], int *__ierr){
*__ierr = PetscSortInt(*n,X);
}
PETSC_EXTERN void PETSC_STDCALL  petscsortedremovedupsint_(PetscInt *n,PetscInt X[], int *__ierr){
*__ierr = PetscSortedRemoveDupsInt(n,X);
}
PETSC_EXTERN void PETSC_STDCALL  petscsortremovedupsint_(PetscInt *n,PetscInt X[], int *__ierr){
*__ierr = PetscSortRemoveDupsInt(n,X);
}
PETSC_EXTERN void PETSC_STDCALL  petscfindint_(PetscInt *key,PetscInt *n, PetscInt X[],PetscInt *loc, int *__ierr){
*__ierr = PetscFindInt(*key,*n,X,loc);
}
PETSC_EXTERN void PETSC_STDCALL  petsccheckdupsint_(PetscInt *n, PetscInt X[],PetscBool *dups, int *__ierr){
*__ierr = PetscCheckDupsInt(*n,X,dups);
}
PETSC_EXTERN void PETSC_STDCALL  petscfindmpiint_(PetscMPIInt *key,PetscInt *n, PetscMPIInt X[],PetscInt *loc, int *__ierr){
*__ierr = PetscFindMPIInt(*key,*n,X,loc);
}
PETSC_EXTERN void PETSC_STDCALL  petscsortintwitharray_(PetscInt *n,PetscInt X[],PetscInt Y[], int *__ierr){
*__ierr = PetscSortIntWithArray(*n,X,Y);
}
PETSC_EXTERN void PETSC_STDCALL  petscsortintwitharraypair_(PetscInt *n,PetscInt X[],PetscInt Y[],PetscInt Z[], int *__ierr){
*__ierr = PetscSortIntWithArrayPair(*n,X,Y,Z);
}
PETSC_EXTERN void PETSC_STDCALL  petscsortmpiint_(PetscInt *n,PetscMPIInt X[], int *__ierr){
*__ierr = PetscSortMPIInt(*n,X);
}
PETSC_EXTERN void PETSC_STDCALL  petscsortremovedupsmpiint_(PetscInt *n,PetscMPIInt X[], int *__ierr){
*__ierr = PetscSortRemoveDupsMPIInt(n,X);
}
PETSC_EXTERN void PETSC_STDCALL  petscsortmpiintwitharray_(PetscMPIInt *n,PetscMPIInt X[],PetscMPIInt Y[], int *__ierr){
*__ierr = PetscSortMPIIntWithArray(*n,X,Y);
}
PETSC_EXTERN void PETSC_STDCALL  petscsortmpiintwithintarray_(PetscMPIInt *n,PetscMPIInt X[],PetscInt Y[], int *__ierr){
*__ierr = PetscSortMPIIntWithIntArray(*n,X,Y);
}
PETSC_EXTERN void PETSC_STDCALL  petscsortintwithscalararray_(PetscInt *n,PetscInt X[],PetscScalar Y[], int *__ierr){
*__ierr = PetscSortIntWithScalarArray(*n,X,Y);
}
PETSC_EXTERN void PETSC_STDCALL  petscmergeintarray_(PetscInt *an, PetscInt aI[],PetscInt *bn, PetscInt bI[],PetscInt *n,PetscInt **L, int *__ierr){
*__ierr = PetscMergeIntArray(*an,aI,*bn,bI,n,L);
}
PETSC_EXTERN void PETSC_STDCALL  petscmergeintarraypair_(PetscInt *an, PetscInt aI[], PetscInt aJ[],PetscInt *bn, PetscInt bI[], PetscInt bJ[],PetscInt *n,PetscInt **L,PetscInt **J, int *__ierr){
*__ierr = PetscMergeIntArrayPair(*an,aI,aJ,*bn,bI,bJ,n,L,J);
}
PETSC_EXTERN void PETSC_STDCALL  petscmergempiintarray_(PetscInt *an, PetscMPIInt aI[],PetscInt *bn, PetscMPIInt bI[],PetscInt *n,PetscMPIInt **L, int *__ierr){
*__ierr = PetscMergeMPIIntArray(*an,aI,*bn,bI,n,L);
}
#if defined(__cplusplus)
}
#endif
