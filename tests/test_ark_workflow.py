from __future__ import annotations

import sys
import unittest
from pathlib import Path

import numpy as np


ROOT = Path(__file__).resolve().parents[1]
PYTHON_DIR = ROOT / "python"
if str(PYTHON_DIR) not in sys.path:
    sys.path.insert(0, str(PYTHON_DIR))

from NeoSUNDIALS.ark_problems import (  # noqa: E402
    ark_brusselator_problem,
    ark_linear_decay_problem,
    ark_van_der_pol_problem,
)
from NeoSUNDIALS.ark_workflow import (  # noqa: E402
    ARKSolverConfig,
    DIRK_IMPLICIT_MIDPOINT,
    ERK_RK4,
    compute_reference_error,
    solve_ark_problem,
    solve_ark_problem_uniform,
)


class ARKWorkflowTests(unittest.TestCase):
    def test_dirk_linear_decay_matches_exact_solution(self) -> None:
        problem = ark_linear_decay_problem(rate=1.25, initial_value=1.5, method=DIRK_IMPLICIT_MIDPOINT)
        result = solve_ark_problem_uniform(
            problem,
            ARKSolverConfig(t_final=0.4, h_init=1e-3, h_max=0.05, atol=1e-10, rtol=1e-7),
            num_points=21,
        )

        self.assertEqual(result.problem_name, "ark_linear_decay")
        self.assertGreater(len(result.step_history), 0)
        self.assertGreater(result.summary.accepted_steps, 0)
        self.assertLess(compute_reference_error(problem, result), 1.5e-2)

    def test_ark_problem_catalog_covers_both_method_families(self) -> None:
        self.assertEqual(ark_brusselator_problem().method, DIRK_IMPLICIT_MIDPOINT)
        self.assertEqual(ark_van_der_pol_problem().method, ERK_RK4)

    def test_solve_ark_problem_handles_output_grid_that_starts_after_initial_time(self) -> None:
        problem = ark_linear_decay_problem(rate=1.0, method=DIRK_IMPLICIT_MIDPOINT)
        output_times = np.array([0.1, 0.2, 0.3], dtype=np.float64)
        result = solve_ark_problem(problem, ARKSolverConfig(t_final=0.3), output_times=output_times)
        self.assertGreater(result.output_states[0][0], result.output_states[1][0])


if __name__ == "__main__":
    unittest.main()
