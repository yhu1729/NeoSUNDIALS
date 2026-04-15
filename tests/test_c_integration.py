from __future__ import annotations

import unittest
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
TEST_BUILD_DIR = ROOT / 'build' / 'c_tests'

class CIntegrationTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.exes = [
            TEST_BUILD_DIR / 'test_sbdf_core',
            TEST_BUILD_DIR / 'test_arkode_core',
            TEST_BUILD_DIR / 'test_nvector_serial',
            TEST_BUILD_DIR / 'test_arkode_adapt',
            TEST_BUILD_DIR / 'test_arkode_interp',
            TEST_BUILD_DIR / 'test_arkode_tstop',
            TEST_BUILD_DIR / 'test_arkode_reset',
        ]
        for exe in cls.exes:
            cls.assertTrue(exe.exists(), f'{exe} not built. Run "make tests" first.')

    def test_all_c_tests_pass(self):
        for exe in self.exes:
            result = subprocess.run([str(exe)], capture_output=True, text=True, timeout=30)
            self.assertEqual(result.returncode, 0, f'{exe} failed exit {result.returncode}\nstdout: {result.stdout}\nstderr: {result.stderr}')
            self.assertIn('passed', result.stdout.lower(), f'{exe} did not print "passed"\nstdout: {result.stdout}')
            self.assertNotIn('FAILED', result.stdout, f'{exe} printed "FAILED"\nstdout: {result.stdout}')
            self.assertNotIn('FAIL', result.stderr, f'{exe} printed "FAIL" in stderr')

if __name__ == '__main__':
    unittest.main()

