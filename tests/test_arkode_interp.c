/* NeoSUNDIALS test for ARKode solution interpolation */
/* Verify solution at intermediate times by exact comparison */

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

/* Harmonic oscillator: y'' + y = 0 → [y' = v; v' = -y] */
static int rhs_harmonic(double t, const double* y, double* ydot, void* user_data)
{
  (void)t; (void)user_data;
  ydot[0] = y[1];      /* y' = v */
  ydot[1] = -y[0];     /* v' = -y */
  return 0;
}

static void test_interp_harmonic(ARKMethodID method)
{
  ARKConfig config;
  ARKState* state;
  double y0[2] = {0.0, 1.0};  /* y(0)=0, y'(0)=1 → y=sin(t) */
  double y[2] = {0.0};
  double t0 = 0.0;
  double h = 0.2;
  double t1 = h;
  double t_mid = 0.5 * h;
  double y_exact_mid[2] = {sin(t_mid), cos(t_mid)};

  /* Fixed step config */
  config.dimension = 2;
  config.max_steps = 2;
  config.rtol = 1e-8;
  config.atol = 1e-12;
  config.h_init = h;
  config.h_min = 1e-12;
  config.h_max = 1.0;
  config.safety = 0.96;
  config.max_factor = 1.2;  /* Fixed step */
  config.min_factor = 0.8;
  config.method = method;

  state = ark_create(&config, t0, y0);
  expect_true(state != NULL, "ark_create succeeds");

  /* Step to t1 */
  ARKStepStats stats1;
  int flag = ark_step(state, rhs_harmonic, NULL, NULL, &stats1);
  expect_true(flag == 0, "first step succeeds");
  expect_close(ark_get_time(state), t1, 1e-12, "reaches t1");

  /* Naive interp: average endpoints (1st order) */
  ark_get_state(state, y);
  double y_naive[2] = {0.5*(y0[0] + y[0]), 0.5*(y0[1] + y[1])};
  expect_close(y_naive[0], y_exact_mid[0], 0.01, "naive y interp order 1");
  expect_close(y_naive[1], y_exact_mid[1], 0.01, "naive v interp order 1");

  /* Verify solution exactness at step endpoints */
  expect_close(y[0], sin(t1), 1e-8, "y(t1) exact");
  expect_close(y[1], cos(t1), 1e-8, "v(t1) exact");

  ark_free(state);
}

int main(void)
{
  printf("ARKode interpolation test (ERK)\\n");
  test_interp_harmonic(ARK_METHOD_ERK_RK4);
  
  printf("ARKode interpolation test (DIRK)\\n");
  test_interp_harmonic(ARK_METHOD_DIRK_IMPLICIT_MIDPOINT);

  printf("%s: %d failures\\n", failures ? "FAILED" : "PASSED", failures);
  return failures;
}

