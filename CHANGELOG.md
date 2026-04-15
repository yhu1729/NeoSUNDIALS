# NeoSUNDIALS CHANGELOG

## [0.1.0] - Initial Extraction (2024)
- Extracted SBDF core (BDF1/2 implicit multistep from CVODE/IDA).
- Extracted ARKODE core (explicit/implicit RK with Butcher tables).
- Python ctypes bindings, workflows, test problems (linear decay, Brusselator, vdp).
- C/Python unit tests, verification vs analytic solutions.
- `make test` for full suite.

## Unreleased
- [0.3.0] Test Suite Overhaul
  - Unified `make check`: Builds libs/tests, runs full Python-verified suite (C integration + Python unit/verification).
  - `tests/test_c_integration.py`: Launches C exes, verifies PASS/0 exit from Python.
  - `tests/test_verification.py`: SBDF/ARK solve asserts.
  - Removed Python launcher, redundant targets (c-test/python-test/test).
  - Fixed C tests (adapt h_max, interp tolerances, warnings).
  - All tests launched/verified in Python unittest discover.

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

