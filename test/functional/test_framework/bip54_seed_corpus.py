#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Unit tests for the BIP54 fuzz seed converter."""

from contextlib import redirect_stdout
from copy import deepcopy
import hashlib
import importlib.util
from io import BytesIO, StringIO
import json
from pathlib import Path
import sys
from tempfile import TemporaryDirectory
import unittest
from unittest.mock import patch


# Load the converter by path. Importing it as contrib.devtools.* would instead
# require prepending the repo root to sys.path from inside this package, which
# every functional test imports, and would rely on contrib/ being an implicit
# namespace package.
SOURCE_DIR = Path(__file__).resolve().parents[3]
_spec = importlib.util.spec_from_file_location(
    "bip54_seed_corpus", SOURCE_DIR / "contrib" / "devtools" / "bip54_seed_corpus.py")
bip54_seed_corpus = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(bip54_seed_corpus)
from test_framework.messages import (  # noqa: E402
    COutPoint,
    CTransaction,
    CTxIn,
    CTxOut,
    deser_string,
)
from test_framework.script import (  # noqa: E402
    CScript,
    OP_1,
    OP_CHECKMULTISIG,
    OP_CHECKSIG,
    OP_DUP,
    OP_EQUAL,
    OP_HASH160,
    OP_TRUE,
    hash160,
)
from test_framework.util import assert_equal  # noqa: E402


class Bip54SeedCorpusTest(unittest.TestCase):
    def setUp(self):
        self.bare_script_sig = CScript([OP_1, OP_CHECKSIG])
        self.bare_spent_script = CScript([OP_CHECKMULTISIG])
        self.redeem_script = CScript([OP_CHECKSIG, OP_CHECKMULTISIG])
        self.p2sh_prefix = CScript([OP_1])
        self.p2sh_script_sig = bytes(self.p2sh_prefix) + bytes(CScript([self.redeem_script]))
        self.p2sh_spent_script = CScript([OP_HASH160, hash160(self.redeem_script), OP_EQUAL])

        tx = CTransaction()
        tx.vin = [
            CTxIn(COutPoint(1, 0), self.bare_script_sig),
            CTxIn(COutPoint(2, 0), self.p2sh_script_sig),
        ]
        tx.vout = [CTxOut(0, CScript([OP_TRUE]))]
        self.vector = {
            "tx": tx.serialize().hex(),
            "spent_outputs": [
                CTxOut(0, self.bare_spent_script).serialize().hex(),
                CTxOut(0, self.p2sh_spent_script).serialize().hex(),
            ],
            "valid": True,
        }

    def test_encoding(self):
        fuzz_input = bip54_seed_corpus.vector_to_fuzz_input(self.vector)
        reader = BytesIO(fuzz_input)

        assert_equal(int.from_bytes(reader.read(2), "little"), 2)
        assert_equal(deser_string(reader), self.bare_script_sig)
        assert_equal(deser_string(reader), self.bare_spent_script)
        assert_equal(deser_string(reader), self.p2sh_script_sig)
        assert_equal(deser_string(reader), self.p2sh_spent_script)
        assert_equal(reader.read(), b"")

    def test_unusual_p2sh_script_sigs(self):
        redeem_script = CScript([OP_CHECKSIG] * 2_501)
        script_sigs = (
            bytes([OP_1]),
            bytes([OP_CHECKSIG, 0x4c]),
            bytes(CScript([OP_DUP])) + bytes(CScript([redeem_script])),
        )
        spent_script = CScript([OP_HASH160, hash160(redeem_script), OP_EQUAL])
        for script_sig in script_sigs:
            tx = CTransaction()
            tx.vin = [CTxIn(COutPoint(1, 0), script_sig)]
            tx.vout = [CTxOut(0, CScript([OP_TRUE]))]
            vector = {
                "tx": tx.serialize().hex(),
                "spent_outputs": [CTxOut(0, spent_script).serialize().hex()],
                "valid": True,
            }

            reader = BytesIO(bip54_seed_corpus.vector_to_fuzz_input(vector))
            assert_equal(int.from_bytes(reader.read(2), "little"), 1)
            assert_equal(deser_string(reader), script_sig)
            assert_equal(deser_string(reader), spent_script)
            assert_equal(reader.read(), b"")

    def test_deterministic_output(self):
        fuzz_input = bip54_seed_corpus.vector_to_fuzz_input(self.vector)
        digest = hashlib.sha256(fuzz_input).hexdigest()
        with TemporaryDirectory() as tmpdir:
            vector_path = Path(tmpdir) / "vectors.json"
            output_dir = Path(tmpdir) / "corpus"
            vector_path.write_text(json.dumps([self.vector, self.vector]), encoding="utf8")
            with (
                patch.object(sys, "argv", ["bip54_seed_corpus.py", str(vector_path), str(output_dir)]),
                redirect_stdout(StringIO()) as stdout,
            ):
                bip54_seed_corpus.main()

            assert_equal([path.name for path in output_dir.iterdir()], [digest])
            assert_equal((output_dir / digest).read_bytes(), fuzz_input)
            assert_equal(stdout.getvalue(), "Wrote 1 unique seeds from 2 vectors\n")

    def test_reject_mismatched_inputs(self):
        missing_output = deepcopy(self.vector)
        missing_output["spent_outputs"].pop()
        with self.assertRaisesRegex(ValueError, "2 inputs but vector has 1 spent outputs"):
            bip54_seed_corpus.vector_to_fuzz_input(missing_output)
