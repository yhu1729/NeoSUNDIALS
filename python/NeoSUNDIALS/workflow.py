from __future__ import annotations

from dataclasses import dataclass
from typing import Callable

import numpy as np

from .native import NativeSolver, SBDFConfig
from .numerics import (
    ExactSolutionFn,
    linear_interpolate,
    max_output_error,
    uniform_output_times,
    validate_output_times,
)


ArrayFn = Callable[[float, np.ndarray], np.ndarray]
ResidualFn = Callable[[float, np.ndarray, np.ndarray], np.ndarray]


@dataclass
class ODEProblem:
    name: str
    dimension: int
    initial_time: float
    initial_state: np.ndarray
    rhs: ArrayFn
    jacobian: ArrayFn | None = None
    exact_solution: ExactSolutionFn | None = None


@dataclass
class DAEProblem:
    name: str
    dimension: int
    initial_time: float
    initial_state: np.ndarray
    residual: ResidualFn
    initial_ydot: np.ndarray | None = None
    exact_solution: ExactSolutionFn | None = None


@dataclass
class SolverConfig:
    t_final: float
    rtol: float = 1e-6
    atol: float = 1e-9
    h_init: float = 1e-3
    h_min: float = 1e-10
    h_max: float = 0.5
    max_order: int = 2
    max_steps: int = 50000
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


def _solve_residual_for_ydot(
    residual: ResidualFn,
    t: float,
    y: np.ndarray,
    ydot_guess: np.ndarray,
    *,
    max_iters: int = 8,
    tol: float = 1e-10,
) -> np.ndarray:
    dim = y.shape[0]
    ydot = np.asarray(ydot_guess, dtype=np.float64).copy()
    eps = 1e-8

    for _ in range(max_iters):
        res = np.asarray(residual(t, y, ydot), dtype=np.float64)
        if res.shape != (dim,):
            raise ValueError(f"Residual callback must return shape ({dim},), got {res.shape}")
        if not np.all(np.isfinite(res)):
            raise ValueError("Residual callback returned non-finite values")
        if np.linalg.norm(res, ord=2) <= tol:
            return ydot

        jac = np.zeros((dim, dim), dtype=np.float64)
        for j in range(dim):
            trial = ydot.copy()
            delta = eps * (1.0 + abs(ydot[j]))
            trial[j] += delta
            res_p = np.asarray(residual(t, y, trial), dtype=np.float64)
            if res_p.shape != (dim,):
                raise ValueError(f"Residual callback must return shape ({dim},), got {res_p.shape}")
            if not np.all(np.isfinite(res_p)):
                raise ValueError("Residual callback returned non-finite values")
            jac[:, j] = (res_p - res) / delta

        try:
            delta_ydot = np.linalg.solve(jac, -res)
        except np.linalg.LinAlgError as exc:
            raise RuntimeError("Failed to solve residual Jacobian system for ydot") from exc
        ydot += delta_ydot
        if np.linalg.norm(delta_ydot, ord=2) <= tol:
            return ydot

    raise RuntimeError("Residual Newton solve did not converge")


