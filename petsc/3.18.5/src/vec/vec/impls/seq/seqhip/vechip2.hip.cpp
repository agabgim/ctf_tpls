/*
   Implements the sequential hip vectors.
*/

#define PETSC_SKIP_SPINLOCK

#include <petscconf.h>
#include <petsc/private/vecimpl.h>
#include <../src/vec/vec/impls/dvecimpl.h>
#include <petsc/private/hipvecimpl.h>

#include <hip/hip_runtime.h>
#include <thrust/device_ptr.h>
#include <thrust/transform.h>
#include <thrust/functional.h>
#include <thrust/reduce.h>

#if defined(PETSC_USE_COMPLEX)
/* SPOCK compilation issues, need to unroll division and multiplication with complex numbers */
struct PetscDivideComplex {
  __host__ __device__ PetscScalar operator()(const PetscScalar &lhs, const PetscScalar &rhs)
  {
    PetscReal lx = PetscRealPart(lhs);
    PetscReal ly = PetscImaginaryPart(lhs);
    PetscReal rx = PetscRealPart(rhs);
    PetscReal ry = PetscImaginaryPart(rhs);
    PetscReal n  = rx * rx + ry * ry;
    return PetscComplex((lx * rx + ly * ry) / n, (rx * ly - lx * ry) / n);
  }
};

struct PetscMultiplyComplex {
  __host__ __device__ PetscScalar operator()(const PetscScalar &lhs, const PetscScalar &rhs)
  {
    PetscReal lx = PetscRealPart(lhs);
    PetscReal ly = PetscImaginaryPart(lhs);
    PetscReal rx = PetscRealPart(rhs);
    PetscReal ry = PetscImaginaryPart(rhs);
    return PetscComplex(lx * rx - ly * ry, ly * rx + lx * ry);
  }
};
#endif

/*
    Allocates space for the vector array on the GPU if it does not exist.
    Does NOT change the PetscHIPFlag for the vector
    Does NOT zero the HIP array

 */
PetscErrorCode VecHIPAllocateCheck(Vec v)
{
  Vec_HIP  *vechip;
  PetscBool option_set;

  PetscFunctionBegin;
  if (!v->spptr) {
    PetscReal pinned_memory_min;

    PetscCall(PetscCalloc(sizeof(Vec_HIP), &v->spptr));
    vechip = (Vec_HIP *)v->spptr;
    PetscCallHIP(hipMalloc((void **)&vechip->GPUarray_allocated, sizeof(PetscScalar) * ((PetscBLASInt)v->map->n)));
    vechip->GPUarray = vechip->GPUarray_allocated;
    if (v->offloadmask == PETSC_OFFLOAD_UNALLOCATED) {
      if (v->data && ((Vec_Seq *)v->data)->array) {
        v->offloadmask = PETSC_OFFLOAD_CPU;
      } else {
        v->offloadmask = PETSC_OFFLOAD_GPU;
      }
    }
    pinned_memory_min = 0;

    /* Need to parse command line for minimum size to use for pinned memory allocations on host here.
       Note: This same code duplicated in VecCreate_SeqHIP_Private() and VecCreate_MPIHIP_Private(). Is there a good way to avoid this? */
    PetscOptionsBegin(PetscObjectComm((PetscObject)v), ((PetscObject)v)->prefix, "VECHIP Options", "Vec");
    PetscCall(PetscOptionsReal("-vec_pinned_memory_min", "Minimum size (in bytes) for an allocation to use pinned memory on host", "VecSetPinnedMemoryMin", pinned_memory_min, &pinned_memory_min, &option_set));
    if (option_set) v->minimum_bytes_pinned_memory = pinned_memory_min;
    PetscOptionsEnd();
  }
  PetscFunctionReturn(0);
}

/* Copies a vector from the CPU to the GPU unless we already have an up-to-date copy on the GPU */
PetscErrorCode VecHIPCopyToGPU(Vec v)
{
  Vec_HIP     *vechip;
  PetscScalar *varray;

  PetscFunctionBegin;
  PetscCheckTypeNames(v, VECSEQHIP, VECMPIHIP);
  PetscCall(VecHIPAllocateCheck(v));
  if (v->offloadmask == PETSC_OFFLOAD_CPU) {
    PetscCall(PetscLogEventBegin(VEC_HIPCopyToGPU, v, 0, 0, 0));
    vechip = (Vec_HIP *)v->spptr;
    varray = vechip->GPUarray;
    PetscCallHIP(hipMemcpy(varray, ((Vec_Seq *)v->data)->array, v->map->n * sizeof(PetscScalar), hipMemcpyHostToDevice));
    PetscCall(PetscLogCpuToGpu((v->map->n) * sizeof(PetscScalar)));
    PetscCall(PetscLogEventEnd(VEC_HIPCopyToGPU, v, 0, 0, 0));
    v->offloadmask = PETSC_OFFLOAD_BOTH;
  }
  PetscFunctionReturn(0);
}

/*
     VecHIPCopyFromGPU - Copies a vector from the GPU to the CPU unless we already have an up-to-date copy on the CPU
*/
PetscErrorCode VecHIPCopyFromGPU(Vec v)
{
  Vec_HIP     *vechip;
  PetscScalar *varray;

  PetscFunctionBegin;
  PetscCheckTypeNames(v, VECSEQHIP, VECMPIHIP);
  PetscCall(VecHIPAllocateCheckHost(v));
  if (v->offloadmask == PETSC_OFFLOAD_GPU) {
    PetscCall(PetscLogEventBegin(VEC_HIPCopyFromGPU, v, 0, 0, 0));
    vechip = (Vec_HIP *)v->spptr;
    varray = vechip->GPUarray;
    PetscCallHIP(hipMemcpy(((Vec_Seq *)v->data)->array, varray, v->map->n * sizeof(PetscScalar), hipMemcpyDeviceToHost));
    PetscCall(PetscLogGpuToCpu((v->map->n) * sizeof(PetscScalar)));
    PetscCall(PetscLogEventEnd(VEC_HIPCopyFromGPU, v, 0, 0, 0));
    v->offloadmask = PETSC_OFFLOAD_BOTH;
  }
  PetscFunctionReturn(0);
}

/*MC
   VECSEQHIP - VECSEQHIP = "seqhip" - The basic sequential vector, modified to use HIP

   Options Database Keys:
. -vec_type seqhip - sets the vector type to VECSEQHIP during a call to VecSetFromOptions()

  Level: beginner

.seealso: `VecCreate()`, `VecSetType()`, `VecSetFromOptions()`, `VecCreateSeqWithArray()`, `VECMPI`, `VecType`, `VecCreateMPI()`, `VecCreateSeq()`
M*/

PetscErrorCode VecAYPX_SeqHIP(Vec yin, PetscScalar alpha, Vec xin)
{
  const PetscScalar *xarray;
  PetscScalar       *yarray;
  PetscBLASInt       one = 1, bn = 0;
  PetscScalar        sone = 1.0;
  hipblasHandle_t    hipblasv2handle;

  PetscFunctionBegin;
  PetscCall(PetscHIPBLASGetHandle(&hipblasv2handle));
  PetscCall(PetscBLASIntCast(yin->map->n, &bn));
  PetscCall(VecHIPGetArrayRead(xin, &xarray));
  PetscCall(VecHIPGetArray(yin, &yarray));
  PetscCall(PetscLogGpuTimeBegin());
  if (alpha == (PetscScalar)0.0) {
    PetscCallHIP(hipMemcpy(yarray, xarray, bn * sizeof(PetscScalar), hipMemcpyDeviceToDevice));
  } else if (alpha == (PetscScalar)1.0) {
    PetscCallHIPBLAS(hipblasXaxpy(hipblasv2handle, bn, &alpha, xarray, one, yarray, one));
    PetscCall(PetscLogGpuFlops(1.0 * yin->map->n));
  } else {
    PetscCallHIPBLAS(hipblasXscal(hipblasv2handle, bn, &alpha, yarray, one));
    PetscCallHIPBLAS(hipblasXaxpy(hipblasv2handle, bn, &sone, xarray, one, yarray, one));
    PetscCall(PetscLogGpuFlops(2.0 * yin->map->n));
  }
  PetscCall(PetscLogGpuTimeEnd());
  PetscCall(VecHIPRestoreArrayRead(xin, &xarray));
  PetscCall(VecHIPRestoreArray(yin, &yarray));
  PetscCall(PetscLogCpuToGpuScalar(sizeof(PetscScalar)));
  PetscFunctionReturn(0);
}

PetscErrorCode VecAXPY_SeqHIP(Vec yin, PetscScalar alpha, Vec xin)
{
  const PetscScalar *xarray;
  PetscScalar       *yarray;
  PetscBLASInt       one = 1, bn = 0;
  hipblasHandle_t    hipblasv2handle;
  PetscBool          xiship;

  PetscFunctionBegin;
  if (alpha == (PetscScalar)0.0) PetscFunctionReturn(0);
  PetscCall(PetscHIPBLASGetHandle(&hipblasv2handle));
  PetscCall(PetscObjectTypeCompareAny((PetscObject)xin, &xiship, VECSEQHIP, VECMPIHIP, ""));
  if (xiship) {
    PetscCall(PetscBLASIntCast(yin->map->n, &bn));
    PetscCall(VecHIPGetArrayRead(xin, &xarray));
    PetscCall(VecHIPGetArray(yin, &yarray));
    PetscCall(PetscLogGpuTimeBegin());
    PetscCallHIPBLAS(hipblasXaxpy(hipblasv2handle, bn, &alpha, xarray, one, yarray, one));
    PetscCall(PetscLogGpuTimeEnd());
    PetscCall(VecHIPRestoreArrayRead(xin, &xarray));
    PetscCall(VecHIPRestoreArray(yin, &yarray));
    PetscCall(PetscLogGpuFlops(2.0 * yin->map->n));
    PetscCall(PetscLogCpuToGpuScalar(sizeof(PetscScalar)));
  } else {
    PetscCall(VecAXPY_Seq(yin, alpha, xin));
  }
  PetscFunctionReturn(0);
}

