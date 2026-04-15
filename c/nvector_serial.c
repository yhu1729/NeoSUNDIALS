#include "nvector_serial.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>

#define ZERO  0.0
#define ONE   1.0

/* Direct access macros */
#define NV_LENGTH(v)   (((struct N_VectorContent_Serial *)(v))->length)
#define NV_DATA(v)     (((struct N_VectorContent_Serial *)(v))->data)
#define NV_OWN_DATA(v) (((struct N_VectorContent_Serial *)(v))->own_data)

static double VMax_serial(const double *x, sunindextype N) {
  double mx = x[0];
  for (sunindextype i = 1; i < N; i++) if (x[i] > mx) mx = x[i];
  return mx;
}

static double VMin_serial(const double *x, sunindextype N) {
  double mn = x[0];
  for (sunindextype i = 1; i < N; i++) if (x[i] < mn) mn = x[i];
  return mn;
}

static void VCopy_serial(sunindextype N, const double *x, double *z) {
  memcpy(z, x, N * sizeof(double));
}

static void VSum_serial(sunindextype N, const double *x, const double *y, double *z) {
  for (sunindextype i = 0; i < N; i++) z[i] = x[i] + y[i];
}

static void VScaleSum_serial(double a, const double *x, double b, const double *y, double *z, sunindextype N) {
  for (sunindextype i = 0; i < N; i++) z[i] = a*x[i] + b*y[i];
}

static void VScale_serial(double c, const double *x, double *z, sunindextype N) {
  for (sunindextype i = 0; i < N; i++) z[i] = c * x[i];
}

static void VAxpy_serial(double a, const double *x, double *y, sunindextype N) {
  for (sunindextype i = 0; i < N; i++) y[i] += a * x[i];
}

static void VProd_serial(sunindextype N, const double *x, const double *y, double *z) {
  for (sunindextype i = 0; i < N; i++) z[i] = x[i] * y[i];
}

static void VDiv_serial(sunindextype N, const double *x, const double *y, double *z) {
  for (sunindextype i = 0; i < N; i++) z[i] = x[i] / y[i];
}

static double VDot_serial(const double *x, const double *y, sunindextype N) {
  double sum = 0.0;
  for (sunindextype i = 0; i < N; i++) sum += x[i] * y[i];
  return sum;
}

static double VMaxNorm_serial(const double *x, sunindextype N) {
  double max = 0.0;
  for (sunindextype i = 0; i < N; i++) {
    double f = fabs(x[i]);
    if (f > max) max = f;
  }
  return max;
}

static double VL1Norm_serial(const double *x, sunindextype N) {
  double sum = 0.0;
  for (sunindextype i = 0; i < N; i++) sum += fabs(x[i]);
  return sum;
}

static double VWL2Norm_serial(const double *x, sunindextype N) {
  double sum = 0.0;
  for (sunindextype i = 0; i < N; i++) sum += x[i] * x[i];
  return sqrt(sum);
}

static double VWrmsNorm_serial(const double *x, const double *w, sunindextype N) {
  double sumsqw = 0.0;
  double sumw = N;
  for (sunindextype i = 0; i < N; i++) {
    sumsqw += x[i]*x[i];
  }
  return sqrt(sumsqw / sumw);
}

static void VAbs_serial(sunindextype N, const double *x, double *z) {
  for (sunindextype i = 0; i < N; i++) z[i] = fabs(x[i]);
}

static void VInv_serial(sunindextype N, const double *x, double *z) {
  for (sunindextype i = 0; i < N; i++) z[i] = 1.0 / x[i];
}

static void VAddConst_serial(double c, const double *x, double *z, sunindextype N) {
  for (sunindextype i = 0; i < N; i++) z[i] = x[i] + c;
}

static int VConstrMask_serial(const double *c, const double *x, double *m, sunindextype N) {
  int test = 0;
  for (sunindextype i = 0; i < N; i++) {
    m[i] = 0.0;
    if (c[i] * x[i] < 0.0) {
      m[i] = c[i];
      test = 1;
    }
  }
  return test;
}

static double VMinQuotient_serial(const double *num, const double *denom, sunindextype N) {
  double minq = HUGE_VAL;
  for (sunindextype i = 0; i < N; i++) {
    if (denom[i] != 0.0) {
      double q = num[i] / denom[i];
      if (q < minq) minq = q;
    }
  }
  return minq;
}

static int VInvTest_serial(const double *x, double *z, sunindextype N) {
  for (sunindextype i = 0; i < N; i++) {
    if (fabs(x[i]) < 1e-14) return 1;
    z[i] = 1.0 / x[i];
  }
  return 0;
}

static void VCompare_serial(double c, const double *x, double *z, sunindextype N) {
  for (sunindextype i = 0; i < N; i++) z[i] = (fabs(x[i]) >= c) ? ONE : ZERO;
}

static double VWSqrSum_serial(const double *x, const double *w, sunindextype N) {
  double sum = 0.0;
  for (sunindextype i = 0; i < N; i++) sum += x[i]*x[i];
  return sum;
}

N_Vector N_VNew_Serial(sunindextype length) {
  struct N_VectorContent_Serial *v = malloc(sizeof(struct N_VectorContent_Serial));
  if (!v) return NULL;
  v->length = length;
  v->own_data = 1;
  v->data = length > 0 ? calloc(length, sizeof(double)) : NULL;
  if (length > 0 && !v->data) {
    free(v);
    return NULL;
  }
  return (N_Vector)v;
}