def _make_residual_rhs_and_jac(problem: DAEProblem) -> tuple[ArrayFn, ArrayFn]:
    dim = int(problem.dimension)
    if dim <= 0:
        raise ValueError("DAE problem dimension must be positive")

    if problem.initial_ydot is None:
        ydot_state = np.zeros(dim, dtype=np.float64)
    else:
        ydot_state = np.asarray(problem.initial_ydot, dtype=np.float64).copy()
    if ydot_state.shape != (dim,):
        raise ValueError(f"Initial ydot must have shape ({dim},), got {ydot_state.shape}")

    def rhs(t: float, y: np.ndarray) -> np.ndarray:
        y_vec = np.asarray(y, dtype=np.float64)
        if y_vec.shape != (dim,):
            raise ValueError(f"State passed to DAE residual has shape {y_vec.shape}, expected ({dim},)")
        return _solve_residual_for_ydot(problem.residual, float(t), y_vec, ydot_state)

    def jacobian(t: float, y: np.ndarray) -> np.ndarray:
        t_f = float(t)
        y_vec = np.asarray(y, dtype=np.float64)
        if y_vec.shape != (dim,):
            raise ValueError(f"State passed to DAE residual has shape {y_vec.shape}, expected ({dim},)")

        ydot = _solve_residual_for_ydot(problem.residual, t_f, y_vec, ydot_state)
        f0 = np.asarray(problem.residual(t_f, y_vec, ydot), dtype=np.float64)
        if f0.shape != (dim,):
            raise ValueError(f"Residual callback must return shape ({dim},), got {f0.shape}")
        if not np.all(np.isfinite(f0)):
            raise ValueError("Residual callback returned non-finite values")

        dF_dy = np.zeros((dim, dim), dtype=np.float64)
        dF_dydot = np.zeros((dim, dim), dtype=np.float64)
        eps = 1e-8

        for j in range(dim):
            delta_y = eps * (1.0 + abs(y_vec[j]))
            y_trial = y_vec.copy()
            y_trial[j] += delta_y
            f_y = np.asarray(problem.residual(t_f, y_trial, ydot), dtype=np.float64)
            if f_y.shape != (dim,):
                raise ValueError(f"Residual callback must return shape ({dim},), got {f_y.shape}")
            dF_dy[:, j] = (f_y - f0) / delta_y

            delta_ydot = eps * (1.0 + abs(ydot[j]))
            ydot_trial = ydot.copy()
            ydot_trial[j] += delta_ydot
            f_ydot = np.asarray(problem.residual(t_f, y_vec, ydot_trial), dtype=np.float64)
            if f_ydot.shape != (dim,):
                raise ValueError(f"Residual callback must return shape ({dim},), got {f_ydot.shape}")
            dF_dydot[:, j] = (f_ydot - f0) / delta_ydot

        try:
            return -np.linalg.solve(dF_dydot, dF_dy)
        except np.linalg.LinAlgError as exc:
            raise RuntimeError("Failed to recover DAE Jacobian from residual derivatives") from exc

    return rhs, jacobian


def solve_dae_problem(problem: DAEProblem, config: SolverConfig, output_times=None) -> RunResult:
    # Residual-based DAE mode is currently stabilized on BDF1 while the
    # dedicated DAE coefficient/history path is still being extracted.
    dae_config = SolverConfig(
        t_final=config.t_final,
        rtol=max(config.rtol, 1e-5),
        atol=max(config.atol, 1e-8),
        h_init=config.h_init,
        h_min=max(config.h_min, 1e-8),
        h_max=config.h_max,
        max_order=1,
        max_steps=max(config.max_steps, 200000),
        max_newton_iters=config.max_newton_iters,
        safety=config.safety,
        min_factor=config.min_factor,
        max_factor=config.max_factor,
        newton_tol=max(config.newton_tol, 0.2),
    )
    rhs_fn, jac_fn = _make_residual_rhs_and_jac(problem)
    ode_problem = ODEProblem(
        name=problem.name,
        dimension=problem.dimension,
        initial_time=problem.initial_time,
        initial_state=np.asarray(problem.initial_state, dtype=np.float64),
        rhs=rhs_fn,
        jacobian=jac_fn,
        exact_solution=problem.exact_solution,
    )
    return solve_problem(ode_problem, dae_config, output_times=output_times)


def solve_dae_problem_uniform(problem: DAEProblem, config: SolverConfig, num_points: int = 101) -> RunResult:
    return solve_dae_problem(
        problem,
        config,
        output_times=uniform_output_times(problem.initial_time, config.t_final, num_points),
    )


def solve_problem_uniform(problem: ODEProblem, config: SolverConfig, num_points: int = 101) -> RunResult:
    return solve_problem(
        problem,
        config,
        output_times=uniform_output_times(problem.initial_time, config.t_final, num_points),
    )


def compute_reference_error(problem: ODEProblem, result: RunResult) -> float:
    if problem.exact_solution is None:
        raise ValueError("problem does not define an exact solution")
    return max_output_error(result.output_times, result.output_states, problem.exact_solution)


def solve_problem(problem: ODEProblem, config: SolverConfig, output_times=None) -> RunResult:
    output_times = validate_output_times(problem.initial_time, config.t_final, output_times)

    solver = NativeSolver(
        config=config.to_native(problem.dimension),
        t0=problem.initial_time,
        y0=np.asarray(problem.initial_state, dtype=np.float64),
        rhs=problem.rhs,
        jac=problem.jacobian,
    )

    step_history: list[StepRecord] = []
    output_states = np.empty((len(output_times), problem.dimension), dtype=np.float64)
    prev_t = problem.initial_time
    prev_y = np.asarray(problem.initial_state, dtype=np.float64).copy()
    output_index = 0

    while output_index < len(output_times) and output_times[output_index] <= prev_t + 1e-15:
        output_states[output_index] = prev_y
        output_index += 1

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
            output_states[output_index] = linear_interpolate(
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
