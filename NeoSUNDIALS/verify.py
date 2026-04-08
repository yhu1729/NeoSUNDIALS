from __future__ import annotations

import sys
from pathlib import Path

import numpy as np


ROOT = Path(__file__).resolve().parent
PYTHON_DIR = ROOT / "python"
if str(PYTHON_DIR) not in sys.path:
    sys.path.insert(0, str(PYTHON_DIR))

from sundials_extracted_core import (  # noqa: E402
    SolverConfig,
    lotka_volterra_problem,
    robertson_problem,
    solve_problem,
)


def run_case(name, problem, config, output_times):
    result = solve_problem(problem, config, output_times=output_times)
    print(f"{name}:")
    print(
        "  final t={:.6f} steps={} rejected={} rhs={} jac={}".format(
            result.summary.current_t,
            result.summary.steps,
            result.summary.rejected_steps,
            result.summary.rhs_evals,
            result.summary.jac_evals,
        )
    )
    print(f"  final state={result.output_states[-1]}")
    print(f"  last error norm={result.summary.last_error_norm:.3e}")
    return result


def main():
    lv = run_case(
        "Lotka-Volterra",
        lotka_volterra_problem(),
        SolverConfig(t_final=10.0, h_init=1e-2, h_max=0.2, atol=1e-8, rtol=1e-6),
        np.linspace(0.0, 10.0, 41),
    )

    rb = run_case(
        "Robertson",
        robertson_problem(),
        SolverConfig(t_final=1.0, h_init=1e-6, h_max=0.05, atol=1e-10, rtol=1e-6),
        np.linspace(0.0, 1.0, 26),
    )

    if not np.all(np.isfinite(lv.output_states)):
        raise SystemExit("Lotka-Volterra produced non-finite output")
    if not np.all(np.isfinite(rb.output_states)):
        raise SystemExit("Robertson produced non-finite output")


if __name__ == "__main__":
    main()
