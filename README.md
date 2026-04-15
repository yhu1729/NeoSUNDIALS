# NeoSUNDIALS

NeoSUNDIALS is an AI-assisted exploration of how much of the SUNDIALS solver
stack can be re-expressed as a smaller, easier-to-review codebase.

This repository is not a drop-in replacement for upstream SUNDIALS. It is a
research and engineering project with two complementary goals:

- extract the core numerical ideas behind SUNDIALS time-stepping methods into a
  compact implementation
- use AI-generated code as a first draft, with humans acting as reviewers,
  editors, and correctness backstops

In practice, the project is a small solver playground built around selected
ideas from CVODE, IDA, and ARKODE, plus the original upstream SUNDIALS source
tree for reference.

## Repository Layout

The repo has two main parts:

- `sundials/`: a vendored copy of upstream SUNDIALS used as source material and
  comparison
- the extracted NeoSUNDIALS implementation at the repository root

The extracted implementation currently includes:

- NVector serial layer (self-contained abstraction for vectors/ops).
- compact C solver kernels for a simplified SBDF-style implicit multistep core
  and an ARKODE-style Runge-Kutta core
- Python workflow code that loads the native libraries with `ctypes`, defines
  example problems, and runs verification examples
- C unit tests (SBDF/ARK/NVector) and Python verification scripts
- `CHANGELOG.md` tracking ports.

## Public API Contract

### C layer

| Area | Entry points | Contract |
|------|--------------|----------|
| SBDF | `sbdf_create`, `sbdf_step`, `sbdf_get_state`, `sbdf_get_summary`, `sbdf_set_step_size`, `sbdf_free` | Caller owns callback correctness and input buffers. Functions return integer status codes (`0` success). |
| ARK | `ark_create`, `ark_step`, `ark_get_state`, `ark_get_summary`, `ark_set_step_size`, `ark_free` | Same ownership and return-code model as SBDF. |
| NVector | `N_VNew_Serial`, `N_VDestroy_Serial`, vector ops in `nvector_serial.c` | Serial-only implementation with direct heap ownership rules in `N_VSetArrayPointer_Serial`. |

### Python layer

| Area | Entry points | Contract |
|------|--------------|----------|
| SBDF workflow | `python/NeoSUNDIALS/workflow.py` | Raises Python exceptions on invalid setup and native nonzero return codes. |
| ARK workflow | `python/NeoSUNDIALS/ark_workflow.py` | Same error model as SBDF workflow. |
| Native bridges | `python/NeoSUNDIALS/native.py`, `python/NeoSUNDIALS/ark_native.py` | Uses `ctypes`; callbacks must return correctly shaped numeric arrays and should avoid raising. |

## Extracted Solver Scope

The extracted code intentionally focuses on the algorithmic spine rather than
feature completeness. It currently includes:

- weighted RMS norms
- variable-step BDF coefficient generation
- polynomial prediction from solution history
- Newton correction with dense linear solves
- finite-difference Jacobian fallback
- adaptive step-size control with simplified BDF1/BDF2 order selection
- ARKODE-style stage-based Runge-Kutta stepping
- built-in explicit and diagonally implicit Butcher tables
- step-doubling error estimation and adaptive step control

## Known Deviations And Risks

- This is not a compatibility implementation of the full SUNDIALS API.
- Dense linear algebra only; no sparse or Krylov linear solver backends.
- No rootfinding, sensitivity analysis, constraints, projection, or events.
- Some test files in `tests/` are exploratory artifacts and are not wired into
  the default build (`make tests` / `make check`).
- Current NVector weighted-norm routines in `c/nvector_serial.c` should be
  treated as under review (the weight argument is currently unused).

This extraction is intentionally smaller than full SUNDIALS. It does not aim to
reproduce the full feature surface such as large solver stacks, rootfinding,
sensitivity analysis, or production-scale APIs.

## Key Paths

| Directory/File | Description |
|---------------|-------------|
| `c/` | Extracted C cores: `arkode_core.[ch]`, `sbdf_core.[ch]`, `nvector_serial.[ch]` |
| `python/NeoSUNDIALS/` | Python workflows, problems, ctypes bindings |
| `tests/` | C unit tests (`test_*.c`) |
| `EXTRACTION_NOTES.md` | Upstream algorithm mappings |
| [TODO.md](TODO.md) | Development roadmap |
| [CHANGELOG.md](CHANGELOG.md) | Version history |

[![License](https://img.shields.io/badge/License-BSD--3--Clause-blue.svg)](https://opensource.org/licenses/BSD-3-Clause)

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

## Running The Project

Typical entry points from the repository root:

```bash
make check
python -m unittest discover tests -p "test_*.py" -v
```

The Python test suite executes both unit tests and higher-level SBDF/ARK
verification cases. The Python workflow builds native libraries on first use
with the system C compiler if they are not already present.

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

- AI generates or refactors code
- humans review for numerical correctness, design quality, and maintainability
- humans step in directly when the AI loses context, makes unsafe assumptions,
  or stalls

This repository is therefore both a solver experiment and a workflow experiment
about using AI as a code-writing partner without giving up human engineering
oversight.
