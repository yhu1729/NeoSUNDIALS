# NeoSUNDIALS CHANGELOG

## Unreleased
- Clarified the project direction as a compact vertical-slice extraction, not a
  SUNDIALS compatibility layer.
- Expanded README architecture, status, usage, and DAE caveat sections.
- Updated extraction notes to document the native DAE residual path, fallback
  behavior, callback validation guarantees, and current test matrix.
- Expanded the routine-porting checklist with scope, stabilization-policy, and
  high-value follow-up guidance.

## [0.4.0] - DAE Workflow Vertical Slice
- Added experimental residual-form DAE workflow in
  `python/NeoSUNDIALS/workflow.py`: `DAEProblem`, `solve_dae_problem`, and
  `solve_dae_problem_uniform`.
- Added a residual-to-RHS bridge using a Newton solve for `ydot` from
  `F(t, y, ydot) = 0`.
- Added native residual-step entry point in `c/sbdf_core.[ch]`
  (`sbdf_step_residual`) and Python binding support in
  `python/NeoSUNDIALS/native.py`.
- DAE workflow now attempts native residual stepping first and falls back to the
  Python residual bridge when the native path reports convergence/work-limit
  statuses.
- Added `dae_linear_decay_problem` in `python/NeoSUNDIALS/problems.py`.
- Exported DAE APIs from `python/NeoSUNDIALS/__init__.py`.
- Added workflow tests for DAE nominal solve and residual failure handling.

## [0.3.1] - Phase 1 Hardening
- Fixed weighted NVector operations in `c/nvector_serial.c`
  (`N_VWL2Norm_Serial`, `N_VWrmsNorm_Serial`, `N_VWSqrSumLocal_Serial`) to
  consume the weight vector.
- Expanded `tests/test_nvector_serial.c` with non-uniform weight checks for
  weighted norm behavior.
- Hardened Python native callback bridges in `python/NeoSUNDIALS/native.py` and
  `python/NeoSUNDIALS/ark_native.py` with callback output shape/finiteness
  validation, callback exception propagation through step failures, and
  initial-state dimensionality checks at solver construction.
- Added workflow failure-path tests for malformed RHS/Jacobian callback outputs
  in `tests/test_workflow.py` and `tests/test_ark_workflow.py`.

## [0.3.0] - Test Suite Overhaul
- Unified `make check` to build libraries/tests and run the full
  Python-verified suite, including C integration and Python unit/verification
  tests.
- Added `tests/test_c_integration.py` to launch C executables and verify PASS/0
  exits from Python.
- Added `tests/test_verification.py` SBDF/ARK solve assertions.
- Removed the Python launcher and redundant `c-test`, `python-test`, and `test`
  targets.
- Fixed C tests around adaptation `h_max`, interpolation tolerances, and
  warnings.
- All tests are launched and verified through Python unittest discovery.

## [0.2.0] - NVector Serial Layer
- Added `c/nvector_serial.[ch]` minimal self-contained `N_Vector`.
- Added serial vector operations: Create, Clone, Destroy, LinearSum, Scale,
  Axpy, Dot, WRMSNorm, Min, Max, and Abs.
- Preserved legacy `content` pointer access.
- `tests/test_nvector_serial.c` passing.
- Makefile: `libnvector_serial`, `make c-test` full suite passes.

## [0.1.0] - Initial Extraction
- SBDF core (BDF1/2 implicit multistep).
- ARKODE core (explicit/implicit RK, Butcher tables).
- Python ctypes bindings + problems (decay, Brusselator, vdp).
- C/Python tests, `make test` runner.
