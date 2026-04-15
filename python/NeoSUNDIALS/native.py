from __future__ import annotations

import ctypes
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path

import numpy as np


ROOT = Path(__file__).resolve().parents[2]
C_DIR = ROOT / "c"
BUILD_DIR = ROOT / "build"


class SBDFConfig(ctypes.Structure):
    _fields_ = [
        ("dimension", ctypes.c_int),
        ("max_order", ctypes.c_int),
        ("max_steps", ctypes.c_int),
        ("max_newton_iters", ctypes.c_int),
        ("rtol", ctypes.c_double),
        ("atol", ctypes.c_double),
        ("h_init", ctypes.c_double),
        ("h_min", ctypes.c_double),
        ("h_max", ctypes.c_double),
        ("safety", ctypes.c_double),
        ("min_factor", ctypes.c_double),
        ("max_factor", ctypes.c_double),
        ("newton_tol", ctypes.c_double),
    ]


class SBDFStepStats(ctypes.Structure):
    _fields_ = [
        ("step_index", ctypes.c_int),
        ("accepted", ctypes.c_int),
        ("order", ctypes.c_int),
        ("newton_iters", ctypes.c_int),
        ("t_start", ctypes.c_double),
        ("t_end", ctypes.c_double),
        ("h_used", ctypes.c_double),
        ("h_next", ctypes.c_double),
        ("error_norm", ctypes.c_double),
        ("newton_norm", ctypes.c_double),
    ]


class SBDFSummary(ctypes.Structure):
    _fields_ = [
        ("status", ctypes.c_int),
        ("steps", ctypes.c_int),
        ("accepted_steps", ctypes.c_int),
        ("rejected_steps", ctypes.c_int),
        ("rhs_evals", ctypes.c_int),
        ("jac_evals", ctypes.c_int),
        ("newton_iters", ctypes.c_int),
        ("current_t", ctypes.c_double),
        ("current_h", ctypes.c_double),
        ("last_error_norm", ctypes.c_double),
        ("last_newton_norm", ctypes.c_double),
    ]


RHS_CALLBACK = ctypes.CFUNCTYPE(
    ctypes.c_int,
    ctypes.c_double,
    ctypes.POINTER(ctypes.c_double),
    ctypes.POINTER(ctypes.c_double),
    ctypes.c_void_p,
)
JAC_CALLBACK = ctypes.CFUNCTYPE(
    ctypes.c_int,
    ctypes.c_double,
    ctypes.POINTER(ctypes.c_double),
    ctypes.POINTER(ctypes.c_double),
    ctypes.c_void_p,
)


def _shared_library_name() -> str:
    if sys.platform == "darwin":
        return "libsbdf_core.dylib"
    if sys.platform.startswith("win"):
        return "sbdf_core.dll"
    return "libsbdf_core.so"


def build_native_library() -> Path:
    BUILD_DIR.mkdir(parents=True, exist_ok=True)
    lib_path = BUILD_DIR / _shared_library_name()
    source = C_DIR / "sbdf_core.c"
    header = C_DIR / "sbdf_core.h"

    needs_build = not lib_path.exists()
    if not needs_build:
        lib_mtime = lib_path.stat().st_mtime
        needs_build = source.stat().st_mtime > lib_mtime or header.stat().st_mtime > lib_mtime

    if not needs_build:
        return lib_path

    if sys.platform == "darwin":
        cmd = [
            "cc",
            "-O3",
            "-dynamiclib",
            "-fPIC",
            "-std=c99",
            str(source),
            "-lm",
            "-o",
            str(lib_path),
        ]
    elif sys.platform.startswith("win"):
        cmd = ["cc", "-O3", "-shared", "-std=c99", str(source), "-o", str(lib_path)]
    else:
        cmd = ["cc", "-O3", "-shared", "-fPIC", "-std=c99", str(source), "-lm", "-o", str(lib_path)]

    subprocess.run(cmd, check=True)
    return lib_path


def load_native_library() -> ctypes.CDLL:
    lib = ctypes.CDLL(str(build_native_library()))
    state_ptr = ctypes.c_void_p

    lib.sbdf_create.argtypes = [ctypes.POINTER(SBDFConfig), ctypes.c_double, ctypes.POINTER(ctypes.c_double)]
    lib.sbdf_create.restype = state_ptr

    lib.sbdf_free.argtypes = [state_ptr]
    lib.sbdf_free.restype = None

    lib.sbdf_step.argtypes = [state_ptr, RHS_CALLBACK, JAC_CALLBACK, ctypes.c_void_p, ctypes.POINTER(SBDFStepStats)]
    lib.sbdf_step.restype = ctypes.c_int

    lib.sbdf_get_state.argtypes = [state_ptr, ctypes.POINTER(ctypes.c_double)]
    lib.sbdf_get_state.restype = ctypes.c_int

    lib.sbdf_get_summary.argtypes = [state_ptr, ctypes.POINTER(SBDFSummary)]
    lib.sbdf_get_summary.restype = ctypes.c_int

    lib.sbdf_get_time.argtypes = [state_ptr]
    lib.sbdf_get_time.restype = ctypes.c_double

    lib.sbdf_get_dimension.argtypes = [state_ptr]
    lib.sbdf_get_dimension.restype = ctypes.c_int

    lib.sbdf_set_step_size.argtypes = [state_ptr, ctypes.c_double]
    lib.sbdf_set_step_size.restype = ctypes.c_int

    return lib


