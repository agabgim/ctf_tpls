#include "petscsys.h"
#include "petscfix.h"
#include "petsc/private/fortranimpl.h"
/* spacepoly.c */
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

#include "petscfe.h"
#ifdef PETSC_HAVE_FORTRAN_CAPS
#define petscspacepolynomialsettensor_ PETSCSPACEPOLYNOMIALSETTENSOR
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE) && !defined(FORTRANDOUBLEUNDERSCORE)
#define petscspacepolynomialsettensor_ petscspacepolynomialsettensor
#endif
#ifdef PETSC_HAVE_FORTRAN_CAPS
#define petscspacepolynomialgettensor_ PETSCSPACEPOLYNOMIALGETTENSOR
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE) && !defined(FORTRANDOUBLEUNDERSCORE)
#define petscspacepolynomialgettensor_ petscspacepolynomialgettensor
#endif
#ifdef PETSC_HAVE_FORTRAN_CAPS
#define petscspacepolynomialsetsymmetric_ PETSCSPACEPOLYNOMIALSETSYMMETRIC
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE) && !defined(FORTRANDOUBLEUNDERSCORE)
#define petscspacepolynomialsetsymmetric_ petscspacepolynomialsetsymmetric
#endif
#ifdef PETSC_HAVE_FORTRAN_CAPS
#define petscspacepolynomialgetsymmetric_ PETSCSPACEPOLYNOMIALGETSYMMETRIC
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE) && !defined(FORTRANDOUBLEUNDERSCORE)
#define petscspacepolynomialgetsymmetric_ petscspacepolynomialgetsymmetric
#endif


/* Definitions of Fortran Wrapper routines */
#if defined(__cplusplus)
extern "C" {
#endif
PETSC_EXTERN void PETSC_STDCALL  petscspacepolynomialsettensor_(PetscSpace sp,PetscBool *tensor, int *__ierr){
*__ierr = PetscSpacePolynomialSetTensor(
	(PetscSpace)PetscToPointer((sp) ),*tensor);
}
PETSC_EXTERN void PETSC_STDCALL  petscspacepolynomialgettensor_(PetscSpace sp,PetscBool *tensor, int *__ierr){
*__ierr = PetscSpacePolynomialGetTensor(
	(PetscSpace)PetscToPointer((sp) ),tensor);
}
PETSC_EXTERN void PETSC_STDCALL  petscspacepolynomialsetsymmetric_(PetscSpace sp,PetscBool *sym, int *__ierr){
*__ierr = PetscSpacePolynomialSetSymmetric(
	(PetscSpace)PetscToPointer((sp) ),*sym);
}
PETSC_EXTERN void PETSC_STDCALL  petscspacepolynomialgetsymmetric_(PetscSpace sp,PetscBool *sym, int *__ierr){
*__ierr = PetscSpacePolynomialGetSymmetric(
	(PetscSpace)PetscToPointer((sp) ),sym);
}
#if defined(__cplusplus)
}
#endif
