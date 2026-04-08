# Extraction Notes

This directory distills the parts of SUNDIALS that most clearly separate into
numerical kernel logic and workflow/orchestration logic.

## Core Numerical Algorithms Extracted From SUNDIALS

The C implementation in `c/sbdf_core.c` mirrors the algorithmic structure found
in the following upstream areas:

- `sundials/src/cvode/cvode.c`
  - `cvPredict`
  - `cvSetBDF`
  - `cvSetTqBDF`
  - `cvNls`
  - `cvDoErrorTest`
  - `cvCompleteStep`
- `sundials/src/ida/ida.c`
  - `IDAStep`
  - `IDASetCoeffs`
  - `IDAPredict`
  - `IDANls`
  - `IDATestError`
  - `IDACompleteStep`
- `sundials/doc/cvode/guide/source/Mathematics.rst`
- `sundials/doc/ida/guide/source/Mathematics.rst`

The extracted numerical kernel keeps these ideas:

- implicit BDF stepping
- variable-step coefficient construction from recent step history
- predictor/corrector stepping
- Newton solves for the nonlinear implicit update
- dense Jacobian-based linear solves
- WRMS tolerance scaling
- local error-based step adaptation

The extraction intentionally reduces scope:

- BDF order support is reduced to BDF1/BDF2 in the actual stepper
- dense linear algebra only
- ODE-style interface instead of the full CVODE/IDA feature surface
- no rootfinding, constraints, sensitivity analysis, projection, or plugin
  solver stacks

## Core Workflow Algorithms Extracted From SUNDIALS

The Python workflow in `python/NeoSUNDIALS/workflow.py` mirrors the
shape used throughout:

- `sundials/examples/python/*`
- `sundials/bindings/sundials4py/test/problems/problem.py`
- the main `CVode` and `IDASolve` driver loops in the C sources

The extracted workflow layer owns:

- problem definition
- native solver construction
- per-step advancement
- interpolation to user output times
- step-history capture
- run summaries and diagnostics

This preserves the same high-level split SUNDIALS uses:

- C owns the numerical inner loop
- higher-level language code owns setup, execution policy, and reporting

## ARKODE Extraction

The second extraction pass adds a compact ARKODE-inspired core around:

- `sundials/src/arkode/arkode_erkstep.c`
- `sundials/src/arkode/arkode_arkstep.c`
- `sundials/src/arkode/arkode_adapt.c`
- `sundials/src/arkode/arkode_butcher.c`
- `sundials/src/arkode/arkode_butcher_erk.c`
- `sundials/doc/arkode/guide/source/Mathematics.rst`
- `sundials/examples/python/arkode/*`

The C implementation in `c/arkode_core.c` keeps these core ideas:

- stage-based Runge-Kutta stepping
- built-in Butcher-table selection
- explicit and diagonally implicit stage handling
- Newton solves for implicit stages
- dense Jacobian or finite-difference Jacobian fallback
- ARKODE-style error-driven step adaptation

The Python implementation in `python/NeoSUNDIALS/ark_workflow.py`
keeps the higher-level ARKODE workflow ideas:

- create a problem object
- create the native stepper
- advance internally with adaptive steps
- sample at requested output times
- interpolate between step endpoints
- gather statistics and diagnostics
