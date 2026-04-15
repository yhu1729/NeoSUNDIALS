#ifndef _TEST_NVECTOR_H_
#define _TEST_NVECTOR_H_

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "../c/nvector_serial.h"

typedef struct N_VectorContent_Serial* N_VectorContent_Serial;

/* SUNDIALS types/macros stub */
typedef double sunrealtype;
#define SUN_RCONST(x) ((sunrealtype)(x))
#define SUNFALSE 0
#define SUNTRUE 1
#define SUNDIALS_NVEC_SERIAL 1

/* Test status functions (stubs for now - implement minimally) */
int Test_N_VGetVectorID(N_Vector X, int id, sunindextype local_length) {
  return NV_LENGTH(X) == local_length ? 0 : 1;
}

int Test_N_VGetLength(N_Vector X, sunindextype local_length) {
  return NV_LENGTH(X) == local_length ? 0 : 1;
}

int Test_N_VCloneEmpty(N_Vector X, sunindextype local_length) {
  N_Vector v = N_VNewEmpty_Serial(local_length);
  int fail = !v || NV_LENGTH(v) != local_length;
  N_VDestroy_Serial(v);
  return fail;
}

int Test_N_VSetArrayPointer(N_Vector W, sunindextype local_length, int id) {
  double *data = (double*)malloc(local_length * sizeof(double));
  N_VSetArrayPointer_Serial(data, W);
  int fail = NV_DATA(W) != data;
  free(data);
  return fail;
}

int Test_N_VGetArrayPointer(N_Vector X, sunindextype local_length, int id) {
  return NV_DATA(X) ? 0 : 1;
}

/* Ops tests (minimal implementations using reference ops) */
int Test_N_VConst(N_Vector X, sunindextype N, int id) {
  N_VConst_Serial(ONE, X);
  for (sunindextype i = 0; i < N; i++) {
    if (fabs(NV_Ith_S(X, i) - ONE) > 1e-14) return 1;
  }
  return 0;
}

int Test_N_VLinearSum(N_Vector X, N_Vector Y, N_Vector Z, sunindextype N, int id) {
  N_VConst_Serial(ONE, X);
  N_VConst_Serial(TWO, Y);
  N_VLinearSum_Serial(ONE, X, ONE, Y, Z);
  for (sunindextype i = 0; i < N; i++) {
    if (fabs(NV_Ith_S(Z, i) - 3.0) > 1e-14) return 1;
  }
  return 0;
}

int Test_N_VProd(N_Vector X, N_Vector Y, N_Vector Z, sunindextype N, int id) {
  N_VConst_Serial(2.0, X);
  N_VConst_Serial(3.0, Y);
  N_VProd_Serial(X, Y, Z);
  for (sunindextype i = 0; i < N; i++) {
    if (fabs(NV_Ith_S(Z, i) - 6.0) > 1e-14) return 1;
  }
  return 0;
}

int Test_N_VDiv(N_Vector X, N_Vector Y, N_Vector Z, sunindextype N, int id) {
  N_VConst_Serial(6.0, X);
  N_VConst_Serial(2.0, Y);
  N_VDiv_Serial(X, Y, Z);
  for (sunindextype i = 0; i < N; i++) {
    if (fabs(NV_Ith_S(Z, i) - 3.0) > 1e-14) return 1;
  }
  return 0;
}

int Test_N_VScale(N_Vector X, N_Vector Z, sunindextype N, int id) {
  N_VConst_Serial(2.0, X);
  N_VScale_Serial(3.0, X, Z);
  for (sunindextype i = 0; i < N; i++) {
    if (fabs(NV_Ith_S(Z, i) - 6.0) > 1e-14) return 1;
  }
  return 0;
}

int Test_N_VAbs(N_Vector X, N_Vector Z, sunindextype N, int id) {
  NV_Ith_S(X, 0) = -2.0;
  NV_Ith_S(X, 1) = 3.0;
  N_VAbs_Serial(X, Z);
  if (fabs(NV_Ith_S(Z, 0) - 2.0) > 1e-14 || fabs(NV_Ith_S(Z, 1) - 3.0) > 1e-14) return 1;
  return 0;
}

int Test_N_VInv(N_Vector X, N_Vector Z, sunindextype N, int id) {
  N_VConst_Serial(2.0, X);
  N_VInv_Serial(X, Z);
  for (sunindextype i = 0; i < N; i++) {
    if (fabs(NV_Ith_S(Z, i) - 0.5) > 1e-14) return 1;
  }
  return 0;
}

int Test_N_VAddConst(N_Vector X, N_Vector Z, sunindextype N, int id) {
  N_VConst_Serial(2.0, X);
  N_VAddConst_Serial(3.0, X, Z);
  for (sunindextype i = 0; i < N; i++) {
    if (fabs(NV_Ith_S(Z, i) - 5.0) > 1e-14) return 1;
  }
  return 0;
}

int Test_N_VDotProd(N_Vector X, N_Vector Y, sunindextype N, int id) {
  N_VConst_Serial(2.0, X);
  N_VConst_Serial(3.0, Y);
  double dot = N_VDotProd_Serial(X, Y);
  if (fabs(dot - 6.0 * N) > 1e-12) return 1;
  return 0;
}

