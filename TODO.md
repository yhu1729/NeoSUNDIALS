# TODO: Routine Porting Checklist

For the higher-level staged plan, see [PORTING_ROADMAP.md](PORTING_ROADMAP.md).
This file stays focused on the checklist for each individual routine or
workflow port.

Use this checklist for each new SUNDIALS routine/workflow port so extraction,
testing, and docs stay consistent.

The project direction is vertical-slice extraction: each milestone should add
the smallest reviewable set of C kernel, Python bridge/workflow, tests, and
documentation needed for one coherent capability. Do not port broad API surface
area without an end-to-end problem that exercises it.

## 1) Scope And Lineage

- [ ] Identify exact upstream SUNDIALS routines/doc sections being extracted.
- [ ] Define the minimum supported subset for this milestone.
- [ ] Record explicit non-goals/deferred features.
- [ ] Decide whether the feature belongs in the extracted root implementation
      or only in the vendored `sundials/` reference tree.

## 2) C Core Integration (`c/*.c`, `c/*.h`)

- [ ] Add/extend opaque state and lifecycle (`create`, `step/solve`, `destroy`).
- [ ] Keep return code contract (`0` success, nonzero failure).
- [ ] Ensure tolerant handling of invalid arguments and callback failures.
- [ ] Update headers and comments to reflect supported behavior.

## 3) Python Native Bridge (`python/NeoSUNDIALS/*_native.py`)

- [ ] Wire ctypes signatures and ownership rules.
- [ ] Validate callback outputs (shape/type/finite) before returning to C.
- [ ] Map nonzero native statuses to clear Python exceptions.

## 4) Workflow Layer (`workflow.py`, `ark_workflow.py`, `problems.py`)

- [ ] Add user-facing config/problem fields required by the new routine.
- [ ] Preserve adaptive stepping + output sampling semantics.
- [ ] Keep diagnostics and run summaries backward compatible.
- [ ] Document any workflow-level stabilization policy, such as clamped step
      sizes, reduced order, fallback paths, or relaxed tolerances.

## 5) Tests (required before merge)

- [ ] C unit tests for core routine behavior and error paths.
- [ ] Python workflow test for nominal run and output-shape invariants.
- [ ] Verification test for one representative problem.
- [ ] Regression test for at least one failure path.
- [ ] `make check` passes locally.

## 6) Documentation And Tracking

- [ ] Update `EXTRACTION_NOTES.md` extraction mapping + testing matrix.
- [ ] Update `README.md` supported features and caveats.
- [ ] Add release note entry in `CHANGELOG.md`.

## Current High-Value Follow-Ups

- [ ] Document native status codes and Python exception mapping for SBDF, DAE,
      ARK, and NVector failure modes.
- [ ] Harden native DAE residual stepping enough to reduce or remove the Python
      residual-bridge fallback.
- [ ] Add a nontrivial DAE verification problem beyond linear decay.
- [ ] Decide whether ARK interpolation/tstop/reset should remain
      workflow-level emulation or become native API surface.
- [ ] Add ARK method metadata validation before broadening built-in table
      coverage.
- [ ] Evaluate a shared dense linear-solver helper only if it removes real
      duplication across solver kernels.
- [ ] Keep `README.md`, `EXTRACTION_NOTES.md`, and this checklist synchronized
      whenever a solver capability changes.
