# NeoSUNDIALS Porting Roadmap

This roadmap translates the upstream SUNDIALS documentation into a staged plan
for growing NeoSUNDIALS without losing the small, auditable vertical-slice
model. It is based on the vendored SUNDIALS documentation under
`sundials/doc/`, not on API compatibility goals.

## Source Documents Reviewed

Primary SUNDIALS source material:

- `sundials/doc/superbuild/source/index.rst`
- `sundials/doc/superbuild/source/sundials/index.rst`
- `sundials/doc/superbuild/source/nvectors/index.rst`
- `sundials/doc/superbuild/source/sunmatrix/index.rst`
- `sundials/doc/shared/sunlinsol/SUNLinSol_Introduction.rst`
- `sundials/doc/superbuild/source/sunnonlinsol/index.rst`
- `sundials/doc/superbuild/source/sunmemory/index.rst`
- `sundials/doc/superbuild/source/sunadaptcontroller/index.rst`
- `sundials/doc/superbuild/source/sunstepper/index.rst`
- `sundials/doc/arkode/guide/source/Introduction.rst`
- `sundials/doc/arkode/guide/source/Organization.rst`
- `sundials/doc/arkode/guide/source/Mathematics.rst`
- `sundials/doc/cvode/guide/source/Introduction.rst`
- `sundials/doc/cvode/guide/source/Mathematics.rst`
- `sundials/doc/ida/guide/source/Mathematics.rst`
- `sundials/doc/cvodes/guide/source/Introduction.rst`
- `sundials/doc/cvodes/guide/source/Usage/FSA.rst`
- `sundials/doc/idas/guide/source/Introduction.rst`
- `sundials/doc/idas/guide/source/Usage/FSA.rst`
- `sundials/doc/kinsol/guide/source/Introduction.rst`
- `sundials/doc/kinsol/guide/source/Mathematics.rst`

## Roadmap Principles

- Preserve algorithmic lineage, not upstream ABI or full API parity.
- Keep each milestone as a complete vertical slice: native kernel, Python
  bridge, workflow, examples/problems, tests, and docs.
- Promote shared infrastructure only when at least two solver slices need it.
- Prefer deterministic serial behavior before adding parallel, GPU, sparse, or
  distributed-memory surfaces.
- Treat sensitivity analysis, adjoints, rootfinding, and large solver stacks as
  explicit product decisions, not incidental ports.

## Upstream Architecture Implications

SUNDIALS is organized around package solvers built on shared abstractions:
`N_Vector`, `SUNMatrix`, `SUNLinearSolver`, `SUNNonlinearSolver`,
`SUNMemoryHelper`, adaptivity controllers, and package-specific solver memory.
NeoSUNDIALS currently has compact equivalents for a serial vector layer, dense
linear algebra embedded in solver kernels, Newton loops embedded in solver
kernels, and workflow-owned interpolation/output policy.

The main architectural decision is therefore whether each shared SUNDIALS
concept remains embedded in the current compact kernels or becomes an explicit
NeoSUNDIALS interface. The default should be embedded until duplication or
cross-solver inconsistency creates a correctness or maintainability problem.

## Phase 0: Stabilize Current Vertical Slices

Goal: make the existing SBDF, DAE residual, ARK, and serial NVector slices
predictable before adding breadth.

Milestones:

- Document native status codes and Python exception mapping for SBDF, DAE, ARK,
  and NVector failures.
- Harden DAE residual stepping enough to reduce dependence on the Python
  residual-to-RHS fallback.
- Add a nontrivial DAE verification problem with an algebraic component, not
  only residual-form linear decay.
- Decide whether ARK `tstop`, reset, and interpolation behavior should remain
  workflow-owned or become native API surface.
- Keep the current dense serial implementation as the baseline oracle for later
  solver modularization.

Validation:

- `make check`
- targeted C tests for native failure statuses and callback failure paths
- Python tests for exception messages and fallback behavior

Risks:

- DAE residual support can appear correct on ODE-like residuals while still
  failing on algebraic constraints or inconsistent initial conditions.
- Workflow-level interpolation can mask native-step semantics if solver state
  and output state are not tested separately.

