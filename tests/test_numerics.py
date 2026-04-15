from __future__ import annotations

import sys
import unittest
from pathlib import Path

import numpy as np


ROOT = Path(__file__).resolve().parents[1]
PYTHON_DIR = ROOT / "python"
if str(PYTHON_DIR) not in sys.path:
    sys.path.insert(0, str(PYTHON_DIR))

from NeoSUNDIALS.numerics import (  # noqa: E402
    cubic_hermite_interpolate,
    linear_interpolate,
    uniform_output_times,
    validate_output_times,
)


class NumericsTests(unittest.TestCase):
    def test_uniform_output_times_spans_interval(self) -> None:
        times = uniform_output_times(0.0, 1.0, 5)
        np.testing.assert_allclose(times, np.array([0.0, 0.25, 0.5, 0.75, 1.0]))

    def test_validate_output_times_rejects_decreasing_sequence(self) -> None:
        with self.assertRaisesRegex(ValueError, "nondecreasing"):
            validate_output_times(0.0, 1.0, [0.0, 0.5, 0.4, 1.0])

    def test_linear_interpolate_midpoint(self) -> None:
        y = linear_interpolate(0.0, np.array([1.0, 3.0]), 2.0, np.array([5.0, 7.0]), 1.0)
        np.testing.assert_allclose(y, np.array([3.0, 5.0]))

    def test_cubic_hermite_interpolate_matches_quadratic(self) -> None:
        def poly(t: float) -> float:
            return t * t + 2.0 * t + 1.0

        def dpoly(t: float) -> float:
            return 2.0 * t + 2.0

        y = cubic_hermite_interpolate(
            0.0,
            np.array([poly(0.0)]),
            np.array([dpoly(0.0)]),
            1.0,
            np.array([poly(1.0)]),
            np.array([dpoly(1.0)]),
            0.25,
        )
        np.testing.assert_allclose(y, np.array([poly(0.25)]))


if __name__ == "__main__":
    unittest.main()
