from __future__ import annotations

import numpy as np

from .ark_workflow import ARKProblem, DIRK_IMPLICIT_MIDPOINT, ERK_RK4


def ark_linear_decay_problem(
    rate: float = 2.0, initial_value: float = 1.0, method: int = DIRK_IMPLICIT_MIDPOINT
) -> ARKProblem:
    def rhs(_t: float, y: np.ndarray) -> np.ndarray:
        return np.array([-rate * y[0]], dtype=np.float64)

    def jac(_t: float, _y: np.ndarray) -> np.ndarray:
        return np.array([[-rate]], dtype=np.float64)

    def exact_solution(t: float) -> np.ndarray:
        return np.array([initial_value * np.exp(-rate * t)], dtype=np.float64)

    return ARKProblem(
        name="ark_linear_decay",
        dimension=1,
        initial_time=0.0,
        initial_state=np.array([initial_value], dtype=np.float64),
        rhs=rhs,
        jacobian=jac,
        method=method,
        exact_solution=exact_solution,
    )


def ark_lotka_volterra_problem() -> ARKProblem:
    params = np.array([1.5, 1.0, 3.0, 1.0], dtype=np.float64)

    def rhs(_t: float, y: np.ndarray) -> np.ndarray:
        return np.array(
            [
                params[0] * y[0] - params[1] * y[0] * y[1],
                -params[2] * y[1] + params[3] * y[0] * y[1],
            ],
            dtype=np.float64,
        )

    return ARKProblem(
        name="ark_lotka_volterra",
        dimension=2,
        initial_time=0.0,
        initial_state=np.array([1.0, 1.0], dtype=np.float64),
        rhs=rhs,
        method=ERK_RK4,
    )


def ark_van_der_pol_problem(mu: float = 3.0, method: int = ERK_RK4) -> ARKProblem:
    def rhs(_t: float, y: np.ndarray) -> np.ndarray:
        return np.array(
            [
                y[1],
                mu * (1.0 - y[0] * y[0]) * y[1] - y[0],
            ],
            dtype=np.float64,
        )

    def jac(_t: float, y: np.ndarray) -> np.ndarray:
        return np.array(
            [
                [0.0, 1.0],
                [-2.0 * mu * y[0] * y[1] - 1.0, mu * (1.0 - y[0] * y[0])],
            ],
            dtype=np.float64,
        )

    return ARKProblem(
        name="ark_van_der_pol",
        dimension=2,
        initial_time=0.0,
        initial_state=np.array([2.0, 0.0], dtype=np.float64),
        rhs=rhs,
        jacobian=jac,
        method=method,
    )


def ark_brusselator_problem() -> ARKProblem:
    a = 1.0
    b = 3.5
    ep = 5.0e-3

    def rhs(_t: float, y: np.ndarray) -> np.ndarray:
        u, v, w = y
        return np.array(
            [
                a - (w + 1.0) * u + v * u * u,
                w * u - v * u * u,
                (b - w) / ep - w * u,
            ],
            dtype=np.float64,
        )

    def jac(_t: float, y: np.ndarray) -> np.ndarray:
        u, v, w = y
        return np.array(
            [
                [-(w + 1.0) + 2.0 * u * v, u * u, -u],
                [w - 2.0 * u * v, -u * u, u],
                [-w, 0.0, -1.0 / ep - u],
            ],
            dtype=np.float64,
        )

    return ARKProblem(
        name="ark_brusselator",
        dimension=3,
        initial_time=0.0,
        initial_state=np.array([1.2, 3.1, 3.0], dtype=np.float64),
        rhs=rhs,
        jacobian=jac,
        method=DIRK_IMPLICIT_MIDPOINT,
    )