PetscErrorCode VecPointwiseDivide_SeqHIP(Vec win, Vec xin, Vec yin)
{
  PetscInt                              n      = xin->map->n;
  const PetscScalar                    *xarray = NULL, *yarray = NULL;
  PetscScalar                          *warray = NULL;
  thrust::device_ptr<const PetscScalar> xptr, yptr;
  thrust::device_ptr<PetscScalar>       wptr;

  PetscFunctionBegin;
  if (xin->boundtocpu || yin->boundtocpu) {
    PetscCall(VecPointwiseDivide_Seq(win, xin, yin));
    PetscFunctionReturn(0);
  }
  PetscCall(VecHIPGetArrayWrite(win, &warray));
  PetscCall(VecHIPGetArrayRead(xin, &xarray));
  PetscCall(VecHIPGetArrayRead(yin, &yarray));
  PetscCall(PetscLogGpuTimeBegin());
  try {
    wptr = thrust::device_pointer_cast(warray);
    xptr = thrust::device_pointer_cast(xarray);
    yptr = thrust::device_pointer_cast(yarray);
#if defined(PETSC_USE_COMPLEX)
    thrust::transform(xptr, xptr + n, yptr, wptr, PetscDivideComplex());
#else
    thrust::transform(xptr, xptr + n, yptr, wptr, thrust::divides<PetscScalar>());
#endif
  } catch (char *ex) {
    SETERRQ(PETSC_COMM_SELF, PETSC_ERR_LIB, "Thrust error: %s", ex);
  }
  PetscCall(PetscLogGpuTimeEnd());
  PetscCall(PetscLogGpuFlops(n));
  PetscCall(VecHIPRestoreArrayRead(xin, &xarray));
  PetscCall(VecHIPRestoreArrayRead(yin, &yarray));
  PetscCall(VecHIPRestoreArrayWrite(win, &warray));
  PetscFunctionReturn(0);
}

PetscErrorCode VecWAXPY_SeqHIP(Vec win, PetscScalar alpha, Vec xin, Vec yin)
{
  const PetscScalar *xarray = NULL, *yarray = NULL;
  PetscScalar       *warray = NULL;
  PetscBLASInt       one = 1, bn = 0;
  hipblasHandle_t    hipblasv2handle;

  PetscFunctionBegin;
  PetscCall(PetscHIPBLASGetHandle(&hipblasv2handle));
  PetscCall(PetscBLASIntCast(win->map->n, &bn));
  if (alpha == (PetscScalar)0.0) {
    PetscCall(VecCopy_SeqHIP(yin, win));
  } else {
    PetscCall(VecHIPGetArrayRead(xin, &xarray));
    PetscCall(VecHIPGetArrayRead(yin, &yarray));
    PetscCall(VecHIPGetArrayWrite(win, &warray));
    PetscCall(PetscLogGpuTimeBegin());
    PetscCallHIP(hipMemcpy(warray, yarray, win->map->n * sizeof(PetscScalar), hipMemcpyDeviceToDevice));
    PetscCallHIPBLAS(hipblasXaxpy(hipblasv2handle, bn, &alpha, xarray, one, warray, one));
    PetscCall(PetscLogGpuTimeEnd());
    PetscCall(PetscLogGpuFlops(2 * win->map->n));
    PetscCall(VecHIPRestoreArrayRead(xin, &xarray));
    PetscCall(VecHIPRestoreArrayRead(yin, &yarray));
    PetscCall(VecHIPRestoreArrayWrite(win, &warray));
    PetscCall(PetscLogCpuToGpuScalar(sizeof(PetscScalar)));
  }
  PetscFunctionReturn(0);
}

PetscErrorCode VecMAXPY_SeqHIP(Vec xin, PetscInt nv, const PetscScalar *alpha, Vec *y)
{
  PetscInt           n = xin->map->n, j;
  PetscScalar       *xarray;
  const PetscScalar *yarray;
  PetscBLASInt       one = 1, bn = 0;
  hipblasHandle_t    hipblasv2handle;

  PetscFunctionBegin;
  PetscCall(PetscLogGpuFlops(nv * 2.0 * n));
  PetscCall(PetscLogCpuToGpuScalar(nv * sizeof(PetscScalar)));
  PetscCall(PetscHIPBLASGetHandle(&hipblasv2handle));
  PetscCall(PetscBLASIntCast(n, &bn));
  PetscCall(VecHIPGetArray(xin, &xarray));
  PetscCall(PetscLogGpuTimeBegin());
  for (j = 0; j < nv; j++) {
    PetscCall(VecHIPGetArrayRead(y[j], &yarray));
    PetscCallHIPBLAS(hipblasXaxpy(hipblasv2handle, bn, alpha + j, yarray, one, xarray, one));
    PetscCall(VecHIPRestoreArrayRead(y[j], &yarray));
  }
  PetscCall(PetscLogGpuTimeEnd());
  PetscCall(VecHIPRestoreArray(xin, &xarray));
  PetscFunctionReturn(0);
}

PetscErrorCode VecDot_SeqHIP(Vec xin, Vec yin, PetscScalar *z)
{
  const PetscScalar *xarray, *yarray;
  PetscBLASInt       one = 1, bn = 0;
  hipblasHandle_t    hipblasv2handle;

  PetscFunctionBegin;
  PetscCall(PetscHIPBLASGetHandle(&hipblasv2handle));
  PetscCall(PetscBLASIntCast(yin->map->n, &bn));
  PetscCall(VecHIPGetArrayRead(xin, &xarray));
  PetscCall(VecHIPGetArrayRead(yin, &yarray));
  /* arguments y, x are reversed because BLAS complex conjugates the first argument, PETSc the second */
  PetscCall(PetscLogGpuTimeBegin());
  PetscCallHIPBLAS(hipblasXdot(hipblasv2handle, bn, yarray, one, xarray, one, z));
  PetscCall(PetscLogGpuTimeEnd());
  if (xin->map->n > 0) PetscCall(PetscLogGpuFlops(2.0 * xin->map->n - 1));
  PetscCall(PetscLogGpuToCpu(sizeof(PetscScalar)));
  PetscCall(VecHIPRestoreArrayRead(xin, &xarray));
  PetscCall(VecHIPRestoreArrayRead(yin, &yarray));
  PetscFunctionReturn(0);
}

//
// HIP kernels for MDot to follow
//

// set work group size to be a power of 2 (128 is usually a good compromise between portability and speed)
#define MDOT_WORKGROUP_SIZE 128
#define MDOT_WORKGROUP_NUM  128

#if !defined(PETSC_USE_COMPLEX)
// M = 2:
__global__ void VecMDot_SeqHIP_kernel2(const PetscScalar *x, const PetscScalar *y0, const PetscScalar *y1, PetscInt size, PetscScalar *group_results)
{
  __shared__ PetscScalar tmp_buffer[2 * MDOT_WORKGROUP_SIZE];
  PetscInt               entries_per_group = (size - 1) / gridDim.x + 1;
  entries_per_group                        = (entries_per_group == 0) ? 1 : entries_per_group; // for very small vectors, a group should still do some work
  PetscInt vec_start_index                 = blockIdx.x * entries_per_group;
  PetscInt vec_stop_index                  = PetscMin((blockIdx.x + 1) * entries_per_group, size); // don't go beyond vec size

  PetscScalar entry_x    = 0;
  PetscScalar group_sum0 = 0;
  PetscScalar group_sum1 = 0;
  for (PetscInt i = vec_start_index + threadIdx.x; i < vec_stop_index; i += blockDim.x) {
    entry_x = x[i]; // load only once from global memory!
    group_sum0 += entry_x * y0[i];
    group_sum1 += entry_x * y1[i];
  }
  tmp_buffer[threadIdx.x]                       = group_sum0;
  tmp_buffer[threadIdx.x + MDOT_WORKGROUP_SIZE] = group_sum1;

  // parallel reduction
  for (PetscInt stride = blockDim.x / 2; stride > 0; stride /= 2) {
    __syncthreads();
    if (threadIdx.x < stride) {
      tmp_buffer[threadIdx.x] += tmp_buffer[threadIdx.x + stride];
      tmp_buffer[threadIdx.x + MDOT_WORKGROUP_SIZE] += tmp_buffer[threadIdx.x + stride + MDOT_WORKGROUP_SIZE];
    }
  }

  // write result of group to group_results
  if (threadIdx.x == 0) {
    group_results[blockIdx.x]             = tmp_buffer[0];
    group_results[blockIdx.x + gridDim.x] = tmp_buffer[MDOT_WORKGROUP_SIZE];
  }
}

