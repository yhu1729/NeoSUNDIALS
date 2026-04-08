#include "arkode_core.h"

#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define ARK_OK 0
#define ARK_ERR_INPUT -1
#define ARK_ERR_MEMORY -2
#define ARK_ERR_CALLBACK -3
#define ARK_ERR_CONVERGENCE -4
#define ARK_ERR_WORK_LIMIT -5

#define ARK_MAX_STAGES 4

typedef struct
{
  int stages;
  int order;
  int implicit;
  double A[ARK_MAX_STAGES][ARK_MAX_STAGES];
  double b[ARK_MAX_STAGES];
  double c[ARK_MAX_STAGES];
} ARKTable;

struct ARKState
{
  ARKConfig config;
  ARKTable table;
  int dimension;
  int step_index;
  int accepted_steps;
  int rejected_steps;
  int rhs_evals;
  int jac_evals;
  int newton_iters;
  double t;
  double h;
  double last_error_norm;
  double* y;
  double* y_prev;
  double* f_prev;
  double* y_trial;
  double* y_half;
  double* y_stage;
  double* stage_rhs[ARK_MAX_STAGES];
  double* stage_state[ARK_MAX_STAGES];
  double* matrix;
  double* rhs_vec;
  double* jacobian_buf;
  int* pivots;
};

static double ark_abs(double value) { return value < 0.0 ? -value : value; }

static double ark_clamp(double value, double lower, double upper)
{
  if (value < lower) { return lower; }
  if (value > upper) { return upper; }
  return value;
}

static void ark_copy(int n, const double* src, double* dst)
{
  memcpy(dst, src, (size_t)n * sizeof(double));
}

static void ark_fill_table(ARKTable* table, ARKMethodID method)
{
  memset(table, 0, sizeof(*table));
  if (method == ARK_METHOD_DIRK_IMPLICIT_MIDPOINT)
  {
    table->stages = 1;
    table->order = 2;
    table->implicit = 1;
    table->A[0][0] = 0.5;
    table->b[0] = 1.0;
    table->c[0] = 0.5;
    return;
  }

  table->stages = 4;
  table->order = 4;
  table->implicit = 0;
  table->A[1][0] = 0.5;
  table->A[2][1] = 0.5;
  table->A[3][2] = 1.0;
  table->b[0] = 1.0 / 6.0;
  table->b[1] = 1.0 / 3.0;
  table->b[2] = 1.0 / 3.0;
  table->b[3] = 1.0 / 6.0;
  table->c[0] = 0.0;
  table->c[1] = 0.5;
  table->c[2] = 0.5;
  table->c[3] = 1.0;
}

static double ark_wrms_norm(int n, const double* vec, const double* ref, double rtol,
                            double atol)
{
  double sum = 0.0;
  int i;
  for (i = 0; i < n; ++i)
  {
    const double weight = 1.0 / (rtol * ark_abs(ref[i]) + atol);
    const double scaled = vec[i] * weight;
    sum += scaled * scaled;
  }
  return sqrt(sum / (double)n);
}

static int ark_eval_rhs(ARKState* state, ark_rhs_fn rhs, void* user_data, double t,
                        const double* y, double* ydot)
{
  const int flag = rhs(t, y, ydot, user_data);
  state->rhs_evals += 1;
  return flag;
}

static int ark_solve_dense(int n, double* matrix, double* rhs, int* pivots)
{
  int i;
  int j;
  int k;

  for (k = 0; k < n; ++k)
  {
    int pivot = k;
    double max_abs = ark_abs(matrix[k * n + k]);
    for (i = k + 1; i < n; ++i)
    {
      const double candidate = ark_abs(matrix[i * n + k]);
      if (candidate > max_abs)
      {
        max_abs = candidate;
        pivot = i;
      }
    }

    if (max_abs < 1e-14) { return ARK_ERR_CONVERGENCE; }

    pivots[k] = pivot;
    if (pivot != k)
    {
      for (j = k; j < n; ++j)
      {
        const double tmp = matrix[k * n + j];
        matrix[k * n + j] = matrix[pivot * n + j];
        matrix[pivot * n + j] = tmp;
      }
      {
        const double tmp = rhs[k];
        rhs[k] = rhs[pivot];
        rhs[pivot] = tmp;
      }
    }

    for (i = k + 1; i < n; ++i)
    {
      const double factor = matrix[i * n + k] / matrix[k * n + k];
      matrix[i * n + k] = 0.0;
      for (j = k + 1; j < n; ++j)
      {
        matrix[i * n + j] -= factor * matrix[k * n + j];
      }
      rhs[i] -= factor * rhs[k];
    }
  }

  for (i = n - 1; i >= 0; --i)
  {
    double sum = rhs[i];
    for (j = i + 1; j < n; ++j) { sum -= matrix[i * n + j] * rhs[j]; }
    rhs[i] = sum / matrix[i * n + i];
  }

  return ARK_OK;
}