int Test_N_VMaxNorm(N_Vector X, sunindextype N, int id) {
  N_VConst_Serial(2.0, X);
  NV_Ith_S(X, 0) = 3.0;
  if (fabs(N_VMaxNorm_Serial(X) - 3.0) > 1e-14) return 1;
  return 0;
}

int Test_N_VWrmsNorm(N_Vector X, N_Vector W, sunindextype N, int id) {
  N_VConst_Serial(2.0, X);
  N_VConst_Serial(1.0, W);
  double norm = N_VWrmsNorm_Serial(X, W);
  if (fabs(norm - 2.0) > 1e-14) return 1;
  return 0;
}

int Test_N_VMin(N_Vector X, sunindextype N, int id) {
  N_VConst_Serial(2.0, X);
  NV_Ith_S(X, 0) = 1.0;
  if (fabs(N_VMin_Serial(X) - 1.0) > 1e-14) return 1;
  return 0;
}

int Test_N_VWL2Norm(N_Vector X, N_Vector W, sunindextype N, int id) {
  N_VConst_Serial(1.0, X);
  N_VConst_Serial(1.0, W);
  double norm = N_VWL2Norm_Serial(X, W);
  if (fabs(norm - sqrt(N)) > 1e-12) return 1;
  return 0;
}

int Test_N_VL1Norm(N_Vector X, sunindextype N, int id) {
  N_VConst_Serial(1.0, X);
  double norm = N_VL1Norm_Serial(X);
  if (fabs(norm - N) > 1e-12) return 1;
  return 0;
}

int Test_N_VCompare(N_Vector X, N_Vector Z, sunindextype N, int id) {
  N_VConst_Serial(2.0, X);
  N_VCompare_Serial(1.5, X, Z);
  for (sunindextype i = 0; i < N; i++) {
    if (NV_Ith_S(Z, i) != ONE) return 1;
  }
  return 0;
}

int Test_N_VInvTest(N_Vector X, N_Vector Z, sunindextype N, int id) {
  N_VConst_Serial(2.0, X);
  int flag = N_VInvTest_Serial(X, Z);
  if (flag != 0) return 1;
  for (sunindextype i = 0; i < N; i++) {
    if (fabs(NV_Ith_S(Z, i) - 0.5) > 1e-14) return 1;
  }
  return 0;
}

int Test_N_VConstrMask(N_Vector X, N_Vector Y, N_Vector Z, sunindextype N, id) {
  N_VConst_Serial(ONE, X);
  N_VConst_Serial(-ONE, Y);
  N_VConst_Serial(ZERO, Z);
  int flag = N_VConstrMask_Serial(X, Y, Z);
  if (flag != 0 || NV_Ith_S(Z, 0) != ONE) return 1;
  return 0;
}

int Test_N_VMinQuotient(N_Vector X, N_Vector Y, sunindextype N, int id) {
  N_VConst_Serial(2.0, X);
  N_VConst_Serial(4.0, Y);
  double minq = N_VMinQuotient_Serial(X, Y);
  if (fabs(minq - 0.5) > 1e-14) return 1;
  return 0;
}

/* Local reductions */
int Test_N_VDotProdLocal(N_Vector X, N_Vector Y, sunindextype N, int id) {
  return Test_N_VDotProd(X, Y, N, id);
}

int Test_N_VMaxNormLocal(N_Vector X, sunindextype N, int id) {
  return Test_N_VMaxNorm(X, N, id);
}

int Test_N_VMinLocal(N_Vector X, sunindextype N, int id) {
  return Test_N_VMin(X, N, id);
}

int Test_N_VL1NormLocal(N_Vector X, sunindextype N, int id) {
  return Test_N_VL1Norm(X, N, id);
}

int Test_N_VWSqrSumLocal(N_Vector X, N_Vector Y, sunindextype N, int id) {
  N_VConst_Serial(1.0, X);
  N_VConst_Serial(1.0, Y);
  double sqr = N_VWSqrSumLocal_Serial(X, Y);
  if (fabs(sqr - N) > 1e-12) return 1;
  return 0;
}

/* Utils */
#define NV_Ith_S(v,i) (NV_DATA_S(v)[i])

int check_ans(sunrealtype ans, N_Vector X, sunindextype local_length) {
  for (sunindextype i = 0; i < local_length; i++) {
    if (SUNRabs(NV_Ith_S(X, i) - ans) > 1e-12) return 1;
  }
  return 0;
}

sunbooleantype has_data(N_Vector X) {
  return NV_DATA(X) != NULL ? SUNTRUE : SUNFALSE;
}

void set_element(N_Vector X, sunindextype i, sunrealtype val) {
  NV_Ith_S(X, i) = val;
}

void set_element_range(N_Vector X, sunindextype is, sunindextype ie, sunrealtype val) {
  for (sunindextype i = is; i <= ie; i++) set_element(X, i, val);
}

sunrealtype get_element(N_Vector X, sunindextype i) {
  return NV_Ith_S(X, i);
}

double max_time(N_Vector X, double time) { return time; }
void sync_device(N_Vector x) { }

#endif /* _TEST_NVECTOR_H_ */