// M = 3:
__global__ void VecMDot_SeqHIP_kernel3(const PetscScalar *x, const PetscScalar *y0, const PetscScalar *y1, const PetscScalar *y2, PetscInt size, PetscScalar *group_results)
{
  __shared__ PetscScalar tmp_buffer[3 * MDOT_WORKGROUP_SIZE];
  PetscInt               entries_per_group = (size - 1) / gridDim.x + 1;
  entries_per_group                        = (entries_per_group == 0) ? 1 : entries_per_group; // for very small vectors, a group should still do some work
  PetscInt vec_start_index                 = blockIdx.x * entries_per_group;
  PetscInt vec_stop_index                  = PetscMin((blockIdx.x + 1) * entries_per_group, size); // don't go beyond vec size

  PetscScalar entry_x    = 0;
  PetscScalar group_sum0 = 0;
  PetscScalar group_sum1 = 0;
  PetscScalar group_sum2 = 0;
  for (PetscInt i = vec_start_index + threadIdx.x; i < vec_stop_index; i += blockDim.x) {
    entry_x = x[i]; // load only once from global memory!
    group_sum0 += entry_x * y0[i];
    group_sum1 += entry_x * y1[i];
    group_sum2 += entry_x * y2[i];
  }
  tmp_buffer[threadIdx.x]                           = group_sum0;
  tmp_buffer[threadIdx.x + MDOT_WORKGROUP_SIZE]     = group_sum1;
  tmp_buffer[threadIdx.x + 2 * MDOT_WORKGROUP_SIZE] = group_sum2;

  // parallel reduction
  for (PetscInt stride = blockDim.x / 2; stride > 0; stride /= 2) {
    __syncthreads();
    if (threadIdx.x < stride) {
      tmp_buffer[threadIdx.x] += tmp_buffer[threadIdx.x + stride];
      tmp_buffer[threadIdx.x + MDOT_WORKGROUP_SIZE] += tmp_buffer[threadIdx.x + stride + MDOT_WORKGROUP_SIZE];
      tmp_buffer[threadIdx.x + 2 * MDOT_WORKGROUP_SIZE] += tmp_buffer[threadIdx.x + stride + 2 * MDOT_WORKGROUP_SIZE];
    }
  }

  // write result of group to group_results
  if (threadIdx.x == 0) {
    group_results[blockIdx.x]                 = tmp_buffer[0];
    group_results[blockIdx.x + gridDim.x]     = tmp_buffer[MDOT_WORKGROUP_SIZE];
    group_results[blockIdx.x + 2 * gridDim.x] = tmp_buffer[2 * MDOT_WORKGROUP_SIZE];
  }
}

// M = 4:
__global__ void VecMDot_SeqHIP_kernel4(const PetscScalar *x, const PetscScalar *y0, const PetscScalar *y1, const PetscScalar *y2, const PetscScalar *y3, PetscInt size, PetscScalar *group_results)
{
  __shared__ PetscScalar tmp_buffer[4 * MDOT_WORKGROUP_SIZE];
  PetscInt               entries_per_group = (size - 1) / gridDim.x + 1;
  entries_per_group                        = (entries_per_group == 0) ? 1 : entries_per_group; // for very small vectors, a group should still do some work
  PetscInt vec_start_index                 = blockIdx.x * entries_per_group;
  PetscInt vec_stop_index                  = PetscMin((blockIdx.x + 1) * entries_per_group, size); // don't go beyond vec size

  PetscScalar entry_x    = 0;
  PetscScalar group_sum0 = 0;
  PetscScalar group_sum1 = 0;
  PetscScalar group_sum2 = 0;
  PetscScalar group_sum3 = 0;
  for (PetscInt i = vec_start_index + threadIdx.x; i < vec_stop_index; i += blockDim.x) {
    entry_x = x[i]; // load only once from global memory!
    group_sum0 += entry_x * y0[i];
    group_sum1 += entry_x * y1[i];
    group_sum2 += entry_x * y2[i];
    group_sum3 += entry_x * y3[i];
  }
  tmp_buffer[threadIdx.x]                           = group_sum0;
  tmp_buffer[threadIdx.x + MDOT_WORKGROUP_SIZE]     = group_sum1;
  tmp_buffer[threadIdx.x + 2 * MDOT_WORKGROUP_SIZE] = group_sum2;
  tmp_buffer[threadIdx.x + 3 * MDOT_WORKGROUP_SIZE] = group_sum3;

  // parallel reduction
  for (PetscInt stride = blockDim.x / 2; stride > 0; stride /= 2) {
    __syncthreads();
    if (threadIdx.x < stride) {
      tmp_buffer[threadIdx.x] += tmp_buffer[threadIdx.x + stride];
      tmp_buffer[threadIdx.x + MDOT_WORKGROUP_SIZE] += tmp_buffer[threadIdx.x + stride + MDOT_WORKGROUP_SIZE];
      tmp_buffer[threadIdx.x + 2 * MDOT_WORKGROUP_SIZE] += tmp_buffer[threadIdx.x + stride + 2 * MDOT_WORKGROUP_SIZE];
      tmp_buffer[threadIdx.x + 3 * MDOT_WORKGROUP_SIZE] += tmp_buffer[threadIdx.x + stride + 3 * MDOT_WORKGROUP_SIZE];
    }
  }

  // write result of group to group_results
  if (threadIdx.x == 0) {
    group_results[blockIdx.x]                 = tmp_buffer[0];
    group_results[blockIdx.x + gridDim.x]     = tmp_buffer[MDOT_WORKGROUP_SIZE];
    group_results[blockIdx.x + 2 * gridDim.x] = tmp_buffer[2 * MDOT_WORKGROUP_SIZE];
    group_results[blockIdx.x + 3 * gridDim.x] = tmp_buffer[3 * MDOT_WORKGROUP_SIZE];
  }
}

// M = 8:
__global__ void VecMDot_SeqHIP_kernel8(const PetscScalar *x, const PetscScalar *y0, const PetscScalar *y1, const PetscScalar *y2, const PetscScalar *y3, const PetscScalar *y4, const PetscScalar *y5, const PetscScalar *y6, const PetscScalar *y7, PetscInt size, PetscScalar *group_results)
{
  __shared__ PetscScalar tmp_buffer[8 * MDOT_WORKGROUP_SIZE];
  PetscInt               entries_per_group = (size - 1) / gridDim.x + 1;
  entries_per_group                        = (entries_per_group == 0) ? 1 : entries_per_group; // for very small vectors, a group should still do some work
  PetscInt vec_start_index                 = blockIdx.x * entries_per_group;
  PetscInt vec_stop_index                  = PetscMin((blockIdx.x + 1) * entries_per_group, size); // don't go beyond vec size

  PetscScalar entry_x    = 0;
  PetscScalar group_sum0 = 0;
  PetscScalar group_sum1 = 0;
  PetscScalar group_sum2 = 0;
  PetscScalar group_sum3 = 0;
  PetscScalar group_sum4 = 0;
  PetscScalar group_sum5 = 0;
  PetscScalar group_sum6 = 0;
  PetscScalar group_sum7 = 0;
  for (PetscInt i = vec_start_index + threadIdx.x; i < vec_stop_index; i += blockDim.x) {
    entry_x = x[i]; // load only once from global memory!
    group_sum0 += entry_x * y0[i];
    group_sum1 += entry_x * y1[i];
    group_sum2 += entry_x * y2[i];
    group_sum3 += entry_x * y3[i];
    group_sum4 += entry_x * y4[i];
    group_sum5 += entry_x * y5[i];
    group_sum6 += entry_x * y6[i];
    group_sum7 += entry_x * y7[i];
  }
  tmp_buffer[threadIdx.x]                           = group_sum0;
  tmp_buffer[threadIdx.x + MDOT_WORKGROUP_SIZE]     = group_sum1;
  tmp_buffer[threadIdx.x + 2 * MDOT_WORKGROUP_SIZE] = group_sum2;
  tmp_buffer[threadIdx.x + 3 * MDOT_WORKGROUP_SIZE] = group_sum3;
  tmp_buffer[threadIdx.x + 4 * MDOT_WORKGROUP_SIZE] = group_sum4;
  tmp_buffer[threadIdx.x + 5 * MDOT_WORKGROUP_SIZE] = group_sum5;
  tmp_buffer[threadIdx.x + 6 * MDOT_WORKGROUP_SIZE] = group_sum6;
  tmp_buffer[threadIdx.x + 7 * MDOT_WORKGROUP_SIZE] = group_sum7;

  // parallel reduction
  for (PetscInt stride = blockDim.x / 2; stride > 0; stride /= 2) {
    __syncthreads();
    if (threadIdx.x < stride) {
      tmp_buffer[threadIdx.x] += tmp_buffer[threadIdx.x + stride];
      tmp_buffer[threadIdx.x + MDOT_WORKGROUP_SIZE] += tmp_buffer[threadIdx.x + stride + MDOT_WORKGROUP_SIZE];
      tmp_buffer[threadIdx.x + 2 * MDOT_WORKGROUP_SIZE] += tmp_buffer[threadIdx.x + stride + 2 * MDOT_WORKGROUP_SIZE];
      tmp_buffer[threadIdx.x + 3 * MDOT_WORKGROUP_SIZE] += tmp_buffer[threadIdx.x + stride + 3 * MDOT_WORKGROUP_SIZE];
      tmp_buffer[threadIdx.x + 4 * MDOT_WORKGROUP_SIZE] += tmp_buffer[threadIdx.x + stride + 4 * MDOT_WORKGROUP_SIZE];
      tmp_buffer[threadIdx.x + 5 * MDOT_WORKGROUP_SIZE] += tmp_buffer[threadIdx.x + stride + 5 * MDOT_WORKGROUP_SIZE];
      tmp_buffer[threadIdx.x + 6 * MDOT_WORKGROUP_SIZE] += tmp_buffer[threadIdx.x + stride + 6 * MDOT_WORKGROUP_SIZE];
      tmp_buffer[threadIdx.x + 7 * MDOT_WORKGROUP_SIZE] += tmp_buffer[threadIdx.x + stride + 7 * MDOT_WORKGROUP_SIZE];
    }
  }

  // write result of group to group_results
  if (threadIdx.x == 0) {
    group_results[blockIdx.x]                 = tmp_buffer[0];
    group_results[blockIdx.x + gridDim.x]     = tmp_buffer[MDOT_WORKGROUP_SIZE];
    group_results[blockIdx.x + 2 * gridDim.x] = tmp_buffer[2 * MDOT_WORKGROUP_SIZE];
    group_results[blockIdx.x + 3 * gridDim.x] = tmp_buffer[3 * MDOT_WORKGROUP_SIZE];
    group_results[blockIdx.x + 4 * gridDim.x] = tmp_buffer[4 * MDOT_WORKGROUP_SIZE];
    group_results[blockIdx.x + 5 * gridDim.x] = tmp_buffer[5 * MDOT_WORKGROUP_SIZE];
    group_results[blockIdx.x + 6 * gridDim.x] = tmp_buffer[6 * MDOT_WORKGROUP_SIZE];
    group_results[blockIdx.x + 7 * gridDim.x] = tmp_buffer[7 * MDOT_WORKGROUP_SIZE];
  }
}
#endif /* !defined(PETSC_USE_COMPLEX) */

