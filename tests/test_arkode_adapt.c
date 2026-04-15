/* NeoSUNDIALS port of SUNDIALS ark_test_adapt.c */
/* Simplified for core ARKode: Test adaptive step growth with zero RHS y'=0 */

#include "../c/arkode_core.h"
#include "../c/nvector_serial.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
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

static int rhs_zero(double t, const double* y, double* ydot, void* user_data)
{
  (void)t; (void)y; (void)user_data;
  ydot[0] = 0.0;
  return 0;
}

/* Take steps with y'=0, verify step growth to h_max */
static void test_step_growth(ARKMethodID method)
{
  ARKConfig config;
  ARKState* state;
  ARKSummary summary;
  double y0[1] = {1.0};
  double y[1] = {0.0};
  (void)y; /* unused in this test */
  const double h_init = 1e-3;
  const double h_max = 0.1;
  const double growth = 1.5;  /* expected growth factor */

  config.dimension = 1;
  config.max_steps = 20;
  config.max_newton_iters = 4;
  config.rtol = 1e-8;
  config.atol = 1e-12;
  config.h_init = h_init;
  config.h_min = 1e-10;
  config.h_max = h_max;
  config.safety = 0.9;
  config.max_factor = growth;
  config.method = method;

  state = ark_create(&config, 0.0, y0);
  expect_true(state != NULL, "ark_create succeeds");

  double t = 0.0;
  double h_prev = h_init;
  int step = 0;
  while (t < 0.3 && step < 15) {
    ARKStepStats stats;
    int flag = ark_step(state, rhs_zero, NULL, NULL, &stats);
    expect_true(flag == 0, "ark_step succeeds");
    
    t = ark_get_time(state);
    ark_get_summary(state, &summary);
    
    double h_curr = summary.current_h;
    printf("Step %d: t=%.6g h=%.6g (prev=%.6g)\n", step, t, h_curr, h_prev);
    
    /* Check growth approaching h_max, skip if near/at h_max */
    if (step > 2) {
      double expected_growth = fmin(growth, h_max / h_prev);
      expect_close(h_curr / h_prev, expected_growth, 0.25, "step growth factor");
    }
    h_prev = h_curr;
    step++;
  }
  
  expect_true(summary.current_h <= h_max * (1 + 1e-6), "h respects h_max");
  ark_free(state);
}

int main(void)
{
  printf("ARKode adapt unit test (ERK RK4)\n");
  test_step_growth(ARK_METHOD_ERK_RK4);
  
  printf("ARKode adapt unit test (DIRK Implicit Midpoint)\n");
  test_step_growth(ARK_METHOD_DIRK_IMPLICIT_MIDPOINT);
  
  printf("%s: %d failures\n", failures ? "FAILED" : "PASSED", failures);
  return failures > 0;
}

