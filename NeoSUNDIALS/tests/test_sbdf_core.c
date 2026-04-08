#include "../c/sbdf_core.h"

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

static int rhs_zero(double t, const double* y, double* dydt, void* user_data)
{
  (void)t;
  (void)y;
  (void)user_data;
  dydt[0] = 0.0;
  return 0;
}

static int jac_zero(double t, const double* y, double* jacobian, void* user_data)
{
  (void)t;
  (void)y;
  (void)user_data;
  jacobian[0] = 0.0;
  return 0;
}

static void test_create_getters_and_setter(void)
{
  SBDFConfig config;
  SBDFState* state;
  SBDFSummary summary;
  double y0[1] = {1.0};
  double y[1] = {0.0};

  config.dimension = 1;
  config.max_order = 2;
  config.max_steps = 50;
  config.max_newton_iters = 8;
  config.rtol = 1e-6;
  config.atol = 1e-9;
  config.h_init = 1e-2;
  config.h_min = 1e-8;
  config.h_max = 0.2;
  config.safety = 0.9;
  config.min_factor = 0.2;
  config.max_factor = 5.0;
  config.newton_tol = 0.1;

  state = sbdf_create(&config, 0.0, y0);
  expect_true(state != NULL, "sbdf_create returns a state");
  expect_true(sbdf_get_dimension(state) == 1, "sbdf_get_dimension reports correct size");
  expect_close(sbdf_get_time(state), 0.0, 1e-14, "initial time is preserved");
  expect_true(sbdf_set_step_size(state, 5e-2) == 0, "sbdf_set_step_size succeeds");
  expect_true(sbdf_get_state(state, y) == 0, "sbdf_get_state succeeds");
  expect_close(y[0], 1.0, 1e-14, "initial state is preserved");
  expect_true(sbdf_get_summary(state, &summary) == 0, "sbdf_get_summary succeeds");
  expect_true(summary.steps == 0, "summary starts at zero steps");
  sbdf_free(state);
}

static void test_integrate_zero_rhs(void)
{
  SBDFConfig config;
  SBDFState* state;
  SBDFSummary summary;
  SBDFStepStats stats;
  double y0[1] = {1.0};
  double y[1] = {0.0};
  const double tf = 0.25;

  config.dimension = 1;
  config.max_order = 2;
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

  state = sbdf_create(&config, 0.0, y0);
  expect_true(state != NULL, "sbdf_create succeeds for integration test");
  if (state == NULL) { return; }

  while (sbdf_get_time(state) < tf - 1e-14)
  {
    double remaining = tf - sbdf_get_time(state);
    sbdf_get_summary(state, &summary);
    expect_true(sbdf_set_step_size(
                  state, remaining < summary.current_h ? remaining : summary.current_h) == 0,
                "sbdf_set_step_size succeeds during integration");
    expect_true(sbdf_step(state, rhs_zero, jac_zero, NULL, &stats) == 0,
                "sbdf_step succeeds");
    expect_true(stats.accepted == 1, "step is accepted");
    expect_true(stats.error_norm <= 1.0 + 1e-12, "accepted step has bounded error estimate");
  }

  expect_true(sbdf_get_state(state, y) == 0, "final state can be read");
  expect_close(sbdf_get_time(state), tf, 1e-12, "final time hits target");
  expect_close(y[0], 1.0, 1e-12, "zero RHS preserves the state");

  expect_true(sbdf_get_summary(state, &summary) == 0, "final summary is available");
  expect_true(summary.accepted_steps > 0, "summary records accepted steps");
  expect_true(summary.rhs_evals > 0, "summary records RHS evaluations");
  expect_true(summary.jac_evals > 0, "summary records Jacobian evaluations");
  sbdf_free(state);
}

int main(void)
{
  test_create_getters_and_setter();
  test_integrate_zero_rhs();

  if (failures != 0)
  {
    fprintf(stderr, "SBDF C unit tests failed: %d\n", failures);
    return 1;
  }

  printf("SBDF C unit tests passed.\n");
  return 0;
}