PetscErrorCode VecMDot_SeqHIP(Vec xin, PetscInt nv, const Vec yin[], PetscScalar *z)
{
  PetscInt           i, n = xin->map->n, current_y_index = 0;
  const PetscScalar *xptr, *y0ptr, *y1ptr, *y2ptr, *y3ptr, *y4ptr, *y5ptr, *y6ptr, *y7ptr;
#if !defined(PETSC_USE_COMPLEX)
  PetscInt     nv1 = ((nv % 4) == 1) ? nv - 1 : nv, j;
  PetscScalar *group_results_gpu, *group_results_cpu;
#endif
  PetscBLASInt    one = 1, bn = 0;
  hipblasHandle_t hipblasv2handle;

  PetscFunctionBegin;
  PetscCall(PetscHIPBLASGetHandle(&hipblasv2handle));
  PetscCall(PetscBLASIntCast(xin->map->n, &bn));
  PetscCheck(nv > 0, PETSC_COMM_SELF, PETSC_ERR_LIB, "Number of vectors provided to VecMDot_SeqHIP not positive.");
  /* Handle the case of local size zero first */
  if (!xin->map->n) {
    for (i = 0; i < nv; ++i) z[i] = 0;
    PetscFunctionReturn(0);
  }

#if !defined(PETSC_USE_COMPLEX)
  // allocate scratchpad memory for the results of individual work groups:
  PetscCallHIP(hipMalloc((void **)&group_results_gpu, nv1 * sizeof(PetscScalar) * MDOT_WORKGROUP_NUM));
  PetscCall(PetscMalloc1(nv1 * MDOT_WORKGROUP_NUM, &group_results_cpu));
#endif
  PetscCall(VecHIPGetArrayRead(xin, &xptr));
  PetscCall(PetscLogGpuTimeBegin());

  while (current_y_index < nv) {
    switch (nv - current_y_index) {
    case 7:
    case 6:
    case 5:
    case 4:
      PetscCall(VecHIPGetArrayRead(yin[current_y_index], &y0ptr));
      PetscCall(VecHIPGetArrayRead(yin[current_y_index + 1], &y1ptr));
      PetscCall(VecHIPGetArrayRead(yin[current_y_index + 2], &y2ptr));
      PetscCall(VecHIPGetArrayRead(yin[current_y_index + 3], &y3ptr));
#if defined(PETSC_USE_COMPLEX)
      PetscCallHIPBLAS(hipblasXdot(hipblasv2handle, bn, y0ptr, one, xptr, one, &z[current_y_index]));
      PetscCallHIPBLAS(hipblasXdot(hipblasv2handle, bn, y1ptr, one, xptr, one, &z[current_y_index + 1]));
      PetscCallHIPBLAS(hipblasXdot(hipblasv2handle, bn, y2ptr, one, xptr, one, &z[current_y_index + 2]));
      PetscCallHIPBLAS(hipblasXdot(hipblasv2handle, bn, y3ptr, one, xptr, one, &z[current_y_index + 3]));
#else
      hipLaunchKernelGGL(VecMDot_SeqHIP_kernel4, dim3(MDOT_WORKGROUP_NUM), dim3(MDOT_WORKGROUP_SIZE), 0, 0, xptr, y0ptr, y1ptr, y2ptr, y3ptr, n, group_results_gpu + current_y_index * MDOT_WORKGROUP_NUM);
#endif
      PetscCall(VecHIPRestoreArrayRead(yin[current_y_index], &y0ptr));
      PetscCall(VecHIPRestoreArrayRead(yin[current_y_index + 1], &y1ptr));
      PetscCall(VecHIPRestoreArrayRead(yin[current_y_index + 2], &y2ptr));
      PetscCall(VecHIPRestoreArrayRead(yin[current_y_index + 3], &y3ptr));
      current_y_index += 4;
      break;

    case 3:
      PetscCall(VecHIPGetArrayRead(yin[current_y_index], &y0ptr));
      PetscCall(VecHIPGetArrayRead(yin[current_y_index + 1], &y1ptr));
      PetscCall(VecHIPGetArrayRead(yin[current_y_index + 2], &y2ptr));

#if defined(PETSC_USE_COMPLEX)
      PetscCallHIPBLAS(hipblasXdot(hipblasv2handle, bn, y0ptr, one, xptr, one, &z[current_y_index]));
      PetscCallHIPBLAS(hipblasXdot(hipblasv2handle, bn, y1ptr, one, xptr, one, &z[current_y_index + 1]));
      PetscCallHIPBLAS(hipblasXdot(hipblasv2handle, bn, y2ptr, one, xptr, one, &z[current_y_index + 2]));
#else
      hipLaunchKernelGGL(VecMDot_SeqHIP_kernel3, dim3(MDOT_WORKGROUP_NUM), dim3(MDOT_WORKGROUP_SIZE), 0, 0, xptr, y0ptr, y1ptr, y2ptr, n, group_results_gpu + current_y_index * MDOT_WORKGROUP_NUM);
#endif
      PetscCall(VecHIPRestoreArrayRead(yin[current_y_index], &y0ptr));
      PetscCall(VecHIPRestoreArrayRead(yin[current_y_index + 1], &y1ptr));
      PetscCall(VecHIPRestoreArrayRead(yin[current_y_index + 2], &y2ptr));
      current_y_index += 3;
      break;

    case 2:
      PetscCall(VecHIPGetArrayRead(yin[current_y_index], &y0ptr));
      PetscCall(VecHIPGetArrayRead(yin[current_y_index + 1], &y1ptr));
#if defined(PETSC_USE_COMPLEX)
      PetscCallHIPBLAS(hipblasXdot(hipblasv2handle, bn, y0ptr, one, xptr, one, &z[current_y_index]));
      PetscCallHIPBLAS(hipblasXdot(hipblasv2handle, bn, y1ptr, one, xptr, one, &z[current_y_index + 1]));
#else
      hipLaunchKernelGGL(VecMDot_SeqHIP_kernel2, dim3(MDOT_WORKGROUP_NUM), dim3(MDOT_WORKGROUP_SIZE), 0, 0, xptr, y0ptr, y1ptr, n, group_results_gpu + current_y_index * MDOT_WORKGROUP_NUM);
#endif
      PetscCall(VecHIPRestoreArrayRead(yin[current_y_index], &y0ptr));
      PetscCall(VecHIPRestoreArrayRead(yin[current_y_index + 1], &y1ptr));
      current_y_index += 2;
      break;

    case 1:
      PetscCall(VecHIPGetArrayRead(yin[current_y_index], &y0ptr));
      PetscCallHIPBLAS(hipblasXdot(hipblasv2handle, bn, y0ptr, one, xptr, one, &z[current_y_index]));
      PetscCall(VecHIPRestoreArrayRead(yin[current_y_index], &y0ptr));
      current_y_index += 1;
      break;

    default: // 8 or more vectors left
      PetscCall(VecHIPGetArrayRead(yin[current_y_index], &y0ptr));
      PetscCall(VecHIPGetArrayRead(yin[current_y_index + 1], &y1ptr));
      PetscCall(VecHIPGetArrayRead(yin[current_y_index + 2], &y2ptr));
      PetscCall(VecHIPGetArrayRead(yin[current_y_index + 3], &y3ptr));
      PetscCall(VecHIPGetArrayRead(yin[current_y_index + 4], &y4ptr));
      PetscCall(VecHIPGetArrayRead(yin[current_y_index + 5], &y5ptr));
      PetscCall(VecHIPGetArrayRead(yin[current_y_index + 6], &y6ptr));
      PetscCall(VecHIPGetArrayRead(yin[current_y_index + 7], &y7ptr));
#if defined(PETSC_USE_COMPLEX)
      PetscCallHIPBLAS(hipblasXdot(hipblasv2handle, bn, y0ptr, one, xptr, one, &z[current_y_index]));
      PetscCallHIPBLAS(hipblasXdot(hipblasv2handle, bn, y1ptr, one, xptr, one, &z[current_y_index + 1]));
      PetscCallHIPBLAS(hipblasXdot(hipblasv2handle, bn, y2ptr, one, xptr, one, &z[current_y_index + 2]));
      PetscCallHIPBLAS(hipblasXdot(hipblasv2handle, bn, y3ptr, one, xptr, one, &z[current_y_index + 3]));
      PetscCallHIPBLAS(hipblasXdot(hipblasv2handle, bn, y4ptr, one, xptr, one, &z[current_y_index + 4]));
      PetscCallHIPBLAS(hipblasXdot(hipblasv2handle, bn, y5ptr, one, xptr, one, &z[current_y_index + 5]));
      PetscCallHIPBLAS(hipblasXdot(hipblasv2handle, bn, y6ptr, one, xptr, one, &z[current_y_index + 6]));
      PetscCallHIPBLAS(hipblasXdot(hipblasv2handle, bn, y7ptr, one, xptr, one, &z[current_y_index + 7]));
#else
      hipLaunchKernelGGL(VecMDot_SeqHIP_kernel8, dim3(MDOT_WORKGROUP_NUM), dim3(MDOT_WORKGROUP_SIZE), 0, 0, xptr, y0ptr, y1ptr, y2ptr, y3ptr, y4ptr, y5ptr, y6ptr, y7ptr, n, group_results_gpu + current_y_index * MDOT_WORKGROUP_NUM);
#endif
      PetscCall(VecHIPRestoreArrayRead(yin[current_y_index], &y0ptr));
      PetscCall(VecHIPRestoreArrayRead(yin[current_y_index + 1], &y1ptr));
      PetscCall(VecHIPRestoreArrayRead(yin[current_y_index + 2], &y2ptr));
      PetscCall(VecHIPRestoreArrayRead(yin[current_y_index + 3], &y3ptr));
      PetscCall(VecHIPRestoreArrayRead(yin[current_y_index + 4], &y4ptr));
      PetscCall(VecHIPRestoreArrayRead(yin[current_y_index + 5], &y5ptr));
      PetscCall(VecHIPRestoreArrayRead(yin[current_y_index + 6], &y6ptr));
      PetscCall(VecHIPRestoreArrayRead(yin[current_y_index + 7], &y7ptr));
      current_y_index += 8;
      break;
    }
  }
  PetscCall(PetscLogGpuTimeEnd());
  PetscCall(VecHIPRestoreArrayRead(xin, &xptr));

#if defined(PETSC_USE_COMPLEX)
  PetscCall(PetscLogGpuToCpu(nv * sizeof(PetscScalar)));
#else
  // copy results to CPU
  PetscCallHIP(hipMemcpy(group_results_cpu, group_results_gpu, nv1 * sizeof(PetscScalar) * MDOT_WORKGROUP_NUM, hipMemcpyDeviceToHost));

  // sum group results into z
  for (j = 0; j < nv1; ++j) {
    z[j] = 0;
    for (i = j * MDOT_WORKGROUP_NUM; i < (j + 1) * MDOT_WORKGROUP_NUM; ++i) z[j] += group_results_cpu[i];
  }
  PetscCall(PetscLogFlops(nv1 * MDOT_WORKGROUP_NUM));
  PetscCallHIP(hipFree(group_results_gpu));
  PetscCall(PetscFree(group_results_cpu));
  PetscCall(PetscLogGpuToCpu(nv1 * sizeof(PetscScalar) * MDOT_WORKGROUP_NUM));
#endif
  PetscCall(PetscLogGpuFlops(PetscMax(nv * (2.0 * n - 1), 0.0)));
  PetscFunctionReturn(0);
}

