from __future__ import annotations

import subprocess
import sys
from pathlib import Path

import numpy as np


ROOT = Path(__file__).resolve().parent
PYTHON_DIR = ROOT / "python"
if str(PYTHON_DIR) not in sys.path:
    sys.path.insert(0, str(PYTHON_DIR))

from NeoSUNDIALS import (  # noqa: E402
    ARKSolverConfig,
    SolverConfig,
    ark_brusselator_problem,
    ark_lotka_volterra_problem,
    lotka_volterra_problem,
    robertson_problem,
    solve_ark_problem,
    solve_problem,
)


def run_unit_tests() -> None:
    cmd = [sys.executable, "-m", "unittest", "discover", "-s", "tests", "-p", "test_*.py"]
    subprocess.run(cmd, check=True)


def run_sbdf_verification() -> None:
    cases = [
        (
            "Lotka-Volterra",
            lotka_volterra_problem(),
            SolverConfig(t_final=10.0, h_init=1e-2, h_max=0.2, atol=1e-8, rtol=1e-6),
            np.linspace(0.0, 10.0, 41),
        ),
        (
            "Robertson",
            robertson_problem(),
            SolverConfig(t_final=1.0, h_init=1e-6, h_max=0.05, atol=1e-10, rtol=1e-6),
            np.linspace(0.0, 1.0, 26),
        ),
    ]

    for name, problem, config, output_times in cases:
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
        if not np.all(np.isfinite(result.output_states)):
            raise SystemExit(f"{name} produced non-finite output")


def run_ark_verification() -> None:
    cases = [
        (
            "ARK ERK Lotka-Volterra",
            ark_lotka_volterra_problem(),
            ARKSolverConfig(t_final=10.0, h_init=1e-2, h_max=0.2, atol=1e-8, rtol=1e-6),
            np.linspace(0.0, 10.0, 41),
        ),
        (
            "ARK DIRK Brusselator",
            ark_brusselator_problem(),
            ARKSolverConfig(t_final=2.0, h_init=1e-3, h_max=0.05, atol=1e-8, rtol=1e-5),
            np.linspace(0.0, 2.0, 41),
        ),
    ]

    for name, problem, config, output_times in cases:
        result = solve_ark_problem(problem, config, output_times=output_times)
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
        if not np.all(np.isfinite(result.output_states)):
            raise SystemExit(f"{name} produced non-finite output")


def main() -> None:
    print("==> Running Python unit tests")
    run_unit_tests()
    print("==> Running SBDF verification cases")
    run_sbdf_verification()
    print("==> Running ARK verification cases")
    run_ark_verification()
    print("All Python tests passed.")


if __name__ == "__main__":
    main()
