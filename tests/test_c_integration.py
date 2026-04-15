from __future__ import annotations

import os
import subprocess
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
TEST_BUILD_DIR = ROOT / "build" / "c_tests"
RUN_C_TESTS = os.environ.get("NEOSUNDIALS_RUN_C_TESTS") == "1"


class CIntegrationTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if not RUN_C_TESTS:
            raise unittest.SkipTest("set NEOSUNDIALS_RUN_C_TESTS=1 to run C binaries from unittest")
        cls.exes = sorted(
            path
            for path in TEST_BUILD_DIR.glob("test_*")
            if path.is_file() and os.access(path, os.X_OK)
        )
        if not cls.exes:
            raise AssertionError(f'No C test executables found in {TEST_BUILD_DIR}. Run "make tests" first.')

    def test_all_c_tests_pass(self):
        for exe in self.exes:
            with self.subTest(exe=exe.name):
                result = subprocess.run([str(exe)], capture_output=True, text=True, timeout=30)
                self.assertEqual(
                    result.returncode,
                    0,
                    f"{exe} failed exit {result.returncode}\nstdout: {result.stdout}\nstderr: {result.stderr}",
                )
                self.assertNotIn("FAILED", result.stdout, f'{exe} printed "FAILED"\nstdout: {result.stdout}')
                self.assertNotIn("FAIL", result.stderr, f'{exe} printed "FAIL" in stderr')
                self.assertRegex(
                    result.stdout.lower(),
                    r"pass(ed)?",
                    f"{exe} did not print PASS/PASSED\nstdout: {result.stdout}",
                )


if __name__ == "__main__":
    unittest.main()
