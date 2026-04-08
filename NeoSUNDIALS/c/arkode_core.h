#ifndef EXTRACTED_ARKODE_CORE_H
#define EXTRACTED_ARKODE_CORE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*ark_rhs_fn)(double t, const double* y, double* ydot, void* user_data);
typedef int (*ark_jac_fn)(double t, const double* y, double* jacobian, void* user_data);

typedef enum
{
  ARK_METHOD_ERK_RK4 = 0,
  ARK_METHOD_DIRK_IMPLICIT_MIDPOINT = 1
} ARKMethodID;

typedef struct
{
  int dimension;
  int max_steps;
  int max_newton_iters;
  double rtol;
  double atol;
  double h_init;
  double h_min;
  double h_max;
  double safety;
  double min_factor;
  double max_factor;
  double newton_tol;
  ARKMethodID method;
} ARKConfig;

typedef struct
{
  int step_index;
  int accepted;
  int stage_evals;
  int newton_iters;
  double t_start;
  double t_end;
  double h_used;
  double h_next;
  double error_norm;
} ARKStepStats;

typedef struct
{
  int status;
  int steps;
  int accepted_steps;
  int rejected_steps;
  int rhs_evals;
  int jac_evals;
  int newton_iters;
  double current_t;
  double current_h;
  double last_error_norm;
} ARKSummary;

typedef struct ARKState ARKState;

ARKState* ark_create(const ARKConfig* config, double t0, const double* y0);
void ark_free(ARKState* state);

int ark_step(ARKState* state, ark_rhs_fn rhs, ark_jac_fn jac, void* user_data,
             ARKStepStats* stats);

int ark_get_state(const ARKState* state, double* out_y);
int ark_get_summary(const ARKState* state, ARKSummary* summary);
double ark_get_time(const ARKState* state);
int ark_get_dimension(const ARKState* state);
int ark_set_step_size(ARKState* state, double h);

#ifdef __cplusplus
}
#endif

#endif
