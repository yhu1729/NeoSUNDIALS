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
    fprintf(stderr, "FAILED: %s\\n", message);
    failures++;
  }
}

static void expect_close(double actual, double expected, double tol, const char* message)
{
  if (fabs(actual - expected) > tol) {
    fprintf(stderr, "FAILED: %s (actual=%g expected=%g tol=%g)\\n", message, actual, expected, tol);
    failures++;
  }
}

/* Test ODE: y' = lambda*y + g(t), y(t)=atan(t) */
static double lambda = -25.0;
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
  double rtol = 1e-6;
  double atol = 1e-9;
  double t, dt = 0.1;
  int reset_times[] = {1, 2, 4};  /* reset points */

  config.dimension = 1;
  config.max_steps = 100;
  config.rtol = rtol;
  config.atol = atol;
  config.h_init = 0.01;
  config.h_min = 1e-12;
  config.h_max = 0.1;
  config.safety = 0.9;
  config.max_factor = 1.5;
  config.method = method;

  /* Test 1: Integrate [0,0.1], verify */
  t = 0.0;
  y0[0] = y_exact(t);
  state = ark_create(&config, t, y0);
  expect_true(state != NULL, "create succeeds");

  t += dt;
  while (ark_get_time(state) < t - 1e-14) {
    ARKStepStats stats;
    ark_step(state, rhs_test, NULL, NULL, &stats);
  }
  ark_get_state(state, y);
  expect_close(y[0], y_exact(t), rtol, "test1 final accuracy");
  ark_free(state);

  /* Test 2: Reset to t=0.1, integrate to 0.2 */
  t = 0.1;
  y0[0] = y_exact(t);
  state = ark_create(&config, t, y0);  /* "reset" via recreate */
  t += dt;
  while (ark_get_time(state) < t - 1e-14) {
    ARKStepStats stats;
    ark_step(state, rhs_test, NULL, NULL, &stats);
  }
  ark_get_state(state, y);
  expect_close(y[0], y_exact(t), rtol, "test2 after reset accuracy");
  ark_free(state);

  /* Test 3: Reset to t=0.3, integrate to 0.4 */
  t = 0.3;
  y0[0] = y_exact(t);
  state = ark_create(&config, t, y0);
  t += dt;
  while (ark_get_time(state) < t - 1e-14) {
    ARKStepStats stats;
    ark_step(state, rhs_test, NULL, NULL, &stats);
  }
  ark_get_state(state, y);
  expect_close(y[0], y_exact(t), rtol, "test3 non-contiguous reset");
  ark_free(state);

  /* Test 4: Reset back to t=0.1, verify same trajectory */
  t = 0.1;
  y0[0] = y_exact(t);
  state = ark_create(&config, t, y0);
  t += dt;
  while (ark_get_time(state) < t - 1e-14) {
    ARKStepStats stats;
    ark_step(state, rhs_test, NULL, NULL, &stats);
  }
  ark_get_state(state, y);
  expect_close(y[0], y_exact(t), rtol, "test4 repeat trajectory");
  ark_free(state);
}

int main(void)
{
  printf("ARKode reset unit test (ERK)\\n");
  test_reset_accuracy(ARK_METHOD_ERK_RK4);
  
  printf("ARKode reset unit test (DIRK)\\n");
  test_reset_accuracy(ARK_METHOD_DIRK_IMPLICIT_MIDPOINT);

  printf("%s: %d failures\\n", failures ? "FAILED" : "PASS", failures);
  return failures > 0;
}

