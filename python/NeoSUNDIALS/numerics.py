from __future__ import annotations

from typing import Callable

import numpy as np


ArrayFn = Callable[[float, np.ndarray], np.ndarray]
ExactSolutionFn = Callable[[float], np.ndarray]


def uniform_output_times(initial_time: float, t_final: float, num_points: int = 101) -> np.ndarray:
    if num_points < 2:
        raise ValueError("num_points must be at least 2")
    if t_final < initial_time:
        raise ValueError("t_final must be at or after initial_time")
    return np.linspace(initial_time, t_final, num_points, dtype=np.float64)


def validate_output_times(initial_time: float, t_final: float, output_times=None) -> np.ndarray:
    if output_times is None:
        return uniform_output_times(initial_time, t_final)

    times = np.asarray(output_times, dtype=np.float64)
    if times.ndim != 1:
        raise ValueError("output_times must be one-dimensional")
    if len(times) == 0:
        raise ValueError("output_times must be non-empty")
    if times[0] < initial_time - 1e-15:
        raise ValueError("output_times must start at or after the initial time")
    if times[-1] > t_final + 1e-15:
        raise ValueError("output_times must not exceed t_final")
    if np.any(np.diff(times) < -1e-15):
        raise ValueError("output_times must be nondecreasing")
    return times


def linear_interpolate(t0: float, y0: np.ndarray, t1: float, y1: np.ndarray, t: float) -> np.ndarray:
    if abs(t1 - t0) < 1e-15:
        return y1.copy()
    theta = (t - t0) / (t1 - t0)
    return (1.0 - theta) * y0 + theta * y1


def cubic_hermite_interpolate(
    t0: float,
    y0: np.ndarray,
    f0: np.ndarray,
    t1: float,
    y1: np.ndarray,
    f1: np.ndarray,
    t: float,
) -> np.ndarray:
    h = t1 - t0
    if abs(h) < 1e-15:
        return y1.copy()
    s = (t - t0) / h
    h00 = 2.0 * s**3 - 3.0 * s**2 + 1.0
    h10 = s**3 - 2.0 * s**2 + s
    h01 = -2.0 * s**3 + 3.0 * s**2
    h11 = s**3 - s**2
    return h00 * y0 + h * h10 * f0 + h01 * y1 + h * h11 * f1


def max_output_error(
    output_times: np.ndarray, output_states: np.ndarray, exact_solution: ExactSolutionFn
) -> float:
    max_error = 0.0
    for index, time in enumerate(output_times):
        reference = np.asarray(exact_solution(float(time)), dtype=np.float64)
        max_error = max(max_error, float(np.max(np.abs(output_states[index] - reference))))
    return max_error
