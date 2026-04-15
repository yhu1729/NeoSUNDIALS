# TODO: Routine Porting Checklist

Use this checklist for each new SUNDIALS routine/workflow port so extraction,
testing, and docs stay consistent.

## 1) Scope And Lineage

- [ ] Identify exact upstream SUNDIALS routines/doc sections being extracted.
- [ ] Define the minimum supported subset for this milestone.
- [ ] Record explicit non-goals/deferred features.

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
