# Agent Instructions For NeoSUNDIALS

## Project Identity

NeoSUNDIALS is a compact extraction of selected SUNDIALS solver ideas into a
small, auditable codebase. It is not a drop-in replacement for upstream
SUNDIALS and should not grow toward full API or ABI compatibility unless a
human explicitly changes that goal.

Current priorities:

- preserve the algorithmic spine of CVODE, IDA, and ARKODE
- keep the native code dense, serial, deterministic, and directly reviewable
- add one cohesive vertical slice at a time
- validate every solver capability with C tests and Python workflow tests
- use the vendored `sundials/` tree as reference material, not as code to
  casually edit

## Repository Boundaries

- `c/`: extracted native C kernels and serial vector implementation.
- `python/NeoSUNDIALS/`: Python workflows, problem definitions, and `ctypes`
  native bridges.
- `tests/`: C unit tests, Python workflow tests, and verification tests.
- `sundials/`: vendored upstream SUNDIALS reference tree. Do not modify this
  directory unless the task is explicitly to update or patch the vendored
  snapshot.
- `README.md`, `EXTRACTION_NOTES.md`, `PORTING_ROADMAP.md`, `TODO.md`, and
  `CHANGELOG.md`: project tracking and user-facing documentation.

When changing solver behavior, update the relevant docs in the same change.

## Design Rules

- Correctness comes before speed unless performance data justifies otherwise.
- Prefer simple, explicit C over generic infrastructure until at least two
  solver slices need the same abstraction.
- Keep native handles opaque and lifecycles explicit: create, step/solve/get,
  destroy.
- Preserve the existing status-code convention: `0` means success, nonzero
  means failure; Python bridges convert failures into descriptive exceptions.
- Validate callback outputs at the Python boundary for shape, type, and
  finiteness before returning data to C.
- Keep dense serial behavior as the baseline oracle before adding banded,
  sparse, Krylov, parallel, GPU, or external-library support.
- Do not introduce broad dependencies for speculative future coverage.

Avoid claiming support for an upstream SUNDIALS feature until NeoSUNDIALS has a
native implementation, Python workflow exposure, tests, and documentation for
the supported subset.

## C Guidelines

- Prioritize memory safety and deterministic behavior.
- Check all pointer arguments before dereference.
- Avoid undefined behavior, integer overflow in allocation-size calculations,
  and unchecked buffer assumptions.
- Keep ownership rules explicit in headers and tests.
- Use small helper functions when they remove real duplication or isolate
  failure handling; avoid premature framework-style abstractions.
- Keep comments focused on why a numerical or safety decision exists.

## Python Guidelines

- Prefer clear dataclasses, explicit validation, and straightforward control
  flow.
- Keep `ctypes` signatures, ownership rules, and callback error handling close
  to the native bridge code.
- Raise specific Python exceptions with enough context to diagnose native
  callback, convergence, or input-validation failures.
- Keep workflow policy explicit, especially for step caps, fallbacks, relaxed
  tolerances, and interpolation behavior.

## Porting Workflow

Use vertical-slice extraction:

1. Identify the exact upstream SUNDIALS routines and docs used as source
   material.
2. Define the minimum supported subset and explicit non-goals.
3. Implement or modify the C kernel.
4. Wire the Python native bridge and workflow layer.
5. Add targeted C tests, Python workflow tests, and at least one verification
   or regression test when behavior changes.
6. Update `EXTRACTION_NOTES.md`, `README.md`, `TODO.md`, and
   `CHANGELOG.md` when user-visible capabilities change.

Use `PORTING_ROADMAP.md` for feature sequencing. Near-term priorities are:

- document native status codes and Python exception mapping
- harden DAE residual stepping and add a nontrivial DAE verification problem
- decide native versus workflow ownership for ARK `tstop`, reset, and
  interpolation
- add ARK method metadata validation before broadening method coverage
- only factor dense linear algebra or Newton helpers when duplication is real

## Testing

Preferred validation from the repository root:

```bash
make check
```

Useful narrower checks:

```bash
make libs
make tests
python -m unittest discover tests -p "test_*.py" -v
```

Run the smallest meaningful tests during iteration, then run `make check` for
solver behavior changes when feasible. For documentation-only changes, do not
run numerical tests unless the task asks for it.

## Git And Change Hygiene

- Work on the current branch unless the user asks for a new one.
- Do not revert or overwrite user changes.
- Stage and commit only files relevant to the task.
- Use concise conventional commit messages, for example
  `docs: add SUNDIALS porting roadmap` or `fix: handle residual callback errors`.
- Do not push unless a human explicitly asks.

## Common Pitfalls

- Treating the DAE residual path like an ODE RHS bridge can hide algebraic
  constraint errors. Check residual norms and inconsistent initial conditions.
- Generic Newton code can be wrong if it ignores solver-specific convergence
  tests, Jacobian update rules, and error-control coupling.
- Workflow-owned interpolation may not match native solver state semantics.
  Test internal step state and output samples separately when changing it.
- SUNDIALS docs describe many package features that are intentionally out of
  scope here: sensitivity analysis, adjoints, rootfinding, constraints,
  projection, sparse/Krylov solvers, preconditioners, MPI, GPU, and production
  API parity.
