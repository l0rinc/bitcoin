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
    def compile(self, directory):
        source_path = directory / 'main.c'
        binary_path = directory / 'relro'
        source_path.write_text(textwrap.dedent('''
            int main(void) { return 0; }
        '''))
        subprocess.run([
            'cc', '-Wl,-z,relro,-z,now', str(source_path), '-o', str(binary_path)
        ], check=True)
        return binary_path

    def test_elf_relro_accepts_flags1_now(self):
        security_check = load_security_check()
        with tempfile.TemporaryDirectory() as tempdir:
            directory = Path(tempdir)
            binary_path = self.compile(directory)
            binary = lief.parse(str(binary_path))
            self.assertTrue(security_check.check_ELF_RELRO(binary))

            lazy_path = directory / 'relro-lazy'
            subprocess.run([
                'cc', '-Wl,-z,relro,-z,lazy', str(directory / 'main.c'), '-o', str(lazy_path)
            ], check=True)
            self.assertFalse(security_check.check_ELF_RELRO(lief.parse(str(lazy_path))))

            flags = binary.get(lief.ELF.DynamicEntry.TAG.FLAGS)
            flags.value = 0
            flags1 = binary.get(lief.ELF.DynamicEntry.TAG.FLAGS_1)
            self.assertTrue(flags1.has(lief.ELF.DynamicEntryFlags.FLAG.NOW))
            flags1_only_path = directory / 'relro-flags1-now'
            binary.write(str(flags1_only_path))
            flags1_only_path.chmod(0o755)
            subprocess.run([str(flags1_only_path)], check=True)

            flags1_only = lief.parse(str(flags1_only_path))
            self.assertEqual(flags1_only.get(lief.ELF.DynamicEntry.TAG.FLAGS).value, 0)
            self.assertTrue(security_check.check_ELF_RELRO(flags1_only))


if __name__ == '__main__':
    unittest.main()
