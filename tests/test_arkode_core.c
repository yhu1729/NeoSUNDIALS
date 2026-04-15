#include "../c/arkode_core.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static int failures = 0;

static void expect_true(int condition, const char* message)
{
  if (!condition)
  {
    fprintf(stderr, "FAILED: %s\n", message);
    failures++;
  }
}

static void expect_close(double actual, double expected, double tol, const char* message)
{
  if (fabs(actual - expected) > tol)
  {
    fprintf(stderr, "FAILED: %s (actual=%g expected=%g tol=%g)\n", message, actual,
            expected, tol);
    failures++;
  }
}

static int rhs_exp_growth(double t, const double* y, double* ydot, void* user_data)
{
  (void)t;
  (void)user_data;
  ydot[0] = y[0];
  return 0;
}

static int rhs_linear_decay(double t, const double* y, double* ydot, void* user_data)
{
  (void)t;
  (void)user_data;
  ydot[0] = -2.0 * y[0];
  return 0;
}

static int jac_linear_decay(double t, const double* y, double* jacobian, void* user_data)
{
  (void)t;
  (void)y;
  (void)user_data;
  jacobian[0] = -2.0;
  return 0;
}

static int rhs_zero(double t, const double* y, double* ydot, void* user_data)
{
  (void)t;
  (void)y;
  (void)user_data;
  ydot[0] = 0.0;
  return 0;
}

static void test_erk_rk4_exp_growth(void)
{
  ARKConfig config;
  ARKState* state;
  ARKSummary summary;
  ARKStepStats stats;
  double y0[1] = {1.0};
  double y[1] = {0.0};
  const double tf = 0.2;

  config.dimension = 1;
  config.max_steps = 500;
  config.max_newton_iters = 8;
  config.rtol = 1e-7;
  config.atol = 1e-10;
  config.h_init = 1e-3;
  config.h_min = 1e-8;
  config.h_max = 5e-2;
  config.safety = 0.9;
  config.min_factor = 0.2;
  config.max_factor = 4.0;
  config.newton_tol = 0.05;
  config.method = ARK_METHOD_ERK_RK4;

  state = ark_create(&config, 0.0, y0);
  expect_true(state != NULL, "ark_create succeeds for ERK test");
  if (state == NULL) { return; }

  while (ark_get_time(state) < tf - 1e-14)
  {
    int flag;
    double remaining = tf - ark_get_time(state);
    ark_get_summary(state, &summary);
    expect_true(ark_set_step_size(
                  state, remaining < summary.current_h ? remaining : summary.current_h) == 0,
                "ark_set_step_size succeeds during ERK integration");
    flag = ark_step(state, rhs_exp_growth, NULL, NULL, &stats);
    expect_true(flag == 0, "ark_step succeeds for ERK");
    if (flag != 0)
    {
      ark_free(state);
      return;
    }
    expect_true(stats.stage_evals >= 3, "ERK step reports stage evaluations");
  }

  expect_true(ark_get_state(state, y) == 0, "final ERK state can be read");
  expect_close(ark_get_time(state), tf, 1e-12, "ERK reaches target final time");
  expect_close(y[0], exp(tf), 2e-3, "ERK solution matches exponential growth");
  ark_free(state);
}

static void test_dirk_implicit_midpoint_linear_decay(void)
{
  ARKConfig config;
  ARKState* state;
  ARKSummary summary;
  ARKStepStats stats;
  double y0[1] = {1.0};
  double y[1] = {0.0};
  const double tf = 0.3;

  config.dimension = 1;
  config.max_steps = 500;
  config.max_newton_iters = 8;
  config.rtol = 1e-6;
  config.atol = 1e-9;
  config.h_init = 1e-3;
  config.h_min = 1e-8;
  config.h_max = 5e-2;
  config.safety = 0.9;
  config.min_factor = 0.2;
  config.max_factor = 4.0;
  config.newton_tol = 0.05;
  config.method = ARK_METHOD_DIRK_IMPLICIT_MIDPOINT;

  state = ark_create(&config, 0.0, y0);
  expect_true(state != NULL, "ark_create succeeds for DIRK test");
  if (state == NULL) { return; }

  while (ark_get_time(state) < tf - 1e-14)
  {
    int flag;
    double remaining = tf - ark_get_time(state);
    ark_get_summary(state, &summary);
    expect_true(ark_set_step_size(
                  state, remaining < summary.current_h ? remaining : summary.current_h) == 0,
                "ark_set_step_size succeeds during DIRK integration");
    flag = ark_step(state, rhs_linear_decay, jac_linear_decay, NULL, &stats);
    expect_true(flag == 0, "ark_step succeeds for DIRK");
    if (flag != 0)
    {
      ark_free(state);
      return;
    }
    expect_true(stats.newton_iters > 0, "DIRK step performs Newton iterations");
  }

  expect_true(ark_get_state(state, y) == 0, "final DIRK state can be read");
  expect_close(ark_get_time(state), tf, 1e-12, "DIRK reaches target final time");
  expect_close(y[0], exp(-2.0 * tf), 1e-2, "DIRK solution matches linear decay");
  expect_true(ark_get_summary(state, &summary) == 0, "DIRK summary is available");
  expect_true(summary.jac_evals > 0, "DIRK summary records Jacobian evaluations");
  ark_free(state);
}

