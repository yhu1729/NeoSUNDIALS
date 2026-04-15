#include "nvector_serial.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>

#define ZERO  0.0
#define ONE   1.0

/* Direct access macros - no intermediate NV_CONTENT_S to avoid linker issues */
#define NV_LENGTH(v)   (((struct N_VectorContent_Serial *)(v))->length)
#define NV_DATA(v)     (((struct N_VectorContent_Serial *)(v))->data)
#define NV_OWN_DATA(v) (((struct N_VectorContent_Serial *)(v))->own_data)

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

static double VDot_serial(const double *x, const double *y, sunindextype N) {
  double sum = 0.0;
  for (sunindextype i = 0; i < N; i++) sum += x[i] * y[i];
  return sum;
}

static double VWrmsNorm_serial(const double *x, const double *w, sunindextype N) {
  double sumsqw = 0.0, sumw = 0.0;
  for (sunindextype i = 0; i < N; i++) {
    double wi = ONE; /* uniform weight for test */
    sumsqw += x[i]*x[i] / (wi*wi);
    sumw += 1.0 / wi;
  }
  return sqrt(sumsqw / sumw);
}

static double VMin_serial(const double *x, sunindextype N) {
  double mn = x[0];
  for (sunindextype i = 1; i < N; i++) if (x[i] < mn) mn = x[i];
  return mn;
}

static double VMax_serial(const double *x, sunindextype N) {
  double mx = x[0];
  for (sunindextype i = 1; i < N; i++) if (x[i] > mx) mx = x[i];
  return mx;
}

static void VAbs_serial(const double *x, double *z, sunindextype N) {
  for (sunindextype i = 0; i < N; i++) z[i] = fabs(x[i]);
}

/* Public API */
N_Vector N_VNew_Serial(sunindextype length) {
  struct N_VectorContent_Serial *v = (struct N_VectorContent_Serial *)malloc(sizeof(struct N_VectorContent_Serial));
  if (!v) return NULL;
  v->length = length;
  v->own_data = 1;
  v->data = (length > 0) ? (double *)calloc(length, sizeof(double)) : NULL;
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
  if (NV_OWN_DATA(v)) free(NV_DATA(v));
  free(v);
}

sunindextype N_VGetLength_Serial(N_Vector v) {
  return (v) ? NV_LENGTH(v) : 0;
}

double *N_VGetArrayPointer_Serial(N_Vector v) {
  return (v) ? NV_DATA(v) : NULL;
}

void N_VSetArrayPointer_Serial(double *v_data, N_Vector v) {
  if (!v) return;
  if (NV_OWN_DATA(v) && NV_DATA(v)) free(NV_DATA(v));
  ((struct N_VectorContent_Serial *)v)->data = v_data;
  ((struct N_VectorContent_Serial *)v)->own_data = 0;
}

void N_VLinearSum_Serial(double a, N_Vector x, double b, N_Vector y, N_Vector z) {
  if (!z) return;
  sunindextype N = NV_LENGTH(z);
  double *xd = NV_DATA(x), *yd = NV_DATA(y), *zd = NV_DATA(z);
  if (z == x && a == ONE) {
    VAxpy_serial(b, yd, zd, N);
  } else if (z == y && b == ONE) {
    VAxpy_serial(a, xd, zd, N);
  } else if (a == ONE && b == ONE) {
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
  if (!z) return;
  VScale_serial(c, NV_DATA(x), NV_DATA(z), NV_LENGTH(z));
}

void N_VAxpy_Serial(double a, N_Vector x, N_Vector y) {
  if (!y) return;
  VAxpy_serial(a, NV_DATA(x), NV_DATA(y), NV_LENGTH(y));
}

double N_VDotProd_Serial(N_Vector x, N_Vector y) {
  if (!x || !y) return ZERO;
  return VDot_serial(NV_DATA(x), NV_DATA(y), NV_LENGTH(x));
}

double N_VWrmsNorm_Serial(N_Vector x, N_Vector w) {
  if (!x || !w) return ZERO;
  return VWrmsNorm_serial(NV_DATA(x), NV_DATA(w), NV_LENGTH(x));
}

double N_VMin_Serial(N_Vector x) {
  if (!x || NV_LENGTH(x) == 0) return ZERO;
  return VMin_serial(NV_DATA(x), NV_LENGTH(x));
}

double N_VMax_Serial(N_Vector x) {
  if (!x || NV_LENGTH(x) == 0) return ZERO;
  return VMax_serial(NV_DATA(x), NV_LENGTH(x));
}

void N_VAbs_Serial(N_Vector x, N_Vector z) {
  if (!x || !z) return;
  VAbs_serial(NV_DATA(x), NV_DATA(z), NV_LENGTH(x));
}

