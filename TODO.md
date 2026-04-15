# NeoSUNDIALS Porting TODO

## Current Task: Port Tests + NVECTOR Integration

### Test Porting TODO (New)
1. [x] Create `test_nvector.h` stub
2. [x] Create `tests/test_nvector_serial_full.c`
3. [ ] Port ARKODE interp test (needs SUNMatrix/LS stubs)
4. [ ] NVECTOR integration to cores + update C tests
5. [ ] Makefile/docs updates
6. [ ] Verify tests

### Legacy Phase 2: NVECTOR Integration (Merged)
1. [ ] Edit `c/sbdf_core.h/c`
2. [ ] Edit `c/arkode_core.h/c`
3. [ ] Edit C tests

## Backlog
- BDF order 5
- Roots
- Sparse

