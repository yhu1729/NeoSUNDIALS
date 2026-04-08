# Extracted SUNDIALS Core

This directory contains a small reimplementation of the core ideas that drive
the SUNDIALS CVODE/IDA stepping loops:

- Numerical algorithms in C
  - weighted RMS norms
  - variable-step BDF coefficient generation
  - polynomial prediction from solution history
  - Newton correction with dense linear solves
  - finite-difference Jacobian fallback
  - adaptive step-size and basic order selection between BDF1 and BDF2
  - ARKODE-style stage-based Runge-Kutta stepping
  - built-in explicit and diagonally implicit Butcher tables
  - step-doubling error estimation and adaptive step control
- Workflow algorithms in Python
  - problem specification
  - native library build/load
  - run loop over user output times
  - step diagnostics and summary collection
  - example problem catalog
  - Hermite dense output for ARK-style sampling

The extraction is intentionally small rather than feature-complete. It focuses
on the algorithmic spine visible in:

- `sundials/src/cvode/cvode.c`
- `sundials/src/ida/ida.c`
- `sundials/doc/cvode/guide/source/Mathematics.rst`
- `sundials/doc/ida/guide/source/Mathematics.rst`
- `sundials/examples/python/*`

## Layout

- `c/sbdf_core.h`
- `c/sbdf_core.c`
- `c/arkode_core.h`
- `c/arkode_core.c`
- `python/sundials_extracted_core/`
- `tests/`
- `verify.py`
- `verify_arkode.py`
- `run_c_unit_tests.py`

## Running

Use the provided virtual environment:

```bash
./env/bin/python ./sundials_extracted_core/verify.py
./env/bin/python ./sundials_extracted_core/verify_arkode.py
./env/bin/python ./sundials_extracted_core/run_c_unit_tests.py
```

The Python workflow will build the native library on first use with the system
C compiler and then execute two sample problems.
