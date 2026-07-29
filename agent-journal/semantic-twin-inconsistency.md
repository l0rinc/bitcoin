# Campaign #106 — semantic-twin-inconsistency

## Cycle 1 (2026-07-29): hex-decode trailing-garbage twin map — strict-by-design for tx/PSBT, benign lax for block/header/proof

### Draw
Random draw over the 8-goal eligible pool (7 pending + 1 CYCLE-1,
#34 excluded as just-cycled): raw=4582464614074250662, index 6 ->
#106 (first cycle). Branch: audit/semantic-twin-inconsistency from
45cacd68b1 (#34 c2 bookkeeping; lineage anchor audit/resurrection @
5d0155254c). Start state: tracked-clean. Catalog note: #106's
campaign-focus block holds conformance-transplant text — same offset
artifact class as #49/#58/#80/#40/#42; title+slug authoritative.

### Cell selection
The #6 c2 probe (m6) observed verifytxoutproof accepting a proof
with trailing garbage while DecodeTx rejects it — recorded as a
tree-wide consistency note. This campaign completes the map: every
public hex/base64 decoder family, its trailing-garbage behavior,
and whether the difference is intentional.

### The map (code-read + functional proof)
STRICT (full-consumption enforced):
- DecodeHexTx (DecodeTx, core_io.cpp:154-228): both extended and
  legacy decode paths require ssData.empty() (:178/:195; comment
  :162) — strictness is load-bearing: full-consumption is how the
  witness-vs-legacy ambiguity is resolved. The fork adds an
  abort-gated round-trip oracle (:243-264) that even asserts
  trailing-data rejection in single-mode cases. Callers:
  testmempoolaccept, sendrawtransaction, decoderawtransaction,
  signrawtransactionwithkey, submitpackage, getblocktemplate
  (mining.cpp:396).
- DecodeBase64PSBT (node/psbt.cpp): rejects "extra data after PSBT"
  (error seen live in #80 c1's C-samples).
LAX (trailing bytes ignored):
- DecodeHexBlk (core_io.cpp:288-307): no consumption check.
  Callers: submitblock, getblocktemplate proposal.
- DecodeHexBlockHeader (core_io.cpp:269-286): no consumption check.
  Caller: submitheader.
- verifytxoutproof's CMerkleBlock deserialize (#6 c2 m6).

### Functional proof (regtest, /tmp/btc106_probe.py)
- submitblock(valid block + "deadbeefcafe") -> "duplicate" (decode
  succeeded with garbage appended; would be -22 on decode failure).
- testmempoolaccept(valid tx + "deadbeef") -> -22 "TX decode failed"
  (strict reject).
- submitheader(valid header + "deadbeef") -> null (accepted).
- Script preserved: /tmp/btc106_probe.py (2-phase, green).

### Classification / verdict
DISMISSED as a defect; recorded as the sloppiness map. The strict
twins are strict BY DESIGN (tx ambiguity resolution; explicit PSBT
check); the lax twins have no ambiguity and no caller contract
violation — garbage is ignored, the decoded object is identical,
behavior is upstream-matching (DecodeHexBlk/DecodeHexBlockHeader
are unmodified upstream code). No unification patch: tightening the
lax decoders would change observable RPC behavior (rejecting inputs
that succeed today) without an independently supported contract —
the campaign's "no security theater" rule and the minimal-diff rule
both say record, don't touch.

### Exact commands
- reads: core_io.cpp:154-307, rpc/mining.cpp:760-770/1105-1112/
  1150-1158, rpc/mempool.cpp:58
- python3 /tmp/btc106_probe.py --configfile=build-before/test/
  config.ini --tmpdir=/tmp/btc106

### Limitations / queue for cycle 2
- decodescript (ParseHex -> CScript) not probed: scripts are
  lengthless payloads, so "trailing" has no meaning there — noted
  for completeness, no experiment needed.
- Other semantic-twin families for future cells: merkle-root
  computation (BlockMerkleRoot vs BlockWitnessMerkleRoot vs test
  helpers), GetVirtualTransactionSize variants, sighash flag maps
  (SighashFromStr vs PSBT sighash parse).
- The map covers trailing-garbage only; other inconsistency axes
  (error-code choice, null-out behavior) unmapped.

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not
exhausted.

## Cycle 2 (2026-07-29): merkle-root twin differential — 6/6 multi-shape blocks agree; asymmetries classified

### Draw
Re-rank draw over the rebuilt 4-cell queue:
raw=2632070695870877747, index 3 -> #106 (second cycle; c1 queue
cell "merkle-root computation twins"). Branch:
audit/semantic-twin-c2 from 48fab216e5 (#60 c4 bookkeeping).

### Twin map
- C++ ComputeMerkleRoot (consensus/merkle.cpp:46-72): odd-level
  last-element duplication, SHA256D64 pairs, empty -> zero;
  optional CVE-2012-2459 mutation detection.
- C++ BlockMerkleRoot (:73-82) / BlockWitnessMerkleRoot (:84-93):
  the witness twin zeroes the coinbase leaf.
- Python test framework get_merkle_root (messages.py:823-831) /
  calc_witness_merkle_root: structurally identical pairing and
  zero-coinbase convention; NO mutation-detection feature
  (intentional asymmetry — the framework doesn't need it), and
  empty-vector -> IndexError (Python-lax; unreachable: blocks
  always have a coinbase).

### Differential proof (functional, /tmp/btc106_merkle.py)
Node-reported merkleroot (getblock) vs framework recomputation from
the same block's txids: 6/6 blocks agree across tx counts
{3,8,5,5,3,3} (odd/even/multi shapes), "Tests successful".
Harness lessons recorded: verbosity 1 for txid strings (v2 returns
decoded objects); generate(w, n) mines n BLOCKS (~1 tx each), so
multi-tx shapes need send_self_transfer + generate(1).

### Verdict
DISMISSED: the twins agree; the asymmetries are intentional
(mutation flag is C++-only) or unreachable (Python empty-vector).
No defect.

### Exact commands
- python3 /tmp/btc106_merkle.py --configfile=build-before/test/
  config.ini --tmpdir=/tmp/btc106m
- reads: consensus/merkle.cpp:46-93, messages.py:823-845

### Limitations / queue
- Differential is regtest-scale (6 blocks); the pairing logic is
  shape-exhaustive at these sizes, not at 4k-tx mainnet blocks —
  the structure is size-independent (same loop), noted.
- GetVirtualTransactionSize variants + sighash flag maps (c1 queue)
  remain open.

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not
exhausted.
