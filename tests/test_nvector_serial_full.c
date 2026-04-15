/* NeoSUNDIALS port of SUNDIALS NVECTOR serial unit test */
/* Adapted from sundials/test/unit_tests/nvector/serial/test_nvector_serial.c */

#include "test_nvector.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define SUNDIALS_NVEC_SERIAL 1

int main(int argc, char* argv[])
{
  int fails = 0;
  sunindextype length = 127; /* default test length */
  int print_timing = 0;

  if (argc > 1) length = (sunindextype) atol(argv[1]);
  if (argc > 2) print_timing = atoi(argv[2]);

  printf("NeoSUNDIALS NVECTOR_SERIAL full unit test\nLength: %ld\n", (long)length);

  /* Create vectors */
  N_Vector W = N_VNewEmpty_Serial(length);
  N_Vector X = N_VNew_Serial(length);
  N_Vector Y = N_VClone_Serial(X);
  N_Vector Z = N_VClone_Serial(X);

  if (!W || !X || !Y || !Z) {
    printf("[FAIL] Vector allocation\n");
    N_VDestroy_Serial(W); 
    N_VDestroy_Serial(X); 
    N_VDestroy_Serial(Y); 
    N_VDestroy_Serial(Z);
    return 1;
  }

  /* Basic tests */
  fails += Test_N_VGetVectorID(X, SUNDIALS_NVEC_SERIAL, length);
  fails += Test_N_VGetLength(X, length);
  fails += Test_N_VCloneEmpty(X, length);
  fails += Test_N_VSetArrayPointer(W, length, SUNDIALS_NVEC_SERIAL);
  fails += Test_N_VGetArrayPointer(X, length, SUNDIALS_NVEC_SERIAL);

  /* Fill test data: X[i] = i+1 */
  sunrealtype *Xdata = N_VGetArrayPointer_Serial(X);
  for (sunindextype i = 0; i < length; i++) Xdata[i] = (sunrealtype)(i + 1);

  /* Ops tests */
  printf("\\nTesting core vector ops...\\n");
  fails += Test_N_VConst(X, length, SUNDIALS_NVEC_SERIAL);
  fails += Test_N_VLinearSum(X, Y, Z, length, SUNDIALS_NVEC_SERIAL);
  fails += Test_N_VProd(X, Y, Z, length, SUNDIALS_NVEC_SERIAL);
  fails += Test_N_VDiv(X, Y, Z, length, SUNDIALS_NVEC_SERIAL);
  fails += Test_N_VScale(X, Z, length, SUNDIALS_NVEC_SERIAL);
  fails += Test_N_VAbs(X, Z, length, SUNDIALS_NVEC_SERIAL);
  fails += Test_N_VInv(X, Z, length, SUNDIALS_NVEC_SERIAL);
  fails += Test_N_VAddConst(X, Z, length, SUNDIALS_NVEC_SERIAL);
  fails += Test_N_VDotProd(X, Y, length, SUNDIALS_NVEC_SERIAL);
  fails += Test_N_VMaxNorm(X, length, SUNDIALS_NVEC_SERIAL);
  fails += Test_N_VWrmsNorm(X, Y, length, SUNDIALS_NVEC_SERIAL);
  fails += Test_N_VMin(X, length, SUNDIALS_NVEC_SERIAL);
  fails += Test_N_VWL2Norm(X, Y, length, SUNDIALS_NVEC_SERIAL);
  fails += Test_N_VL1Norm(X, length, SUNDIALS_NVEC_SERIAL);
  fails += Test_N_VCompare(X, Z, length, SUNDIALS_NVEC_SERIAL);
  fails += Test_N_VInvTest(X, Z, length, SUNDIALS_NVEC_SERIAL);
  fails += Test_N_VConstrMask(X, Y, Z, length, SUNDIALS_NVEC_SERIAL);
  fails += Test_N_VMinQuotient(X, Y, length, SUNDIALS_NVEC_SERIAL);

  /* Local reductions (serial) */
  printf("\\nTesting local reductions...\\n");
  fails += Test_N_VDotProdLocal(X, Y, length, SUNDIALS_NVEC_SERIAL);
  fails += Test_N_VMaxNormLocal(X, length, SUNDIALS_NVEC_SERIAL);
  fails += Test_N_VMinLocal(X, length, SUNDIALS_NVEC_SERIAL);
  fails += Test_N_VL1NormLocal(X, length, SUNDIALS_NVEC_SERIAL);
  fails += Test_N_VWSqrSumLocal(X, Y, length, SUNDIALS_NVEC_SERIAL);

  /* Cleanup */
  N_VDestroy_Serial(W);
  N_VDestroy_Serial(X);
  N_VDestroy_Serial(Y);
  N_VDestroy_Serial(Z);

  printf("\\n%s: %d fails\\n", fails ? "FAIL" : "PASS", fails);
  return fails > 0;
}