#undef MDOT_WORKGROUP_SIZE
#undef MDOT_WORKGROUP_NUM

PetscErrorCode VecSet_SeqHIP(Vec xin, PetscScalar alpha)
{
  PetscInt                        n      = xin->map->n;
  PetscScalar                    *xarray = NULL;
  thrust::device_ptr<PetscScalar> xptr;

  PetscFunctionBegin;
  PetscCall(VecHIPGetArrayWrite(xin, &xarray));
  PetscCall(PetscLogGpuTimeBegin());
  if (alpha == (PetscScalar)0.0) {
    PetscCallHIP(hipMemset(xarray, 0, n * sizeof(PetscScalar)));
  } else {
    try {
      xptr = thrust::device_pointer_cast(xarray);
      thrust::fill(xptr, xptr + n, alpha);
    } catch (char *ex) {
      SETERRQ(PETSC_COMM_SELF, PETSC_ERR_LIB, "Thrust error: %s", ex);
    }
    PetscCall(PetscLogCpuToGpuScalar(sizeof(PetscScalar)));
  }
  PetscCall(PetscLogGpuTimeEnd());
  PetscCall(VecHIPRestoreArrayWrite(xin, &xarray));
  PetscFunctionReturn(0);
}

struct PetscScalarReciprocal {
  __host__ __device__ PetscScalar operator()(const PetscScalar &s)
  {
#if defined(PETSC_USE_COMPLEX)
    /* SPOCK compilation issue, need to unroll division */
    PetscReal sx = PetscRealPart(s);
    PetscReal sy = PetscImaginaryPart(s);
    PetscReal n  = sx * sx + sy * sy;
    return n != 0.0 ? PetscComplex(sx / n, -sy / n) : 0.0;
#else
    return (s != (PetscScalar)0.0) ? (PetscScalar)1.0 / s : 0.0;
#endif
  }
};

PetscErrorCode VecReciprocal_SeqHIP(Vec v)
{
  PetscInt     n;
  PetscScalar *x;

  PetscFunctionBegin;
  PetscCall(VecGetLocalSize(v, &n));
  PetscCall(VecHIPGetArray(v, &x));
  PetscCall(PetscLogGpuTimeBegin());
  try {
    auto xptr = thrust::device_pointer_cast(x);
    thrust::transform(xptr, xptr + n, xptr, PetscScalarReciprocal());
  } catch (char *ex) {
    SETERRQ(PETSC_COMM_SELF, PETSC_ERR_LIB, "Thrust error: %s", ex);
  }
  PetscCall(PetscLogGpuTimeEnd());
  PetscCall(VecHIPRestoreArray(v, &x));
  PetscFunctionReturn(0);
}

PetscErrorCode VecScale_SeqHIP(Vec xin, PetscScalar alpha)
{
  PetscScalar    *xarray;
  PetscBLASInt    one = 1, bn = 0;
  hipblasHandle_t hipblasv2handle;

  PetscFunctionBegin;
  if (alpha == (PetscScalar)0.0) {
    PetscCall(VecSet_SeqHIP(xin, alpha));
  } else if (alpha != (PetscScalar)1.0) {
    PetscCall(PetscHIPBLASGetHandle(&hipblasv2handle));
    PetscCall(PetscBLASIntCast(xin->map->n, &bn));
    PetscCall(VecHIPGetArray(xin, &xarray));
    PetscCall(PetscLogGpuTimeBegin());
    PetscCallHIPBLAS(hipblasXscal(hipblasv2handle, bn, &alpha, xarray, one));
    PetscCall(VecHIPRestoreArray(xin, &xarray));
    PetscCall(PetscLogGpuTimeEnd());
    PetscCall(PetscLogCpuToGpuScalar(sizeof(PetscScalar)));
    PetscCall(PetscLogGpuFlops(xin->map->n));
  }
  PetscFunctionReturn(0);
}

PetscErrorCode VecTDot_SeqHIP(Vec xin, Vec yin, PetscScalar *z)
{
  const PetscScalar *xarray, *yarray;
  PetscBLASInt       one = 1, bn = 0;
  hipblasHandle_t    hipblasv2handle;

  PetscFunctionBegin;
  PetscCall(PetscHIPBLASGetHandle(&hipblasv2handle));
  PetscCall(PetscBLASIntCast(xin->map->n, &bn));
  PetscCall(VecHIPGetArrayRead(xin, &xarray));
  PetscCall(VecHIPGetArrayRead(yin, &yarray));
  PetscCall(PetscLogGpuTimeBegin());
  PetscCallHIPBLAS(hipblasXdotu(hipblasv2handle, bn, xarray, one, yarray, one, z));
  PetscCall(PetscLogGpuTimeEnd());
  if (xin->map->n > 0) PetscCall(PetscLogGpuFlops(2.0 * xin->map->n - 1));
  PetscCall(PetscLogGpuToCpu(sizeof(PetscScalar)));
  PetscCall(VecHIPRestoreArrayRead(yin, &yarray));
  PetscCall(VecHIPRestoreArrayRead(xin, &xarray));
  PetscFunctionReturn(0);
}

