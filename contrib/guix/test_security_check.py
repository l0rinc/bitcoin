#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

import importlib.util
from pathlib import Path
import shutil
import subprocess
import tempfile
import textwrap
import unittest

try:
    import lief
except ImportError:
    lief = None


def load_security_check():
    path = Path(__file__).with_name('security-check.py')
    spec = importlib.util.spec_from_file_location('security_check', path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


@unittest.skipUnless(lief is not None and shutil.which('cc'), 'requires LIEF and a C compiler')
class SecurityCheckTest(unittest.TestCase):
    def compile(self, directory, name, source, flags):
        source_path = directory / f'{name}.c'
        binary_path = directory / name
        source_path.write_text(textwrap.dedent(source))
        subprocess.run([
            'cc', *flags, str(source_path), '-o', str(binary_path)
        ], check=True)
        return lief.parse(str(binary_path))

    def test_elf_control_flow_requires_ibt_property(self):
        security_check = load_security_check()
        with tempfile.TemporaryDirectory() as tempdir:
            directory = Path(tempdir)
            ibt = self.compile(
                directory,
                'ibt',
                'int main(void) { return 0; }',
                ['-O2', '-fcf-protection=branch'],
            )
            self.assertTrue(security_check.check_ELF_CONTROL_FLOW(ibt))

            manual_endbr = self.compile(
                directory,
                'manual-endbr',
                '''
                __attribute__((naked)) int main(void) {
                    __asm__("endbr64\\n\\txor %eax,%eax\\n\\tret");
                }
                ''',
                ['-O2', '-fcf-protection=none'],
            )
            self.assertFalse(security_check.check_ELF_CONTROL_FLOW(manual_endbr))

            shadow_stack_only = self.compile(
                directory,
                'shadow-stack-only',
                'int main(void) { return 0; }',
                ['-O2', '-fcf-protection=return'],
            )
            self.assertFalse(security_check.check_ELF_CONTROL_FLOW(shadow_stack_only))


if __name__ == '__main__':
    unittest.main()
