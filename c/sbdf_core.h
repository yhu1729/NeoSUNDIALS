#ifndef SBDF_CORE_H
#define SBDF_CORE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*sbdf_rhs_fn)(double t, const double* y, double* dydt, void* user_data);
typedef int (*sbdf_jac_fn)(double t, const double* y, double* jacobian, void* user_data);
typedef int (*sbdf_res_fn)(double t, const double* y, const double* ydot, double* residual,
                           void* user_data);

typedef struct
{
  int dimension;
  int max_order;
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
} SBDFConfig;

typedef struct
{
  int step_index;
  int accepted;
  int order;
  int newton_iters;
  double t_start;
  double t_end;
  double h_used;
  double h_next;
  double error_norm;
  double newton_norm;
} SBDFStepStats;

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
  double last_newton_norm;
} SBDFSummary;

typedef struct SBDFState SBDFState;

SBDFState* sbdf_create(const SBDFConfig* config, double t0, const double* y0);
void sbdf_free(SBDFState* state);

int sbdf_step(SBDFState* state, sbdf_rhs_fn rhs, sbdf_jac_fn jac, void* user_data,
              SBDFStepStats* stats);
int sbdf_step_residual(SBDFState* state, sbdf_res_fn residual, void* user_data,
                       SBDFStepStats* stats);

int sbdf_get_state(const SBDFState* state, double* out_y);
int sbdf_get_summary(const SBDFState* state, SBDFSummary* summary);
double sbdf_get_time(const SBDFState* state);
int sbdf_get_dimension(const SBDFState* state);
int sbdf_set_step_size(SBDFState* state, double h);

#ifdef __cplusplus
}
#endif

#endif