static int ark_form_jacobian(ARKState* state, ark_rhs_fn rhs, ark_jac_fn jac,
                             void* user_data, double t, const double* y, double gamma)
{
  const int n = state->dimension;
  int i;
  int j;

  if (jac != NULL)
  {
    const int flag = jac(t, y, state->jacobian_buf, user_data);
    state->jac_evals += 1;
    if (flag != 0) { return ARK_ERR_CALLBACK; }
    for (i = 0; i < n; ++i)
    {
      for (j = 0; j < n; ++j)
      {
        state->matrix[i * n + j] = -gamma * state->jacobian_buf[i * n + j];
      }
      state->matrix[i * n + i] += 1.0;
    }
    return ARK_OK;
  }

  if (ark_eval_rhs(state, rhs, user_data, t, y, state->stage_rhs[0]) != 0)
  {
    return ARK_ERR_CALLBACK;
  }
  for (j = 0; j < n; ++j)
  {
    const double base = y[j];
    const double delta = 1e-8 * (1.0 + ark_abs(base));
    ark_copy(n, y, state->y_stage);
    state->y_stage[j] += delta;
    if (ark_eval_rhs(state, rhs, user_data, t, state->y_stage, state->stage_rhs[1]) != 0)
    {
      return ARK_ERR_CALLBACK;
    }
    for (i = 0; i < n; ++i)
    {
      const double deriv = (state->stage_rhs[1][i] - state->stage_rhs[0][i]) / delta;
      state->matrix[i * n + j] = -gamma * deriv;
    }
  }
  state->jac_evals += 1;
  for (i = 0; i < n; ++i) { state->matrix[i * n + i] += 1.0; }
  return ARK_OK;
}

static int ark_explicit_step(ARKState* state, double t, double h, ark_rhs_fn rhs,
                             void* user_data, double* y_in, double* y_out, int* stage_evals)
{
  int stage;
  int i;
  const int n = state->dimension;

  for (stage = 0; stage < state->table.stages; ++stage)
  {
    ark_copy(n, y_in, state->stage_state[stage]);
    for (i = 0; i < stage; ++i)
    {
      int j;
      const double coeff = h * state->table.A[stage][i];
      if (coeff == 0.0) { continue; }
      for (j = 0; j < n; ++j)
      {
        state->stage_state[stage][j] += coeff * state->stage_rhs[i][j];
      }
    }

    if (ark_eval_rhs(state, rhs, user_data, t + state->table.c[stage] * h,
                     state->stage_state[stage], state->stage_rhs[stage]) != 0)
    {
      return ARK_ERR_CALLBACK;
    }
    *stage_evals += 1;
  }

  ark_copy(n, y_in, y_out);
  for (stage = 0; stage < state->table.stages; ++stage)
  {
    int j;
    const double coeff = h * state->table.b[stage];
    for (j = 0; j < n; ++j) { y_out[j] += coeff * state->stage_rhs[stage][j]; }
  }
  return ARK_OK;
}

static int ark_implicit_step(ARKState* state, double t, double h, ark_rhs_fn rhs,
                             ark_jac_fn jac, void* user_data, double* y_in,
                             double* y_out, int* stage_evals, int* newton_iters)
{
  const int n = state->dimension;
  const double gamma = h * state->table.A[0][0];
  int i;
  int iter;

  ark_copy(n, y_in, state->stage_state[0]);
  for (iter = 0; iter < state->config.max_newton_iters; ++iter)
  {
    if (ark_eval_rhs(state, rhs, user_data, t + state->table.c[0] * h, state->stage_state[0],
                     state->stage_rhs[0]) != 0)
    {
      return ARK_ERR_CALLBACK;
    }
    *stage_evals += 1;

    for (i = 0; i < n; ++i)
    {
      state->rhs_vec[i] = -(state->stage_state[0][i] - y_in[i] - gamma * state->stage_rhs[0][i]);
    }

    if (ark_form_jacobian(state, rhs, jac, user_data, t + state->table.c[0] * h,
                          state->stage_state[0], gamma) != ARK_OK)
    {
      return ARK_ERR_CALLBACK;
    }
    if (ark_solve_dense(n, state->matrix, state->rhs_vec, state->pivots) != ARK_OK)
    {
      return ARK_ERR_CONVERGENCE;
    }

    for (i = 0; i < n; ++i) { state->stage_state[0][i] += state->rhs_vec[i]; }
    *newton_iters += 1;
    if (ark_wrms_norm(n, state->rhs_vec, state->stage_state[0], state->config.rtol,
                      state->config.atol) <= state->config.newton_tol)
    {
      break;
    }
  }

  if (iter == state->config.max_newton_iters) { return ARK_ERR_CONVERGENCE; }

  ark_copy(n, y_in, y_out);
  for (i = 0; i < n; ++i) { y_out[i] += h * state->stage_rhs[0][i]; }
  return ARK_OK;
}

