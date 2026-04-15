# NeoSUNDIALS TODO

## Legend
- [ ] TODO
- [x] Done

## High Priority (Next Release Blocker)

- [ ] **Integrate NVector_serial into SBDF/ARK cores** (`c/arkode_core.c`, `c/sbdf_core.c`)
  - Replace raw `double*` with `N_Vector` APIs.
  - Update Python `ctypes` workflows (`python/NeoSUNDIALS/ark_workflow.py`, `python/NeoSUNDIALS/workflow.py`).
  - Tests: Extend `tests/test_arkode_core.c`, `tests/test_sbdf_core.c`.

## Next (v0.3.0)

- [ ] **Test suite expansion**
  - C: Add convergence/order tests (`tests/`).
  - Python: Parametrize `run_python_tests.py` (tol, orders, tables).
  - Benchmarks vs upstream (`sundials/examples/`).

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
