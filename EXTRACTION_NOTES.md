# Extraction Notes

## Table of Contents
- [SBDF Core (CVODE/IDA)](#sbdf-core-cvodeida)
- [Workflow Layer](#workflow-layer)
- [ARKODE Core](#arkode-core)
- [Behavioral Guarantees](#behavioral-guarantees)
- [Known Gaps And Intentional Non-Goals](#known-gaps-and-intentional-non-goals)
- [Testing Matrix](#testing-matrix)

This distills SUNDIALS parts into numerical kernels (`c/`) and workflows (`python/`).

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

## Behavioral Guarantees

- Native constructors return opaque state handles or `NULL` on failure.
- Native stepping routines report failures through nonzero status codes.
- Python workflow APIs raise exceptions when native calls fail.
- Time integration uses adaptive step control in both SBDF and ARK cores.
- Dense Jacobian and finite-difference Jacobian paths are both supported.

## Known Gaps And Intentional Non-Goals

- Feature completeness is intentionally reduced versus upstream SUNDIALS.
- Only dense linear algebra is in scope; no sparse/Krylov stacks.
- No rootfinding, events, sensitivity analysis, or projection.
- BDF support in the current implementation is focused on BDF1/BDF2.
- The native Python callback bridge currently assumes callbacks do not raise;
  callback exception handling is a known hardening task.
- NVector weighted norms are present but currently under validation because
  the weight vector is not consumed in all weighted operators.

## Testing Matrix

| Claim | Primary implementation | Test coverage | Status |
|------|-------------------------|---------------|--------|
| SBDF adaptive implicit stepping | `c/sbdf_core.c` | `tests/test_sbdf_core.c`, `tests/test_workflow.py`, `tests/test_verification.py` | Covered |
| ARK explicit/implicit stepping | `c/arkode_core.c` | `tests/test_arkode_core.c`, `tests/test_ark_workflow.py`, `tests/test_verification.py` | Covered |
| ARK adaptation controls | `c/arkode_core.c` | `tests/test_arkode_adapt.c` | Covered |
| NVector core algebra | `c/nvector_serial.c` | `tests/test_nvector_serial.c` | Covered |
| NVector weighted norms | `c/nvector_serial.c` | `tests/test_nvector_serial.c` | Partial (uniform-weight only) |
| ARK interpolation/tstop/reset semantics | `c/arkode_core.c` | `tests/test_arkode_interp.c`, `tests/test_arkode_tstop.c`, `tests/test_arkode_reset.c` | Partial (workflow-level emulation) |