static int ark_take_single_step(ARKState* state, double t, double h, ark_rhs_fn rhs,
                                ark_jac_fn jac, void* user_data, const double* y_in,
                                double* y_out, int* stage_evals, int* newton_iters)
{
  if (state->table.implicit)
  {
    return ark_implicit_step(state, t, h, rhs, jac, user_data, (double*)y_in, y_out,
                             stage_evals, newton_iters);
  }
  return ark_explicit_step(state, t, h, rhs, user_data, (double*)y_in, y_out, stage_evals);
}

ARKState* ark_create(const ARKConfig* config, double t0, const double* y0)
{
  ARKState* state;
  int i;

  if (config == NULL || y0 == NULL || config->dimension <= 0) { return NULL; }

  state = (ARKState*)calloc(1, sizeof(ARKState));
  if (state == NULL) { return NULL; }

  state->config = *config;
  if (state->config.max_steps <= 0) { state->config.max_steps = 10000; }
  if (state->config.max_newton_iters <= 0) { state->config.max_newton_iters = 8; }
  if (state->config.rtol <= 0.0) { state->config.rtol = 1e-6; }
  if (state->config.atol <= 0.0) { state->config.atol = 1e-9; }
  if (state->config.h_init <= 0.0) { state->config.h_init = 1e-3; }
  if (state->config.h_min <= 0.0) { state->config.h_min = 1e-10; }
  if (state->config.h_max <= 0.0) { state->config.h_max = 0.1; }
  if (state->config.safety <= 0.0) { state->config.safety = 0.9; }
  if (state->config.min_factor <= 0.0) { state->config.min_factor = 0.2; }
  if (state->config.max_factor <= 1.0) { state->config.max_factor = 5.0; }
  if (state->config.newton_tol <= 0.0) { state->config.newton_tol = 0.1; }

  state->dimension = state->config.dimension;
  state->t = t0;
  state->h = ark_clamp(state->config.h_init, state->config.h_min, state->config.h_max);
  ark_fill_table(&state->table, state->config.method);

  state->y = (double*)calloc((size_t)state->dimension, sizeof(double));
  state->y_prev = (double*)calloc((size_t)state->dimension, sizeof(double));
  state->f_prev = (double*)calloc((size_t)state->dimension, sizeof(double));
  state->y_trial = (double*)calloc((size_t)state->dimension, sizeof(double));
  state->y_half = (double*)calloc((size_t)state->dimension, sizeof(double));
  state->y_stage = (double*)calloc((size_t)state->dimension, sizeof(double));
  state->matrix =
    (double*)calloc((size_t)state->dimension * (size_t)state->dimension, sizeof(double));
  state->rhs_vec = (double*)calloc((size_t)state->dimension, sizeof(double));
  state->jacobian_buf =
    (double*)calloc((size_t)state->dimension * (size_t)state->dimension, sizeof(double));
  state->pivots = (int*)calloc((size_t)state->dimension, sizeof(int));

  for (i = 0; i < ARK_MAX_STAGES; ++i)
  {
    state->stage_rhs[i] = (double*)calloc((size_t)state->dimension, sizeof(double));
    state->stage_state[i] = (double*)calloc((size_t)state->dimension, sizeof(double));
  }

  if (state->y == NULL || state->y_prev == NULL || state->f_prev == NULL ||
      state->y_trial == NULL || state->y_half == NULL || state->y_stage == NULL ||
      state->matrix == NULL || state->rhs_vec == NULL || state->jacobian_buf == NULL ||
      state->pivots == NULL)
  {
    ark_free(state);
    return NULL;
  }
  for (i = 0; i < ARK_MAX_STAGES; ++i)
  {
    if (state->stage_rhs[i] == NULL || state->stage_state[i] == NULL)
    {
      ark_free(state);
      return NULL;
    }
  }

  ark_copy(state->dimension, y0, state->y);
  ark_copy(state->dimension, y0, state->y_prev);
  return state;
}

void ark_free(ARKState* state)
{
  int i;

  if (state == NULL) { return; }
  free(state->y);
  free(state->y_prev);
  free(state->f_prev);
  free(state->y_trial);
  free(state->y_half);
  free(state->y_stage);
  free(state->matrix);
  free(state->rhs_vec);
  free(state->jacobian_buf);
  free(state->pivots);
  for (i = 0; i < ARK_MAX_STAGES; ++i)
  {
    free(state->stage_rhs[i]);
    free(state->stage_state[i]);
  }
  free(state);
}

