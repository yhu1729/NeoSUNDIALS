/* NeoSUNDIALS port of SUNDIALS ark_test_tstop.c */
/* Test ARKode stop time handling: verify exact tf hit without overshoot */

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

static int rhs_linear(double t, const double* y, double* ydot, void* user_data)
{
  (void)t; (void)y; (void)user_data;
  ydot[0] = 1.0;  /* y' = 1 → y = t + c */
  return 0;
}

static void test_tstop_basic(void)
{
  ARKConfig config;
  ARKState* state;
  double y0[1] = {0.0};  /* y(0) = 0 */
  double y[1] = {0.0};
  double t0 = 0.0;
  double tstop = 0.3;
  double h_init = 0.1;

  config.dimension = 1;
  config.max_steps = 10;
  config.rtol = 1e-8;
  config.atol = 1e-12;
  config.h_init = h_init;
  config.h_min = 1e-12;
  config.h_max = 0.5;
  config.safety = 0.9;
  config.max_factor = 2.0;
  config.method = ARK_METHOD_ERK_RK4;

  /* Set stop time (core API or simulate via remaining time check) */
  state = ark_create(&config, t0, y0);
  expect_true(state != NULL, "ark_create succeeds");

  /* Integrate to exactly tstop=0.3 (3*h=0.3) */
  double t = t0;
  ARKSummary summary;
  while (t < tstop - 1e-14) {
    double remaining = tstop - t;
    ark_get_summary(state, &summary);
    if (remaining < summary.current_h) {
      ark_set_step_size(state, remaining);
    }
    ARKStepStats stats;
    int flag = ark_step(state, rhs_linear, NULL, NULL, &stats);
    expect_true(flag == 0, "ark_step succeeds");
    t = ark_get_time(state);
  }

  expect_close(t, tstop, 1e-12, "hits tstop exactly (no overshoot)");
  ark_get_state(state, y);
  expect_close(y[0], tstop, 1e-10, "y(tstop) = tstop exact");

  ark_free(state);
}

int main(void)
{
  printf("ARKode tstop unit test\n");
  test_tstop_basic();
  
  printf("%s: %d failures\n", failures ? "FAILED" : "PASSED", failures);
  return failures > 0;
}

