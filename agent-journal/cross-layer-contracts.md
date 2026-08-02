# Campaign #31 — cross-layer-contracts

Base: audit/resurrection @ 098a1942a9 (rotation ledger commit for #30).
Branch: audit/cross-layer-contracts. Start state: clean (untracked scratch only).

## Cycle 1 (2026-07-27): descriptors.md default wildcard range off by one — CONFIRMED + doc fix

### Candidates evaluated
1. compressor.h size-tier comment ("up to 121 bytes require 1 byte ...
   16505 bytes require 2") vs encoding — DISMISSED. ScriptCompression::Ser
   uses the custom base-128 VarInt (serialize.h:495, WriteVarInt
   serialize.h:429), not CompactSize. Rechecking the arithmetic: 1-byte tier
   covers nSize <= 127 -> script <= 121 ✓; 2-byte tier covers nSize <= 16511
   ((n>>7)-1 <= 0x7F) -> script <= 16505 ✓. The comment is exactly right;
   my initial CompactSize assumption was the error.
2. rpcauth/rpcwhitelist permission layers (doc/JSON-RPC-interface.md,
   --help, rpcauth.py, httprpc.cpp parsing) — DISMISSED. Implementation
   rejects any third rpcauth field (httprpc.cpp:304), help/doc/generator
   all consistent; rpcwhitelistdefault semantics match GetBoolArg default
   (httprpc.cpp:315).
3. pruneassumevalid contract layers — PARKED as lead: the feature exists
   only on `master` (6eb57cf954 et al.), not in the audit/resurrection base;
   code-level audit requires checking out master. Noted for the fork-focused
   campaigns.
4. -prune / -maxmempool / rpcwhitelist help-vs-impl — spot-checked,
   consistent (help text generated from the same constants).
5. descriptors.md wildcard default range — CONTRADICTION FOUND (below).

### Finding: doc/descriptors.md:200 claims default range "0-1000, inclusive"
Contract table for the ranged-descriptor wildcard (`/*`) default range:
| layer | claim/behavior |
|---|---|
| doc/descriptors.md:200 | "configurable range (by default `0-1000`, inclusive)" — 1001 keys, from eb494125624 (2018, pre-importdescriptors) |
| importdescriptors impl (backup.cpp:181-185) | no "range" -> warning "Range not given, using default keypool range", range_end = m_keypool_size (exclusive) |
| bounds check (backup.cpp:192) | next_index >= range_end rejected -> end exclusive |
| keypool constant (scriptpubkeyman.h:64) | DEFAULT_KEYPOOL_SIZE = 1000 -> indices 0..999 |
| deriveaddresses RPC | no default: errors "Range must be specified for a ranged descriptor" |
| functional tests | encode the warning; no inclusive-1000 behavior |

Authoritative layer: implementation + keypool constant + RPC text (three
layers agree); the doc line is a stale copied claim. Off by one: doc says
1001 default keys, reality is 1000.

### Behavioral proof (scratch regtest, datadir removed after)
- importdesc wpkh(tpub.../0/*) no range -> success + keypool-range warning
- deriveaddresses [999,999] -> getaddressinfo: ismine True
- deriveaddresses [1000,1000] -> getaddressinfo: ismine False
- deriveaddresses with no range -> error "Range must be specified..."

### Fix (commit 1f7f73b02e)
One word: "inclusive" -> "exclusive" in doc/descriptors.md:200.

### Verdict
- CONFIRMED doc-vs-implementation contradiction (user-facing; a user
  relying on the doc would believe index 1000 is watched by default).
  FIXED doc-side; no code change warranted (1000-key default is the
  intended, consistently-implemented behavior).

### Exact commands
- `python3 -c` (test_framework.extendedkey) to mint xpub + descsum
- `./build-before/bin/bitcoind -regtest -datadir=/tmp/btc_regtest_31 -daemon -server -fallbackfee=0.00001`
- `bitcoin-cli -regtest ... createwallet w31wo true ...` / importdescriptors / deriveaddresses / getaddressinfo

### Limitations / leads
- Only the wildcard-range claim was contract-tabled this cycle; other
  descriptor doc claims (checksum algorithm, hardened `h` vs `'`, multipath
  `<0;1>` semantics) not yet cross-checked.
- The same stale-claim pattern (2018 doc lines) may hide elsewhere in
  doc/descriptors.md — e.g. the Examples section claims vs current parser.

### Next queue for this campaign
- gettxoutsetinfo / coinstatsindex hash_type option names across help,
  doc, release notes, implementation.
- Descriptors: verify the doc Examples section parses identically today
  (each example through getdescriptorinfo).

## Cycle 2 (2026-07-28): assumeutxo snapshot-load contract table — doc layer incomplete, closed

### Feature: assumeutxo snapshot loading (loadtxoutset)
Contract table for the load-time activation rules:

| rule | doc/assumeutxo.md | doc/design | RPC help | impl | tests |
|---|---|---|---|---|---|
| content checked vs hardcoded hash | ✓ (line 11) | — | ✓ ("always checked by hash") | ✓ (PopulateAndValidateSnapshot) | ✓ |
| base blockhash in hardcoded list | implied only | — | implied only | ✓ validation.cpp:5823-5829 | ✓ feature_assumeutxo.py:154 |
| height recognized (PopulateAndValidate) | — | — | — | ✓ validation.cpp:6000-6005 | ✓ |
| mempool must be empty | ✗ | ✗ | ✗ | ✓ validation.cpp:5846-5849 | ✓ :243-251 |
| no second snapshot activation | ✗ | ✗ | ✗ | ✓ validation.cpp:5820-5822 | ✓ |
| base header in headers chain / not invalidated | ✗ | ✗ | ✗ | ✓ validation.cpp:5831-5844 | ✓ :265-277 |
| work exceeds active chainstate | ✗ | ✗ | ✗ | ✓ validation.cpp:6012 | ✓ :261-263 |
| network/magic/version match | ✗ | ✗ | ✗ | ✓ | ✓ :122-145 |
| regtest hardcoded entries exist (110/200/299) | — | — | — | ✓ chainparams.cpp:620-640 | ✓ |

Layers: implementation and functional tests fully encode the rules;
doc/assumeutxo.md presented loading as unconditional ("Once you've
obtained the snapshot, you can use the RPC command loadtxoutset to load
it"); doc/design/assumeutxo.md and the RPC help are likewise silent on
rejections. Users could only learn the rules from runtime error strings.
Also verified: the "checked against a hash hardcoded in source code"
claim is TRUE (height/blockhash gating + content-hash comparison), not
just on mainnet — regtest carries its own test entries (110, 200, 299),
and AssumeutxoForHeight/ForBlockhash rejects anything else there.

### Fix (commit f7513f4990)
Added a "Loading is rejected (with an explanatory RPC error) when:"
list to doc/assumeutxo.md's Loading section covering the four
activation rule families (hash not recognized; mempool non-empty;
second activation; base-header/work rules). File-format rules
(magic/version/network) left out as less user-actionable.

### Verdict
- CONFIRMED doc-layer contract gap (no contradiction between impl and
  tests — they agree; the docs were the incomplete layer).
  FIXED doc-side with layer attributions above.

### Exact commands
- `grep -n -B3 -A25 "ActivateSnapshot" src/validation.cpp` (rules cited inline)
- `grep -n "assert_raises_rpc_error" test/functional/feature_assumeutxo.py`
- `sed -n '616,640p' src/kernel/chainparams.cpp` (regtest entries)

### Next queue for this campaign
- Descriptors: verify the doc Examples section parses identically today
  (each example through getdescriptorinfo).

## Cycle 3 (2026-07-28): gettxoutsetinfo use_index silently ignored for default hash_type — CONFIRMED + help fix

### Feature: gettxoutsetinfo hash_type / use_index
Contract table:
| layer | claim/behavior |
|---|---|
| RPC help hash_type (1030) | "'hash_serialized_3' (the legacy algorithm), 'muhash', 'none'" — matches ParseHashType (980-991) exactly; invalid rejected with -8 ✓ |
| release notes 26.0 | hash_serialized_2 removed, replaced by hash_serialized_3 (correctly calculated) ✓ help omits it |
| RPC help use_index (1036, pre-fix) | "Use coinstatsindex, if available." — no restriction stated |
| implementation (GetUTXOStats, 1004-1005) | index consulted ONLY for MUHASH/NONE; HASH_SERIALIZED always full-computes |
| RPCResult schema (1047-1048) | 'transactions'/'disk_size' "(not available when coinstatsindex is used)" — acknowledges index/non-index split ✓ |

Mismatch: with BOTH defaults (hash_type=hash_serialized_3, use_index=true)
the index is silently bypassed; the slow full computation runs while the
help implies the index serves the call. User-facing performance/expectation
gap. The "legacy algorithm" phrasing for hash_serialized_3 is defensible
(it is the older serialization scheme) — recorded, not changed (campaign:
no prose-only rewrites).

### Fix (commit 9396f0b414)
use_index description now: "Use coinstatsindex, if available. Only used
for 'muhash' and 'none' hash types." — matches implementation and the
result-schema convention.

### Verification
- rpc_tests (includes RPCHelpMan consistency check): `*** No errors detected`.

### Verdict
- CONFIRMED help-vs-implementation gap. FIXED at the help layer; no
  behavior change (implementation is the authoritative layer — the
  index physically cannot serve serialized_3).

### Next queue for this campaign
- Descriptors: verify the doc Examples section parses identically today
  (each example through getdescriptorinfo).

## Rotation note
Three bounded cycles complete; rotating per uber-goal policy. Not exhausted.

## Cycle 4 (2026-07-28): descriptors.md examples — 2/21 fail on mainnet (unlabeled testnet keys) — CONFIRMED + doc fix (b2a0c38154)

### Draw
Random draw over the 36-goal eligible pool: raw=12417538186002149333,
index 9 -> #31. Executed the c3 queue item (verify the doc Examples
section parses identically today).

### Method (full extraction, not sampling)
Extracted all 22 backticked strings from the Examples section
(doc/descriptors.md:29-52); one is the prose policy placeholder
`thresh(4,pk(key_1),...)` (key_1..4/t1..3 placeholders — not a real
example, excluded). 21 real examples through getdescriptorinfo on
scratch nodes: regtest, mainnet, signet.

### Results (chain context is the whole story)
- regtest: 6 xpub examples reject (xpub is mainnet-only — chain
  context proof that getdescriptorinfo validates network, not drift).
- mainnet: 19/21 parse. The two multipath examples
  (wsh(sortedmulti) line 49, wsh(thresh) line 50) use TESTNET tpub
  keys and fail with '-5: Multi: key 'tpub...' is not valid' — while
  every earlier example uses mainnet xpub. The doc labels
  mainnet-vs-testnet formats for ADDRESSES but never for these keys.
- signet: both tpub examples parse (they are testnet examples,
  simply unlabeled).
- tr(musig(...)) example parses (musig supported in this tree).
- Upstream master carries the identical unlabeled text (verified) —
  upstream-wide gap.

### Verdict
- CONFIRMED cross-layer drift (doc example vs chain context,
  unlabeled). Severity: documentation (user-facing confusion, no
  parsing/safety impact). FIXED with one clarifying sentence per
  example matching the doc's own convention (b2a0c38154).

### Limitations
- getdescriptorinfo validation is offline-parse level; address
  derivation (multi-* index ranges, e.g. i up to 1000) not expanded.
- The Reference section (lines 53+) prose claims not re-audited.

### Next queue for this campaign
- doc/wallet.md + doc/external-signer.md same extraction treatment
  (any backticked descriptor examples there).

## Rotation note
Four bounded cycles complete; rotating per uber-goal policy. Not exhausted.

## Cycle 5 (2026-08-02, draw 206, raw=13014474695973678266, masked 3791102659118902458, idx 9/27): doc/wallet.md + doc/external-signer.md extraction — zero falsely-parseable descriptor examples; DISMISSED

### Extraction
- doc/wallet.md: ABSENT in this fork (wallet docs reorganized
  upstream; recorded, not a gap).
- doc/external-signer.md (226 lines): all descriptor-ish strings
  extracted and classified:
  - :141 hypothetical wildcard form ['pkh("44'/0'/$'/{0,1}/*")...']
    — explicitly framed as a FUTURE extension ('A future
    extension could add... Perhaps'), not a current-format
    example; its non-standard syntax is therefore not doc drift.
  - :174-183 getdescriptors examples: elided keys (xpub6C...,
    xpub6B....) — illustrative by design, unparseable by
    construction, chain-consistent (main/test labeled at :198's
    tpub displayaddress example).
- No example presents itself as parseable-but-isn't, and no
  unlabeled chain-context drift of the c4 (testnet-keys) class.

### Verdict
DISMISSED: the docs' remaining descriptor surface is honestly
elided/hypothetical; nothing for getdescriptorinfo to refute.
The c4 extraction methodology found no second instance.

### Exact commands
- grep extractions above (patterns + line refs); ls doc/wallet.md
  (absent).

### Limitations / queue
- The hypothetical wildcard syntax could confuse a reader who
  tries it — but it is inside an explicit 'future' paragraph;
  flagged only if the format ever ships.
- descriptors.md Reference prose (c4 queue) remains the only
  open surface.
