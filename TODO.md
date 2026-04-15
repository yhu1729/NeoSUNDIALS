# NeoSUNDIALS TODO - Implementation Tracking

## Legend
- [ ] TODO
- [x] Done

## High Priority (Next Release Blocker)

- [x] **Integrate NVector_serial into SBDF/ARK cores** (`c/arkode_core.c`, `c/sbdf_core.c`)
- [ ] **Test suite expansion**
  - [ ] C: Add convergence/order tests (`tests/`).
  - [ ] Python: Parametrize `run_python_tests.py` (tol, orders, tables).
  - [ ] Benchmarks vs upstream (`sundials/examples/`).

## Current Task: Port ARKode Unit Tests
1. [x] Review/extend `tests/test_sbdf_core.c` (basic coverage exists: create/getters, zero RHS, linear decay)
2. [x] Port `sundials/test/unit_tests/arkode/C_serial/ark_test_adapt.c` → `tests/test_arkode_adapt.c`
3. [x] Port `sundials/test/unit_tests/arkode/C_serial/ark_test_interp.c` → `tests/test_arkode_interp.c`
4. [x] Port `sundials/test/unit_tests/arkode/C_serial/ark_test_tstop.c` → `tests/test_arkode_tstop.c`
5. [x] Port `ark_test_reset.c` → `tests/test_arkode_reset.c`
6. [x] Update `Makefile`: Add new test targets
7. [ ] Run all C tests: `make c-test-all`
8. [ ] Update this TODO.md + CHANGELOG.md

## Next (v0.3.0)

- [ ] **Python distribution**
  - `pyproject.toml` + `pip install -e .` support.
  - Bundle native libs in wheel.

- [ ] **Docs**
  - Sphinx for Python API.
  - Doxygen for C headers.
  - Expand `EXTRACTION_NOTES.md`.

## Future

- [ ] Linear solvers: `sunlinsol_dense` extraction.
- [ ] More Butcher tables (ARKODE explicit/implicit).
- [ ] NVectors: parallel/GPU stubs.
- [ ] Sensitivity stubs (forward/backward).

## Completed
- [x] Extract SBDF core (BDF1/2) `CHANGELOG.md [0.1.0]`
- [x] Extract ARKODE core (ERK/SDIRK) `CHANGELOG.md [0.1.0]`
- [x] Python workflows + problems (decay, Brusselator, VDP)
- [x] NVector_serial impl + tests `[0.2.0]`
- [x] Improve project docs/todos (this file, README, CHANGELOG)

**Tracking**: Update checkboxes after `git commit`. Always `make test` before marking done.
**Status**: Root docs/todos/notes improved (README, CHANGELOG, EXTRACTION_NOTES, TODO.md). sundials/ untouched.
