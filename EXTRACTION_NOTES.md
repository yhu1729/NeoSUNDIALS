# Extraction Notes

## Table of Contents
- [Core Numerical Algorithms Extracted From SUNDIALS](#core-numerical-algorithms-extracted-from-sundials)
- [Core Workflow Algorithms Extracted From SUNDIALS](#core-workflow-algorithms-extracted-from-sundials)
- [ARKODE Extraction](#arkode-extraction)
- [DAE Residual Path](#dae-residual-path)
- [Behavioral Guarantees](#behavioral-guarantees)
- [Known Gaps And Intentional Non-Goals](#known-gaps-and-intentional-non-goals)
- [Porting Conventions](#porting-conventions)
- [Testing Matrix](#testing-matrix)

This document maps NeoSUNDIALS implementation pieces back to the upstream
SUNDIALS routines and documentation they were extracted from. The goal is to
preserve algorithmic lineage, not API compatibility.

NeoSUNDIALS is organized around small vertical slices:

- native C kernels in `c/`
- Python native bridges in `python/NeoSUNDIALS/*_native.py`
- user-facing workflows in `python/NeoSUNDIALS/*_workflow.py` and `workflow.py`
- focused C and Python tests in `tests/`

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
- compact ODE and residual-step interfaces instead of the full CVODE/IDA
  feature surface
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

## DAE Residual Path

The DAE path is a vertical slice inspired by IDA's residual formulation:

- user model: `F(t, y, ydot) = 0`
- native entry point: `sbdf_step_residual`
- Python problem type: `DAEProblem`
- Python workflow: `solve_dae_problem` and `solve_dae_problem_uniform`

The C residual stepper reuses the SBDF state/history machinery and solves the
implicit residual directly for the next `y`. The Python workflow currently
stabilizes this path by adjusting the user-provided `SolverConfig` before
constructing the native solver:

- `max_order` is clamped to `1`
- `h_max` is capped at `1e-3`
- `h_min`, `rtol`, `atol`, `max_steps`, `max_newton_iters`, and `newton_tol`
  are bounded to conservative values

If native residual stepping reports convergence or work-limit status, the
workflow falls back to a Python residual-to-RHS bridge. That bridge solves for
`ydot` from `F(t, y, ydot) = 0`, constructs a finite-difference Jacobian, and
then delegates to the ordinary SBDF ODE workflow. Callback shape and finiteness
checks are enforced in both paths.

This path is intentionally marked experimental because it does not yet expose
the full IDA feature set: no consistent-initial-condition solver, no algebraic
variable classification, no constraints, and no root/event handling.

## Behavioral Guarantees

- Native constructors return opaque state handles or `NULL` on failure.
- Native stepping routines report failures through nonzero status codes.
- Python workflow APIs raise exceptions when native calls fail.
- Time integration uses adaptive step control in both SBDF and ARK cores.
- Dense Jacobian and finite-difference Jacobian paths are both supported.
- Python native bridges validate callback return shape and finiteness before
  returning values to C.
- Python native bridges preserve callback exception context in the raised
  native-step error message.

## Known Gaps And Intentional Non-Goals

- Feature completeness is intentionally reduced versus upstream SUNDIALS.
- Only dense linear algebra is in scope; no sparse/Krylov stacks.
- No rootfinding, events, sensitivity analysis, or projection.
- BDF support in the current implementation is focused on BDF1/BDF2.
- DAE support is experimental and conservative; native residual stepping can
  fall back to the Python residual bridge on convergence/work-limit statuses.
- Only the serial NVector backend is in scope.
- Interpolation is workflow-owned and endpoint-based, not a full clone of
  SUNDIALS dense output internals.

## Porting Conventions

Every new routine or workflow port should follow the same extraction shape so
the project stays compact and reviewable.

- Preserve SUNDIALS algorithmic lineage, not full API parity.
- Add the smallest cohesive routine subset that supports one end-to-end use
  case.
- Keep responsibilities split:
  - C (`c/*.c`, `c/*.h`) owns time-stepping numerics and status codes.
  - Python (`python/NeoSUNDIALS/*`) owns configuration, workflow policy,
    sampling/interpolation, and user-facing errors.
- Keep native handles opaque and lifecycle explicit (`create`/`step`/`destroy`).
- Follow existing status semantics: `0` for success, nonzero for failures, then
  raise Python exceptions with useful context.
- Prefer deterministic defaults in tests and examples (fixed seeds, explicit
  tolerances, explicit output grids).

Each port should update this file with:

- source SUNDIALS routines/docs used for extraction,
- supported subset and explicit exclusions,
- test matrix entries mapped to concrete test files.

## Testing Matrix

| Claim | Primary implementation | Test coverage | Status |
|------|-------------------------|---------------|--------|
| SBDF adaptive implicit stepping | `c/sbdf_core.c` | `tests/test_sbdf_core.c`, `tests/test_workflow.py`, `tests/test_verification.py` | Covered |
| DAE residual workflow | `c/sbdf_core.c`, `python/NeoSUNDIALS/workflow.py`, `python/NeoSUNDIALS/native.py` | `tests/test_workflow.py` | Experimental |
| ARK explicit/implicit stepping | `c/arkode_core.c` | `tests/test_arkode_core.c`, `tests/test_ark_workflow.py`, `tests/test_verification.py` | Covered |
| ARK adaptation controls | `c/arkode_core.c` | `tests/test_arkode_adapt.c` | Covered |
| NVector core algebra | `c/nvector_serial.c` | `tests/test_nvector_serial.c` | Covered |
| NVector weighted norms | `c/nvector_serial.c` | `tests/test_nvector_serial.c` | Covered |
| Python callback validation | `python/NeoSUNDIALS/native.py`, `python/NeoSUNDIALS/ark_native.py` | `tests/test_workflow.py`, `tests/test_ark_workflow.py` | Covered |
| ARK interpolation/tstop/reset semantics | `c/arkode_core.c` | `tests/test_arkode_interp.c`, `tests/test_arkode_tstop.c`, `tests/test_arkode_reset.c` | Partial (workflow-level emulation) |