static void test_dirk_fd_jacobian_linear_decay(void)
{
  ARKConfig config;
  ARKState* state;
  ARKSummary summary;
  ARKStepStats stats;
  double y0[1] = {1.0};
  double y[1] = {0.0};
  const double tf = 0.2;

  config.dimension = 1;
  config.max_steps = 500;
  config.max_newton_iters = 8;
  config.rtol = 1e-6;
  config.atol = 1e-9;
  config.h_init = 1e-3;
  config.h_min = 1e-8;
  config.h_max = 5e-2;
  config.safety = 0.9;
  config.min_factor = 0.2;
  config.max_factor = 4.0;
  config.newton_tol = 0.05;
  config.method = ARK_METHOD_DIRK_IMPLICIT_MIDPOINT;

  state = ark_create(&config, 0.0, y0);
  expect_true(state != NULL, "ark_create succeeds for finite-difference DIRK test");
  if (state == NULL) { return; }

  while (ark_get_time(state) < tf - 1e-14)
  {
    int flag;
    double remaining = tf - ark_get_time(state);
    ark_get_summary(state, &summary);
    expect_true(ark_set_step_size(
                  state, remaining < summary.current_h ? remaining : summary.current_h) == 0,
                "ark_set_step_size succeeds during finite-difference DIRK integration");
    flag = ark_step(state, rhs_linear_decay, NULL, NULL, &stats);
    expect_true(flag == 0, "ark_step succeeds for DIRK finite-difference Jacobian");
    if (flag != 0)
    {
      ark_free(state);
      return;
    }
    expect_true(stats.newton_iters > 0, "DIRK finite-difference path performs Newton iterations");
  }

  expect_true(ark_get_state(state, y) == 0, "DIRK finite-difference final state can be read");
  expect_close(y[0], exp(-2.0 * tf), 8e-3,
               "DIRK finite-difference solution matches linear decay");
  expect_true(ark_get_summary(state, &summary) == 0,
              "DIRK finite-difference summary is available");
  expect_true(summary.jac_evals > 0,
              "DIRK finite-difference summary records Jacobian evaluations");
  expect_true(summary.rhs_evals > summary.jac_evals,
              "DIRK finite-difference path performs extra RHS evaluations");
  ark_free(state);
}

static void test_erk_zero_rhs_preserves_state(void)
{
  ARKConfig config;
  ARKState* state;
  ARKSummary summary;
  ARKStepStats stats;
  double y0[1] = {2.5};
  double y[1] = {0.0};
  const double tf = 0.15;

  config.dimension = 1;
  config.max_steps = 100;
  config.max_newton_iters = 8;
  config.rtol = 1e-7;
  config.atol = 1e-10;
  config.h_init = 1e-3;
  config.h_min = 1e-8;
  config.h_max = 5e-2;
  config.safety = 0.9;
  config.min_factor = 0.2;
  config.max_factor = 4.0;
  config.newton_tol = 0.05;
  config.method = ARK_METHOD_ERK_RK4;

  state = ark_create(&config, 0.0, y0);
  expect_true(state != NULL, "ark_create succeeds for zero-RHS ERK test");
  if (state == NULL) { return; }

  while (ark_get_time(state) < tf - 1e-14)
  {
    int flag;
    double remaining = tf - ark_get_time(state);
    ark_get_summary(state, &summary);
    expect_true(ark_set_step_size(
                  state, remaining < summary.current_h ? remaining : summary.current_h) == 0,
                "ark_set_step_size succeeds during zero-RHS ERK integration");
    flag = ark_step(state, rhs_zero, NULL, NULL, &stats);
    expect_true(flag == 0, "ark_step succeeds for zero-RHS ERK");
    if (flag != 0)
    {
      ark_free(state);
      return;
    }
  }

  expect_true(ark_get_state(state, y) == 0, "zero-RHS ERK final state can be read");
  expect_close(ark_get_time(state), tf, 1e-12, "zero-RHS ERK reaches target final time");
  expect_close(y[0], y0[0], 1e-12, "zero-RHS ERK preserves the state");
  ark_free(state);
}

