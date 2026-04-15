from __future__ import annotations

import unittest
import numpy as np
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parent.parent
PYTHON_DIR = ROOT / 'python'
if str(PYTHON_DIR) not in sys.path:
    sys.path.insert(0, str(PYTHON_DIR))

from NeoSUNDIALS import (
    ARKSolverConfig,
    SolverConfig,
    ark_brusselator_problem,
    ark_lotka_volterra_problem,
    lotka_volterra_problem,
    robertson_problem,
    solve_ark_problem,
    solve_problem,
)


class VerificationTests(unittest.TestCase):
    def test_sbdf_lotka_volterra(self):
        problem = lotka_volterra_problem()
        config = SolverConfig(t_final=10.0, h_init=1e-2, h_max=0.2, atol=1e-8, rtol=1e-6)
        output_times = np.linspace(0.0, 10.0, 41)
        result = solve_problem(problem, config, output_times=output_times)
        self.assertAlmostEqual(result.summary.current_t, 10.0, places=6)
        self.assertTrue(np.all(np.isfinite(result.output_states)))
        self.assertLess(result.summary.last_error_norm, 0.7)

    def test_sbdf_robertson(self):
        problem = robertson_problem()
        config = SolverConfig(t_final=1.0, h_init=1e-6, h_max=0.05, atol=1e-10, rtol=1e-6)
        output_times = np.linspace(0.0, 1.0, 26)
        result = solve_problem(problem, config, output_times=output_times)
        self.assertAlmostEqual(result.summary.current_t, 1.0, places=6)
        self.assertTrue(np.all(np.isfinite(result.output_states)))
        self.assertLess(result.summary.last_error_norm, 0.7)

    def test_ark_erk_lotka_volterra(self):
        problem = ark_lotka_volterra_problem()
        config = ARKSolverConfig(t_final=10.0, h_init=1e-2, h_max=0.2, atol=1e-8, rtol=1e-6)
        output_times = np.linspace(0.0, 10.0, 41)
        result = solve_ark_problem(problem, config, output_times=output_times)
        self.assertAlmostEqual(result.summary.current_t, 10.0, places=6)
        self.assertTrue(np.all(np.isfinite(result.output_states)))
        self.assertLess(result.summary.last_error_norm, 0.7)

    def test_ark_dirk_brusselator(self):
        problem = ark_brusselator_problem()
        config = ARKSolverConfig(t_final=2.0, h_init=1e-3, h_max=0.05, atol=1e-8, rtol=1e-5)
        output_times = np.linspace(0.0, 2.0, 41)
        result = solve_ark_problem(problem, config, output_times=output_times)
        self.assertAlmostEqual(result.summary.current_t, 2.0, places=6)
        self.assertTrue(np.all(np.isfinite(result.output_states)))
        self.assertLess(result.summary.last_error_norm, 0.7)



if __name__ == '__main__':
    unittest.main()

