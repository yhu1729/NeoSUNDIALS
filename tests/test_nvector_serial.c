#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "../c/nvector_serial.h"

#define NELEMS 10

int main(void) {
  int status = 0;
  N_Vector x, y, z, w;
  double *xd, *yd, *zd, *wd;
  double wnorm = 0.0;
  double wl2 = 0.0;
  double wsq = 0.0;
  sunindextype i;

  printf("NVector Serial Unit Tests\n");

  /* Test 1: Create and check length */
  x = N_VNew_Serial(NELEMS);
  if (!x || NV_LENGTH_S(x) != NELEMS) {
    printf("[FAIL] N_VNew_Serial or length\n");
    status = 1;
  } else {
    printf("[PASS] N_VNew_Serial (%ld elems)\n", (long)NV_LENGTH_S(x));
  }

  /* Fill test data 1 to 10 */
  xd = N_VGetArrayPointer_Serial(x);
  for (i = 0; i < NELEMS; i++) xd[i] = (double)i + 1.0;

  /* Test 2: Clone */
  y = N_VClone_Serial(x);
  if (!y) {
    printf("[FAIL] N_VClone_Serial\n");
    status = 1;
  } else {
    yd = N_VGetArrayPointer_Serial(y);
    int match = 1;
    for (i = 0; i < NELEMS; i++) if (fabs(yd[i] - xd[i]) > 1e-14) match = 0;
    printf("%s N_VClone_Serial copy\n", match ? "[PASS]" : "[FAIL]");
    if (!match) status = 1;
  }

  /* Test 3: Scale */
  z = N_VNew_Serial(NELEMS);
  zd = N_VGetArrayPointer_Serial(z);
  N_VScale_Serial(2.0, x, z);
  int scale_match = 1;
  for (i = 0; i < NELEMS; i++) if (fabs(zd[i] - 2.0*(i+1)) > 1e-14) scale_match = 0;
  printf("%s N_VScale_Serial\n", scale_match ? "[PASS]" : "[FAIL]");
  if (!scale_match) status = 1;

  /* Test 4: Axpy */
  N_VAxpy_Serial(-1.0, z, y); /* y -= z (should be 1,3,5,...,19) */
  int axpy_match = 1;
  for (i = 0; i < NELEMS; i++) if (fabs(yd[i] - ((i+1) - 2*(i+1))) > 1e-14) axpy_match = 0;
  printf("%s N_VAxpy_Serial\n", axpy_match ? "[PASS]" : "[FAIL]");
  if (!axpy_match) status = 1;

  /* Test 5: WrmsNorm with unit weights - RMS norm of 1..10 ~ 6.20483682293 */
  w = N_VNew_Serial(NELEMS);
  if (!w) {
    printf("[FAIL] N_VNew_Serial for weights\n");
    status = 1;
  } else {
    wd = N_VGetArrayPointer_Serial(w);
    for (i = 0; i < NELEMS; i++) wd[i] = 1.0;
    wnorm = N_VWrmsNorm_Serial(x, w);
    printf("WrmsNorm(x,ones) = %.12g\n", wnorm);
    if (fabs(wnorm - 6.20483682293) > 1e-10) {
      printf("[FAIL] N_VWrmsNorm_Serial\n");
      status = 1;
    } else {
      printf("[PASS] N_VWrmsNorm_Serial\n");
    }

    /* Test 6: Weighted WL2 and weighted square sum with non-uniform weights */
    for (i = 0; i < NELEMS; i++) wd[i] = 1.0 / (double)(i + 1);
    wl2 = N_VWL2Norm_Serial(x, w);
    wsq = N_VWSqrSumLocal_Serial(x, w);
    if (fabs(wl2 - sqrt((double)NELEMS)) > 1e-12 || fabs(wsq - (double)NELEMS) > 1e-12) {
      printf("[FAIL] Weighted WL2/WSqr (%.12g, %.12g)\n", wl2, wsq);
      status = 1;
    } else {
      printf("[PASS] Weighted WL2/WSqr\n");
    }
  }

  /* Test 7: Min/Max */
  double minv = N_VMin_Serial(x);
  double maxv = N_VMax_Serial(x);
  if (fabs(minv - 1.0) > 1e-14 || fabs(maxv - 10.0) > 1e-14) {
    printf("[FAIL] N_VMin/Max_Serial (%.6g, %.6g)\n", minv, maxv);
    status = 1;
  } else {
    printf("[PASS] N_VMin/Max_Serial (1.0, 10.0)\n");
  }

  /* Cleanup */
  N_VDestroy_Serial(x);
  N_VDestroy_Serial(y);
  N_VDestroy_Serial(z);
  N_VDestroy_Serial(w);

  printf("Tests %s (%d failures)\n", status ? "FAILED" : "PASSED", status);
  return status;
}

