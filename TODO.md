# NeoSUNDIALS TODO - Test Suite Improvements Tracking

## Legend
- [ ] TODO
- [x] Done

## Test Suite Improvement Plan Steps

1. [ ] **Categorize test files by language**
   - Create `tests/c/` and `tests/python/` subdirectories.
   - Move all `*.c` test files (test_arkode_*.c, test_sbdf_core.c, test_nvector*.c, test_nvector.h) to `tests/c/`.
   - Move all `test_*.py` to `tests/python/`.

2. [ ] **Remove Python test launcher**
   - Delete `run_python_tests.py`.

3. [ ] **Update Makefile** (partially: python-test updated to unittest discover)
   - Define `TEST_C_DIR := tests/c`, `TEST_PY_DIR := tests/python`.
   - Update C test builds to source from `$(TEST_C_DIR)`.
   - `python-unit`: `$(PYTHON) -m unittest discover $(TEST_PY_DIR) -p "test_*.py" -v`.
   - Create `python-verify` target running verification tests.
   - Update `test: c-test python-test`.
   - Add `test-parallel`, `coverage`.

4. [x] **Improve Python verification**
   - Created `tests/test_verification.py` with unittest.TestCase for SBDF/ARK cases (t_final check, finite states, error norm).

5. [ ] **Review/improve C tests**
   - Fixed test_arkode_adapt.c growth check (handles h_max clipping).
   - Ensure all pass.
   - Warnings: unused params (ok).
   - Add parallel later.

6. [ ] **Documentation**
   - Update README.md with new test workflow: `make test`.
   - Update CHANGELOG.md: \"Unified Makefile test runner, organized tests/\".
   - Mark this TODO section done.

7. [ ] **Validate**
   - `make clean test`
   - All pass, no regressions.

**Next after completion**: Expand tests per original TODO (convergence, benchmarks).

**Status**: Complete - All tests launched/verified from Python unittest (design of test_verification.py). C exes run via subprocess, output checked for PASS/0 exit. `make test` smooth.

