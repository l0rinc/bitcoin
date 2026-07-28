# Campaign #51 — differential-metamorphic

Base: audit/resurrection @ 5d0155254c (rotation ledger commit for #31 cycle-3).
Branch: audit/differential-metamorphic. Start state: clean (untracked scratch only).

## Cycle 1 (2026-07-28): ConnectBlock/DisconnectBlock apply-revert pair — oracle gap closed (47e5bf2f95)

### Draw
Random draw over the 45-goal eligible pool: raw=12735176910626288282,
index 24 -> #51.

### Pair selection / gap proof
Pair: apply/revert — ConnectBlock vs DisconnectBlock on the UTXO set.
Gap proof: ZERO DisconnectBlock callers in unit or fuzz tests
(grep across src/test, src/wallet/test, src/test/fuzz); functional
reorg tests (mempool_reorg.py, feature_reorg.py) assert balances and
mempool state, never raw UTXO-set equality. The only in-tree mention
of disconnect semantics was a comment in coins_tests.cpp:653.

### Domain / permitted differences
Domain: full UTXO set of the active chainstate after
ConnectBlock(B) + DisconnectBlock(B) for a valid B. Contract: exact
identity with the pre-connect set — the CTxUndo machinery restores
spent inputs exactly and the block's own outputs are removed exactly.
Permitted difference: none at the set level (tip hash alone is
insufficient as oracle).

### Test (47e5bf2f95)
validation_chainstatemanager_tests/
chainstatemanager_disconnect_restores_coins_exactly:
- mineBlocks(1) to mature the first coinbase; spend it (signed P2PK);
- snapshot the full set by flush + DB-cursor iteration (CCoinsViewCache
  cursors are disabled in this tree — ccoins_cache_cursor_unsupported
  documents the error — so the set must be flushed to the DB first);
- connect, disconnect, drain the disconnectpool (public clear(); its
  destructor asserts on non-empty; MaybeUpdateMempoolForReorg is
  protected);
- assert tip restoration AND element-wise set identity
  (outpoint/amount/script/height/coinbase flag).

### Oracle sensitivity (staged clean/mutation/repaired)
- clean: No errors detected.
- mutation: validation.cpp ApplyTxInUndo skipped (scratch edit):
  test FAILS with [101 != 100] size mismatch + element mismatch —
  the undone coinbase is precisely what the oracle pins.
- repaired (git checkout src/validation.cpp): suite green again,
  full validation_chainstatemanager_tests green.

### Verdict
- Oracle gap CLOSED; no production defect found (undo machinery is
  exact for this domain). BIP30 duplicate-coinbase paths noted as a
  distinct domain for a future cycle.

### Framework notes (for future cycle users)
- CCoinsViewCache::Cursor() always throws in this tree; flush + DB
  cursor is the enumeration path.
- DisconnectedBlockTransactions must be drained (clear() is public;
  MaybeUpdateMempoolForReorg is protected).

### Limitations
- Single-block, single-spend domain; multi-block reorgs with
  conflicting txs (BIP30 shape) not covered.
- The fork's prevoutfetchthreads option (visible in
  chainstatemanager_loadblockindex) does parallel prevout fetch on
  connect — the disconnect domain with fetched prevouts not
  differentially tested.

### Exact commands
- `cmake --build build-before -j4 --target test_bitcoin`
- `build-before/bin/test_bitcoin --run_test=validation_chainstatemanager_tests[...]`

### Next queue for this campaign
- BIP30 duplicate-coinbase disconnect domain (fClean/undo interplay).
- Fee-diagram incremental-vs-recompute differential (TxGraph diagram
  vs fresh replay — needs an instrumentation hook).

## Rotation note
One bounded cycle complete; rotating per uber-goal policy. Not exhausted.
