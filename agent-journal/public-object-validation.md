# Public object parsing and validation variant analysis

## Cycle 310: extended public-key metadata identity across ordered containers

### Selection, gate, and excluded cells

- Selector: `shuf -i 0-109 -n 1` -> `15` (`public-object-validation`)
- Branch: `uber-cycle-310-public-object-validation-20260802`
- Cycle-start/base HEAD: `199365c2793ca9ece8692dc65946864a5fe1d1d8`; selection commit `601460a135cebd0236bb2a4b99033fa61d2df2d8`; `origin/master` at `556988790a7f961693a8fd93f73725baea66476a`.
- Catalog SHA-256 at selection: `0f3f6c11ee008c76cb88250fdbc6f6abd713e72206f1c3b94daeb8e9983ed172`.
- Prior Goal 15 cells for compact recovery headers, Taproot x-only metadata, descriptor inference, PSBT global xpub wire identity, direct xpub rejection, and raw P2PK representation were searched and excluded. Direct Bech32/Base58 canonicalization and hybrid-key acceptance were also classified as intentional compatibility boundaries in this cycle.

### Hypothesis and contract

`CExtPubKey::operator==` compares depth, parent fingerprint, child number, chain code, and public key, while the ordered comparator in `src/pubkey.h` compared only public key and chain code. Consequently, two unequal public objects could become equivalent keys in every default `std::set` or `std::map`. The trust boundary is a descriptor or wallet caller supplying valid BIP32 public objects with distinct provenance metadata. The expected contract is that an API enumerating or pairing extended keys must not silently select one metadata-bearing xpub, and an xprv returned for an xpub must neuter back to that same object.

This reaches production aggregation directly. `Descriptor::GetPubKeys()` inserts root xpubs into `std::set<CExtPubKey>`. `wallet/rpc/wallet.cpp:gethdkeys` uses the resulting keys for `wallet_xpubs` and `wallet_xprvs`. If equivalent comparator keys occur in separate descriptors, the xpub map retains one representative while the xprv assignment can overwrite its value with the later key's depth/fingerprint/child metadata. The RPC can then emit an xpub and an xprv whose metadata do not match, and it can omit a distinct descriptor root. `PSBTXPubKeyComparator` is intentionally separate: PSBT global keys are unique by complete serialized xpub including version bytes; the generic `CExtPubKey::operator==` deliberately excludes version bytes.

### Independent applicability and before evidence

The temporary BIP32 test decoded the first BIP32 vector xpub, cloned it, and changed the clone to depth `1`, fingerprint `11 22 33 44`, and child `1`, leaving the valid public key and chain code unchanged. `operator==` correctly reported the objects unequal. The old comparator nevertheless made `std::set<CExtPubKey>` contain one object. The exact old focused command was:

`TMPDIR=/data/my_storage/tmp/cycle310-public-object /data/my_storage/tmp/cycle246-wallet/bin/test_bitcoin --run_test=bip32_tests/extpubkey_metadata_set_identity_probe --log_level=all`

The initial old-behavior probe passed with `keys.size() == 1`, `private_keys.size() == 1`, the stored private metadata equal to the second object, and descriptor extraction returning one xpub. After changing the regression expectations to the contract, the same old source exited with `keys.size() [1 != 2]` and a fatal `private_keys.size() [1 != 2]`. The old map behavior is the exact `wallet_xprvs[xpub] = CExtKey(xpub, key)` update pattern, not a synthetic comparator-only example.

The fixture also built `wsh(multi(2,xpub1,xpub2))` through the production descriptor parser, so distinct metadata is accepted at the public descriptor boundary. This is not invalid curve data, malformed BIP32 framing, or a consensus case; it is a valid public-object identity collision in API aggregation.

### Fix and verification

The comparator now orders public key, chain code, depth, fingerprint, and child number, making ordered-container equivalence consistent with `operator==`. Version bytes remain outside this comparator because they remain outside `operator==`; PSBT keeps its complete serialized-key comparator. The permanent `bip32_tests/extpubkey_metadata_identity` regression checks two set entries, two map entries, preservation of each map value's metadata, `CExtKey::Neuter()` matching the corresponding xpub, and two xpubs returned by `Descriptor::GetPubKeys()`.

- Repaired build: `TMPDIR=/data/my_storage/tmp/cycle310-public-object ninja -C /data/my_storage/tmp/cycle246-wallet test_bitcoin -j4`, exit `0`.
- Focused repaired test: `bip32_tests/extpubkey_metadata_identity`, all checks passed, `*** No errors detected`.
- Relevant repaired suites: `bip32_tests,descriptor_tests,psbt_tests,wallet_rpc_tests`, seed `31003`, 38 test cases selected, exit `0`, and `*** No errors detected`. Log: `/data/my_storage/tmp/cycle310-public-object/relevant-suites-final.log`.
- `git diff --check` passed. The first slower `-j1` build was stopped after it was clear that four-job compilation could use the dedicated scratch `TMPDIR`; no protected process was stopped.

### Verdict and handoff

Confirmed local public-object aggregation defect and fixed in the shared comparator contract. The impact is loss or mismatch of BIP32 provenance metadata in descriptor/wallet enumeration and xpub/xprv output; no consensus acceptance, private-key disclosure, or funds-loss primitive was established. The selected journal, source, and regression belong in one self-contained finding commit. Add a follow-up catalog goal for explicit extended-key identity matrices across descriptors, wallet RPC, migration, backup, and PSBT, then rebase and close the cycle state separately.

## Cycle 299: PSBT global xpub serialized-key identity

### Selection and gate

