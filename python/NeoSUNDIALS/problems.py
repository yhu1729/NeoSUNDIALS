from __future__ import annotations

import numpy as np

from .workflow import ODEProblem


def linear_decay_problem(rate: float = 2.0, initial_value: float = 1.0) -> ODEProblem:
    def rhs(_t: float, y: np.ndarray) -> np.ndarray:
        return np.array([-rate * y[0]], dtype=np.float64)

    def jac(_t: float, _y: np.ndarray) -> np.ndarray:
        return np.array([[-rate]], dtype=np.float64)

    def exact_solution(t: float) -> np.ndarray:
        return np.array([initial_value * np.exp(-rate * t)], dtype=np.float64)

    return ODEProblem(
        name="linear_decay",
        dimension=1,
        initial_time=0.0,
        initial_state=np.array([initial_value], dtype=np.float64),
        rhs=rhs,
        jacobian=jac,
        exact_solution=exact_solution,
    )


def van_der_pol_problem(mu: float = 5.0) -> ODEProblem:
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

    return ODEProblem(
        name="van_der_pol",
        dimension=2,
        initial_time=0.0,
        initial_state=np.array([2.0, 0.0], dtype=np.float64),
        rhs=rhs,
        jacobian=jac,
    )


def brusselator_problem(a: float = 1.0, b: float = 3.5) -> ODEProblem:
    def rhs(_t: float, y: np.ndarray) -> np.ndarray:
        u, v = y
        return np.array(
            [
                a - (b + 1.0) * u + u * u * v,
                b * u - u * u * v,
            ],
            dtype=np.float64,
        )

    def jac(_t: float, y: np.ndarray) -> np.ndarray:
        u, v = y
        return np.array(
            [
                [-(b + 1.0) + 2.0 * u * v, u * u],
                [b - 2.0 * u * v, -u * u],
            ],
            dtype=np.float64,
        )

    return ODEProblem(
        name="brusselator",
        dimension=2,
        initial_time=0.0,
        initial_state=np.array([1.2, 3.1], dtype=np.float64),
        rhs=rhs,
        jacobian=jac,
    )


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