int ark_step(ARKState* state, ark_rhs_fn rhs, ark_jac_fn jac, void* user_data,
             ARKStepStats* stats)
{
  int retries = 0;

  if (state == NULL || rhs == NULL || stats == NULL) { return ARK_ERR_INPUT; }
  if (state->step_index >= state->config.max_steps) { return ARK_ERR_WORK_LIMIT; }

  while (retries < 12)
  {
    double h = ark_clamp(state->h, state->config.h_min, state->config.h_max);
    int stage_evals = 0;
    int newton_iters = 0;
    int status;
    double error_norm;
    double factor;
    int i;

    status = ark_take_single_step(state, state->t, h, rhs, jac, user_data, state->y,
                                  state->y_trial, &stage_evals, &newton_iters);
    if (status != ARK_OK) { return status; }

    status = ark_take_single_step(state, state->t, 0.5 * h, rhs, jac, user_data, state->y,
                                  state->y_half, &stage_evals, &newton_iters);
    if (status != ARK_OK) { return status; }

    status =
      ark_take_single_step(state, state->t + 0.5 * h, 0.5 * h, rhs, jac, user_data,
                           state->y_half, state->y_stage, &stage_evals, &newton_iters);
    if (status != ARK_OK) { return status; }

    for (i = 0; i < state->dimension; ++i) { state->rhs_vec[i] = state->y_stage[i] - state->y_trial[i]; }
    error_norm = ark_wrms_norm(state->dimension, state->rhs_vec, state->y_stage,
                               state->config.rtol, state->config.atol);

    if (error_norm <= 1.0)
    {
      factor = state->config.safety *
               pow(error_norm < 1e-12 ? 1e-12 : error_norm,
                   -1.0 / ((double)state->table.order + 1.0));
      factor = ark_clamp(factor, state->config.min_factor, state->config.max_factor);

      ark_copy(state->dimension, state->y, state->y_prev);
      ark_copy(state->dimension, state->y_stage, state->y);
      if (ark_eval_rhs(state, rhs, user_data, state->t + h, state->y, state->f_prev) != 0)
      {
        return ARK_ERR_CALLBACK;
      }

      state->t += h;
      state->h = ark_clamp(h * factor, state->config.h_min, state->config.h_max);
      state->step_index += 1;
      state->accepted_steps += 1;
      state->newton_iters += newton_iters;
      state->last_error_norm = error_norm;

      stats->step_index = state->step_index;
      stats->accepted = 1;
      stats->stage_evals = stage_evals;
      stats->newton_iters = newton_iters;
      stats->t_start = state->t - h;
      stats->t_end = state->t;
      stats->h_used = h;
      stats->h_next = state->h;
      stats->error_norm = error_norm;
      return ARK_OK;
    }

    factor = state->config.safety *
             pow(error_norm < 1e-12 ? 1e-12 : error_norm,
                 -1.0 / ((double)state->table.order + 1.0));
    factor = ark_clamp(factor, state->config.min_factor, 0.8);
    state->h = ark_clamp(h * factor, state->config.h_min, state->config.h_max);
    state->rejected_steps += 1;
    retries += 1;
    if (state->h <= state->config.h_min + 1e-18) { return ARK_ERR_CONVERGENCE; }
  }

  return ARK_ERR_CONVERGENCE;
}

int ark_get_state(const ARKState* state, double* out_y)
{
  if (state == NULL || out_y == NULL) { return ARK_ERR_INPUT; }
  ark_copy(state->dimension, state->y, out_y);
  return ARK_OK;
}

int ark_get_summary(const ARKState* state, ARKSummary* summary)
{
  if (state == NULL || summary == NULL) { return ARK_ERR_INPUT; }
  summary->status = ARK_OK;
  summary->steps = state->step_index;
  summary->accepted_steps = state->accepted_steps;
  summary->rejected_steps = state->rejected_steps;
  summary->rhs_evals = state->rhs_evals;
  summary->jac_evals = state->jac_evals;
  summary->newton_iters = state->newton_iters;
  summary->current_t = state->t;
  summary->current_h = state->h;
  summary->last_error_norm = state->last_error_norm;
  return ARK_OK;
}

double ark_get_time(const ARKState* state)
{
  if (state == NULL) { return 0.0; }
  return state->t;
}

int ark_get_dimension(const ARKState* state)
{
  if (state == NULL) { return 0; }
  return state->dimension;
}

int ark_set_step_size(ARKState* state, double h)
{
  if (state == NULL || h <= 0.0) { return ARK_ERR_INPUT; }
  state->h = ark_clamp(h, state->config.h_min, state->config.h_max);
  return ARK_OK;
}
