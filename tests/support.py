from __future__ import annotations

import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
PYTHON_DIR = ROOT / "python"


def add_python_path() -> None:
    python_dir = str(PYTHON_DIR)
    if python_dir not in sys.path:
        sys.path.insert(0, python_dir)
