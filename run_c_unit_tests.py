from __future__ import annotations

import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path


ROOT = Path(__file__).resolve().parent
BUILD_DIR = ROOT / "build" / "c_tests"
TEST_DIR = ROOT / "tests"
C_DIR = ROOT / "c"


@dataclass
class CTestTarget:
    name: str
    source: Path
    implementation: Path


def executable_name(name: str) -> str:
    if sys.platform.startswith("win"):
        return f"{name}.exe"
    return name


def compile_target(target: CTestTarget) -> Path:
    BUILD_DIR.mkdir(parents=True, exist_ok=True)
    output = BUILD_DIR / executable_name(target.name)

    cmd = [
        "cc",
        "-O2",
        "-std=c99",
        str(target.source),
        str(target.implementation),
        "-lm",
        "-o",
        str(output),
    ]
    subprocess.run(cmd, check=True)
    return output


def run_target(target: CTestTarget) -> None:
    executable = compile_target(target)
    result = subprocess.run(
        [str(executable)],
        text=True,
        capture_output=True,
        check=False,
    )
    if result.stdout:
        print(result.stdout, end="")
    if result.stderr:
        print(result.stderr, end="", file=sys.stderr)
    if result.returncode != 0:
        raise SystemExit(f"{target.name} failed with exit code {result.returncode}")


def main() -> None:
    targets = [
        CTestTarget(
            name="test_sbdf_core",
            source=TEST_DIR / "test_sbdf_core.c",
            implementation=C_DIR / "sbdf_core.c",
        ),
        CTestTarget(
            name="test_arkode_core",
            source=TEST_DIR / "test_arkode_core.c",
            implementation=C_DIR / "arkode_core.c",
        ),
    ]

    for target in targets:
        print(f"==> Building and running {target.name}")
        run_target(target)

    print("All C unit tests passed.")


if __name__ == "__main__":
    main()