static void test_additional_explicit_methods_on_exp_growth(void)
{
  ARKMethodID methods[3] = {
    ARK_METHOD_ERK_FORWARD_EULER,
    ARK_METHOD_ERK_HEUN_EULER,
    ARK_METHOD_ERK_BOGACKI_SHAMPINE
  };
  const double tolerances[3] = {3e-2, 5e-3, 5e-4};
  const char* labels[3] = {
    "forward Euler",
    "Heun-Euler",
    "Bogacki-Shampine"
  };
  int method_index;

  for (method_index = 0; method_index < 3; ++method_index)
  {
    ARKConfig config;
    ARKState* state;
    ARKSummary summary;
    ARKStepStats stats;
    double y0[1] = {1.0};
    double y[1] = {0.0};
    const double tf = 0.2;

    config.dimension = 1;
    config.max_steps = 400;
    config.max_newton_iters = 8;
    config.rtol = 1e-7;
    config.atol = 1e-10;
    config.h_init = 1e-3;
    config.h_min = 1e-8;
    config.h_max = 5e-2;
    config.safety = 0.9;
    config.min_factor = 0.2;
    config.max_factor = 4.0;
    config.newton_tol = 0.05;
    config.method = methods[method_index];

    state = ark_create(&config, 0.0, y0);
    expect_true(state != NULL, "ark_create succeeds for additional explicit method test");
    if (state == NULL) { return; }

    while (ark_get_time(state) < tf - 1e-14)
    {
      int flag;
      double remaining = tf - ark_get_time(state);
      ark_get_summary(state, &summary);
      expect_true(ark_set_step_size(
                    state, remaining < summary.current_h ? remaining : summary.current_h) == 0,
                  "ark_set_step_size succeeds during additional explicit method test");
      flag = ark_step(state, rhs_exp_growth, NULL, NULL, &stats);
      expect_true(flag == 0, labels[method_index]);
      if (flag != 0)
      {
        ark_free(state);
        return;
      }
    }

    expect_true(ark_get_state(state, y) == 0, "explicit-method final state can be read");
    expect_close(y[0], exp(tf), tolerances[method_index],
                 "additional explicit method matches exponential growth");
    ark_free(state);
  }
}

static void test_backward_euler_linear_decay(void)
{
  ARKConfig config;
  ARKState* state;
  ARKSummary summary;
  ARKStepStats stats;
  double y0[1] = {1.0};
  double y[1] = {0.0};
  const double tf = 0.2;

  config.dimension = 1;
  config.max_steps = 500;
  config.max_newton_iters = 8;
  config.rtol = 1e-6;
  config.atol = 1e-9;
  config.h_init = 1e-3;
  config.h_min = 1e-8;
  config.h_max = 5e-2;
  config.safety = 0.9;
  config.min_factor = 0.2;
  config.max_factor = 4.0;
  config.newton_tol = 0.05;
  config.method = ARK_METHOD_DIRK_BACKWARD_EULER;

  state = ark_create(&config, 0.0, y0);
  expect_true(state != NULL, "ark_create succeeds for backward Euler test");
  if (state == NULL) { return; }

  while (ark_get_time(state) < tf - 1e-14)
  {
    int flag;
    double remaining = tf - ark_get_time(state);
    ark_get_summary(state, &summary);
    expect_true(ark_set_step_size(
                  state, remaining < summary.current_h ? remaining : summary.current_h) == 0,
                "ark_set_step_size succeeds during backward Euler integration");
    flag = ark_step(state, rhs_linear_decay, jac_linear_decay, NULL, &stats);
    expect_true(flag == 0, "ark_step succeeds for backward Euler");
    if (flag != 0)
    {
      ark_free(state);
      return;
    }
    expect_true(stats.newton_iters > 0, "backward Euler performs Newton iterations");
  }

  expect_true(ark_get_state(state, y) == 0, "backward Euler final state can be read");
  expect_close(y[0], exp(-2.0 * tf), 1.2e-2, "backward Euler matches linear decay");
  ark_free(state);
}

int main(void)
{
  test_erk_rk4_exp_growth();
  test_dirk_implicit_midpoint_linear_decay();
  test_dirk_fd_jacobian_linear_decay();
  test_erk_zero_rhs_preserves_state();
  test_additional_explicit_methods_on_exp_growth();
  test_backward_euler_linear_decay();

  if (failures != 0)
  {
    fprintf(stderr, "ARKODE C unit tests failed: %d\n", failures);
    return 1;
  }

  printf("ARKODE C unit tests passed.\n");
  return 0;
}
