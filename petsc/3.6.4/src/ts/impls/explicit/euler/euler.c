/*
       Code for Timestepping with explicit Euler.
*/
#include <petsc/private/tsimpl.h>                /*I   "petscts.h"   I*/

typedef struct {
  Vec update;     /* work vector where new solution is formed  */
} TS_Euler;

#undef __FUNCT__
#define __FUNCT__ "TSStep_Euler"
static PetscErrorCode TSStep_Euler(TS ts)
{
  TS_Euler       *euler = (TS_Euler*)ts->data;
  Vec            sol    = ts->vec_sol,update = euler->update;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = TSPreStep(ts);CHKERRQ(ierr);
  ierr = TSPreStage(ts,ts->ptime);CHKERRQ(ierr);
  ierr = TSComputeRHSFunction(ts,ts->ptime,sol,update);CHKERRQ(ierr);
  ierr = VecAXPY(sol,ts->time_step,update);CHKERRQ(ierr);
  ierr = TSPostStage(ts,ts->ptime,0,&sol);CHKERRQ(ierr);
  ts->ptime += ts->time_step;
  ts->steps++;
  PetscFunctionReturn(0);
}
/*------------------------------------------------------------*/

#undef __FUNCT__
#define __FUNCT__ "TSSetUp_Euler"
static PetscErrorCode TSSetUp_Euler(TS ts)
{
  TS_Euler       *euler = (TS_Euler*)ts->data;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = VecDuplicate(ts->vec_sol,&euler->update);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TSReset_Euler"
static PetscErrorCode TSReset_Euler(TS ts)
{
  TS_Euler       *euler = (TS_Euler*)ts->data;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = VecDestroy(&euler->update);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TSDestroy_Euler"
static PetscErrorCode TSDestroy_Euler(TS ts)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = TSReset_Euler(ts);CHKERRQ(ierr);
  ierr = PetscFree(ts->data);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}
/*------------------------------------------------------------*/

#undef __FUNCT__
#define __FUNCT__ "TSSetFromOptions_Euler"
static PetscErrorCode TSSetFromOptions_Euler(PetscOptions *PetscOptionsObject,TS ts)
{
  PetscFunctionBegin;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TSView_Euler"
static PetscErrorCode TSView_Euler(TS ts,PetscViewer viewer)
{
  PetscFunctionBegin;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TSInterpolate_Euler"
static PetscErrorCode TSInterpolate_Euler(TS ts,PetscReal t,Vec X)
{
  TS_Euler       *euler = (TS_Euler*)ts->data;
  Vec            update = euler->update;
  PetscReal      alpha = (ts->ptime - t)/ts->time_step;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = VecWAXPY(X,-ts->time_step,update,ts->vec_sol);CHKERRQ(ierr);
  ierr = VecAXPBY(X,1.0-alpha,alpha,ts->vec_sol);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TSComputeLinearStability_Euler"
PetscErrorCode TSComputeLinearStability_Euler(TS ts,PetscReal xr,PetscReal xi,PetscReal *yr,PetscReal *yi)
{
  PetscFunctionBegin;
  *yr = 1.0 + xr;
  *yi = xi;
  PetscFunctionReturn(0);
}
/* ------------------------------------------------------------ */

/*MC
      TSEULER - ODE solver using the explicit forward Euler method

  Level: beginner

.seealso:  TSCreate(), TS, TSSetType(), TSBEULER

M*/
#undef __FUNCT__
#define __FUNCT__ "TSCreate_Euler"
PETSC_EXTERN PetscErrorCode TSCreate_Euler(TS ts)
{
  TS_Euler       *euler;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ts->ops->setup           = TSSetUp_Euler;
  ts->ops->step            = TSStep_Euler;
  ts->ops->reset           = TSReset_Euler;
  ts->ops->destroy         = TSDestroy_Euler;
  ts->ops->setfromoptions  = TSSetFromOptions_Euler;
  ts->ops->view            = TSView_Euler;
  ts->ops->interpolate     = TSInterpolate_Euler;
  ts->ops->linearstability = TSComputeLinearStability_Euler;

  ierr = PetscNewLog(ts,&euler);CHKERRQ(ierr);
  ts->data = (void*)euler;
  PetscFunctionReturn(0);
}
