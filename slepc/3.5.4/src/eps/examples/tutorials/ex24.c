/*
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

static char help[] = "Spectrum folding for a standard symmetric eigenproblem.\n\n"
  "The problem matrix is the 2-D Laplacian.\n\n"
  "The command line options are:\n"
  "  -n <n>, where <n> = number of grid subdivisions in x dimension.\n"
  "  -m <m>, where <m> = number of grid subdivisions in y dimension.\n";

#include <slepceps.h>

/*
   User context for spectrum folding
*/
typedef struct {
  Mat       A;
  Vec       w;
  PetscReal target;
} CTX_FOLD;

/*
   Auxiliary routines
*/
PetscErrorCode MatMult_Fold(Mat,Vec,Vec);
PetscErrorCode RayleighQuotient(Mat,Vec,PetscScalar*);
PetscErrorCode ComputeResidualNorm(Mat,PetscScalar,Vec,PetscReal*);

#undef __FUNCT__
#define __FUNCT__ "main"
int main(int argc,char **argv)
{
  Mat            A,M,P;       /* problem matrix, shell matrix and preconditioner */
  Vec            x;           /* eigenvector */
  EPS            eps;         /* eigenproblem solver context */
  ST             st;          /* spectral transformation */
  KSP            ksp;
  PC             pc;
  EPSType        type;
  CTX_FOLD       *ctx;
  PetscInt       nconv,N,n=10,m,Istart,Iend,II,its,i,j;
  PetscReal      error,re,target=2.1;
  PetscScalar    lambda;
  PetscBool      flag;
  PetscErrorCode ierr;

  SlepcInitialize(&argc,&argv,(char*)0,help);

  ierr = PetscOptionsGetInt(NULL,"-n",&n,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsGetInt(NULL,"-m",&m,&flag);CHKERRQ(ierr);
  if (!flag) m=n;
  ierr = PetscOptionsGetReal(NULL,"-target",&target,NULL);CHKERRQ(ierr);
  N = n*m;
  ierr = PetscPrintf(PETSC_COMM_WORLD,"\nSpectrum Folding, N=%D (%Dx%D grid) target=%f\n\n",N,n,m,(double)target);CHKERRQ(ierr);

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     Compute the 5-point stencil Laplacian
     - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

  ierr = MatCreate(PETSC_COMM_WORLD,&A);CHKERRQ(ierr);
  ierr = MatSetSizes(A,PETSC_DECIDE,PETSC_DECIDE,N,N);CHKERRQ(ierr);
  ierr = MatSetFromOptions(A);CHKERRQ(ierr);
  ierr = MatSetUp(A);CHKERRQ(ierr);

  ierr = MatGetOwnershipRange(A,&Istart,&Iend);CHKERRQ(ierr);
  for (II=Istart;II<Iend;II++) {
    i = II/n; j = II-i*n;
    if (i>0) { ierr = MatSetValue(A,II,II-n,-1.0,INSERT_VALUES);CHKERRQ(ierr); }
    if (i<m-1) { ierr = MatSetValue(A,II,II+n,-1.0,INSERT_VALUES);CHKERRQ(ierr); }
    if (j>0) { ierr = MatSetValue(A,II,II-1,-1.0,INSERT_VALUES);CHKERRQ(ierr); }
    if (j<n-1) { ierr = MatSetValue(A,II,II+1,-1.0,INSERT_VALUES);CHKERRQ(ierr); }
    ierr = MatSetValue(A,II,II,4.0,INSERT_VALUES);CHKERRQ(ierr);
  }

  ierr = MatAssemblyBegin(A,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(A,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatGetVecs(A,&x,NULL);CHKERRQ(ierr);

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                Create shell matrix to perform spectrum folding
     - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
  ierr = PetscNew(&ctx);CHKERRQ(ierr);
  ctx->A = A;
  ctx->target = target;
  ierr = VecDuplicate(x,&ctx->w);CHKERRQ(ierr);

  ierr = MatCreateShell(PETSC_COMM_WORLD,N,N,N,N,ctx,&M);CHKERRQ(ierr);
  ierr = MatSetFromOptions(M);CHKERRQ(ierr);
  ierr = MatShellSetOperation(M,MATOP_MULT,(void(*)())MatMult_Fold);CHKERRQ(ierr);

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                Create the eigensolver and set various options
     - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

  ierr = EPSCreate(PETSC_COMM_WORLD,&eps);CHKERRQ(ierr);
  ierr = EPSSetOperators(eps,M,NULL);CHKERRQ(ierr);
  ierr = EPSSetProblemType(eps,EPS_HEP);CHKERRQ(ierr);
  ierr = EPSSetWhichEigenpairs(eps,EPS_SMALLEST_REAL);CHKERRQ(ierr);
  ierr = EPSSetFromOptions(eps);CHKERRQ(ierr);

  ierr = PetscObjectTypeCompareAny((PetscObject)eps,&flag,EPSBLOPEX,EPSRQCG,"");CHKERRQ(ierr);
  if (flag) {
    /*
       Build preconditioner specific for this application (diagonal of A^2)
    */
    ierr = MatGetRowSum(A,x);CHKERRQ(ierr);
    ierr = VecScale(x,-1.0);CHKERRQ(ierr);
    ierr = VecShift(x,20.0);CHKERRQ(ierr);
    ierr = MatCreate(PETSC_COMM_WORLD,&P);CHKERRQ(ierr);
    ierr = MatSetSizes(P,PETSC_DECIDE,PETSC_DECIDE,N,N);CHKERRQ(ierr);
    ierr = MatSetFromOptions(P);CHKERRQ(ierr);
    ierr = MatSetUp(P);CHKERRQ(ierr);
    ierr = MatDiagonalSet(P,x,INSERT_VALUES);CHKERRQ(ierr);
    /*
       Set diagonal preconditioner
    */
    ierr = EPSGetST(eps,&st);CHKERRQ(ierr);
    ierr = STSetType(st,STPRECOND);CHKERRQ(ierr);
    ierr = STPrecondSetMatForPC(st,P);CHKERRQ(ierr);
    ierr = MatDestroy(&P);CHKERRQ(ierr);
    ierr = STGetKSP(st,&ksp);CHKERRQ(ierr);
    ierr = KSPGetPC(ksp,&pc);CHKERRQ(ierr);
    ierr = PCSetType(pc,PCJACOBI);CHKERRQ(ierr);
  }

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                      Solve the eigensystem
     - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

  ierr = EPSSolve(eps);CHKERRQ(ierr);

  ierr = EPSGetIterationNumber(eps,&its);CHKERRQ(ierr);
  ierr = PetscPrintf(PETSC_COMM_WORLD," Number of iterations of the method: %D\n",its);CHKERRQ(ierr);
  ierr = EPSGetType(eps,&type);CHKERRQ(ierr);
  ierr = PetscPrintf(PETSC_COMM_WORLD," Solution method: %s\n\n",type);CHKERRQ(ierr);

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                    Display solution and clean up
     - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

  ierr = EPSGetConverged(eps,&nconv);CHKERRQ(ierr);
  ierr = PetscPrintf(PETSC_COMM_WORLD," Number of converged eigenpairs: %D\n\n",nconv);CHKERRQ(ierr);
  if (nconv>0) {
    /*
       Display result
    */
    ierr = PetscPrintf(PETSC_COMM_WORLD,
         "           k              ||Ax-kx||\n"
         "   ----------------- ------------------\n");CHKERRQ(ierr);

    for (i=0;i<nconv;i++) {
      /*
        Get i-th eigenvector, compute eigenvalue approximation from
        Rayleigh quotient and compute residual norm
      */
      ierr = EPSGetEigenpair(eps,i,NULL,NULL,x,NULL);CHKERRQ(ierr);
      ierr = RayleighQuotient(A,x,&lambda);CHKERRQ(ierr);
      ierr = ComputeResidualNorm(A,lambda,x,&error);CHKERRQ(ierr);

#if defined(PETSC_USE_COMPLEX)
      re = PetscRealPart(lambda);
#else
      re = lambda;
#endif
      ierr = PetscPrintf(PETSC_COMM_WORLD,"   %12f       %12.2g\n",(double)re,(double)error);CHKERRQ(ierr);
    }
    ierr = PetscPrintf(PETSC_COMM_WORLD,"\n");CHKERRQ(ierr);
  }

  ierr = EPSDestroy(&eps);CHKERRQ(ierr);
  ierr = MatDestroy(&A);CHKERRQ(ierr);
  ierr = MatDestroy(&M);CHKERRQ(ierr);
  ierr = VecDestroy(&ctx->w);CHKERRQ(ierr);
  ierr = VecDestroy(&x);CHKERRQ(ierr);
  ierr = PetscFree(ctx);CHKERRQ(ierr);
  ierr = SlepcFinalize();
  return 0;
}

#undef __FUNCT__
#define __FUNCT__ "MatMult_Fold"
/*
    Matrix-vector product subroutine for the spectrum folding.
       y <-- (A-t*I)^2*x
 */
PetscErrorCode MatMult_Fold(Mat M,Vec x,Vec y)
{
  CTX_FOLD       *ctx;
  PetscScalar    sigma;
  PetscErrorCode ierr;

  PetscFunctionBeginUser;
  ierr = MatShellGetContext(M,&ctx);CHKERRQ(ierr);
  sigma = -ctx->target;
  ierr = MatMult(ctx->A,x,ctx->w);CHKERRQ(ierr);
  ierr = VecAXPY(ctx->w,sigma,x);CHKERRQ(ierr);
  ierr = MatMult(ctx->A,ctx->w,y);CHKERRQ(ierr);
  ierr = VecAXPY(y,sigma,ctx->w);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "RayleighQuotient"
/*
    Computes the Rayleigh quotient of a vector x
       r <-- x^T*A*x       (assumes x has unit norm)
 */
PetscErrorCode RayleighQuotient(Mat A,Vec x,PetscScalar *r)
{
  Vec            Ax;
  PetscErrorCode ierr;

  PetscFunctionBeginUser;
  ierr = VecDuplicate(x,&Ax);CHKERRQ(ierr);
  ierr = MatMult(A,x,Ax);CHKERRQ(ierr);
  ierr = VecDot(Ax,x,r);CHKERRQ(ierr);
  ierr = VecDestroy(&Ax);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "ComputeResidualNorm"
/*
    Computes the residual norm of an approximate eigenvector x, |A*x-lambda*x|
 */
PetscErrorCode ComputeResidualNorm(Mat A,PetscScalar lambda,Vec x,PetscReal *r)
{
  Vec            Ax;
  PetscErrorCode ierr;

  PetscFunctionBeginUser;
  ierr = VecDuplicate(x,&Ax);CHKERRQ(ierr);
  ierr = MatMult(A,x,Ax);CHKERRQ(ierr);
  ierr = VecAXPY(Ax,-lambda,x);CHKERRQ(ierr);
  ierr = VecNorm(Ax,NORM_2,r);CHKERRQ(ierr);
  ierr = VecDestroy(&Ax);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

