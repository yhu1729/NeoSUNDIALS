from __future__ import annotations

from dataclasses import dataclass
from typing import Callable

import numpy as np

from .ark_native import ARKConfig, NativeARKSolver
from .numerics import (
    ExactSolutionFn,
    cubic_hermite_interpolate,
    max_output_error,
    uniform_output_times,
    validate_output_times,
)


ArrayFn = Callable[[float, np.ndarray], np.ndarray]


ERK_RK4 = 0
DIRK_IMPLICIT_MIDPOINT = 1


@dataclass
class ARKProblem:
    name: str
    dimension: int
    initial_time: float
    initial_state: np.ndarray
    rhs: ArrayFn
    jacobian: ArrayFn | None = None
    method: int = ERK_RK4
    exact_solution: ExactSolutionFn | None = None


@dataclass
class ARKSolverConfig:
    t_final: float
    rtol: float = 1e-6
    atol: float = 1e-9
    h_init: float = 1e-3
    h_min: float = 1e-10
    h_max: float = 0.1
    max_steps: int = 20000
    max_newton_iters: int = 8
    safety: float = 0.9
    min_factor: float = 0.2
    max_factor: float = 5.0
    newton_tol: float = 0.1

    def to_native(self, problem: ARKProblem) -> ARKConfig:
        return ARKConfig(
            dimension=problem.dimension,
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
            method=problem.method,
        )


@dataclass
class ARKStepRecord:
    step_index: int
    stage_evals: int
    newton_iters: int
    t_start: float
    t_end: float
    h_used: float
    h_next: float
    error_norm: float


@dataclass
class ARKRunSummary:
    steps: int
    accepted_steps: int
    rejected_steps: int
    rhs_evals: int
    jac_evals: int
    newton_iters: int
    current_t: float
    current_h: float
    last_error_norm: float


@dataclass
class ARKRunResult:
    problem_name: str
    output_times: np.ndarray
    output_states: np.ndarray
    step_history: list[ARKStepRecord]
    summary: ARKRunSummary


def solve_ark_problem_uniform(
    problem: ARKProblem, config: ARKSolverConfig, num_points: int = 101
) -> ARKRunResult:
    return solve_ark_problem(
        problem,
        config,
        output_times=uniform_output_times(problem.initial_time, config.t_final, num_points),
    )


def compute_reference_error(problem: ARKProblem, result: ARKRunResult) -> float:
    if problem.exact_solution is None:
        raise ValueError("problem does not define an exact solution")
    return max_output_error(result.output_times, result.output_states, problem.exact_solution)


def solve_ark_problem(problem: ARKProblem, config: ARKSolverConfig, output_times=None) -> ARKRunResult:
    output_times = validate_output_times(problem.initial_time, config.t_final, output_times)

    solver = NativeARKSolver(
        config=config.to_native(problem),
        t0=problem.initial_time,
        y0=np.asarray(problem.initial_state, dtype=np.float64),
        rhs=problem.rhs,
        jac=problem.jacobian,
    )

    step_history: list[ARKStepRecord] = []
    output_states = np.empty((len(output_times), problem.dimension), dtype=np.float64)

    prev_t = problem.initial_time
    prev_y = np.asarray(problem.initial_state, dtype=np.float64).copy()
    prev_f = problem.rhs(prev_t, prev_y)
    output_index = 0

    while output_index < len(output_times) and output_times[output_index] <= prev_t + 1e-15:
        output_states[output_index] = prev_y
        output_index += 1

    while solver.time() < config.t_final - 1e-15:
        remaining = config.t_final - solver.time()
        solver.set_step_size(min(remaining, solver.summary().current_h, config.h_max))
        step = solver.step()
        curr_t = solver.time()
        curr_y = solver.state()
        curr_f = problem.rhs(curr_t, curr_y)

        step_history.append(
            ARKStepRecord(
                step_index=step.step_index,
                stage_evals=step.stage_evals,
                newton_iters=step.newton_iters,
                t_start=step.t_start,
                t_end=step.t_end,
                h_used=step.h_used,
                h_next=step.h_next,
                error_norm=step.error_norm,
            )
        )

        while output_index < len(output_times) and output_times[output_index] <= curr_t + 1e-15:
            output_states[output_index] = cubic_hermite_interpolate(
                prev_t, prev_y, prev_f, curr_t, curr_y, curr_f, output_times[output_index]
            )
            output_index += 1

        prev_t = curr_t
        prev_y = curr_y
        prev_f = curr_f

    while output_index < len(output_times):
        output_states[output_index] = prev_y
        output_index += 1

    summary = solver.summary()
    return ARKRunResult(
        problem_name=problem.name,
        output_times=output_times,
        output_states=output_states,
        step_history=step_history,
        summary=ARKRunSummary(
            steps=summary.steps,
            accepted_steps=summary.accepted_steps,
            rejected_steps=summary.rejected_steps,
            rhs_evals=summary.rhs_evals,
            jac_evals=summary.jac_evals,
            newton_iters=summary.newton_iters,
            current_t=summary.current_t,
            current_h=summary.current_h,
            last_error_norm=summary.last_error_norm,
        ),
    )
