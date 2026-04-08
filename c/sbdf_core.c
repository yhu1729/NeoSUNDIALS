#include "sbdf_core.h"

#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define SBDF_OK 0
#define SBDF_ERR_INPUT -1
#define SBDF_ERR_MEMORY -2
#define SBDF_ERR_CALLBACK -3
#define SBDF_ERR_CONVERGENCE -4
#define SBDF_ERR_WORK_LIMIT -5

#define SBDF_MAX_ORDER 5

struct SBDFState
{
  SBDFConfig config;
  int dimension;
  int current_order;
  int history_len;
  int step_index;
  int accepted_steps;
  int rejected_steps;
  int rhs_evals;
  int jac_evals;
  int newton_iters;
  double t;
  double h;
  double last_error_norm;
  double last_newton_norm;
  double* y_hist[SBDF_MAX_ORDER];
  double step_hist[SBDF_MAX_ORDER];
  double* work_a;
  double* work_b;
  double* work_c;
  double* work_d;
  double* work_e;
  double* jacobian_buf;
  double* matrix;
  double* rhs_vec;
  int* pivots;
};

static double sbdf_abs(double value) { return value < 0.0 ? -value : value; }

static double sbdf_clamp(double value, double lower, double upper)
{
  if (value < lower) { return lower; }
  if (value > upper) { return upper; }
  return value;
}

static void sbdf_copy(int n, const double* src, double* dst)
{
  memcpy(dst, src, (size_t)n * sizeof(double));
}

static void sbdf_scale(int n, double factor, const double* src, double* dst)
{
  int i;
  for (i = 0; i < n; ++i) { dst[i] = factor * src[i]; }
}

static void sbdf_axpy(int n, double alpha, const double* x, double* y)
{
  int i;
  for (i = 0; i < n; ++i) { y[i] += alpha * x[i]; }
}

static double sbdf_wrms_norm(int n, const double* vec, const double* ref, double rtol,
                             double atol)
{
  double sum = 0.0;
  int i;
  for (i = 0; i < n; ++i)
  {
    const double weight = 1.0 / (rtol * sbdf_abs(ref[i]) + atol);
    const double scaled = vec[i] * weight;
    sum += scaled * scaled;
  }
  return sqrt(sum / (double)n);
}

static int sbdf_eval_rhs(SBDFState* state, sbdf_rhs_fn rhs, void* user_data, double t,
                         const double* y, double* dydt)
{
  const int flag = rhs(t, y, dydt, user_data);
  state->rhs_evals += 1;
  return flag;
}

