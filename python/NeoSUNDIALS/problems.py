from __future__ import annotations

from dataclasses import dataclass

import numpy as np

from .workflow import ODEProblem


def lotka_volterra_problem() -> ODEProblem:
    params = np.array([1.5, 1.0, 3.0, 1.0], dtype=np.float64)

    def rhs(_t: float, y: np.ndarray) -> np.ndarray:
        return np.array(
            [
                params[0] * y[0] - params[1] * y[0] * y[1],
                -params[2] * y[1] + params[3] * y[0] * y[1],
            ],
            dtype=np.float64,
        )

    def jac(_t: float, y: np.ndarray) -> np.ndarray:
        return np.array(
            [
                [params[0] - params[1] * y[1], -params[1] * y[0]],
                [params[3] * y[1], -params[2] + params[3] * y[0]],
            ],
            dtype=np.float64,
        )

    return ODEProblem(
        name="lotka_volterra",
        dimension=2,
        initial_time=0.0,
        initial_state=np.array([1.0, 1.0], dtype=np.float64),
        rhs=rhs,
        jacobian=jac,
    )


def robertson_problem() -> ODEProblem:
    def rhs(_t: float, y: np.ndarray) -> np.ndarray:
        return np.array(
            [
                -0.04 * y[0] + 1.0e4 * y[1] * y[2],
                0.04 * y[0] - 1.0e4 * y[1] * y[2] - 3.0e7 * y[1] * y[1],
                3.0e7 * y[1] * y[1],
            ],
            dtype=np.float64,
        )

    def jac(_t: float, y: np.ndarray) -> np.ndarray:
        return np.array(
            [
                [-0.04, 1.0e4 * y[2], 1.0e4 * y[1]],
                [0.04, -1.0e4 * y[2] - 6.0e7 * y[1], -1.0e4 * y[1]],
                [0.0, 6.0e7 * y[1], 0.0],
            ],
            dtype=np.float64,
        )

    return ODEProblem(
        name="robertson",
        dimension=3,
        initial_time=0.0,
        initial_state=np.array([1.0, 0.0, 0.0], dtype=np.float64),
        rhs=rhs,
        jacobian=jac,
    )
