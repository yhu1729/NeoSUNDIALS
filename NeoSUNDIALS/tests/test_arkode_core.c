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
  config.max_steps = 200;
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
    double remaining = tf - ark_get_time(state);
    ark_get_summary(state, &summary);
    expect_true(ark_set_step_size(
                  state, remaining < summary.current_h ? remaining : summary.current_h) == 0,
                "ark_set_step_size succeeds during ERK integration");
    expect_true(ark_step(state, rhs_exp_growth, NULL, NULL, &stats) == 0,
                "ark_step succeeds for ERK");
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
  config.max_steps = 200;
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
    double remaining = tf - ark_get_time(state);
    ark_get_summary(state, &summary);
    expect_true(ark_set_step_size(
                  state, remaining < summary.current_h ? remaining : summary.current_h) == 0,
                "ark_set_step_size succeeds during DIRK integration");
    expect_true(ark_step(state, rhs_linear_decay, jac_linear_decay, NULL, &stats) == 0,
                "ark_step succeeds for DIRK");
    expect_true(stats.newton_iters > 0, "DIRK step performs Newton iterations");
  }

  expect_true(ark_get_state(state, y) == 0, "final DIRK state can be read");
  expect_close(ark_get_time(state), tf, 1e-12, "DIRK reaches target final time");
  expect_close(y[0], exp(-2.0 * tf), 1e-2, "DIRK solution matches linear decay");
  expect_true(ark_get_summary(state, &summary) == 0, "DIRK summary is available");
  expect_true(summary.jac_evals > 0, "DIRK summary records Jacobian evaluations");
  ark_free(state);
}

int main(void)
{
  test_erk_rk4_exp_growth();
  test_dirk_implicit_midpoint_linear_decay();

  if (failures != 0)
  {
    fprintf(stderr, "ARKODE C unit tests failed: %d\n", failures);
    return 1;
  }

  printf("ARKODE C unit tests passed.\n");
  return 0;
}
