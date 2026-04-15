from __future__ import annotations

import sys
import unittest
from pathlib import Path

import numpy as np


ROOT = Path(__file__).resolve().parents[1]
PYTHON_DIR = ROOT / "python"
if str(PYTHON_DIR) not in sys.path:
    sys.path.insert(0, str(PYTHON_DIR))

from NeoSUNDIALS.problems import brusselator_problem, linear_decay_problem, van_der_pol_problem  # noqa: E402
from NeoSUNDIALS.workflow import ODEProblem  # noqa: E402
from NeoSUNDIALS.workflow import (  # noqa: E402
    SolverConfig,
    compute_reference_error,
    solve_problem,
    solve_problem_uniform,
)


class WorkflowTests(unittest.TestCase):
    def test_solve_problem_uniform_linear_decay_matches_exact_solution(self) -> None:
        problem = linear_decay_problem(rate=1.5, initial_value=2.0)
        result = solve_problem_uniform(
            problem,
            SolverConfig(t_final=0.4, h_init=1e-3, h_max=0.05, atol=1e-10, rtol=1e-7),
            num_points=21,
        )

        self.assertEqual(result.problem_name, "linear_decay")
        self.assertGreater(len(result.step_history), 0)
        self.assertGreater(result.summary.accepted_steps, 0)
        self.assertLess(compute_reference_error(problem, result), 1.0e-5)
        self.assertLess(result.output_states[-1][0], result.output_states[0][0])

    def test_solve_problem_accepts_repeated_output_times(self) -> None:
        problem = linear_decay_problem()
        output_times = np.array([0.0, 0.1, 0.1, 0.2], dtype=np.float64)
        result = solve_problem(problem, SolverConfig(t_final=0.2), output_times=output_times)
        np.testing.assert_allclose(result.output_times, output_times)
        np.testing.assert_allclose(result.output_states[1], result.output_states[2])

    def test_solve_problem_handles_output_grid_that_starts_after_initial_time(self) -> None:
        problem = linear_decay_problem(rate=1.0, initial_value=1.0)
        output_times = np.array([0.1, 0.2, 0.3], dtype=np.float64)
        result = solve_problem(problem, SolverConfig(t_final=0.3), output_times=output_times)
        self.assertGreater(result.output_states[0][0], result.output_states[1][0])

    def test_catalog_problems_have_expected_shapes(self) -> None:
        self.assertEqual(brusselator_problem().dimension, 2)
        self.assertEqual(van_der_pol_problem().initial_state.shape, (2,))

    def test_solve_problem_reports_rhs_callback_shape_errors(self) -> None:
        def bad_rhs(_t: float, _y: np.ndarray) -> np.ndarray:
            return np.array([1.0, 2.0], dtype=np.float64)

        problem = ODEProblem(
            name="bad_rhs_shape",
            dimension=1,
            initial_time=0.0,
            initial_state=np.array([1.0], dtype=np.float64),
            rhs=bad_rhs,
        )
        with self.assertRaisesRegex(RuntimeError, "RHS callback failed"):
            solve_problem(problem, SolverConfig(t_final=0.1, h_init=1e-3, h_max=0.01))


if __name__ == "__main__":
    unittest.main()