## Phase 1: Shared Numerical Infrastructure Boundaries

Goal: introduce explicit boundaries only where SUNDIALS docs show recurring
cross-package concepts and NeoSUNDIALS already has duplication pressure.

Milestones:

- Define a minimal dense matrix helper or `SUNMatrix`-inspired internal module
  if dense Jacobian assembly/solve logic is duplicated across SBDF, DAE, ARK,
  and future KINSOL work.
- Define a minimal linear-solver result contract for direct dense solves:
  success, singular matrix, invalid input, and callback-induced failure.
- Factor Newton iteration policy only after comparing SBDF, IDA-style residual,
  ARK implicit stages, and KINSOL requirements.
- Keep the serial `NVector` API stable and add missing operations only when a
  solver milestone needs them.
- Add a developer note documenting which upstream shared interfaces are
  intentionally not represented yet: sparse matrices, Krylov solvers,
  preconditioners, memory helpers, and device vectors.

Validation:

- native tests that compare refactored dense solves against existing solver
  behavior
- regression tests for singular Jacobian and nonfinite callback output

Risks:

- Premature abstraction can obscure solver math and make review harder.
- A generic Newton layer may be wrong if it ignores solver-specific convergence
  tests, Jacobian update rules, and error-control coupling.

## Phase 2: CVODE-Like ODE Depth

Goal: deepen the ODE multistep slice before adding large new packages.

Milestones:

- Extend BDF support beyond the current BDF1/BDF2 limit only with direct tests
  for coefficient construction, order selection, step rejection, and dense
  output.
- Decide whether Adams methods are in scope. They are part of CVODE for
  nonstiff systems, but ARK explicit methods may already cover the practical
  NeoSUNDIALS use case.
- Add rootfinding/event detection as a separate vertical slice if downstream
  workflows need event boundaries.
- Evaluate CVODE-style constraints and projection separately from IDA-style
  algebraic variables.
- Improve interpolation from endpoint-only workflow interpolation toward
  solver-owned dense output only when accuracy tests show the need.

Validation:

- manufactured ODE solutions with known order behavior
- step rejection tests with discontinuous or rapidly changing RHS
- event-location tests if rootfinding is added

Risks:

- Higher BDF order increases history, coefficient, and local-error complexity.
- Adding Adams and BDF together may broaden the API faster than the test suite
  can constrain behavior.

## Phase 3: IDA-Like DAE Correctness

Goal: turn the experimental residual path into a constrained, documented DAE
solver slice.

Milestones:

- Require or compute consistent initial `y0` and `ydot0` for supported DAE
  classes.
- Add explicit differential/algebraic component metadata before omitting
  algebraic variables from local error tests.
- Implement a native residual Newton path with the IDA Jacobian form
  `dF/dy + alpha dF/dydot` or document why finite-difference residual columns
  are sufficient for the supported subset.
- Add tests for inconsistent initial conditions, algebraic constraint drift,
  and residual callback failures.
- Keep index-one semi-explicit systems as the first supported nontrivial class.

Validation:

- semi-explicit index-one DAE examples with known invariants
- residual norm checks at output points
- tests that fail when algebraic constraints drift

Risks:

- Treating every residual as an ODE can silently produce wrong results.
- Consistent initial condition logic is a solver in its own right and must be
  scoped as a separate milestone.

## Phase 4: ARKODE Breadth With Method Discipline

Goal: grow ARK support along documented ARKODE module boundaries.

Milestones:

- Separate ERK-only, DIRK-only, and IMEX ARK use cases in the Python workflow
  instead of representing every method as a generic table choice.
- Add split RHS support for `fE(t, y) + fI(t, y)` before claiming ARKStep-like
  ImEx behavior.
- Add method metadata for order, embedding, stiffness assumptions, and dense
  output capability.
- Evaluate Lagrange interpolation for stiff problems only after the history
  storage and accuracy tests exist.
- Defer MRIStep, SplittingStep, SPRKStep, ForcingStep, mass matrices, and
  custom inner steppers until there is a concrete workflow requiring them.

Validation:

- split stiff/nonstiff problems where explicit-only integration fails or is
  impractical
- method-order tests for each built-in table
- implicit-stage convergence and failure-path tests

Risks:

- A table-driven implementation can accept invalid method combinations unless
  method metadata is validated aggressively.
- Split RHS support changes callback ownership and exception handling in both C
  and Python.

## Phase 5: Nonlinear Algebra And KINSOL

Goal: port KINSOL only if standalone nonlinear solves are useful outside the
time integrators, or if a shared Newton layer has become justified.

Milestones:

- Start with a dense serial Newton solver for `F(u) = 0` with explicit scaling
  vectors and clear convergence statuses.
- Add finite-difference dense Jacobian support and optional analytic Jacobian.
- Add line search only after tests show plain Newton fails on relevant
  examples.
- Defer fixed-point and Anderson acceleration until there is a clear workflow.
- Avoid claiming Newton-Krylov support without a real iterative linear solver
  and preconditioner contract.

Validation:

- nonlinear systems with known roots and scaling sensitivity
- singular Jacobian and stalled-step tests
- callback exception propagation tests

Risks:

- Sharing Newton code too early can flatten important differences between
  KINSOL, CVODE, IDA, and ARKODE convergence criteria.
- Line search and constraints alter failure modes and need explicit tests.

## Phase 6: Sensitivity And Adjoint Capabilities

Goal: treat CVODES/IDAS sensitivity work as a major feature family.

Milestones:

- Begin with forward sensitivities for ODEs, using explicit parameter metadata
  and either user-supplied sensitivity RHS or internal difference quotients.
- Decide whether sensitivities participate in error control before exposing
  tolerances.
- Add DAE sensitivities only after DAE initial conditions and residual Jacobian
  behavior are stable.
- Defer adjoint sensitivity analysis until checkpointing, dense output, and
  backward integration semantics are proven.
- Keep quadrature sensitivity as a separate feature after base sensitivities.

Validation:

- finite-difference parameter checks against forward sensitivity output
- sensitivity tolerance tests
- memory ownership tests for arrays of sensitivity vectors

Risks:

- Sensitivity arrays multiply state ownership complexity across C and Python.
- Adjoint support requires checkpointing and interpolation correctness that the
  current workflow does not yet provide.

## Phase 7: Optional Scale-Out Features

Goal: add larger SUNDIALS infrastructure only when the compact solver has
compelling use cases and tests.

Milestones:

- Add banded direct solvers before sparse or Krylov solvers if target problems
  have local coupling.
- Add a Krylov interface only with a real preconditioner setup/solve contract
  and scaled residual stopping tests.
- Keep GPU, MPI, OpenMP, and external solver libraries out of scope until a
  benchmark demonstrates dense serial limitations.
- Consider `SUNMemoryHelper`-style memory ownership only if non-CPU arrays or
  zero-copy exchange become requirements.
- Treat official `sundials4py` patterns as input for Python ownership design,
  but keep `ctypes` until there is a strong reason to add binding dependencies.

Validation:

- benchmark-driven acceptance criteria
- compatibility tests for each vector/matrix/solver pairing
- leak and ownership tests around external memory

Risks:

- Parallel and GPU support changes determinism, reproducibility, and memory
  ownership assumptions.
- External dependencies increase build complexity and should not be added for
  speculative coverage.

## Near-Term Prioritized Work

1. Document status codes and exception mapping for current native solvers.
2. Add a nontrivial DAE verification problem and residual norm checks.
3. Decide native versus workflow ownership for ARK `tstop`, reset, and
   interpolation.
4. Add a small dense linear-solver helper only if it removes duplicated logic
   without hiding solver-specific convergence behavior.
5. Add method metadata validation for ARK methods before expanding table
   coverage.

## Explicit Non-Goals For Now

- Drop-in SUNDIALS API or ABI compatibility.
- Sparse, Krylov, preconditioned, distributed, threaded, or GPU solver stacks.
- CVODES/IDAS adjoint checkpointing.
- Full SUNDIALS package coverage.
- Broad dependency additions for bindings or external solver libraries.
