#ifndef NVECTOR_SERIAL_H
#define NVECTOR_SERIAL_H

typedef long sunindextype;

typedef struct N_VectorContent_Serial *N_Vector;

struct N_VectorContent_Serial {
  sunindextype length;
  double *data;
  sunindextype own_data; /* 1 if data owned */
};

#define NV_LENGTH_S(v)   (((struct N_VectorContent_Serial *)(v))->length)
#define NV_DATA_S(v)     (((struct N_VectorContent_Serial *)(v))->data)
#define NV_OWN_DATA_S(v) (((struct N_VectorContent_Serial *)(v))->own_data)

#ifdef __cplusplus
extern "C" {
#endif

N_Vector N_VNew_Serial(sunindextype length);
N_Vector N_VClone_Serial(N_Vector w);
void N_VDestroy_Serial(N_Vector v);
sunindextype N_VGetLength_Serial(N_Vector v);
double *N_VGetArrayPointer_Serial(N_Vector v);
void N_VSetArrayPointer_Serial(double *v_data, N_Vector v);

void N_VLinearSum_Serial(double a, N_Vector x, double b, N_Vector y, N_Vector z);
void N_VScale_Serial(double c, N_Vector x, N_Vector z);
void N_VAxpy_Serial(double a, N_Vector x, N_Vector y);
double N_VDotProd_Serial(N_Vector x, N_Vector y);
double N_VWrmsNorm_Serial(N_Vector x, N_Vector w);
double N_VMin_Serial(N_Vector x);
double N_VMax_Serial(N_Vector x);
void N_VAbs_Serial(N_Vector x, N_Vector z);

#ifdef __cplusplus
}
#endif

#endif /* NVECTOR_SERIAL_H */

