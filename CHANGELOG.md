# NeoSUNDIALS CHANGELOG

## [0.1.0] - Initial Extraction (2024)
- Extracted SBDF core (BDF1/2 implicit multistep from CVODE/IDA).
- Extracted ARKODE core (explicit/implicit RK with Butcher tables).
- Python ctypes bindings, workflows, test problems (linear decay, Brusselator, vdp).
- C/Python unit tests, verification vs analytic solutions.
- `make test` for full suite.

## Unreleased
- [0.4.0] DAE Workflow Vertical Slice
  - Added experimental residual-form DAE workflow in `python/NeoSUNDIALS/workflow.py`:
    - `DAEProblem`, `solve_dae_problem`, `solve_dae_problem_uniform`.
    - residual-to-RHS bridge using a Newton solve for `ydot` from `F(t,y,ydot)=0`.
  - Added native residual-step entry point in `c/sbdf_core.[ch]` (`sbdf_step_residual`) and Python binding support in `python/NeoSUNDIALS/native.py`.
  - DAE workflow now attempts native residual stepping first and falls back to the Python residual bridge when native path reports convergence/work-limit statuses.
  - Added `dae_linear_decay_problem` in `python/NeoSUNDIALS/problems.py`.
  - Exported DAE APIs from `python/NeoSUNDIALS/__init__.py`.
  - Added workflow tests for DAE nominal solve and residual failure handling.
- [0.3.0] Test Suite Overhaul
  - Unified `make check`: Builds libs/tests, runs full Python-verified suite (C integration + Python unit/verification).
  - `tests/test_c_integration.py`: Launches C exes, verifies PASS/0 exit from Python.
  - `tests/test_verification.py`: SBDF/ARK solve asserts.
  - Removed Python launcher, redundant targets (c-test/python-test/test).
  - Fixed C tests (adapt h_max, interp tolerances, warnings).
  - All tests launched/verified in Python unittest discover.
- [0.3.1] Phase 1 Hardening
  - Fixed weighted NVector operations in `c/nvector_serial.c` (`N_VWL2Norm_Serial`, `N_VWrmsNorm_Serial`, `N_VWSqrSumLocal_Serial`) to consume the weight vector.
  - Expanded `tests/test_nvector_serial.c` with non-uniform weight checks for weighted norm behavior.
  - Hardened Python native callback bridges (`python/NeoSUNDIALS/native.py`, `python/NeoSUNDIALS/ark_native.py`) with:
    - callback output shape/finiteness validation,
    - callback exception capture and propagation through step failures,
    - initial-state dimensionality checks at solver construction.
  - Added workflow failure-path tests for malformed RHS/Jacobian callback outputs in:
    - `tests/test_workflow.py`
    - `tests/test_ark_workflow.py`

## [0.2.0] - 2024 - NVector Serial Layer
- Added `c/nvector_serial.[ch]` minimal `N_Vector` (self-contained).
  - Ops: Create/Clone/Destroy, LinearSum/Scale/Axpy/Dot/WRMSNorm/Min/Max/Abs.
  - Legacy `content` pointer access.
- `tests/test_nvector_serial.c` passing.
- Makefile: `libnvector_serial`, `make c-test` full suite passes.

## [0.1.0] - 2024 - Initial Extraction
- SBDF core (BDF1/2 implicit multistep).
- ARKODE core (explicit/implicit RK, Butcher tables).
- Python ctypes bindings + problems (decay, Brusselator, vdp).
- C/Python tests, `make test` runner.

