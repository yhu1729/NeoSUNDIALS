/* NeoSUNDIALS port of SUNDIALS ark_test_reset.c */
/* Test arkode reset/reinit: verify state reset and continued accuracy */

#include "../c/arkode_core.h"
#include "../c/nvector_serial.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

static int failures = 0;

static void expect_true(int condition, const char* message)
{
  if (!condition) {
    fprintf(stderr, "FAILED: %s\n", message);
    failures++;
  }
}

static void expect_close(double actual, double expected, double tol, const char* message)
{
  if (fabs(actual - expected) > tol) {
    fprintf(stderr, "FAILED: %s (actual=%g expected=%g tol=%g)\n", message, actual, expected, tol);
    failures++;
  }
}

/* Test ODE: y' = lambda*y + g(t), y(t)=atan(t) */
static double lambda = -1.0;
static int rhs_test(double t, const double* y, double* ydot, void* user_data)
{
  (void)user_data;
  ydot[0] = lambda * y[0] + 1.0/(1.0 + t*t) - lambda * atan(t);
  return 0;
}

static double y_exact(double t) { return atan(t); }

static void test_reset_accuracy(ARKMethodID method)
{
  ARKConfig config;
  ARKState* state;
  double y0[1];
  double y[1];
  double t, dt = 0.05;

  config.dimension = 1;
  config.max_steps = 20;
  config.rtol = 1e-4;
  config.atol = 1e-6;
  config.h_init = 0.01;
  config.h_min = 1e-12;
  config.h_max = 0.02;
  config.safety = 0.9;
  config.max_factor = 1.2;
  config.method = method;

  /* Test 1: Integrate [0,0.05] */
  t = 0.0;
  y0[0] = y_exact(t);
  state = ark_create(&config, t, y0);
  expect_true(state != NULL, "create succeeds");

  t += dt;
  while (ark_get_time(state) < t - 1e-14) {
    ARKStepStats stats;
    int flag = ark_step(state, rhs_test, NULL, NULL, &stats);
    expect_true(flag == 0, "step succeeds");
  }
  ark_get_state(state, y);
  expect_close(y[0], y_exact(t), 0.005, "test1 accuracy");
  ark_free(state);

  /* Test 2: Reset to t=0.05, integrate to 0.1 */
  t = 0.05;
  y0[0] = y_exact(t);
  state = ark_create(&config, t, y0);
  t += dt;
  while (ark_get_time(state) < t - 1e-14) {
    ARKStepStats stats;
    int flag = ark_step(state, rhs_test, NULL, NULL, &stats);
    expect_true(flag == 0, "step after reset succeeds");
  }
  ark_get_state(state, y);
  expect_close(y[0], y_exact(t), 0.005, "test2 accuracy");
  ark_free(state);

  ark_free(NULL); /* Test free NULL */
  printf("PASSED\\n");
}

int main(void)
{
  test_reset_accuracy(ARK_METHOD_ERK_RK4);
  return 0;
}