PetscErrorCode VecCopy_SeqHIP(Vec xin, Vec yin)
{
  const PetscScalar *xarray;
  PetscScalar       *yarray;

  PetscFunctionBegin;
  if (xin != yin) {
    if (xin->offloadmask == PETSC_OFFLOAD_GPU) {
      PetscBool yiship;

      PetscCall(PetscObjectTypeCompareAny((PetscObject)yin, &yiship, VECSEQHIP, VECMPIHIP, ""));
      PetscCall(VecHIPGetArrayRead(xin, &xarray));
      if (yiship) {
        PetscCall(VecHIPGetArrayWrite(yin, &yarray));
      } else {
        PetscCall(VecGetArrayWrite(yin, &yarray));
      }
      PetscCall(PetscLogGpuTimeBegin());
      if (yiship) {
        PetscCallHIP(hipMemcpy(yarray, xarray, yin->map->n * sizeof(PetscScalar), hipMemcpyDeviceToDevice));
      } else {
        PetscCallHIP(hipMemcpy(yarray, xarray, yin->map->n * sizeof(PetscScalar), hipMemcpyDeviceToHost));
      }
      PetscCall(PetscLogGpuTimeEnd());
      PetscCall(VecHIPRestoreArrayRead(xin, &xarray));
      if (yiship) {
        PetscCall(VecHIPRestoreArrayWrite(yin, &yarray));
      } else {
        PetscCall(VecRestoreArrayWrite(yin, &yarray));
      }
    } else if (xin->offloadmask == PETSC_OFFLOAD_CPU) {
      /* copy in CPU if we are on the CPU */
      PetscCall(VecCopy_SeqHIP_Private(xin, yin));
    } else if (xin->offloadmask == PETSC_OFFLOAD_BOTH) {
      /* if xin is valid in both places, see where yin is and copy there (because it's probably where we'll want to next use it) */
      if (yin->offloadmask == PETSC_OFFLOAD_CPU) {
        /* copy in CPU */
        PetscCall(VecCopy_SeqHIP_Private(xin, yin));
      } else if (yin->offloadmask == PETSC_OFFLOAD_GPU) {
        /* copy in GPU */
        PetscCall(VecHIPGetArrayRead(xin, &xarray));
        PetscCall(VecHIPGetArrayWrite(yin, &yarray));
        PetscCall(PetscLogGpuTimeBegin());
        PetscCallHIP(hipMemcpy(yarray, xarray, yin->map->n * sizeof(PetscScalar), hipMemcpyDeviceToDevice));
        PetscCall(PetscLogGpuTimeEnd());
        PetscCall(VecHIPRestoreArrayRead(xin, &xarray));
        PetscCall(VecHIPRestoreArrayWrite(yin, &yarray));
      } else if (yin->offloadmask == PETSC_OFFLOAD_BOTH) {
        /* xin and yin are both valid in both places (or yin was unallocated before the earlier call to allocatecheck
           default to copy in GPU (this is an arbitrary choice) */
        PetscCall(VecHIPGetArrayRead(xin, &xarray));
        PetscCall(VecHIPGetArrayWrite(yin, &yarray));
        PetscCall(PetscLogGpuTimeBegin());
        PetscCallHIP(hipMemcpy(yarray, xarray, yin->map->n * sizeof(PetscScalar), hipMemcpyDeviceToDevice));
        PetscCall(PetscLogGpuTimeEnd());
        PetscCall(VecHIPRestoreArrayRead(xin, &xarray));
        PetscCall(VecHIPRestoreArrayWrite(yin, &yarray));
      } else {
        PetscCall(VecCopy_SeqHIP_Private(xin, yin));
      }
    }
  }
  PetscFunctionReturn(0);
}

PetscErrorCode VecSwap_SeqHIP(Vec xin, Vec yin)
{
  PetscBLASInt    one = 1, bn;
  PetscScalar    *xarray, *yarray;
  hipblasHandle_t hipblasv2handle;

  PetscFunctionBegin;
  PetscCall(PetscHIPBLASGetHandle(&hipblasv2handle));
  PetscCall(PetscBLASIntCast(xin->map->n, &bn));
  if (xin != yin) {
    PetscCall(VecHIPGetArray(xin, &xarray));
    PetscCall(VecHIPGetArray(yin, &yarray));
    PetscCall(PetscLogGpuTimeBegin());
    PetscCallHIPBLAS(hipblasXswap(hipblasv2handle, bn, xarray, one, yarray, one));
    PetscCall(PetscLogGpuTimeEnd());
    PetscCall(VecHIPRestoreArray(xin, &xarray));
    PetscCall(VecHIPRestoreArray(yin, &yarray));
  }
  PetscFunctionReturn(0);
}

PetscErrorCode VecAXPBY_SeqHIP(Vec yin, PetscScalar alpha, PetscScalar beta, Vec xin)
{
  PetscScalar        a = alpha, b = beta;
  const PetscScalar *xarray;
  PetscScalar       *yarray;
  PetscBLASInt       one = 1, bn = 0;
  hipblasHandle_t    hipblasv2handle;

  PetscFunctionBegin;
  PetscCall(PetscHIPBLASGetHandle(&hipblasv2handle));
  PetscCall(PetscBLASIntCast(yin->map->n, &bn));
  if (a == (PetscScalar)0.0) {
    PetscCall(VecScale_SeqHIP(yin, beta));
  } else if (b == (PetscScalar)1.0) {
    PetscCall(VecAXPY_SeqHIP(yin, alpha, xin));
  } else if (a == (PetscScalar)1.0) {
    PetscCall(VecAYPX_SeqHIP(yin, beta, xin));
  } else if (b == (PetscScalar)0.0) {
    PetscCall(VecHIPGetArrayRead(xin, &xarray));
    PetscCall(VecHIPGetArray(yin, &yarray));
    PetscCall(PetscLogGpuTimeBegin());
    PetscCallHIP(hipMemcpy(yarray, xarray, yin->map->n * sizeof(PetscScalar), hipMemcpyDeviceToDevice));
    PetscCallHIPBLAS(hipblasXscal(hipblasv2handle, bn, &alpha, yarray, one));
    PetscCall(PetscLogGpuTimeEnd());
    PetscCall(PetscLogGpuFlops(xin->map->n));
    PetscCall(PetscLogCpuToGpuScalar(sizeof(PetscScalar)));
    PetscCall(VecHIPRestoreArrayRead(xin, &xarray));
    PetscCall(VecHIPRestoreArray(yin, &yarray));
  } else {
    PetscCall(VecHIPGetArrayRead(xin, &xarray));
    PetscCall(VecHIPGetArray(yin, &yarray));
    PetscCall(PetscLogGpuTimeBegin());
    PetscCallHIPBLAS(hipblasXscal(hipblasv2handle, bn, &beta, yarray, one));
    PetscCallHIPBLAS(hipblasXaxpy(hipblasv2handle, bn, &alpha, xarray, one, yarray, one));
    PetscCall(VecHIPRestoreArrayRead(xin, &xarray));
    PetscCall(VecHIPRestoreArray(yin, &yarray));
    PetscCall(PetscLogGpuTimeEnd());
    PetscCall(PetscLogGpuFlops(3.0 * xin->map->n));
    PetscCall(PetscLogCpuToGpuScalar(2 * sizeof(PetscScalar)));
  }
  PetscFunctionReturn(0);
}

PetscErrorCode VecAXPBYPCZ_SeqHIP(Vec zin, PetscScalar alpha, PetscScalar beta, PetscScalar gamma, Vec xin, Vec yin)
{
  PetscInt n = zin->map->n;

  PetscFunctionBegin;
  if (gamma == (PetscScalar)1.0) {
    /* z = ax + b*y + z */
    PetscCall(VecAXPY_SeqHIP(zin, alpha, xin));
    PetscCall(VecAXPY_SeqHIP(zin, beta, yin));
    PetscCall(PetscLogGpuFlops(4.0 * n));
  } else {
    /* z = a*x + b*y + c*z */
    PetscCall(VecScale_SeqHIP(zin, gamma));
    PetscCall(VecAXPY_SeqHIP(zin, alpha, xin));
    PetscCall(VecAXPY_SeqHIP(zin, beta, yin));
    PetscCall(PetscLogGpuFlops(5.0 * n));
  }
  PetscFunctionReturn(0);
}

PetscErrorCode VecPointwiseMult_SeqHIP(Vec win, Vec xin, Vec yin)
{
  PetscInt                              n = win->map->n;
  const PetscScalar                    *xarray, *yarray;
  PetscScalar                          *warray;
  thrust::device_ptr<const PetscScalar> xptr, yptr;
  thrust::device_ptr<PetscScalar>       wptr;

  PetscFunctionBegin;
  if (xin->boundtocpu || yin->boundtocpu) {
    PetscCall(VecPointwiseMult_Seq(win, xin, yin));
    PetscFunctionReturn(0);
  }
  PetscCall(VecHIPGetArrayRead(xin, &xarray));
  PetscCall(VecHIPGetArrayRead(yin, &yarray));
  PetscCall(VecHIPGetArrayWrite(win, &warray));
  PetscCall(PetscLogGpuTimeBegin());
  try {
    wptr = thrust::device_pointer_cast(warray);
    xptr = thrust::device_pointer_cast(xarray);
    yptr = thrust::device_pointer_cast(yarray);
#if defined(PETSC_USE_COMPLEX)
    thrust::transform(xptr, xptr + n, yptr, wptr, PetscMultiplyComplex());
#else
    thrust::transform(xptr, xptr + n, yptr, wptr, thrust::multiplies<PetscScalar>());
#endif
  } catch (char *ex) {
    SETERRQ(PETSC_COMM_SELF, PETSC_ERR_LIB, "Thrust error: %s", ex);
  }
  PetscCall(PetscLogGpuTimeEnd());
  PetscCall(VecHIPRestoreArrayRead(xin, &xarray));
  PetscCall(VecHIPRestoreArrayRead(yin, &yarray));
  PetscCall(VecHIPRestoreArrayWrite(win, &warray));
  PetscCall(PetscLogGpuFlops(n));
  PetscFunctionReturn(0);
}

/* should do infinity norm in hip */

