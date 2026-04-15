# NeoSUNDIALS CHANGELOG

## [0.1.0] - Initial Extraction (2024)
- Extracted SBDF core (BDF1/2 implicit multistep from CVODE/IDA).
- Extracted ARKODE core (explicit/implicit RK with Butcher tables).
- Python ctypes bindings, workflows, test problems (linear decay, Brusselator, vdp).
- C/Python unit tests, verification vs analytic solutions.
- `make test` for full suite.

## [0.2.0] - NVector Serial Layer - Current
- Added `c/nvector_serial.[ch]` minimal N_Vector abstraction (self-contained, no upstream deps).
  - Core ops: N_VNew/Clone/Destroy, LinearSum/Scale/Axpy/Dot/WrmsNorm/Min/Max/Abs.
  - Array pointer bridge for legacy compat.
- `tests/test_nvector_serial.c` unit tests passing.
- Makefile integration (`libnvector_serial`, test target).
- Verified `make c-test` all pass (SBDF/ARK/NVector).

Next: Integrate NVectors into SBDF/ARK cores (replace raw double*).

