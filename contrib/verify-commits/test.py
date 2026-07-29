#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

import importlib.util
from pathlib import Path
import subprocess
import sys
import unittest
from unittest.mock import patch


VERIFY_COMMITS = Path(__file__).with_name('verify-commits.py')


def load_verify_module():
    spec = importlib.util.spec_from_file_location('verify_commits', VERIFY_COMMITS)
    assert spec is not None and spec.loader is not None
    verify = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(verify)
    return verify


class VerifyCommitsTest(unittest.TestCase):
    def test_ancestor_statuses(self):
        verify = load_verify_module()
        with patch.object(verify.subprocess, 'run') as run:
            run.return_value.returncode = 0
            self.assertTrue(verify.is_ancestor('root', 'descendant'))
            run.return_value.returncode = 1
            self.assertFalse(verify.is_ancestor('root', 'unrelated'))

    def test_ancestor_errors_fail_closed(self):
        verify = load_verify_module()
        with patch.object(verify.subprocess, 'run') as run:
            run.return_value.returncode = 128
            with self.assertRaises(SystemExit) as exit_info:
                verify.is_ancestor('root', 'missing')
        self.assertEqual(exit_info.exception.code, 1)

    def test_missing_revision_fails_closed(self):
        result = subprocess.run(
            [sys.executable, str(VERIFY_COMMITS), 'this-object-does-not-exist'],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('failed with status 128', result.stderr)


if __name__ == '__main__':
    unittest.main()