PetscErrorCode VecNorm_SeqHIP(Vec xin, NormType type, PetscReal *z)
{
  PetscInt           n   = xin->map->n;
  PetscBLASInt       one = 1, bn = 0;
  const PetscScalar *xarray;
  hipblasHandle_t    hipblasv2handle;

  PetscFunctionBegin;
  PetscCall(PetscHIPBLASGetHandle(&hipblasv2handle));
  PetscCall(PetscBLASIntCast(n, &bn));
  if (type == NORM_2 || type == NORM_FROBENIUS) {
    PetscCall(VecHIPGetArrayRead(xin, &xarray));
    PetscCall(PetscLogGpuTimeBegin());
    PetscCallHIPBLAS(hipblasXnrm2(hipblasv2handle, bn, xarray, one, z));
    PetscCall(PetscLogGpuTimeEnd());
    PetscCall(VecHIPRestoreArrayRead(xin, &xarray));
    PetscCall(PetscLogGpuFlops(PetscMax(2.0 * n - 1, 0.0)));
  } else if (type == NORM_INFINITY) {
    int i;
    PetscCall(VecHIPGetArrayRead(xin, &xarray));
    PetscCall(PetscLogGpuTimeBegin());
    PetscCallHIPBLAS(hipblasIXamax(hipblasv2handle, bn, xarray, one, &i));
    PetscCall(PetscLogGpuTimeEnd());
    if (bn) {
      PetscScalar zs;
      PetscCallHIP(hipMemcpy(&zs, xarray + i - 1, sizeof(PetscScalar), hipMemcpyDeviceToHost));
      *z = PetscAbsScalar(zs);
    } else *z = 0.0;
    PetscCall(VecHIPRestoreArrayRead(xin, &xarray));
  } else if (type == NORM_1) {
    PetscCall(VecHIPGetArrayRead(xin, &xarray));
    PetscCall(PetscLogGpuTimeBegin());
    PetscCallHIPBLAS(hipblasXasum(hipblasv2handle, bn, xarray, one, z));
    PetscCall(VecHIPRestoreArrayRead(xin, &xarray));
    PetscCall(PetscLogGpuTimeEnd());
    PetscCall(PetscLogGpuFlops(PetscMax(n - 1.0, 0.0)));
  } else if (type == NORM_1_AND_2) {
    PetscCall(VecNorm_SeqHIP(xin, NORM_1, z));
    PetscCall(VecNorm_SeqHIP(xin, NORM_2, z + 1));
  }
  PetscCall(PetscLogGpuToCpu(sizeof(PetscReal)));
  PetscFunctionReturn(0);
}

PetscErrorCode VecDotNorm2_SeqHIP(Vec s, Vec t, PetscScalar *dp, PetscScalar *nm)
{
  PetscFunctionBegin;
  PetscCall(VecDot_SeqHIP(s, t, dp));
  PetscCall(VecDot_SeqHIP(t, t, nm));
  PetscFunctionReturn(0);
}

PetscErrorCode VecDestroy_SeqHIP(Vec v)
{
  PetscFunctionBegin;
  if (v->spptr) {
    if (((Vec_HIP *)v->spptr)->GPUarray_allocated) {
      PetscCallHIP(hipFree(((Vec_HIP *)v->spptr)->GPUarray_allocated));
      ((Vec_HIP *)v->spptr)->GPUarray_allocated = NULL;
    }
    if (((Vec_HIP *)v->spptr)->stream) PetscCallHIP(hipStreamDestroy(((Vec_HIP *)v->spptr)->stream));
  }
  PetscCall(VecDestroy_SeqHIP_Private(v));
  PetscCall(PetscFree(v->spptr));
  PetscFunctionReturn(0);
}

#if defined(PETSC_USE_COMPLEX)
/* SPOCK compilation issue, need to do conjugation ourselves */
struct conjugate {
  __host__ __device__ PetscScalar operator()(const PetscScalar &x) { return PetscScalar(PetscRealPart(x), -PetscImaginaryPart(x)); }
};
#endif

PetscErrorCode VecConjugate_SeqHIP(Vec xin)
{
#if defined(PETSC_USE_COMPLEX)
  PetscScalar                    *xarray;
  PetscInt                        n = xin->map->n;
  thrust::device_ptr<PetscScalar> xptr;

  PetscFunctionBegin;
  PetscCall(VecHIPGetArray(xin, &xarray));
  PetscCall(PetscLogGpuTimeBegin());
  try {
    xptr = thrust::device_pointer_cast(xarray);
    thrust::transform(xptr, xptr + n, xptr, conjugate());
  } catch (char *ex) {
    SETERRQ(PETSC_COMM_SELF, PETSC_ERR_LIB, "Thrust error: %s", ex);
  }
  PetscCall(PetscLogGpuTimeEnd());
  PetscCall(VecHIPRestoreArray(xin, &xarray));
#else
  PetscFunctionBegin;
#endif
  PetscFunctionReturn(0);
}

static inline PetscErrorCode VecGetLocalVectorK_SeqHIP(Vec v, Vec w, PetscBool read)
{
  PetscBool wisseqhip;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(v, VEC_CLASSID, 1);
  PetscValidHeaderSpecific(w, VEC_CLASSID, 2);
  PetscCheckTypeNames(v, VECSEQHIP, VECMPIHIP);
  PetscCall(PetscObjectTypeCompare((PetscObject)w, VECSEQHIP, &wisseqhip));
  if (w->data && wisseqhip) {
    if (((Vec_Seq *)w->data)->array_allocated) {
      if (w->pinned_memory) PetscCall(PetscMallocSetHIPHost());
      PetscCall(PetscFree(((Vec_Seq *)w->data)->array_allocated));
      if (w->pinned_memory) {
        PetscCall(PetscMallocResetHIPHost());
        w->pinned_memory = PETSC_FALSE;
      }
    }
    ((Vec_Seq *)w->data)->array         = NULL;
    ((Vec_Seq *)w->data)->unplacedarray = NULL;
  }
  if (w->spptr && wisseqhip) {
    if (((Vec_HIP *)w->spptr)->GPUarray) {
      PetscCallHIP(hipFree(((Vec_HIP *)w->spptr)->GPUarray));
      ((Vec_HIP *)w->spptr)->GPUarray = NULL;
    }
    if (((Vec_HIP *)v->spptr)->stream) PetscCallHIP(hipStreamDestroy(((Vec_HIP *)w->spptr)->stream));
    PetscCall(PetscFree(w->spptr));
  }

  if (v->petscnative && wisseqhip) {
    PetscCall(PetscFree(w->data));
    w->data          = v->data;
    w->offloadmask   = v->offloadmask;
    w->pinned_memory = v->pinned_memory;
    w->spptr         = v->spptr;
    PetscCall(PetscObjectStateIncrease((PetscObject)w));
  } else {
    if (read) {
      PetscCall(VecGetArrayRead(v, (const PetscScalar **)&((Vec_Seq *)w->data)->array));
    } else {
      PetscCall(VecGetArray(v, &((Vec_Seq *)w->data)->array));
    }
    w->offloadmask = PETSC_OFFLOAD_CPU;
    if (wisseqhip) PetscCall(VecHIPAllocateCheck(w));
  }
  PetscFunctionReturn(0);
}

static inline PetscErrorCode VecRestoreLocalVectorK_SeqHIP(Vec v, Vec w, PetscBool read)
{
  PetscBool wisseqhip;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(v, VEC_CLASSID, 1);
  PetscValidHeaderSpecific(w, VEC_CLASSID, 2);
  PetscCheckTypeNames(v, VECSEQHIP, VECMPIHIP);
  PetscCall(PetscObjectTypeCompare((PetscObject)w, VECSEQHIP, &wisseqhip));
  if (v->petscnative && wisseqhip) {
    v->data          = w->data;
    v->offloadmask   = w->offloadmask;
    v->pinned_memory = w->pinned_memory;
    v->spptr         = w->spptr;
    w->data          = 0;
    w->offloadmask   = PETSC_OFFLOAD_UNALLOCATED;
    w->spptr         = 0;
  } else {
    if (read) {
      PetscCall(VecRestoreArrayRead(v, (const PetscScalar **)&((Vec_Seq *)w->data)->array));
    } else {
      PetscCall(VecRestoreArray(v, &((Vec_Seq *)w->data)->array));
    }
    if ((Vec_HIP *)w->spptr && wisseqhip) {
      PetscCallHIP(hipFree(((Vec_HIP *)w->spptr)->GPUarray));
      ((Vec_HIP *)w->spptr)->GPUarray = NULL;
      if (((Vec_HIP *)v->spptr)->stream) PetscCallHIP(hipStreamDestroy(((Vec_HIP *)w->spptr)->stream));
      PetscCall(PetscFree(w->spptr));
    }
  }
  PetscFunctionReturn(0);
}

PetscErrorCode VecGetLocalVector_SeqHIP(Vec v, Vec w)
{
  PetscFunctionBegin;
  PetscCall(VecGetLocalVectorK_SeqHIP(v, w, PETSC_FALSE));
  PetscFunctionReturn(0);
}

PetscErrorCode VecGetLocalVectorRead_SeqHIP(Vec v, Vec w)
{
  PetscFunctionBegin;
  PetscCall(VecGetLocalVectorK_SeqHIP(v, w, PETSC_TRUE));
  PetscFunctionReturn(0);
}