@dataclass
class NativeStep:
    step_index: int
    order: int
    newton_iters: int
    t_start: float
    t_end: float
    h_used: float
    h_next: float
    error_norm: float
    newton_norm: float


@dataclass
class NativeSummary:
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


class NativeSolver:
    def __init__(self, config: SBDFConfig, t0: float, y0: np.ndarray, rhs, jac=None):
        self._lib = load_native_library()
        self._dim = int(config.dimension)
        self._rhs_python = rhs
        self._jac_python = jac
        self._last_callback_error: str | None = None

        self._rhs_cb = RHS_CALLBACK(self._rhs_bridge)
        self._jac_cb = JAC_CALLBACK(self._jac_bridge) if jac is not None else JAC_CALLBACK()

        y0 = np.asarray(y0, dtype=np.float64)
        if y0.shape != (self._dim,):
            raise ValueError(f"Initial state must have shape ({self._dim},), got {y0.shape}")
        self._state = self._lib.sbdf_create(
            ctypes.byref(config),
            ctypes.c_double(t0),
            y0.ctypes.data_as(ctypes.POINTER(ctypes.c_double)),
        )
        if not self._state:
            raise MemoryError("Failed to create native SBDF solver state")

    def __del__(self):
        state = getattr(self, "_state", None)
        if state:
            self._lib.sbdf_free(state)
            self._state = None

    def _rhs_bridge(self, t, y_ptr, dydt_ptr, _user_data):
        try:
            y = np.ctypeslib.as_array(y_ptr, shape=(self._dim,))
            dydt = np.ctypeslib.as_array(dydt_ptr, shape=(self._dim,))
            rhs_val = np.asarray(self._rhs_python(float(t), y.copy()), dtype=np.float64)
            if rhs_val.shape != (self._dim,):
                raise ValueError(f"RHS callback must return shape ({self._dim},), got {rhs_val.shape}")
            if not np.all(np.isfinite(rhs_val)):
                raise ValueError("RHS callback returned non-finite values")
            dydt[:] = rhs_val
            self._last_callback_error = None
            return 0
        except Exception as exc:
            self._last_callback_error = f"RHS callback failed at t={float(t):.16g}: {exc}"
            return -1

    def _jac_bridge(self, t, y_ptr, jac_ptr, _user_data):
        try:
            y = np.ctypeslib.as_array(y_ptr, shape=(self._dim,))
            jac = np.ctypeslib.as_array(jac_ptr, shape=(self._dim * self._dim,))
            jac_val = np.asarray(self._jac_python(float(t), y.copy()), dtype=np.float64)
            if jac_val.shape != (self._dim, self._dim):
                raise ValueError(
                    f"Jacobian callback must return shape ({self._dim}, {self._dim}), got {jac_val.shape}"
                )
            if not np.all(np.isfinite(jac_val)):
                raise ValueError("Jacobian callback returned non-finite values")
            jac[:] = jac_val.reshape(-1)
            self._last_callback_error = None
            return 0
        except Exception as exc:
            self._last_callback_error = f"Jacobian callback failed at t={float(t):.16g}: {exc}"
            return -1

    def step(self) -> NativeStep:
        stats = SBDFStepStats()
        self._last_callback_error = None
        flag = self._lib.sbdf_step(self._state, self._rhs_cb, self._jac_cb, None, ctypes.byref(stats))
        if flag != 0:
            if self._last_callback_error is not None:
                raise RuntimeError(f"Native SBDF step failed with status {flag}: {self._last_callback_error}")
            raise RuntimeError(f"Native SBDF step failed with status {flag}")
        return NativeStep(
            step_index=stats.step_index,
            order=stats.order,
            newton_iters=stats.newton_iters,
            t_start=stats.t_start,
            t_end=stats.t_end,
            h_used=stats.h_used,
            h_next=stats.h_next,
            error_norm=stats.error_norm,
            newton_norm=stats.newton_norm,
        )

    def time(self) -> float:
        return float(self._lib.sbdf_get_time(self._state))

    def set_step_size(self, h: float) -> None:
        flag = self._lib.sbdf_set_step_size(self._state, ctypes.c_double(h))
        if flag != 0:
            raise RuntimeError(f"Failed to set step size: {flag}")

    def state(self) -> np.ndarray:
        output = np.empty(self._dim, dtype=np.float64)
        flag = self._lib.sbdf_get_state(self._state, output.ctypes.data_as(ctypes.POINTER(ctypes.c_double)))
        if flag != 0:
            raise RuntimeError(f"Failed to fetch native state: {flag}")
        return output

    def summary(self) -> NativeSummary:
        summary = SBDFSummary()
        flag = self._lib.sbdf_get_summary(self._state, ctypes.byref(summary))
        if flag != 0:
            raise RuntimeError(f"Failed to fetch summary: {flag}")
        return NativeSummary(
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
        )