static int sbdf_solve_dense(int n, double* matrix, double* rhs, int* pivots)
{
  int i;
  int j;
  int k;

  for (k = 0; k < n; ++k)
  {
    int pivot = k;
    double max_abs = sbdf_abs(matrix[k * n + k]);
    for (i = k + 1; i < n; ++i)
    {
      const double candidate = sbdf_abs(matrix[i * n + k]);
      if (candidate > max_abs)
      {
        max_abs = candidate;
        pivot = i;
      }
    }

    if (max_abs < 1e-14) { return SBDF_ERR_CONVERGENCE; }

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

  return SBDF_OK;
}

static int sbdf_solve_small_system(int n, double* matrix, double* rhs, int* pivots)
{
  return sbdf_solve_dense(n, matrix, rhs, pivots);
}

static void sbdf_build_bdf_nodes(int order, double h, const double* step_hist,
                                 double* nodes)
{
  double cumulative = 0.0;
  int i;

  nodes[0] = 0.0;
  cumulative += h;
  for (i = 1; i <= order; ++i)
  {
    nodes[i] = -cumulative / h;
    if (i <= order - 1) { cumulative += step_hist[i - 1]; }
  }
}

static int sbdf_compute_derivative_weights(int order, double h,
                                           const double* step_hist, double* alpha,
                                           int* pivots)
{
  double nodes[SBDF_MAX_ORDER + 1];
  double matrix[(SBDF_MAX_ORDER + 1) * (SBDF_MAX_ORDER + 1)];
  double rhs[SBDF_MAX_ORDER + 1];
  int row;
  int col;
  const int size = order + 1;

  sbdf_build_bdf_nodes(order, h, step_hist, nodes);

  for (row = 0; row < size; ++row)
  {
    double power = 1.0;
    rhs[row] = (row == 1) ? 1.0 : 0.0;
    for (col = 0; col < size; ++col)
    {
      if (col == 0) { power = 1.0; }
      else { power = pow(nodes[col], (double)row); }
      matrix[row * size + col] = power;
    }
  }

  if (sbdf_solve_small_system(size, matrix, rhs, pivots) != SBDF_OK)
  {
    return SBDF_ERR_CONVERGENCE;
  }

  for (row = 0; row < size; ++row) { alpha[row] = rhs[row]; }
  return SBDF_OK;
}

static int sbdf_compute_extrapolation_weights(int order, double h,
                                              const double* step_hist, double* weights,
                                              int* pivots)
{
  double nodes[SBDF_MAX_ORDER];
  double matrix[SBDF_MAX_ORDER * SBDF_MAX_ORDER];
  double rhs[SBDF_MAX_ORDER];
  double cumulative = h;
  int row;
  int col;

  for (col = 0; col < order; ++col)
  {
    nodes[col] = -cumulative / h;
    if (col < order - 1) { cumulative += step_hist[col]; }
  }

  for (row = 0; row < order; ++row)
  {
    rhs[row] = (row == 0) ? 1.0 : 0.0;
    for (col = 0; col < order; ++col)
    {
      matrix[row * order + col] = pow(nodes[col], (double)row);
    }
  }

  if (sbdf_solve_small_system(order, matrix, rhs, pivots) != SBDF_OK)
  {
    return SBDF_ERR_CONVERGENCE;
  }

  for (row = 0; row < order; ++row) { weights[row] = rhs[row]; }
  return SBDF_OK;
}

static void sbdf_predict_state(const SBDFState* state, int order, double h, double* y_pred)
{
  double weights[SBDF_MAX_ORDER];
  int pivots[SBDF_MAX_ORDER];
  int i;

  if (order <= 1)
  {
    sbdf_copy(state->dimension, state->y_hist[0], y_pred);
    return;
  }

  if (sbdf_compute_extrapolation_weights(order, h, state->step_hist, weights, pivots) !=
      SBDF_OK)
  {
    sbdf_copy(state->dimension, state->y_hist[0], y_pred);
    return;
  }

  memset(y_pred, 0, (size_t)state->dimension * sizeof(double));
  for (i = 0; i < order; ++i)
  {
    sbdf_axpy(state->dimension, weights[i], state->y_hist[i], y_pred);
  }
}

static int sbdf_form_jacobian(SBDFState* state, sbdf_rhs_fn rhs, sbdf_jac_fn jac,
                              void* user_data, double t_new, const double* y,
                              double alpha0, double h, double* matrix)
{
  const int n = state->dimension;
  int i;
  int j;

  if (jac != NULL)
  {
    const int flag = jac(t_new, y, state->jacobian_buf, user_data);
    state->jac_evals += 1;
    if (flag != 0) { return SBDF_ERR_CALLBACK; }
    for (i = 0; i < n; ++i)
    {
      for (j = 0; j < n; ++j)
      {
        matrix[i * n + j] = -h * state->jacobian_buf[i * n + j];
      }
      matrix[i * n + i] += alpha0;
    }
    return SBDF_OK;
  }

  if (sbdf_eval_rhs(state, rhs, user_data, t_new, y, state->work_b) != 0)
  {
    return SBDF_ERR_CALLBACK;
  }

  for (j = 0; j < n; ++j)
  {
    const double base = y[j];
    const double delta = 1e-8 * (1.0 + sbdf_abs(base));
    sbdf_copy(n, y, state->work_c);
    state->work_c[j] += delta;
    if (sbdf_eval_rhs(state, rhs, user_data, t_new, state->work_c, state->work_d) != 0)
    {
      return SBDF_ERR_CALLBACK;
    }
    for (i = 0; i < n; ++i)
    {
      const double deriv = (state->work_d[i] - state->work_b[i]) / delta;
      matrix[i * n + j] = -h * deriv;
    }
  }

  state->jac_evals += 1;
  for (i = 0; i < n; ++i) { matrix[i * n + i] += alpha0; }
  return SBDF_OK;
}

static int sbdf_attempt_step(SBDFState* state, int order, double h, sbdf_rhs_fn rhs,
                             sbdf_jac_fn jac, void* user_data, double* y_out,
                             double* error_proxy, double* newton_norm, int* iters_out)
{
  const int n = state->dimension;
  double alpha[SBDF_MAX_ORDER + 1];
  int iter;
  int i;

  sbdf_predict_state(state, order, h, state->work_a);
  sbdf_copy(n, state->work_a, y_out);

  if (sbdf_compute_derivative_weights(order, h, state->step_hist, alpha, state->pivots) !=
      SBDF_OK)
  {
    return SBDF_ERR_CONVERGENCE;
  }

  for (iter = 0; iter < state->config.max_newton_iters; ++iter)
  {
    if (sbdf_eval_rhs(state, rhs, user_data, state->t + h, y_out, state->work_b) != 0)
    {
      return SBDF_ERR_CALLBACK;
    }

    for (i = 0; i < n; ++i)
    {
      double residual = alpha[0] * y_out[i] - h * state->work_b[i];
      int hist;
      for (hist = 1; hist <= order; ++hist)
      {
        residual += alpha[hist] * state->y_hist[hist - 1][i];
      }
      state->rhs_vec[i] = -residual;
    }

    if (sbdf_form_jacobian(state, rhs, jac, user_data, state->t + h, y_out, alpha[0], h,
                           state->matrix) != SBDF_OK)
    {
      return SBDF_ERR_CALLBACK;
    }

    if (sbdf_solve_dense(n, state->matrix, state->rhs_vec, state->pivots) != SBDF_OK)
    {
      return SBDF_ERR_CONVERGENCE;
    }

    sbdf_axpy(n, 1.0, state->rhs_vec, y_out);
    *newton_norm =
      sbdf_wrms_norm(n, state->rhs_vec, y_out, state->config.rtol, state->config.atol);

    if (*newton_norm <= state->config.newton_tol)
    {
      *iters_out = iter + 1;
      break;
    }
  }

  if (iter == state->config.max_newton_iters) { return SBDF_ERR_CONVERGENCE; }

  for (i = 0; i < n; ++i) { state->work_e[i] = y_out[i] - state->work_a[i]; }
  *error_proxy =
    sbdf_wrms_norm(n, state->work_e, y_out, state->config.rtol, state->config.atol);
  return SBDF_OK;
}

static void sbdf_accept_step(SBDFState* state, const double* y_new, double h, int order)
{
  double* old_last = state->y_hist[SBDF_MAX_ORDER - 1];
  int i;

  for (i = SBDF_MAX_ORDER - 1; i > 0; --i) { state->y_hist[i] = state->y_hist[i - 1]; }
  state->y_hist[0] = old_last;
  sbdf_copy(state->dimension, y_new, state->y_hist[0]);

  for (i = SBDF_MAX_ORDER - 1; i > 0; --i) { state->step_hist[i] = state->step_hist[i - 1]; }
  state->step_hist[0] = h;

  if (state->history_len < SBDF_MAX_ORDER) { state->history_len += 1; }

  state->t += h;
  state->current_order = order;
  state->accepted_steps += 1;
}

SBDFState* sbdf_create(const SBDFConfig* config, double t0, const double* y0)
{
  SBDFState* state;
  int i;

  if (config == NULL || y0 == NULL || config->dimension <= 0) { return NULL; }

  state = (SBDFState*)calloc(1, sizeof(SBDFState));
  if (state == NULL) { return NULL; }

  state->config = *config;
  if (state->config.max_order < 1) { state->config.max_order = 1; }
  if (state->config.max_order > 2) { state->config.max_order = 2; }
  if (state->config.max_steps <= 0) { state->config.max_steps = 10000; }
  if (state->config.max_newton_iters <= 0) { state->config.max_newton_iters = 8; }
  if (state->config.rtol <= 0.0) { state->config.rtol = 1e-6; }
  if (state->config.atol <= 0.0) { state->config.atol = 1e-9; }
  if (state->config.h_init <= 0.0) { state->config.h_init = 1e-3; }
  if (state->config.h_min <= 0.0) { state->config.h_min = 1e-10; }
  if (state->config.h_max <= 0.0) { state->config.h_max = 1.0; }
  if (state->config.safety <= 0.0) { state->config.safety = 0.9; }
  if (state->config.min_factor <= 0.0) { state->config.min_factor = 0.2; }
  if (state->config.max_factor <= 1.0) { state->config.max_factor = 5.0; }
  if (state->config.newton_tol <= 0.0) { state->config.newton_tol = 0.1; }

  state->dimension = state->config.dimension;
  state->current_order = 1;
  state->history_len = 1;
  state->t = t0;
  state->h = sbdf_clamp(state->config.h_init, state->config.h_min, state->config.h_max);

  for (i = 0; i < SBDF_MAX_ORDER; ++i)
  {
    state->y_hist[i] = (double*)calloc((size_t)state->dimension, sizeof(double));
    if (state->y_hist[i] == NULL)
    {
      sbdf_free(state);
      return NULL;
    }
  }

  state->work_a = (double*)calloc((size_t)state->dimension, sizeof(double));
  state->work_b = (double*)calloc((size_t)state->dimension, sizeof(double));
  state->work_c = (double*)calloc((size_t)state->dimension, sizeof(double));
  state->work_d = (double*)calloc((size_t)state->dimension, sizeof(double));
  state->work_e = (double*)calloc((size_t)state->dimension, sizeof(double));
  state->jacobian_buf =
    (double*)calloc((size_t)state->dimension * (size_t)state->dimension, sizeof(double));
  state->matrix =
    (double*)calloc((size_t)state->dimension * (size_t)state->dimension, sizeof(double));
  state->rhs_vec = (double*)calloc((size_t)state->dimension, sizeof(double));
  state->pivots = (int*)calloc((size_t)state->dimension, sizeof(int));

  if (state->work_a == NULL || state->work_b == NULL || state->work_c == NULL ||
      state->work_d == NULL || state->work_e == NULL || state->jacobian_buf == NULL ||
      state->matrix == NULL || state->rhs_vec == NULL || state->pivots == NULL)
  {
    sbdf_free(state);
    return NULL;
  }

  sbdf_copy(state->dimension, y0, state->y_hist[0]);
  return state;
}

void sbdf_free(SBDFState* state)
{
  int i;

  if (state == NULL) { return; }

  for (i = 0; i < SBDF_MAX_ORDER; ++i) { free(state->y_hist[i]); }
  free(state->work_a);
  free(state->work_b);
  free(state->work_c);
  free(state->work_d);
  free(state->work_e);
  free(state->jacobian_buf);
  free(state->matrix);
  free(state->rhs_vec);
  free(state->pivots);
  free(state);
}

int sbdf_step(SBDFState* state, sbdf_rhs_fn rhs, sbdf_jac_fn jac, void* user_data,
              SBDFStepStats* stats)
{
  double h;
  int local_retries = 0;

  if (state == NULL || rhs == NULL || stats == NULL) { return SBDF_ERR_INPUT; }
  if (state->step_index >= state->config.max_steps) { return SBDF_ERR_WORK_LIMIT; }

  while (local_retries < 12)
  {
    double y1_norm = 0.0;
    double y2_norm = 0.0;
    double newton1 = 0.0;
    double newton2 = 0.0;
    int iters1 = 0;
    int iters2 = 0;
    int have_order2 = 0;
    int status1;
    int status2 = SBDF_ERR_CONVERGENCE;
    int accepted_order;
    double accepted_error;
    double accepted_newton;
    double next_factor;

    h = sbdf_clamp(state->h, state->config.h_min, state->config.h_max);
    status1 = sbdf_attempt_step(state, 1, h, rhs, jac, user_data, state->work_c,
                                &y1_norm, &newton1, &iters1);
    if (status1 != SBDF_OK) { return status1; }

    have_order2 = state->config.max_order >= 2 && state->history_len >= 2;
    if (have_order2)
    {
      status2 = sbdf_attempt_step(state, 2, h, rhs, jac, user_data, state->work_d,
                                  &y2_norm, &newton2, &iters2);
      if (status2 == SBDF_OK)
      {
        int i;
        for (i = 0; i < state->dimension; ++i)
        {
          state->work_e[i] = state->work_d[i] - state->work_c[i];
        }
        y2_norm = sbdf_wrms_norm(state->dimension, state->work_e, state->work_d,
                                 state->config.rtol, state->config.atol);
      }
      else
      {
        have_order2 = 0;
      }
    }

    if (have_order2 && y2_norm <= 1.0)
    {
      accepted_order = 2;
      accepted_error = y2_norm;
      accepted_newton = newton2;
      sbdf_accept_step(state, state->work_d, h, 2);
      state->newton_iters += iters2;
    }
    else if (y1_norm <= 1.0)
    {
      accepted_order = 1;
      accepted_error = y1_norm;
      accepted_newton = newton1;
      sbdf_accept_step(state, state->work_c, h, 1);
      state->newton_iters += iters1;
    }
    else
    {
      const double best_error = have_order2 ? (y2_norm < y1_norm ? y2_norm : y1_norm) : y1_norm;
      const double exponent = have_order2 && y2_norm < y1_norm ? (1.0 / 3.0) : 0.5;
      next_factor = state->config.safety * pow(best_error, -exponent);
      next_factor = sbdf_clamp(next_factor, state->config.min_factor, 0.8);
      state->h = sbdf_clamp(h * next_factor, state->config.h_min, state->config.h_max);
      state->rejected_steps += 1;
      local_retries += 1;
      if (state->h <= state->config.h_min + 1e-18) { return SBDF_ERR_CONVERGENCE; }
      continue;
    }

    next_factor = state->config.safety *
                  pow(accepted_error < 1e-12 ? 1e-12 : accepted_error,
                      accepted_order == 1 ? -0.5 : -1.0 / 3.0);
    next_factor =
      sbdf_clamp(next_factor, state->config.min_factor, state->config.max_factor);
    state->h = sbdf_clamp(h * next_factor, state->config.h_min, state->config.h_max);

    state->step_index += 1;
    state->last_error_norm = accepted_error;
    state->last_newton_norm = accepted_newton;

    stats->step_index = state->step_index;
    stats->accepted = 1;
    stats->order = accepted_order;
    stats->newton_iters = accepted_order == 1 ? iters1 : iters2;
    stats->t_start = state->t - h;
    stats->t_end = state->t;
    stats->h_used = h;
    stats->h_next = state->h;
    stats->error_norm = accepted_error;
    stats->newton_norm = accepted_newton;
    return SBDF_OK;
  }

  return SBDF_ERR_CONVERGENCE;
}

int sbdf_get_state(const SBDFState* state, double* out_y)
{
  if (state == NULL || out_y == NULL) { return SBDF_ERR_INPUT; }
  sbdf_copy(state->dimension, state->y_hist[0], out_y);
  return SBDF_OK;
}

int sbdf_get_summary(const SBDFState* state, SBDFSummary* summary)
{
  if (state == NULL || summary == NULL) { return SBDF_ERR_INPUT; }
  summary->status = SBDF_OK;
  summary->steps = state->step_index;
  summary->accepted_steps = state->accepted_steps;
  summary->rejected_steps = state->rejected_steps;
  summary->rhs_evals = state->rhs_evals;
  summary->jac_evals = state->jac_evals;
  summary->newton_iters = state->newton_iters;
  summary->current_t = state->t;
  summary->current_h = state->h;
  summary->last_error_norm = state->last_error_norm;
  summary->last_newton_norm = state->last_newton_norm;
  return SBDF_OK;
}

double sbdf_get_time(const SBDFState* state)
{
  if (state == NULL) { return 0.0; }
  return state->t;
}

int sbdf_get_dimension(const SBDFState* state)
{
  if (state == NULL) { return 0; }
  return state->dimension;
}

int sbdf_set_step_size(SBDFState* state, double h)
{
  if (state == NULL || h <= 0.0) { return SBDF_ERR_INPUT; }
  state->h = sbdf_clamp(h, state->config.h_min, state->config.h_max);
  return SBDF_OK;
}