PetscErrorCode VecRestoreLocalVector_SeqHIP(Vec v, Vec w)
{
  PetscFunctionBegin;
  PetscCall(VecRestoreLocalVectorK_SeqHIP(v, w, PETSC_FALSE));
  PetscFunctionReturn(0);
}

PetscErrorCode VecRestoreLocalVectorRead_SeqHIP(Vec v, Vec w)
{
  PetscFunctionBegin;
  PetscCall(VecRestoreLocalVectorK_SeqHIP(v, w, PETSC_TRUE));
  PetscFunctionReturn(0);
}

struct petscrealpart : public thrust::unary_function<PetscScalar, PetscReal> {
  __host__ __device__ PetscReal operator()(const PetscScalar &x) { return PetscRealPart(x); }
};

struct petscrealparti : public thrust::unary_function<thrust::tuple<PetscScalar, PetscInt>, thrust::tuple<PetscReal, PetscInt>> {
  __host__ __device__ thrust::tuple<PetscReal, PetscInt> operator()(const thrust::tuple<PetscScalar, PetscInt> &x) { return thrust::make_tuple(PetscRealPart(x.get<0>()), x.get<1>()); }
};

struct petscmax : public thrust::binary_function<PetscReal, PetscReal, PetscReal> {
  __host__ __device__ PetscReal operator()(const PetscReal &x, const PetscReal &y) { return x < y ? y : x; }
};

struct petscmaxi : public thrust::binary_function<thrust::tuple<PetscReal, PetscInt>, thrust::tuple<PetscReal, PetscInt>, thrust::tuple<PetscReal, PetscInt>> {
  __host__ __device__ thrust::tuple<PetscReal, PetscInt> operator()(const thrust::tuple<PetscReal, PetscInt> &x, const thrust::tuple<PetscReal, PetscInt> &y)
  {
    return x.get<0>() < y.get<0>() ? thrust::make_tuple(y.get<0>(), y.get<1>()) : (x.get<0>() != y.get<0>() ? thrust::make_tuple(x.get<0>(), x.get<1>()) : (x.get<1>() < y.get<1>() ? thrust::make_tuple(x.get<0>(), x.get<1>()) : thrust::make_tuple(y.get<0>(), y.get<1>())));
  }
};

struct petscmin : public thrust::binary_function<PetscReal, PetscReal, PetscReal> {
  __host__ __device__ PetscReal operator()(const PetscReal &x, const PetscReal &y) { return x < y ? x : y; }
};

struct petscmini : public thrust::binary_function<thrust::tuple<PetscReal, PetscInt>, thrust::tuple<PetscReal, PetscInt>, thrust::tuple<PetscReal, PetscInt>> {
  __host__ __device__ thrust::tuple<PetscReal, PetscInt> operator()(const thrust::tuple<PetscReal, PetscInt> &x, const thrust::tuple<PetscReal, PetscInt> &y)
  {
    return x.get<0>() > y.get<0>() ? thrust::make_tuple(y.get<0>(), y.get<1>()) : (x.get<0>() != y.get<0>() ? thrust::make_tuple(x.get<0>(), x.get<1>()) : (x.get<1>() < y.get<1>() ? thrust::make_tuple(x.get<0>(), x.get<1>()) : thrust::make_tuple(y.get<0>(), y.get<1>())));
  }
};

PetscErrorCode VecMax_SeqHIP(Vec v, PetscInt *p, PetscReal *m)
{
  PetscInt                              n = v->map->n;
  const PetscScalar                    *av;
  thrust::device_ptr<const PetscScalar> avpt;

  PetscFunctionBegin;
  PetscCheckTypeNames(v, VECSEQHIP, VECMPIHIP);
  if (!n) {
    *m = PETSC_MIN_REAL;
    if (p) *p = -1;
    PetscFunctionReturn(0);
  }
  PetscCall(VecHIPGetArrayRead(v, &av));
  avpt = thrust::device_pointer_cast(av);
  PetscCall(PetscLogGpuTimeBegin());
  if (p) {
    thrust::tuple<PetscReal, PetscInt> res(PETSC_MIN_REAL, -1);
    auto                               zibit = thrust::make_zip_iterator(thrust::make_tuple(avpt, thrust::counting_iterator<PetscInt>(0)));
    try {
#if defined(PETSC_USE_COMPLEX)
      res = thrust::transform_reduce(zibit, zibit + n, petscrealparti(), res, petscmaxi());
#else
      res = thrust::reduce(zibit, zibit + n, res, petscmaxi());
#endif
    } catch (char *ex) {
      SETERRQ(PETSC_COMM_SELF, PETSC_ERR_LIB, "Thrust error: %s", ex);
    }
    *m = res.get<0>();
    *p = res.get<1>();
  } else {
    try {
#if defined(PETSC_USE_COMPLEX)
      *m = thrust::transform_reduce(avpt, avpt + n, petscrealpart(), PETSC_MIN_REAL, petscmax());
#else
      *m = thrust::reduce(avpt, avpt + n, PETSC_MIN_REAL, petscmax());
#endif
    } catch (char *ex) {
      SETERRQ(PETSC_COMM_SELF, PETSC_ERR_LIB, "Thrust error: %s", ex);
    }
  }
  PetscCall(PetscLogGpuTimeEnd());
  PetscCall(VecHIPRestoreArrayRead(v, &av));
  PetscFunctionReturn(0);
}

PetscErrorCode VecMin_SeqHIP(Vec v, PetscInt *p, PetscReal *m)
{
  PetscInt                              n = v->map->n;
  const PetscScalar                    *av;
  thrust::device_ptr<const PetscScalar> avpt;

  PetscFunctionBegin;
  PetscCheckTypeNames(v, VECSEQHIP, VECMPIHIP);
  if (!n) {
    *m = PETSC_MAX_REAL;
    if (p) *p = -1;
    PetscFunctionReturn(0);
  }
  PetscCall(VecHIPGetArrayRead(v, &av));
  avpt = thrust::device_pointer_cast(av);
  PetscCall(PetscLogGpuTimeBegin());
  if (p) {
    thrust::tuple<PetscReal, PetscInt> res(PETSC_MAX_REAL, -1);
    auto                               zibit = thrust::make_zip_iterator(thrust::make_tuple(avpt, thrust::counting_iterator<PetscInt>(0)));
    try {
#if defined(PETSC_USE_COMPLEX)
      res = thrust::transform_reduce(zibit, zibit + n, petscrealparti(), res, petscmini());
#else
      res = thrust::reduce(zibit, zibit + n, res, petscmini());
#endif
    } catch (char *ex) {
      SETERRQ(PETSC_COMM_SELF, PETSC_ERR_LIB, "Thrust error: %s", ex);
    }
    *m = res.get<0>();
    *p = res.get<1>();
  } else {
    try {
#if defined(PETSC_USE_COMPLEX)
      *m = thrust::transform_reduce(avpt, avpt + n, petscrealpart(), PETSC_MAX_REAL, petscmin());
#else
      *m = thrust::reduce(avpt, avpt + n, PETSC_MAX_REAL, petscmin());
#endif
    } catch (char *ex) {
      SETERRQ(PETSC_COMM_SELF, PETSC_ERR_LIB, "Thrust error: %s", ex);
    }
  }
  PetscCall(PetscLogGpuTimeEnd());
  PetscCall(VecHIPRestoreArrayRead(v, &av));
  PetscFunctionReturn(0);
}

PetscErrorCode VecSum_SeqHIP(Vec v, PetscScalar *sum)
{
  PetscInt                              n = v->map->n;
  const PetscScalar                    *a;
  thrust::device_ptr<const PetscScalar> dptr;

  PetscFunctionBegin;
  PetscCheckTypeNames(v, VECSEQHIP, VECMPIHIP);
  PetscCall(VecHIPGetArrayRead(v, &a));
  dptr = thrust::device_pointer_cast(a);
  PetscCall(PetscLogGpuTimeBegin());
  try {
    *sum = thrust::reduce(dptr, dptr + n, PetscScalar(0.0));
  } catch (char *ex) {
    SETERRQ(PETSC_COMM_SELF, PETSC_ERR_LIB, "Thrust error: %s", ex);
  }
  PetscCall(PetscLogGpuTimeEnd());
  PetscCall(VecHIPRestoreArrayRead(v, &a));
  PetscFunctionReturn(0);
}

struct petscshift : public thrust::unary_function<PetscScalar, PetscScalar> {
  const PetscScalar shift_;
  petscshift(PetscScalar shift) : shift_(shift) { }
  __host__ __device__ PetscScalar operator()(PetscScalar x) { return x + shift_; }
};

PetscErrorCode VecShift_SeqHIP(Vec v, PetscScalar shift)
{
  PetscInt                        n = v->map->n;
  PetscScalar                    *a;
  thrust::device_ptr<PetscScalar> dptr;

  PetscFunctionBegin;
  PetscCheckTypeNames(v, VECSEQHIP, VECMPIHIP);
  PetscCall(VecHIPGetArray(v, &a));
  dptr = thrust::device_pointer_cast(a);
  PetscCall(PetscLogGpuTimeBegin());
  try {
    thrust::transform(dptr, dptr + n, dptr, petscshift(shift)); /* in-place transform */
  } catch (char *ex) {
    SETERRQ(PETSC_COMM_SELF, PETSC_ERR_LIB, "Thrust error: %s", ex);
  }
  PetscCall(PetscLogGpuTimeEnd());
  PetscCall(VecHIPRestoreArray(v, &a));
  PetscFunctionReturn(0);
}
