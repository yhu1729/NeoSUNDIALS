from __future__ import annotations

from dataclasses import dataclass
from typing import Callable

import numpy as np

from .native import NativeSolver, SBDFConfig


ArrayFn = Callable[[float, np.ndarray], np.ndarray]


@dataclass
class ODEProblem:
    name: str
    dimension: int
    initial_time: float
    initial_state: np.ndarray
    rhs: ArrayFn
    jacobian: ArrayFn | None = None


@dataclass
class SolverConfig:
    t_final: float
    rtol: float = 1e-6
    atol: float = 1e-9
    h_init: float = 1e-3
    h_min: float = 1e-10
    h_max: float = 0.5
    max_order: int = 2
    max_steps: int = 10000
    max_newton_iters: int = 8
    safety: float = 0.9
    min_factor: float = 0.2
    max_factor: float = 5.0
    newton_tol: float = 0.1

    def to_native(self, dimension: int) -> SBDFConfig:
        return SBDFConfig(
            dimension=dimension,
            max_order=self.max_order,
            max_steps=self.max_steps,
            max_newton_iters=self.max_newton_iters,
            rtol=self.rtol,
            atol=self.atol,
            h_init=self.h_init,
            h_min=self.h_min,
            h_max=self.h_max,
            safety=self.safety,
            min_factor=self.min_factor,
            max_factor=self.max_factor,
            newton_tol=self.newton_tol,
        )


@dataclass
class StepRecord:
    step_index: int
    order: int
    t_start: float
    t_end: float
    h_used: float
    h_next: float
    error_norm: float
    newton_norm: float
    newton_iters: int


@dataclass
class RunSummary:
    steps: int
    accepted_steps: int
    rejected_steps: int
    rhs_evals: int
    jac_evals: int
    newton_iters: int
    current_t: float
    current_h: float
    last_error_norm: float
    last_newton_norm: float


@dataclass
class RunResult:
    problem_name: str
    output_times: np.ndarray
    output_states: np.ndarray
    step_history: list[StepRecord]
    summary: RunSummary


def _linear_interpolate(t0: float, y0: np.ndarray, t1: float, y1: np.ndarray, t: float) -> np.ndarray:
    if abs(t1 - t0) < 1e-15:
        return y1.copy()
    theta = (t - t0) / (t1 - t0)
    return (1.0 - theta) * y0 + theta * y1


def solve_problem(problem: ODEProblem, config: SolverConfig, output_times=None) -> RunResult:
    if output_times is None:
        output_times = np.linspace(problem.initial_time, config.t_final, 101, dtype=np.float64)
    else:
        output_times = np.asarray(output_times, dtype=np.float64)

    if output_times[0] < problem.initial_time - 1e-15:
        raise ValueError("output_times must start at or after the initial time")
    if output_times[-1] > config.t_final + 1e-15:
        raise ValueError("output_times must not exceed t_final")

    solver = NativeSolver(
        config=config.to_native(problem.dimension),
        t0=problem.initial_time,
        y0=np.asarray(problem.initial_state, dtype=np.float64),
        rhs=problem.rhs,
        jac=problem.jacobian,
    )

    step_history: list[StepRecord] = []
    output_states = np.empty((len(output_times), problem.dimension), dtype=np.float64)
    output_states[0] = np.asarray(problem.initial_state, dtype=np.float64)

    output_index = 1
    prev_t = problem.initial_time
    prev_y = np.asarray(problem.initial_state, dtype=np.float64).copy()

    while solver.time() < config.t_final - 1e-15:
        remaining = config.t_final - solver.time()
        if remaining <= 0.0:
            break
        solver.set_step_size(min(remaining, solver.summary().current_h, config.h_max))
        step = solver.step()
        curr_t = solver.time()
        curr_y = solver.state()

        step_history.append(
            StepRecord(
                step_index=step.step_index,
                order=step.order,
                t_start=step.t_start,
                t_end=step.t_end,
                h_used=step.h_used,
                h_next=step.h_next,
                error_norm=step.error_norm,
                newton_norm=step.newton_norm,
                newton_iters=step.newton_iters,
            )
        )

        while output_index < len(output_times) and output_times[output_index] <= curr_t + 1e-15:
            output_states[output_index] = _linear_interpolate(
                prev_t, prev_y, curr_t, curr_y, output_times[output_index]
            )
            output_index += 1

        prev_t = curr_t
        prev_y = curr_y

    while output_index < len(output_times):
        output_states[output_index] = prev_y
        output_index += 1

    summary = solver.summary()
    return RunResult(
        problem_name=problem.name,
        output_times=output_times,
        output_states=output_states,
        step_history=step_history,
        summary=RunSummary(
            steps=summary.steps,
            accepted_steps=summary.accepted_steps,
            rejected_steps=summary.rejected_steps,
            rhs_evals=summary.rhs_evals,
            jac_evals=summary.jac_evals,
            newton_iters=summary.newton_iters,
            current_t=summary.current_t,
            current_h=summary.current_h,
            last_error_norm=summary.last_error_norm,
            last_newton_norm=summary.last_newton_norm,
        ),
    )