N_Vector N_VClone_Serial(N_Vector w) {
  sunindextype N = NV_LENGTH(w);
  N_Vector v = N_VNew_Serial(N);
  if (!v) return NULL;
  VCopy_serial(N, NV_DATA(w), NV_DATA(v));
  return v;
}

void N_VDestroy_Serial(N_Vector v) {
  if (!v) return;
  if (NV_OWN_DATA(v) && NV_DATA(v)) free(NV_DATA(v));
  free(v);
}

sunindextype N_VGetLength_Serial(N_Vector v) {
  return v ? NV_LENGTH(v) : 0;
}

double *N_VGetArrayPointer_Serial(N_Vector v) {
  return v ? NV_DATA(v) : NULL;
}

void N_VSetArrayPointer_Serial(double *v_data, N_Vector v) {
  if (!v) return;
  if (NV_OWN_DATA(v) && NV_DATA(v)) free(NV_DATA(v));
  ((struct N_VectorContent_Serial *)v)->data = v_data;
  ((struct N_VectorContent_Serial *)v)->own_data = 0;
}

void N_VLinearSum_Serial(double a, N_Vector x, double b, N_Vector y, N_Vector z) {
  sunindextype N = NV_LENGTH(z);
  double *xd = NV_DATA(x);
  double *yd = NV_DATA(y);
  double *zd = NV_DATA(z);
  if (a == ONE && b == ONE) {
    VSum_serial(N, xd, yd, zd);
  } else if (a == ONE && b == ZERO) {
    VCopy_serial(N, xd, zd);
  } else if (a == ZERO && b == ONE) {
    VCopy_serial(N, yd, zd);
  } else {
    VScaleSum_serial(a, xd, b, yd, zd, N);
  }
}

void N_VScale_Serial(double c, N_Vector x, N_Vector z) {
  VScale_serial(c, NV_DATA(x), NV_DATA(z), NV_LENGTH(z));
}

void N_VAxpy_Serial(double a, N_Vector x, N_Vector y) {
  VAxpy_serial(a, NV_DATA(x), NV_DATA(y), NV_LENGTH(y));
}

void N_VProd_Serial(N_Vector x, N_Vector y, N_Vector z) {
  VProd_serial(NV_LENGTH(z), NV_DATA(x), NV_DATA(y), NV_DATA(z));
}

void N_VDiv_Serial(N_Vector x, N_Vector y, N_Vector z) {
  VDiv_serial(NV_LENGTH(z), NV_DATA(x), NV_DATA(y), NV_DATA(z));
}

double N_VDotProd_Serial(N_Vector x, N_Vector y) {
  return VDot_serial(NV_DATA(x), NV_DATA(y), NV_LENGTH(x));
}

double N_VMaxNorm_Serial(N_Vector x) {
  return VMaxNorm_serial(NV_DATA(x), NV_LENGTH(x));
}

double N_VL1Norm_Serial(N_Vector x) {
  return VL1Norm_serial(NV_DATA(x), NV_LENGTH(x));
}

double N_VWL2Norm_Serial(N_Vector x, N_Vector w) {
  return VWL2Norm_serial(NV_DATA(x), NV_LENGTH(x));
}

double N_VWrmsNorm_Serial(N_Vector x, N_Vector w) {
  return VWrmsNorm_serial(NV_DATA(x), NV_DATA(w), NV_LENGTH(x));
}

double N_VMin_Serial(N_Vector x) {
  return VMin_serial(NV_DATA(x), NV_LENGTH(x));
}

double N_VMax_Serial(N_Vector x) {
  return VMax_serial(NV_DATA(x), NV_LENGTH(x));
}

void N_VAbs_Serial(N_Vector x, N_Vector z) {
  VAbs_serial(NV_LENGTH(x), NV_DATA(x), NV_DATA(z));
}

void N_VInv_Serial(N_Vector x, N_Vector z) {
  VInv_serial(NV_LENGTH(x), NV_DATA(x), NV_DATA(z));
}

void N_VAddConst_Serial(double c, N_Vector x, N_Vector z) {
  VAddConst_serial(c, NV_DATA(x), NV_DATA(z), NV_LENGTH(x));
}

int N_VConstrMask_Serial(N_Vector c, N_Vector x, N_Vector m) {
  sunindextype N = NV_LENGTH(m);
  double *cd = NV_DATA(c);
  double *xd = NV_DATA(x);
  double *md = NV_DATA(m);
  return VConstrMask_serial(cd, xd, md, N);
}

double N_VMinQuotient_Serial(N_Vector num, N_Vector denom) {
  sunindextype N = NV_LENGTH(num);
  return VMinQuotient_serial(NV_DATA(num), NV_DATA(denom), N);
}

int N_VInvTest_Serial(N_Vector x, N_Vector z) {
  sunindextype N = NV_LENGTH(x);
  return VInvTest_serial(NV_DATA(x), NV_DATA(z), N);
}

void N_VCompare_Serial(double c, N_Vector x, N_Vector z) {
  VCompare_serial(c, NV_DATA(x), NV_DATA(z), NV_LENGTH(x));
}

double N_VWSqrSumLocal_Serial(N_Vector x, N_Vector w) {
  sunindextype N = NV_LENGTH(x);
  return VWSqrSum_serial(NV_DATA(x), NV_DATA(w), N);
}