- Selector: `shuf -i 0-98 -n 1`
- Draw: `15`
- Slug: `public-object-validation`
- Branch: `uber-cycle-299-public-object-validation-20260802`
- Cycle-start HEAD: `f30c241b4348a5b72bd6401cab52b635f95d3ec6`; `origin/master` at `556988790a7f961693a8fd93f73725baea66476a`; merge-base `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; start divergence `45 1388` (`origin/master...HEAD`).
- The fixed catalog, random prompt, goals TSV, and uber protocol hashes at selection were `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`, `10408ad01c000bba65c1fff135cf2d7d92508bf8a8549141e3d6880f7fe0d4ec`, `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`, and `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`.
- Protected test processes `777094`, `956381`, `1138182`, `1157959`, `1312049`, `1312050`, and `1346200` were preserved and not touched. Earlier Goal 15 cells for full-key descriptor inference, x-only leaf inference, Taproot lookup, compact recovery headers, and Taproot PSBT x-only metadata were searched and excluded.

### Contract and independent applicability check

The candidate is the global PSBT xpub map. BIP-174 defines `PSBT_GLOBAL_XPUB` key data as the complete 78-byte serialized BIP32 extended public key, and its duplicate-key rule applies to the complete key, not only the derived public point or chain code: https://github.com/bitcoin/bips/blob/master/bip-0174.mediawiki. Therefore two records that differ in version bytes, depth, fingerprint, child number, chain code, or public key are distinct wire keys when all other PSBT framing is valid.

The implementation used `std::set<CExtPubKey>` for `PartiallySignedTransaction::m_xpubs`. `CExtPubKey::operator<` compared only `pubkey` and `chaincode`, while `EncodeWithVersion()` deliberately retained version bytes so a decoded xpub could be serialized with the same representation. The set therefore collapsed distinct PSBT keys before serialization. The global duplicate-key lookup correctly rejects an identical `<keytype,keydata>` vector, so this was not a duplicate-key acceptance issue; it was loss of a distinct key.

### Reproducer and verdict

The deterministic unit fixture created a valid PSBTv2 global map with two xpub records sharing depth, origin, child, chain code, and public key, but using `0488b21e` and `049d7cb2` version bytes. Both records have the same valid HD keypath value. The pre-fix command was:

`TMPDIR=/data/my_storage/tmp/cycle299-xpub /data/my_storage/tmp/cycle246-wallet/bin/test_bitcoin --run_test=psbt_tests/global_xpub_versions_roundtrip --log_level=all --random=29901`

The pre-fix build succeeded after correcting the fixture's C-array size expression, and the test exited 201 with `check SerializePSBT(*decoded) == serialized has failed`. This is a self-contained proof that parsing accepted the wire data but reserialization lost or changed a record.

### Fix and regression oracle

`PSBTXPubKeyComparator` now orders xpubs by all fields represented by the serialized 78-byte key, including version bytes. The PSBT xpub map and its duplicate-tracking set use this comparator; descriptor and wallet sets retain their existing derivation-material comparator semantics. The RPC decoder loop was changed to hold the map entry by const reference and pass a const fingerprint span.

The fixed build `ninja -C /data/my_storage/tmp/cycle246-wallet test_bitcoin bitcoin_node -j2` exited 0. The focused fixed test with seed `29902` exited 0, asserting one origin entry, two retained xpubs, and exact serialized round-trip equality. The complete `psbt_tests` suite with seed `29903` passed all 15 cases with no errors. `git diff --check` passed. The original six-parameter generalization remains queued for a subsequent independent test rather than being assumed from the version-only witness.

### Verdict and handoff

Confirmed local PSBT parser/serializer correctness defect with public, caller-supplied metadata as the trust boundary. The impact is loss of a valid distinct global xpub record during decode/merge/re-encode, not consensus acceptance or private-key exposure. Commit the source, regression, and this journal entry together. After that commit, extend the catalog with a learned serialized-key identity campaign, rebase the cycle branch onto the freshly fetched `origin/master`, close the uber state separately, and draw from the expanded catalog. Remaining Goal 15 queue: direct address/key decoding variants, public wrapper representation parity, release-version parser differences, and PSBT key identity fields beyond version bytes.

## Cycle 168: direct address/key decoding and wrapper validation follow-up

### Selection and gate

- Selector: `shuf -i 0-98 -n 1`
- Draw: `15`
- Slug: `public-object-validation`
- Branch: `uber-cycle-168-public-object-validation-20260730`
- Gate HEAD: `74f50d5012f425ba0bc7f07d397270286422836d`; `origin/master` at `67efced1fc83a0b7215cc1513e7c4754fee0f12f`; merge-base `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; `origin/master...HEAD=42 1117`.
- Catalog SHA256: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`
- Uber protocol SHA256: `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`
- Corrected goals TSV SHA256: `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`
- The fresh gate passed fetch, tracked/index cleanliness, `git diff --check`, catalog/protocol/TSV hashes, and preservation of unrelated PIDs `777094` and `956381`. Cycles 27, 51, 75, and 166 were searched before accepting this draw; their full-key inference, x-only leaf inference, Taproot lookup, and compact-header cells are excluded.

### Scope and active hypothesis queue

This cycle continues goal 15 only for direct public-object decoding and wrapper boundaries that were not closed by the prior descriptor and compact-signature campaigns. The trust boundary is caller-supplied public bytes or text; the output contract includes validity, canonicalization, error reporting, and safe reuse of an output object. I will trace each candidate through its producer, parser, serializer, consumer, tests, history, and release-version behavior before treating a mismatch as a finding.

Initial queue:

1. Direct `HexToPubKey`, `XOnlyPubKey`, address, and destination decoding variants, with malformed/truncated/noncanonical inputs and reused outputs.
2. Public RPC/API wrappers that translate decoded keys, scripts, signatures, or addresses and may accept one representation while emitting another that its corresponding parser rejects.
3. Release-to-release parser differences for public objects, only after current direct paths are mapped and prior key-I/O/parser cells are excluded.

Status: confirmed and fixed. Cycle 168 closed the Taproot PSBT x-only metadata validation cell; direct address/key decoding and release-version parser differences remain queued.

## Cycle 168: Taproot PSBT x-only metadata must be curve-valid

### Contract and source/history evidence

The direct parser map is a caller-supplied PSBT boundary. The affected fields are public-key metadata, not arbitrary witness-program bytes:

- `PSBT_IN_TAP_SCRIPT_SIG` carries an x-only public key plus a leaf hash in its key data.
- `PSBT_IN_TAP_BIP32_DERIVATION` carries an x-only public key in its key data.
- `PSBT_IN_TAP_INTERNAL_KEY` carries an x-only public key in its value.
- The output map has the corresponding internal-key and BIP32-derivation fields.

BIP371 (https://bips.dev/371/) describes each of these values as an x-only public key. BIP340 (https://github.com/bitcoin/bips/blob/master/bip-0340.mediawiki) defines the public-key domain through `lift_x`: the x coordinate must be below the field modulus and have a valid square root. `XOnlyPubKey::IsFullyValid()` is the existing libsecp256k1-backed implementation of that contract.

Ordinary PSBT public-key and BIP32 fields already call `CPubKey::IsFullyValid()`. MuSig2 PSBT deserialization was strengthened by `d221d1c633` after invalid curve points caused downstream failures. In contrast, all five Taproot x-only sites constructed or deserialized `XOnlyPubKey` values and inserted them into maps without checking curve membership. This is distinct from a v1 witness output program: BIP341 intentionally permits arbitrary 32-byte output programs, while these BIP371 fields explicitly represent public keys.

The trust boundary is an untrusted or stale PSBT supplied through raw decoding, RPC, wallet import, or another PSBT consumer. The old parser accepted an all-zero x-only value, then `PSBTInput::FillSignatureData()` exposed it through `taproot_misc_pubkeys` and `tap_pubkeys`; output filling exposed the same value to signing-provider metadata. The result was an internally stored object that was not a BIP340 public key. No consensus acceptance or funds-loss primitive was established; this is a parser/API correctness defect with downstream failure and resource/DoS risk for consumers that attempt to use the invalid metadata.

### Reproducer and verification

The focused unit probe serializes each malformed field through the real `SpanReader` map parser, using all-zero x-only bytes and otherwise valid PSBTv2 framing. Before the production edit, the test asserted successful parsing and passed 8 assertions. The accepted cases were:

1. input Taproot script-path signature key;
2. input Taproot BIP32 derivation key;
3. input Taproot internal key value;
4. output Taproot internal key value; and
5. output Taproot BIP32 derivation key.

The pre-fix build and result are preserved in `/data/my_storage/tmp/cycle168-pref_fix_build4.log` and `/data/my_storage/tmp/cycle168-pref_fix_test4.log`. The first probe fixture omitted the required map separator and was discarded; the second used PSBTv2 without required transaction fields and was discarded. Neither exercised production behavior as a result.

### Fix and regression oracle

`src/psbt.h` now rejects an x-only value unless `IsFullyValid()` succeeds at each of the five deserialization sites. The fixed regression expects `std::ios_base::failure` for each malformed field. Existing value-boundary tests were updated to use `XOnlyPubKey::NUMS_H` so they still reach their intended nested-value length checks instead of failing earlier on the new public-key contract.

The normal build `CCACHE_DIR=/data/my_storage/tmp/cycle168-ccache cmake --build /data/my_storage/tmp/cycle89-build --target test_bitcoin -j2` exited 0. The focused regression passed 5 assertions (`/data/my_storage/tmp/cycle168-post_fix_test.log`), and the full `psbt_tests` suite passed 14 cases and 149 assertions (`/data/my_storage/tmp/cycle168-psbt_suite2.log`). An independent Clang 19 UBSan/alignment/object-size build rebuilt the affected targets successfully and passed the complete PSBT suite with 14 cases and 149 assertions; logs are `/data/my_storage/tmp/cycle168-ubsan-build.log` and `/data/my_storage/tmp/cycle168-ubsan-test.log`. `git diff --check` passed.

The complete 1,229-case normal unit suite was attempted twice but could not provide a repository result: the shared root filesystem reached the test harness disk guard and block-file flush then failed with an I/O error. The overlapping retry was stopped after discovering the first invocation was still running; only those cycle-168 processes were stopped. Generated `/tmp` test scratch was removed with no active descriptors. Persistent PIDs `777094` and `956381` were preserved.

### Verdict and handoff

Confirmed local public-object parser contract defect. Commit the source, regression, and this journal entry as one self-contained change, then close the cycle with a separate state-only uber-goal commit. Do not reopen this Taproot x-only metadata cell without a distinct field, caller, backend, or recurrence signal. Continue with direct address/key decoding, malformed public API wrappers, and release-version parser differences.

## Cycle 166: compact recovery header domain and public-key parser parity

### Selection and gate

- Selector: `shuf -i 0-98 -n 1`
- Draw: `15`
- Slug: `public-object-validation`
- Branch: `uber-cycle-166-public-object-validation-20260730`
- Gate HEAD: `2691ebfa27984989a1f19ed390ce79e4cfd4ba41`
- Base: `origin/master` at `67efced1fc83a0b7215cc1513e7c4754fee0f12`; merge-base `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; `origin/master...HEAD=1113 42`.
- Catalog SHA256: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`
- Uber protocol SHA256: `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`
- Corrected goals TSV SHA256: `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`
- The fresh gate passed with tracked source clean. The prior full-key P2PK inference (`c125061aa4`), tracked x-only/taproot leaf inference (`a81f58f3bf`), and Taproot compressed-key lookup (`d02e22867b`) were searched and excluded as already closed cells.

### Scope and active hypothesis

This cycle targets direct public-object parser and API-wrapper boundaries rather than descriptor inference. The primary hypothesis is that `CPubKey::RecoverCompact()` accepts compact-signature recovery headers outside the documented `27..34` domain because it derives the recovery ID and compression bit with masked arithmetic instead of validating the header first. `CKey::SignCompact()` emits only `27..34`, and the compact format's header contract is therefore a finite eight-value domain. An out-of-domain header that recovers a key would be a noncanonical parser acceptance and cross-wrapper consistency defect; an out-of-domain header that fails would establish only the negative contract.

The trust boundary is a caller-supplied 65-byte compact signature, including its first header byte. The output-state contract is that failure invalidates a reused `CPubKey`, while success produces a fully valid key whose compression state reflects the header. I will compare `SignCompact()` output, `RecoverCompact()`, message-signing RPC callers, history, and tests, then use a deterministic valid signature with each header class and an independent libsecp/reference check. Address, `HexToPubKey()`, and direct `DecodeDestination()` paths remain secondary queue cells unless this hypothesis is dismissed.

Status: confirmed; the compact recovery-header cell produced a source finding and regression commit in this cycle.

### Source/history contract and confirmed finding

The public `CKey::SignCompact()` contract in `src/key.h` defines one header byte with values `0x1B` through `0x1E`, plus `0x04` for compressed keys. Its implementation writes exactly `27 + rec + (fCompressed ? 4 : 0)`, so the complete producer domain is `27..34`. `CPubKey::RecoverCompact()` had kept the recovery ID and compression bit with `& 3` and `& 4` but did not first enforce that domain. The expression is unchanged since `d2e74c55bdd` (2014), and the same parser is used by `MessageVerify()` in `src/common/signmessage.cpp`.

A deterministic scratch probe used one valid compact signature and tried every first-byte value from `0` through `255`. Before the change, headers such as `0`, `3`, `4`, `7`, `8`, `11`, and their repetitions every eight values recovered the same valid public keys as canonical headers. After the change, only `27`, `28`, `31`, and `32` were accepted for that signature, which are exactly its valid recovery/compression combinations. The before/after outputs are preserved at `/data/my_storage/tmp/cycle166-recover-header-probe.log` and `/data/my_storage/tmp/cycle166-recover-header-probe-after.log`.

This is a confirmed public parser differential. A caller supplying a 65-byte compact signature could provide a noncanonical header that `RecoverCompact()` and the message-verification wrapper treated as equivalent to a canonical signature. The fix rejects headers below `27` or above `34` before deriving masked fields and uses the existing failure path, which invalidates a reused `CPubKey`. This is a format-validation correction, not a consensus-validation change; the underlying 64-byte signature and recovery ID remain handled by libsecp256k1 exactly as before.

### Regression and mutation evidence

- Corrected TSAN build: `cmake --build /data/my_storage/tmp/cycle163-tsan --target test_bitcoin -j2`, exit 0. Build log: `/data/my_storage/tmp/cycle166-restored-build.log`.
- Corrected `key_tests`: 15 cases and 897 assertions passed with seed `166003`; the same new test also passed as a one-case filter with seed `166006`. Logs: `/data/my_storage/tmp/cycle166-key-suite.log` and `/data/my_storage/tmp/cycle166-restored-key.log`.
- Corrected `util_tests`: 80 cases and 3,994 assertions passed with seed `166002`; the new `MessageVerify()` test also passed as a one-case filter with seed `166007`. Logs: `/data/my_storage/tmp/cycle166-util-suite.log` and `/data/my_storage/tmp/cycle166-restored-util.log`.
- Mutation proof removed only the new header range check. The key regression then failed because the noncanonical header recovered a valid key (2 of 5 assertions failed, exit 201), and the message-wrapper regression failed because the old path returned the successful/non-error result (exit 201). Logs: `/data/my_storage/tmp/cycle166-mutated-key.log` and `/data/my_storage/tmp/cycle166-mutated-util.log`.
- An earlier attempted parallel run used a nonexistent `TMPDIR` and shared stateful test setup. It failed during harness initialization with `filesystem_error` and was classified as an invalid test run, not a product result. The affected processes were interrupted, the directory was created, and all recorded validation above was rerun serially.

### Handoff

Commit `src/pubkey.cpp`, the focused key and message-verification regressions, and this journal entry together with the required author. Then close the cycle with a state-only uber-goal commit. The remaining queue is direct address/key decoding, malformed API-wrapper inputs, and release-version parser differences; do not reopen the compact-header cell without a new contract or regression signal.

## Cycle 51: taproot inference must reject off-curve x-only leaf keys

### Selection and gate

- Selector: `shuf -i 0-98 -n 1`
- Draw: `15`
- Slug: `public-object-validation`
- Branch: `fuzz-contract-cluster-oracles-20260709`
- Gate HEAD: `7fab5aba5a3fc152c56217460fdfb44beec508c3`
- Base: `origin/master` at `7dea464d6b51a69bd99a0451be8aaf3a26313eb6`; merge-base `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; `origin/master...HEAD=2 868`.
- Catalog SHA256: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`
- Uber protocol SHA256: `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`
- Corrected goals TSV SHA256: `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`
- The gate fetch passed, tracked and staged state was clean, and no relevant process was running. The prior cycle-27 P2PK inference finding and commit `c125061aa4` were searched before selecting this distinct x-only/taproot leaf cell.

### Contract and path map

`InferDescriptor(script, provider)` must return a descriptor whose public string can be parsed by `Parse()` and serialized back without changing the inferred contract. When a script tree contains a byte-level tapscript that cannot be represented by a structured descriptor, inference must fall back to a descriptor that preserves the output script rather than constructing an invalid `pk(...)` key provider.

The relevant validity domain is a 32-byte x-only value. `XOnlyPubKey::IsFullyValid()` is the authoritative curve-membership check. `ParsePubkeyInner()` already applies the same check to 32-byte keys in P2TR context, so `InferXOnlyPubkey()` must not emit a provider for an off-curve value. The direct 34-byte tapscript branch in `InferScript()` must also avoid wrapping a null provider in `PKDescriptor`.

### Minimal reproducer and confirmed finding

The deterministic test builds a tracked taproot tree with:

```text
leaf = <32 zero bytes> OP_CHECKSIG
internal key = XOnlyPubKey::NUMS_H
```

`TaprootBuilder` accepts the byte-level leaf and computes a valid output key. The provider stores that tree, then `InferDescriptor()` reconstructs the output. Before the fix, it produced:

```text
tr(50929b74c1a04954b78b4b6035e97a5e078a5a0f28ec96d547bfee9ace803ac0,pk(0000000000000000000000000000000000000000000000000000000000000000))#vcy6jk8v
```

Feeding this public output back into `Parse()` failed with:

```text
pk(): Pubkey '0000000000000000000000000000000000000000000000000000000000000000' is invalid
```

This is a confirmed local cross-path contract defect. The output of inference was not accepted by the corresponding public parser. It is distinct from cycle 27: that fix covered full P2PK keys, while this path uses `InferXOnlyPubkey()` for a tracked tapscript leaf.

### Fix and mutation evidence

The final source change does two things:

1. `InferXOnlyPubkey()` returns null unless `xkey.IsFullyValid()` succeeds.
2. The direct P2TR leaf branch only constructs `PKDescriptor` when the provider is non-null; otherwise inference continues to the existing fallback path.

The first implementation attempt added only the validity guard. The regression then exposed the caller contract: the direct branch wrapped the null provider and the focused test segfaulted with status 139. Adding the null-aware branch fixed that second defect and produced the intended `rawtr(...)` fallback.

The final regression test asserts that the descriptor starts with `rawtr(`, parses successfully, and has an identical serialized descriptor after parsing. Mutation proof removed only the new `IsFullyValid()` guard while retaining the null-aware caller. The focused test exited 201: the `rawtr(` assertion failed and parsing reported the original `tr(...,pk(0000...))` invalid-key error. The raw mutation log is `/data/my_storage/tmp/cycle51-taproot-mutation.log`; the pre-fix log is `/data/my_storage/tmp/cycle51-taproot-before-fix.log`.

### Validation

- Build: `env TMPDIR=/data/my_storage/tmp/cycle51-taproot cmake --build build_unit_clang19 --target test_bitcoin -j2`; final build exited 0.
- Focused fixed regression: `descriptor_tests/infer_descriptor_taproot_off_curve_leaf_roundtrips`; 1 case, 4 assertions passed. Log: `/data/my_storage/tmp/cycle51-taproot-after-restore.log`.
- Full descriptor suite: 12 cases, 30,419 assertions passed. Log: `/data/my_storage/tmp/cycle51-descriptor-suite.log`.
- Adjacent script control: `script_tests/sign_invalid_miniscript`; 1 case, 1 assertion passed. Log: `/data/my_storage/tmp/cycle51-script-adjacent.log`.
- Adjacent miniscript suite: 3 cases, 10,692 assertions passed. Log: `/data/my_storage/tmp/cycle51-miniscript-suite.log`.
- Rebuilt ASan/UBSan fuzzer: `cmake --build build_fuzz_asan_clang19 --target fuzz -j2`; then `FUZZ=script` with `ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1` over `/data/my_storage/tmp/qa-assets/fuzz_corpora/script`. All 2,537 corpus units completed with `#2537 DONE`, coverage 14,887, feature count 43,263, peak RSS 794 MB, and no sanitizer diagnostic or crash artifact. Log: `/data/my_storage/tmp/cycle51-script-asan.log`; slow-unit files are performance artifacts only.
- `git diff --check` passed. No source, test, fuzz, sanitizer, daemon, or profiling process remains running.

### Handoff

Commit the source/test change with the exact author required by the uber protocol, then close this x-only/taproot cell. The broader goal remains eligible for other parser variants: direct invalid taproot output keys, nested `multi_a`/miniscript key paths, wrapper/binding parity, and future malformed-input recurrence. Do not reopen the P2PK or this tracked-leaf cell without a new caller, parser mode, backend, or regression signal.

## Cycle 27: descriptor inference must round-trip invalid public keys

### Selection and gate

- Selector: `shuf -i 0-98 -n 1`
- Draw: `15`
- Slug: `public-object-validation`
- Branch: `fuzz-contract-cluster-oracles-20260709`
- Base: `origin/master` at `7dea464d6b51a69bd99a0451be8aaf3a26313eb6`
- HEAD at the cycle gate: `ceb274f236881febe70aabeb534bde7f5d8babfd`
- HEAD after the source fix: `c125061aa4`
- Catalog SHA256: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`
- Uber protocol SHA256: `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`
- Gate: `git fetch origin master --quiet` passed; `origin/master...HEAD` was `2 822`; tracked source was clean; no source, test, fuzz, sanitizer, daemon, or profiling process was running.
- Agent-owned journals/catalog files and the pre-existing untracked `test/cache/` were left untouched.

### Scope and contract map

The selected goal compares equivalent public-object parsing and validation paths, with emphasis on malformed, noncanonical, impossible, and mixed-format keys. The first pass covered `CPubKey`, x-only keys, signatures, address/key I/O, scripts, descriptors, PSBT public-key storage, and wrapper/fuzz entry points.

The key contract distinction is intentional in consensus code:

- `CPubKey::IsValid()` checks serialized size and header shape only. `src/pubkey.h` documents that it does not check curve membership, and `CPubKey::Verify()` subsequently parses the key with libsecp256k1 before verification.
- `CPubKey::IsFullyValid()` checks curve membership with `secp256k1_ec_pubkey_parse`.
- Descriptor parsing already requires `IsFullyValid()` in `ParsePubkeyInner`, because a `pk(...)` descriptor must contain a usable curve point.
- Script inference used `InferPubkey()` to create the same `pk(...)` provider but only rejected hybrid encodings with `IsValidNonHybrid()`. It could therefore serialize a syntactically shaped, off-curve key into a descriptor that its own parser rejected.
- The public header for `InferDescriptor()` states that inference falls back to `raw()` when a more specific descriptor cannot be produced. A raw descriptor preserves an otherwise valid script byte-for-byte without asserting that its pushed bytes are a usable public key.

Other reviewed paths were not changed:

- `ExtractDestination()` intentionally uses syntactic validity for P2PK because that is the consensus script classification, and P2PK has no address destination.
- PSBT public-key insertion validates with `IsFullyValid()` before updating its provider map.
- Key I/O clears error locations at entry and existing address/key tests cover malformed base58 and bech32 inputs.
- Signature DER and descriptor checksum regression coverage already exercises prior parser-contract bugs.
- `InferXOnlyPubkey()` and taproot inference are a distinct adjacent cell. It was not changed in this cycle because the confirmed reproducer and minimal fix are for full public keys in P2PK inference; x-only validity and all taproot callers remain in the next queue.

### Fixed corpus and discovery evidence

A deterministic scratch corpus was created outside the repository at `/data/my_storage/tmp/public-object-validation-cycle27/`:

- `key_io/`: empty, valid and invalid Base58, valid/uppercase/noncanonical Bech32, and a short secret;
- `descriptor/`: valid `pk`/`tr`, truncated and hybrid keys, and a bad checksum;
- `script/`: fixed script-text cases;
- `signature/`: empty, short, and malformed DER bytes.

The existing and selected fuzz targets accepted these inputs without sanitizer diagnostics:

- Clang 19 ASan/UBSan libFuzzer: `key_io`, `descriptor_parse`, `parse_script`, and `ecdsa_signature_parse_der_lax`, each with a fixed seed and `-runs=1`;
- GCC 12 ASan/UBSan custom driver: the same four targets, using the driver's input-file interface.

The Clang unit command below passed before the production edit with 46 cases and 34,570 assertions:

```text
build_unit_clang19/bin/test_bitcoin --run_test=key_io_tests,base58_tests,bech32_tests,bip32_tests,descriptor_tests,key_tests,outputtype_tests --log_level=test_suite --report_level=short
```

The sanitizer version of that parser slice also passed before the edit with the same 46 cases and 34,570 assertions. These runs established the broad parser baseline; the defect required a cross-path round-trip oracle rather than a crash oracle.

### Confirmed finding

The following minimal script contains a 33-byte compressed-looking key whose x coordinate is all zero and is not a curve point:

```text
02 0000000000000000000000000000000000000000000000000000000000000000 OP_CHECKSIG
```

Before the fix, a temporary descriptor test constructed this P2PK script, called `InferDescriptor()`, then fed `ToString()` back into `Parse()`. The test failed with exit code 201:

```text
inferred descriptor: pk(020000000000000000000000000000000000000000000000000000000000000000)#rxgq8p4q
Parse error: pk(): Pubkey '020000000000000000000000000000000000000000000000000000000000000000' is invalid
```

This is a confirmed local contract defect. The output of a public inference API was not parseable by the corresponding public descriptor parser, and the invalid key was reachable from a script that the script classifier intentionally treats as P2PK based on its serialized shape. The failure is not a consensus acceptance change: consensus verification still uses the existing full libsecp256k1 parse.

### Fix and regression oracle

Commit `c125061aa4` (`descriptor: keep inferred invalid pubkeys parseable`) adds an `IsFullyValid()` check at the start of `InferPubkey()`. If the key is not a curve point, `InferScript()` continues to its existing raw-script fallback. The new test `descriptor_tests/infer_descriptor_off_curve_pubkey_falls_back_to_raw` asserts all of the following:

1. inference returns a descriptor;
2. the descriptor begins with `raw(`;
3. parsing the inferred descriptor succeeds; and
4. the parsed descriptor serializes back to the identical descriptor string.

The test is sensitive to the old behavior: before the three-line guard, the same test produced the `pk(...)` descriptor and failed during `Parse()`.

### Validation

Normal Clang 19 build and tests:

- `cmake --build build_unit_clang19 --target test_bitcoin -j$(nproc)`: exit 0;
- focused regression: 1 case, 4 assertions passed;
- `build_unit_clang19/bin/test_bitcoin --run_test=descriptor_tests --log_level=test_suite --report_level=short`: 11 cases, 30,415 assertions passed.

Clang 19 ASan/LSan/UBSan build and tests:

- `cmake --build /data/my_storage/tmp/sanitizer-analysis-matrix-cycle26/asan-unit-clang --target test_bitcoin -j$(nproc)`: exit 0;
- focused regression with `ASAN_OPTIONS=detect_leaks=1:halt_on_error=1` and the repository UBSan suppression/options: 1 case, 4 assertions passed;
- full `descriptor_tests` under the same options: 11 cases, 30,415 assertions passed;
- no ASan, LSan, UBSan, runtime-error, or leak diagnostic appeared.

The pre-fix failure, post-fix focused result, normal full-suite result, and sanitizer full-suite result are the independent behavioral and runtime evidence for the source change. `git diff --check` and the staged diff check passed. No source, test, fuzz, sanitizer, daemon, or profiling process remains running.

### Dismissed and adjacent candidates

- P2PK destination extraction using `IsValid()` is intentional consensus classification, not a descriptor round-trip defect.
- PSBT public-key validation and key I/O output/error-state behavior showed no new discrepancy in this cycle.
- Hybrid public keys already fall back to `raw()` and have an existing regression test.
- x-only/taproot inference may have a related validity predicate mismatch. It needs its own caller map and a P2TR round-trip corpus before any change; no finding is claimed yet.

### Handoff

Reopen this goal for the x-only/taproot cell, descriptor inference inside nested script contexts, public-key wrapper/binding parity, or a new malformed-input recurrence. Start by searching this journal and commit `c125061aa4` so the P2PK off-curve recurrence is not rediscovered. The next uber-goal cycle must re-check branch, base/HEAD, dirty state, processes, this ledger, the catalog, existing journals, history, issues, pull requests, and review precedent before drawing a distinct eligible goal.

## Cycle 75: nested public-key parse/serialize and wrapper parity

### Selection and gate

- Selector: `shuf -i 0-98 -n 1`
- Draw: `15`
- Slug: `public-object-validation`
- Branch: `uber-cycle-75-public-object-validation-20260728`
- Gate HEAD: `df9c8d76c7ad57713c656e25a503c4b7b7d01440`
- Base: `origin/master` at `7dea464d6b51a69bd99a0451be8aaf3a26313eb6`
- Catalog SHA256: `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`
- Uber protocol SHA256: `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`
- Corrected goals TSV SHA256: `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`
- The cycle-74 gate passed; tracked source was clean apart from the known journal/catalog artifacts, and no relevant process was running.

### Excluded cells and active scope

Cycle 27's full-key P2PK inference defect and cycle 51's x-only/taproot inference defect are excluded, as are the already-reviewed PSBT, key-I/O, DER, and direct descriptor parser cells. This cycle targets nested `multi_a`/multisig/miniscript public-key parse and serialize paths plus C++ wrapper/output parity. The first pass will compare parser acceptance, descriptor serialization, script reconstruction, failure-state outputs, and any public wrapper or binding that exposes the same objects.

### Active hypothesis

A nested descriptor or wrapper path may accept a public-key representation that its corresponding serializer or consumer cannot parse, may normalize equivalent keys inconsistently, or may leave caller-visible output partially populated after a malformed nested key fails. A finding requires a concrete malformed or noncanonical input, an independently established contract, and a round-trip/output-state reproducer; syntactic differences that are explicitly allowed by descriptor policy are not findings.

Status: active; no source hypothesis is confirmed yet.

## Cycle 75 Completion: Taproot compressed-key private lookup parity

### Contract and discovery

The nested parser map showed that `ParsePubkeyInner()` accepts full compressed public keys in `ParseScriptContext::P2TR`. `multi_a` and Tapscript then serialize those keys as 32-byte x-only pushes, so the key's parity is intentionally not part of the consumer script. `SigningProvider::GetKeyByXOnly()` already searches both parity key IDs. Before this cycle, `ConstPubkeyProvider` used `m_xonly` for both descriptor spelling and private-key lookup, so a full compressed key kept its spelling and incorrectly used only its parity-specific `CKeyID`.

The public descriptor contract supports compressed public keys and x-only keys inside `tr()`, while BIP340 defines x-only keys by their curve x coordinate. The relevant source paths are `src/script/descriptor.cpp:337`, `src/script/descriptor.cpp:1964`, `src/script/signingprovider.h:186`, and the nested Miniscript `KeyParser` path beginning at `src/script/descriptor.cpp:2279`. The exact contract is: preserve the accepted full-key spelling for serialization, but use parity-insensitive x-only lookup whenever the key is consumed in Taproot.

### Confirmed finding

Using secret scalar `2`, the provider contained the even compressed public key while the descriptor used the odd compressed encoding for the same x-only point. These two descriptors expand to the same Tapscript:

```text
tr(<x-only internal key>,multi_a(1,<odd compressed key>))
tr(<x-only internal key>,multi_a(1,<x-only key>))
```

Before the fix, the full compressed form reported `HavePrivateKeys() == false` and `ToPrivateString()` failed even though the equivalent x-only form succeeded. The same mismatch appeared through nested `and_v(v:multi_a(...),after(1))`. This was a confirmed local consumer/private-key discovery defect: a valid accepted Taproot descriptor could not discover the private key required for its parity-insensitive script key. A temporary restoration of the old implementation reproduced four failed assertions and exited with status 201:

```text
descriptor_tests/taproot_compressed_key_uses_xonly_private_key_lookup
MUTATION_STATUS=201
```

### Fix

`ConstPubkeyProvider` now stores separate flags for public descriptor spelling and private-key lookup. Full compressed keys parsed in P2TR retain `m_xonly == false` for stable serialization, but set `m_xonly_lookup == true` so `GetKeyByXOnly()` can find either parity. Existing x-only and WIF Taproot paths retain their previous behavior, and `Clone()` preserves both flags.

The regression covers direct `multi_a`, x-only and full-key equivalence, NUMS internal keys, nested Miniscript, private-string conversion, private-key discovery, and identical expanded scripts. It uses an opposite-parity provider key so a parity-specific lookup cannot pass accidentally.

### Validation

- Normal Clang 19 build: `cmake --build build_unit_clang19 --target test_bitcoin -j2`; exit 0.
- Focused normal regression: `build_unit_clang19/bin/test_bitcoin --run_test=descriptor_tests/taproot_compressed_key_uses_xonly_private_key_lookup --log_level=message`; exit 0.
- Full normal `descriptor_tests`: 13 cases passed with no errors.
- Full normal `miniscript_tests`: 3 cases passed with no errors.
- Clang 19 TSan build: `cmake --build build_unit_tsan_clang19 --target test_bitcoin -j2`; 219/219 build steps passed.
- Combined TSan descriptor and Miniscript suites: 16 cases passed with no errors.
- Clang 19 ASan/UBSan libFuzzer build: `cmake --build build_fuzz_asan_clang19 --target fuzz -j2`; 173/173 build steps passed.
- `FUZZ=descriptor_parse` over 10 deterministic malformed/valid corpus files: 11 runs, no sanitizer diagnostic.
- `FUZZ=miniscript_string` over 5 deterministic malformed/valid corpus files: 6 runs, no sanitizer diagnostic.
- Negative control: restored pre-fix lookup failed with 4 assertions and status 201; the fixed source was rebuilt and the focused regression passed again.
- `git diff --check` passed. No source, fuzz, sanitizer, daemon, or profiling process remains running.

### Dismissed and limitations

No additional wrapper or FFI defect was found in the reviewed public-key paths. Full uncompressed keys remain disallowed in Taproot by the existing parser policy. MuSig aggregate-provider handling and external language bindings were mapped but were not changed because they have separate contracts and no independent failing reproducer in this cycle. The fuzz run was a deterministic smoke over the selected corpus, not a coverage-complete campaign.

### Handoff

Commit the source/test and journal update as one independent finding. Keep this exact parity case closed unless a new caller, serializer, wrapper, backend, or recurrence appears. The broader goal remains eligible for distinct malformed-input, API-wrapper, or release-version cells; the next uber cycle must re-check the gate and draw from the full catalog.

## Cycle 303: direct xpub rejection and intentional P2PK representation

### Selection and scope

The exact post-Cycle-302 draw was `shuf -i 0-102 -n 1` -> goal `15`,
`public-object-validation`, on branch
`uber-cycle-303-public-object-validation-20260802`. The cycle-start HEAD was
`9f9184f137b66e20962b4b6042dc548bdd47c597`, with `origin/master` at
`556988790a7f961693a8fd93f73725baea66476a`. The previous PSBT serialized-key
identity finding, Taproot x-only metadata validation, compact-header, and
descriptor-inference cells were excluded from this cycle.

The first hypothesis was that direct BIP32 extended-public-key decoding might
accept a structurally valid Base58Check xpub whose compressed public key is
off-curve. The second was that syntactic P2PK extraction might normalize or
lose such a public key when converting between script and destination objects.

### Evidence and verdicts

`CExtPubKey::Decode()` stores the serialized fields, then clears the public key
unless `pubkey.IsFullyValid()` succeeds. `DecodeExtPubKey()` and the PSBT global
xpub path both use this decoder and independently require full validity. A
temporary `bip32_tests` probe changed the compressed-key prefix and zeroed its
coordinate in a valid test vector; the rebuilt focused test passed, proving the
malformed xpub is rejected. The probe was removed before cycle close.

`Solver` and `ExtractDestination()` intentionally use syntactic `CPubKey`
validity for a P2PK script. A temporary `script_standard_tests` probe built a
33-byte compressed off-curve key followed by `OP_CHECKSIG`. It passed all five
checks: P2PK classification succeeds, `IsValid()` is true, `IsFullyValid()` is
false, `GetScriptForDestination()` reproduces the original script, and
`EncodeDestination()` is empty because P2PK destinations are not addresses.
The probe was removed before cycle close. This is consensus-preserving raw
script representation, not a parser/serializer mismatch; no source fix is
warranted.

The xpub probe initially failed only because its scratch `TMPDIR` did not
exist. After creating `/data/my_storage/tmp/cycle303-p2pk-probe`, the focused
test passed with no errors. The P2PK probe used the same isolated directory and
also passed with no errors. The modified `test_bitcoin` target rebuilt cleanly
in `/data/my_storage/tmp/cycle246-wallet`; `git diff --check` passed after both
temporary probes were removed. Evidence is limited to the Linux RelWithDebInfo
build and source/history inspection; Windows, other architectures, and a
sanitized run were not needed for this dismissed representation hypothesis.

### Learned adjacent risk

The constructor `XOnlyPubKey(const CPubKey&)` in `src/pubkey.h` takes
`std::span{pubkey}.subspan(1, 32)` without an error channel. A default or
short `CPubKey` therefore violates the span precondition, while a syntactically
valid off-curve key can produce an x-only object that is not fully valid. Most
production callers are preceded by full key validation, but descriptor,
signing, provider, benchmark, test, and fuzz call sites need an explicit map.
The adjacent wallet path also deserves separate review: `LoadKey()` can use
`fSkipCheck=true` after validating a stored `Hash(pubkey, privkey)`, then
`FillableSigningProvider::AddKeyPubKey()` stores the supplied identity without
re-deriving it. Existing history makes this optimization intentional, so this
cycle does not claim a wallet defect.

### Handoff

Add Goal 103, `xonly-cpubkey-preconditions`, to audit every
`XOnlyPubKey(CPubKey)` caller for empty, short, syntactically valid, and
off-curve inputs. Preserve raw script consensus semantics; change code only for
a proven unsafe caller or add a precise contract test. Keep the wallet
keypair-association observation as a lower-priority adjacent queue item.
