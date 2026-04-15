# NeoSUNDIALS Porting TODO

## Current Task: Implement NVector Serial Layer (step-by-step)

### Phase 1: NVector Core
1. [x] Create `c/nvector_serial.h` and `c/nvector_serial.c` with minimal self-contained N_Vector ops:
   - N_VNew_Serial, N_VClone_Serial, N_VDestroy_Serial
   - N_VScale_Serial, N_VAxpy_Serial, N_VLinearSum_Serial
   - N_VWrmsNorm_Serial (uniform weight RMS)
   - N_VMin_Serial, N_VMax_Serial, N_VAbs_Serial
   - N_VGetArrayPointer_Serial, N_VSetArrayPointer_Serial
   - sunindextype length; double *data; direct struct access.

2. [x] Create `tests/test_nvector_serial.c` with unit tests (create/clone/scale/axpy/wnorm/minmax).

3. [x] Edit `Makefile`: Added libnvector_serial, test_nvector_serial to libs/tests/c-test.

### Phase 2: Integrate to SBDF/ARK Cores
4. [ ] Edit `c/sbdf_core.h/c`: Replace double* y/f/ac with N_Vector; use N_V* ops (e.g., ark_copy -> N_VScale(1.0, src, dst)).

5. [ ] Edit `c/arkode_core.h/c`: Same for ARK stage_rhs/y_trial etc.

6. [ ] Edit C tests: `tests/test_sbdf_core.c`, `test_arkode_core.c`.

### Phase 3: Python Bindings & Workflow
7. [ ] Edit `python/NeoSUNDIALS/native.py`, `ark_native.py`: ctypes N_VNew etc.

8. [ ] Edit workflows for NVectors.

### Phase 4: Verify & Docs
9. [ ] `make test`, Python tests.

10. [ ] Docs/CHANGELOG.

## Backlog
- BDF order 5.
- Roots.
- Sparse.

### Phase 2: Integrate to SBDF/ARK Cores
4. [ ] Edit `c/sbdf_core.h/c`: Replace double* y/f with N_Vector; use N_V ops in norms/solves/axpy etc.

5. [ ] Edit `c/arkode_core.h/c`: Same for ARK (y/stage_rhs etc.).

6. [ ] Edit C tests: `tests/test_sbdf_core.c`, `test_arkode_core.c`.

### Phase 3: Python Bindings & Workflow
7. [ ] Edit `python/NeoSUNDIALS/native.py`, `ark_native.py`: ctypes for N_VCreate etc., user-data callbacks take NVectors.

8. [ ] Edit `python/NeoSUNDIALS/workflow.py`, `ark_workflow.py`: Solver takes NVectors (alloc y0 as N_VNew).

9. [ ] Add Python NVector tests to `tests/test_numerics.py` etc.

### Phase 4: Verify & Docs
10. [ ] `make clean && make test && python run_python_tests.py`

11. [ ] Create/update `CHANGELOG.md`, `README.md`, `EXTRACTION_NOTES.md`.

## Backlog
- Expand SBDF to full CVODE BDF order 5.
- Add rootfinding.
- Sparse linear solvers.
- Python NVector wrapper class.

