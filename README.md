# NeoSUNDIALS

[![License](https://img.shields.io/badge/License-BSD--3--Clause-blue.svg)](https://opensource.org/licenses/BSD-3-Clause)

NeoSUNDIALS is a compact, AI-assisted extraction of selected SUNDIALS solver
ideas into a codebase that is small enough to audit directly.

The project is not a drop-in replacement for upstream SUNDIALS. It is a
research and engineering playground for preserving the algorithmic spine of
CVODE, IDA, and ARKODE while deliberately leaving out the production-scale API
surface. The current implementation focuses on dense, serial, deterministic
time-stepping kernels with Python workflows for setup, verification, and
diagnostics.

The intended direction is:

- keep the numerical core understandable before making it broader
- extract one cohesive vertical slice at a time from upstream SUNDIALS
- validate each slice with C unit tests and Python workflow/verification tests
- use AI-generated code as a draft, with human review as the correctness gate

## Repository Layout

The repo has two main parts:

- `sundials/`: a vendored copy of upstream SUNDIALS used as source material and
  comparison. Treat this as reference material unless you are intentionally
  updating the upstream snapshot.
- the extracted NeoSUNDIALS implementation at the repository root.

The extracted implementation currently includes:

- NVector serial layer: self-contained vector storage and serial vector ops.
- compact C solver kernels for a simplified SBDF-style implicit multistep core
  and an ARKODE-style Runge-Kutta core.
- an experimental residual-form DAE path built on the SBDF core.
- Python workflow code that loads the native libraries with `ctypes`, defines
  example problems, samples output grids, and reports diagnostics.
- C unit tests, Python integration tests, and verification examples.
- project tracking docs: `EXTRACTION_NOTES.md`, `TODO.md`, and `CHANGELOG.md`.

## Status At A Glance

| Area | Status | Notes |
|------|--------|-------|
| SBDF ODE stepping | Working vertical slice | BDF1/BDF2, adaptive steps, dense Newton solves, optional analytic Jacobian. |
| DAE residual stepping | Experimental | Uses `F(t, y, ydot) = 0`; tries native residual stepping first and falls back to a Python residual-to-RHS bridge on native convergence/work-limit failures. |
| ARK stepping | Working vertical slice | Explicit and diagonally implicit RK methods with adaptive control. |
| NVector serial ops | Working vertical slice | Serial backend only; weighted norm behavior has non-uniform weight tests. |
| SUNDIALS API compatibility | Not in scope | This repo preserves selected ideas, not ABI/API parity. |

## Architecture

NeoSUNDIALS keeps the same broad separation of concerns as SUNDIALS, but with a
much smaller feature surface:

| Layer | Files | Responsibility |
|-------|-------|----------------|
| C kernels | `c/sbdf_core.[ch]`, `c/arkode_core.[ch]`, `c/nvector_serial.[ch]` | Native stepping, dense linear algebra, callback status codes, step statistics. |
| Native bridges | `python/NeoSUNDIALS/native.py`, `python/NeoSUNDIALS/ark_native.py` | Build/load shared libraries, wire `ctypes` signatures, validate callback outputs, convert native failures to Python exceptions. |
| Workflow layer | `python/NeoSUNDIALS/workflow.py`, `python/NeoSUNDIALS/ark_workflow.py` | Problem/config dataclasses, solve loops, output-time sampling, interpolation, run summaries. |
| Problem catalog | `python/NeoSUNDIALS/problems.py`, `python/NeoSUNDIALS/ark_problems.py` | Small ODE/DAE examples used by tests and verification. |
| Tests | `tests/` | Native C tests plus Python workflow and verification tests. |

## Public API Contract

### C layer

| Area | Entry points | Contract |
|------|--------------|----------|
| SBDF | `sbdf_create`, `sbdf_step`, `sbdf_step_residual`, `sbdf_get_state`, `sbdf_get_summary`, `sbdf_set_step_size`, `sbdf_free` | Caller owns callback correctness and input buffers. Functions return integer status codes (`0` success). |
| ARK | `ark_create`, `ark_step`, `ark_get_state`, `ark_get_summary`, `ark_set_step_size`, `ark_free` | Same ownership and return-code model as SBDF. |
| NVector | `N_VNew_Serial`, `N_VDestroy_Serial`, vector ops in `nvector_serial.c` | Serial-only implementation with direct heap ownership rules in `N_VSetArrayPointer_Serial`. |

### Python layer

| Area | Entry points | Contract |
|------|--------------|----------|
| SBDF workflow | `python/NeoSUNDIALS/workflow.py` | Raises Python exceptions on invalid setup and native nonzero return codes. |
| DAE workflow (experimental) | `python/NeoSUNDIALS/workflow.py` (`DAEProblem`, `solve_dae_problem`) | Accepts residual form `F(t,y,ydot)=0`, attempts native residual stepping, and currently falls back to Python residual-bridge solve on native convergence/work-limit failures. |
| ARK workflow | `python/NeoSUNDIALS/ark_workflow.py` | Same error model as SBDF workflow. |
| Native bridges | `python/NeoSUNDIALS/native.py`, `python/NeoSUNDIALS/ark_native.py` | Uses `ctypes`; validates callback output shapes/finiteness and surfaces callback failures through descriptive Python exceptions. |

## Extracted Solver Scope

The extracted code intentionally focuses on the algorithmic spine instead of
feature completeness. It currently includes:

- weighted RMS norms
- variable-step BDF coefficient generation
- polynomial prediction from solution history
- Newton correction with dense linear solves
- finite-difference Jacobian fallback
- adaptive step-size control with simplified BDF1/BDF2 order selection
- experimental native and Python-bridged DAE residual workflow
  (`F(t, y, ydot) = 0`)
- ARKODE-style stage-based Runge-Kutta stepping
- built-in explicit and diagonally implicit RK methods:
  RK4, forward Euler, Heun-Euler, explicit midpoint, Bogacki-Shampine,
  implicit midpoint, and backward Euler
- step-doubling error estimation and adaptive step control

## Known Deviations And Risks

- This is not a compatibility implementation of the full SUNDIALS API.
- Dense linear algebra only; no sparse or Krylov linear solver backends.
- No rootfinding, sensitivity analysis, constraints, projection, or events.
- Weighted NVector operators are implemented for serial vectors, but only the
  serial backend is currently in scope.
- DAE support is intentionally conservative: the workflow clamps the DAE path
  to BDF1-style stepping, tighter step caps, and relaxed tolerances while the
  native residual stepper is still being hardened.
- The workflow interpolation is endpoint-based: SBDF/DAE uses linear
  interpolation and ARK uses cubic Hermite interpolation from endpoint RHS
  evaluations.

This extraction is intentionally smaller than full SUNDIALS. It does not aim to
reproduce the full feature surface such as large solver stacks, rootfinding,
sensitivity analysis, or production-scale APIs.

## Key Paths

- [c/](c/): extracted C cores, including `arkode_core.[ch]`,
  `sbdf_core.[ch]`, and `nvector_serial.[ch]`.
- [python/NeoSUNDIALS/](python/NeoSUNDIALS/): Python workflows, problems,
  and `ctypes` bindings.
- [tests/](tests/): C unit tests, Python workflow tests, and
  integration/verification tests.
- [EXTRACTION_NOTES.md](EXTRACTION_NOTES.md): upstream algorithm mappings.
- [PORTING_ROADMAP.md](PORTING_ROADMAP.md): staged roadmap for porting more
  SUNDIALS concepts into NeoSUNDIALS.
- [TODO.md](TODO.md): development checklist for routine/workflow ports.
- [CHANGELOG.md](CHANGELOG.md): version history.

## Building The C Components

Build the extracted C code from the repository root with:

```bash
make
```

Useful targets:

- `make libs`: build the shared libraries used by the Python bindings
- `make tests`: build the C unit-test executables
- `make check`: run the Python suite (includes C integration checks)
- `make clean`: remove build artifacts

Build outputs are written under `build/`.

## Running The Test Suite

Typical entry points from the repository root:

```bash
make check
python -m unittest discover tests -p "test_*.py" -v
```

`make check` builds shared libraries, builds C unit-test executables, and then
runs the Python suite. The Python tests also execute selected C test binaries
through `tests/test_c_integration.py`.

The Python native bridge builds shared libraries on first use with the system C
compiler if the expected library is missing or stale.

## Python Usage

Run an SBDF ODE solve:

```python
from NeoSUNDIALS import SolverConfig, linear_decay_problem, solve_problem_uniform

problem = linear_decay_problem(rate=1.5, initial_value=2.0)
config = SolverConfig(t_final=0.4, h_init=1e-3, h_max=0.05)
result = solve_problem_uniform(problem, config, num_points=21)

print(result.output_times[-1], result.output_states[-1])
print(result.summary.accepted_steps)
```

Run an experimental residual-form DAE solve:

```python
from NeoSUNDIALS import SolverConfig, dae_linear_decay_problem, solve_dae_problem_uniform

problem = dae_linear_decay_problem(rate=1.5, initial_value=2.0)
config = SolverConfig(t_final=0.4, h_init=1e-3, h_max=0.05)
result = solve_dae_problem_uniform(problem, config, num_points=21)
```

Run an ARK solve with a selected method:

```python
from NeoSUNDIALS import (
    ARKSolverConfig,
    DIRK_IMPLICIT_MIDPOINT,
    ark_linear_decay_problem,
    solve_ark_problem_uniform,
)

problem = ark_linear_decay_problem(method=DIRK_IMPLICIT_MIDPOINT)
config = ARKSolverConfig(t_final=0.4, h_init=1e-3, h_max=0.05)
result = solve_ark_problem_uniform(problem, config, num_points=21)
```

If running from a checkout without installing the package, put `python/` on
`PYTHONPATH` or follow the test pattern of prepending it to `sys.path`.

## Upstream Lineage

The extracted implementation focuses on the algorithmic ideas visible in:

- `sundials/src/cvode/cvode.c`
- `sundials/src/ida/ida.c`
- `sundials/src/arkode/`
- `sundials/doc/cvode/guide/source/Mathematics.rst`
- `sundials/doc/ida/guide/source/Mathematics.rst`
- `sundials/doc/arkode/guide/source/Mathematics.rst`
- `sundials/examples/python/*`

For more detail on what was extracted and what was intentionally left out, see
`EXTRACTION_NOTES.md`.

## Working Style

The intended development loop is:

- AI generates or refactors code as a draft
- humans review for numerical correctness, design quality, and maintainability
- humans step in directly when the AI loses context, makes unsafe assumptions,
  or stalls
- every new solver slice updates docs and tests alongside implementation

This repository is therefore both a solver experiment and a workflow experiment
about using AI as a code-writing partner without giving up human engineering
oversight.
