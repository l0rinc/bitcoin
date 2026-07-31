# Alternative-implementation compatibility audit

## Cycle 202 selection and gate

- Exact selector: `shuf -i 0-98 -n 1` -> `55` (`alternative-implementation`); no reroll was needed. The immediately preceding Goal 55 cell, Cycle 197 raw transaction and CompactBlock serialization, is explicitly closed and is not reopened. The dedicated branch is `uber-cycle-202-alternative-implementation-20260731`.
- Gate timestamp: `2026-07-31T09:42:41Z`. Start HEAD: `f5e14920c7935f71906c89f178973de13b2d9351`; `origin/master`: `67efced1fc83a0b7215cc1513e7c4754fee0f12f`; merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; direct start divergence `1194 42` from `git rev-list --left-right --count HEAD...origin/master`.
- `git fetch origin master` passed. The tracked/index gate was clean, `git diff --check` passed, and `src/net_processing.cpp` had no residual mutation. Known unrelated untracked artifacts and protected PIDs `777094`, `956381`, `1138182`, and `1157959` were preserved. Pre-cycle uber-state SHA-256: `6d9ab4501cd8d90bc4f7c347d1de5d78f0423faeea4ab6198108d8c854f30da0`.
- Authoritative catalog, prompt, corrected TSV, and uber protocol hashes were `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`, `10408ad01c000bba65c1fff135cf2d7d92508bf8a8549141e3d6880f7fe0d4ec`, `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`, and `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`.

This cycle targets complex Miniscript descriptor compilation and derivation across Core and the pinned Embit snapshot: nested `wsh`/`sh(wsh)`, Taproot script trees, `multi_a` and threshold-like branches, key ordering, wrapper combinations, and CSV/CLTV boundaries. Cycle 155's supported `pkh`, `wpkh`, wrapped SegWit, simple Taproot-keypath, CompactSize/PSBT, descriptor-network, and ADDRv2 cells remain closed. Rust/Go source contracts will be used where executable toolchains are unavailable; Embit's pure-Python implementation and Core's deterministic RPC/test binaries provide the direct execution forms.

### Hypothesis and plan

The falsifiable hypothesis is that a complex, supported descriptor can produce a different witness/redeem script or derived output between Core and an alternative implementation while both accept the descriptor and appear syntactically valid. Such a difference could strand wallet funds or make watch-only/imported descriptors observe the wrong outputs. The trust boundary is descriptor parsing/checksum normalization, key derivation, Miniscript compilation, Taproot leaf/branch hashing, network encoding, and Core's `deriveaddresses` output.

1. Pin and inspect the exact Core, Embit, and available rust-miniscript source contracts; classify unsupported or invalid Miniscript before comparing output.
2. Generate deterministic complex descriptors from fixed public keys and extended keys, including WSH and Taproot trees, and compare Core's derived addresses plus scriptPubKeys with Embit's compiled scripts and addresses across branch/index values.
3. Exercise wrapper and numeric boundaries for `older`/`after`, empty/singleton trees, sorted versus unsorted multisig, duplicate keys, and multipath branches. Preserve exact descriptors, checksums, outputs, and hashes.
4. If a current Core mismatch is reproduced, minimize it and add the narrowest regression. Otherwise close as intentional policy, unsupported reference behavior, invalid-domain behavior, or inconclusive tool evidence with a precise next queue.

## Cycle 202 result: complex Miniscript descriptor differential

### Source inventory and execution boundary

The direct Core oracle was the isolated `/data/my_storage/tmp/cycle202-alt-desc` regtest node, using `/data/my_storage/tmp/cycle84-build/bin/bitcoind` and `bitcoin-cli` version `31.99.0`; it was started with a scratch datadir, RPC port `18602`, no listening P2P socket, and was stopped after the probes. The comparison source was Embit `/data/my_storage/tmp/cycle155-embit`, HEAD `fff7ffa43f6ce088c5ba22cb3877a122bf01dc96` (`2026-06-01`, `bump version and add CHANGELOG.md`). The current Core checkout was at Cycle 202 start HEAD `f5e14920c7935f71906c89f178973de13b2d9351`. The host still has no `rustc`, `cargo`, or `go`, so the pinned rust-bitcoin and btcd trees remained source-only references.

Core's descriptor documentation identifies `wsh()` and `tr()` Miniscript as BIP 379-constrained expressions (`doc/descriptors.md:27,72`). The relevant implementation checks were `src/script/miniscript.cpp:143-205` for fragment typing, `src/script/miniscript.h:1502-1555` and `1735-1747` for recursive duplicate-key detection, and `src/script/descriptor.cpp:1965-1972` for the P2WSH/P2TR compressed-key restriction. The Embit parser has equivalent local type checks in `src/embit/descriptor/miniscript.py:329-335` and `502-511`, but no corresponding recursive duplicate-key check was found in its key/miniscript descriptor path (`src/embit/descriptor/arguments.py:182-197`).

The first fixed-key comparison accidentally compared Embit's CompactSize-prefixed `Script.serialize()` result with Core's raw `scriptPubKey` field. That setup-only mismatch was discarded. The normalized comparison removed the one-byte length prefix before comparing, and all six fixed cases then matched exactly.

### Fixed-key WSH and Taproot matrix

The deterministic compressed keys were:

- `P1 = 03e7d285b4817f83f724cd29394da75dfc84fe639ed147a944e7e6064703b14130`
- `P2 = 0250863ad64a87ae8a2fe83c1af1a8403cb53f53e486d8511dad8a04887e5b2352`
- `P3 = 03b8fa5d5959fa4027ccbf0736a86ccde4242e3051ea363437b4ff0d52598d7cec`

For each descriptor, Core `getdescriptorinfo` plus `deriveaddresses` and `validateaddress` were compared with Embit's `Descriptor.from_string(...).derive(0)`, `address(NETWORKS["regtest"])`, and compiled script. Every row matched in both address and raw scriptPubKey:

| Descriptor cell | Address | Raw scriptPubKey |
| --- | --- | --- |
| `wsh(or_d(pk(P1),pkh(P2)))` | `bcrt1qwqkr2dgk4q7xnpcnm0cg3zhr6glpnxqpudryf3ertf7mgqujdpkqll556m` | `0020702c353516a83c698713dbf0888ae3d23e199801e34644c7235a7db40392686c` |
| `wsh(and_v(v:pk(P1),or_d(pk(P2),older(12960))))` | `bcrt1qu9hj0yv5d7juutul4d9zqzmg02256gc4r3jzlf0hpmvzf3fajncs9g5ev5` | `0020e16f2791946fa5ce2f9fab4a200b687a954d23151c642fa5f70ed824c53d94f1` |
| `wsh(sortedmulti(2,P1,P2,P3))` | `bcrt1qat48925u8glnsfhed3xejjq200ucy5lp8t407e6j9crzdklqqp0sfjgdc2` | `0020eaea72aa9c3a3f3826f96c4d99480a7bf98253e13aeaff67522e0626dbe0005f` |
| `tr(P1,pk(P2))` | `bcrt1prvfm6n6nrfuyg23gultwcgv5lhzfpycekfcw6j9adg8rw9tmtdmqkgh4va` | `51201b13bd4f531a78442a28e7d6ec2194fdc4909319b270ed48bd6a0e37157b5b76` |
| `tr(P1,{pk(P2),pk(P3)})` | `bcrt1pvwzs5afkve9hv7r9e965d2f0f47rfpt0ldfq2jfw7xt8wztf9vgqztppaj` | `512063850a7536664b767865c97546a92f4d7c34856ffb5205492ef1967709692b10` |
| `tr(P1,{and_v(v:multi_a(2,P2,P3),older(3)),multi_a(2,P2,P3)})` | `bcrt1pjurph9faxyn548kvfa72aylv2lgqyyfksyagpg74du3k9nyh457qsh4q47` | `512097061b953d31274a9ecc4f7cae93ec57d0021136813a80a3d56f2362cc97ad3c` |

### Ranged extended-key matrix

To test actual derivation rather than only fixed keys, three valid regtest `tpub` values from the pinned Embit Taproot tests were used with `/0/*`. Core was called with range `[0,2]`; Embit derived indexes 0, 1, and 2. All nine address/script pairs matched.

| Descriptor family | Index 0 | Index 1 | Index 2 |
| --- | --- | --- | --- |
| `wsh(sortedmulti(2,tpub1/0/*,tpub2/0/*,tpub3/0/*))` | `bcrt1qknk2kmx748xf6qj9na4wv8wr686276wufacllpy2y8m078pu09vq6yrhde` / `0020b4ecab6cdea9cc9d02459f6ae61dc3d1f4af69dc4f71ff848a21f6ff1c3c7958` | `bcrt1qfuty7gwkmsd9evupzfdgfu0x4qf7c7er63p4rktnlfx6ns350ftsjad6a7` / `00204f164f21d6dc1a5cb381125a84f1e6a813ec7b23d44351d973fa4da9c2347a57` | `bcrt1q75yva8uy533pcltwr6qxl9c9vyv6ktewj7f6ahmepq5nd9wsl52qgx3saq` / `0020f508ce9f84a4621c7d6e1e806f97056119ab2f2e9793aedf7908293695d0fd14` |
| `wsh(or_d(pk(tpub1/0/*),and_v(v:pkh(tpub2/0/*),older(6))))` | `bcrt1qc44yv2sugauwd3fm62642d79e40s9e6chuspwa6895grqsmjq6nqa8v67f` / `0020c56a462a1c4778e6c53bd2b55537c5cd5f02e758bf201777472d1030437206a6` | `bcrt1qg9mp58xmls39qwhspdppndtlnsnsj5n6ykxf208mj7emdywgxhesu70kpl` / `002041761a1cdbfc22503af00b4219b57f9c2709527a258c953cfb97b3b691c835f3` | `bcrt1qhfgt9fsvx86lxav8j2au2f23287u2wwj0ec4lalxccnxyk7yyceskfqjsm` / `0020ba50b2a60c31f5f3758792bbc525151fdc539d27e715ff7e6c626625bc42633` |
| `tr(tpub1/0/*,{and_v(v:multi_a(2,tpub2/0/*,tpub3/0/*),older(3)),multi_a(2,tpub2/0/*,tpub3/0/*)})` | `bcrt1pxh4hdawtn85lfcvq45clgnmdxvfw5g67595c4jy5tsnas776duuqdr8he7` / `512035eb76f5cb99e9f4e180ad31f44f6d3312ea235ea1698ac8945c27d87bda6f38` | `bcrt1pxy3gv98clx3aputkcw862twqh3xrh7wc0dy4nzy5ele4wautpfks4nwtel` / `512031228614f8f9a3d0f176c38fa52dc0bc4c3bf9d87b49598894cff357778b0a6d` | `bcrt1pup50llxltt20ut847cs5lcwxgqyu757qkv8vau5qf3qfysuj3zuqr7wgmj` / `5120e068fffcdf5ad4fe2cf5f6214fe1c64009cf53c0b30ecef2804c4092439288b8` |

The three exact tpubs were `tpubD6NzVbkrYhZ4Y18xhod7E8V6Sy3YF36bge8HJb4ww1QgTrdkNvCEzcvUmFGQkTJA32gqr3j94iE8vsUzYpv8Pn29JezD9YiYnxgUREhN3QR`, `tpubD6NzVbkrYhZ4YPAbyf6urxqqnmJF79PzQtyERAmvkSVS9fweCTjxjDh22Z5St9fGb1a5DUCv8G27nYupKP1Ctr1pkamJossoetzws1moNRn`, and `tpubD6NzVbkrYhZ4YMQC15JS7QcrsAyfGrGiykweqMmPxTkEVScu7vCZLNpPXW1XphHwzsgmqdHWDQAfucbM72EEB1ZEyfgZxYvkZjYVXx1xS9p`. Mainnet `xpub` rejection in regtest was not counted as a discrepancy; Core's network-specific extended-key prefix policy is already covered and expected.

### Boundary and acceptance matrix

Both implementations accepted and compiled the unique-key nested `sh(wsh(and_v(or_c(...),pk(...))))` forms, `or_b`, `andor`, nested Taproot leaves, `older(1)`, `older(65535)`, `older(65536)`, `after(1)`, `after(500000000)`, `after(500000001)`, and `sortedmulti` thresholds 1 and 3. Both rejected `older(0)`, `after(0)`, `multi(0,...)`, and a threshold greater than the number of keys. Both also accepted duplicate keys in raw `multi`/`sortedmulti` and duplicate Taproot leaves, so the Core duplicate check is a Miniscript-specific sanity rule rather than a generic key parser rule.

Two differences were reduced to exact external compatibility cases:

1. `wsh(and_v(or_c(pk(P1),or_c(pk(P3),v:older(1000))),pk(P1)))` is accepted by Embit and compiles to scriptPubKey `00204363f8064c0890c49615c645d32e742d8aeee1dbb615a7f4e33aae885b27d8a7`. Core rejects it with `and_v(...) is not sane: contains duplicate public keys`. Replacing the final `P1` with the unique `P2` or `P3` made both implementations accept and match. The initial apparent nested-wrapper mismatch was therefore not a type-system divergence; it was the deliberately duplicated key.
2. `wsh(pk(04a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd5b8dec5235a0fa8722476c7709c02559e3aa73aa03918ba2d492eea75abea235))` is accepted by Embit and produces witness script `4104a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd5b8dec5235a0fa8722476c7709c02559e3aa73aa03918ba2d492eea75abea235ac` and scriptPubKey `0020000676452ca2b203855a8f56af1a6297bff43c356072e85de974231aa3d3457e`. Core rejects the descriptor with `pk(): Uncompressed keys are not allowed`. This is an alternative-library acceptance/policy gap: Core's descriptor parser intentionally requires compressed keys in witness contexts, even though the raw script can be represented and hashed.

The duplicate-key case is a report-ready Embit interoperability finding because Embit accepts and derives a descriptor that Core's Miniscript descriptor contract rejects; the uncompressed-key case is a separate report-ready compatibility/policy finding. Neither is a local Bitcoin Core defect. No source mutation, regression test, or production patch is justified. The exact Core-side rule is independently supported by `DuplicateKeyCheck` and the compressed-key branch in the source, while the Embit-side acceptance was reproduced by `Descriptor.from_string` and its compiled outputs.

### Validation and verdict

- `PYTHONPATH=/data/my_storage/tmp/cycle155-embit/src python3 -m unittest -v tests.tests.test_descriptor` passed 10/10 tests at the pinned Embit snapshot. The run used its pure-Python ECC fallback because optional system libsecp256k1 bindings were unavailable.
- `TMPDIR=/data/my_storage/tmp/cycle202-alt-desc/runtime /data/my_storage/tmp/cycle84-build/bin/test_bitcoin --run_test=descriptor_tests --log_level=test_suite` passed 13/13 selected Core descriptor cases with `*** No errors detected`.
- The same command with `--run_test=miniscript_tests` passed all 3 selected Core Miniscript cases with `*** No errors detected`.
- The isolated RPC comparison covered six fixed descriptors, three ranged descriptor families at three indexes each, and the boundary/duplicate matrix. The daemon was stopped after use; no protected process was touched.

Verdict: no local Core change. Core and Embit agree on the supported complex descriptors and actual ranged derivation tested here. Preserve the two external Embit compatibility gaps as report-ready seeds, without claiming either is a repository finding. The next cycle must draw a fresh selector and must not reopen Cycle 155's simple descriptor/key-wrapper cell or Cycle 202's now-closed complex Miniscript cell.

## Cycle 197 selection and gate

- Exact selector: `shuf -i 0-98 -n 1` -> `55` (`alternative-implementation`); no reroll was needed. The dedicated branch is `uber-cycle-197-alternative-implementation-20260731`.
- Gate timestamp: `2026-07-31T08:00:19Z`. Start HEAD: `3d122740b2c50848485be4421144ab31248fe9fe`; `origin/master`: `67efced1fc83a0b7215cc1513e7c4754fee0f12f`; merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`; start divergence from `origin/master...HEAD`: `42 1184`.
- The tracked/index gate was clean and `git diff --check` passed. Known unrelated untracked artifacts and protected PIDs `777094`, `956381`, and `1138182` were preserved. Pre-cycle `agent-journal/uber-goal-state.md` SHA-256: `1787428b9b030c292428a6f6c145b477599bde1e236b81b58a7c28c997f5df1d`.
- Authoritative catalog, prompt, corrected TSV, and uber protocol hashes were `5c847ef77405df14b7e7e8fa50430d11a71dcbac3d84df66d25a168d1e955ea8`, `10408ad01c000bba65c1fff135cf2d7d92508bf8a8549141e3d6880f7fe0d4ec`, `babfb36e1a64d8b4ad310459306fa2dfdb240d644d731e2b795177f93a68f1cb`, and `954a67b016918eb2d71c17ae78a12b38f014bb47ed32fe45a0b6f307e5002fc0`.

This cycle continues the campaign from a new evidence cell. Cycle 155 already covered Core versus Embit/rust-bitcoin/btcd CompactSize and PSBT canonicality, descriptor/key derivation, and ADDRv2 behavior; Cycle 194 covered current advisory variants. Do not reopen those findings. First inventory remaining protocol, transaction, script, wrapper, and persistence boundaries, then select one externally observable behavior with a pinned reference implementation and a deterministic Core fixture. Preserve raw inputs and classify every difference as intentional policy, unsupported feature, reference defect, or a local Core compatibility defect.

## Cycle 197 result: transaction serialization and compact-block compatibility

### Selected cell and source inventory

The new cell was raw transaction extended/witness serialization at the boundaries that are not covered by the earlier CompactSize/PSBT campaign: superfluous witness records, unknown witness flags, truncated fixed-width tails, trailing bytes, and the zero-input marker ambiguity. The Core oracle was the existing `/data/my_storage/tmp/cycle84-build/bin/bitcoin-tx`, built for this checkout with `ninja -C /data/my_storage/tmp/cycle84-build bitcoin-tx -j2`.

The pinned comparison snapshots were unchanged from the gate: bitcoinfuzz `0b02212bc6bf48643d36486acd4d0551df63ee3d`, rust-bitcoin `607e8b2fe0d8f1ebe06923dbbc0ca6afdf00d1d1`, Embit `fff7ffa43f6ce088c5ba22cb3877a122bf01dc96`, and btcd `05585e037ba0690572208dbc46d121a49cc0c4c9`. Rust and Go could not be executed because the host has no `rustc`, `cargo`, or `go`; source contracts and the pure-Python Embit runtime were used instead.

Core's `UnserializeTransaction` in `src/primitives/transaction.h:201-238` requires a nonzero supported flag, reads one witness stack per input, rejects an all-empty witness section as `Superfluous witness record`, rejects unknown optional flags, and then reads the complete four-byte locktime. `DecodeTx` in `src/core_io.cpp:154-220` and `DecodeHexTx` require the serialized input to be fully consumed. The pinned btcd decoder has the corresponding flag, witness-presence, and `io.ReadFull` checks in `wire/msgtx.go:494-637`. The rust-bitcoin decoder rejects unsupported flags and all-empty witness sections in `primitives/src/transaction.rs:519-562`.

### Deterministic malformed-input matrix

The matrix used one minimal one-input/one-output transaction, with the raw bytes retained by SHA-256 rather than relying on a large command-line fixture. `accept` means the parser returned a transaction; `same` means reserialization was byte-identical.

| Case | Bytes / SHA-256 | Core `bitcoin-tx -json` | Embit `Transaction.parse` | Embit serialization |
| --- | --- | --- | --- | --- |
| Legacy canonical | 60 / `e05dd47579d2f0369ea5f4f7cd4d4771932bd85e354d962917476d3f7810cd46` | accept | accept | same |
| SegWit with one witness item | 65 / `bff92bc18572cbc5402efb6d4230feb5f7313c1236100d0297ab4a1c1138d1cb` | accept | accept | same |
| Superfluous empty witness | 63 / `16769f47384f440b25ffdc6faf018b63497c17119a293e9d5e47a29e9a43fa34` | reject: invalid encoding | accept | changed to legacy |
| Unknown witness flag | 65 / `5ca0d35d2ef49542a5fa8fe5b0c31494cd6a8628406025916e2f11f84b571cae` | reject: invalid encoding | reject: invalid SegWit marker | - |
| Legacy locktime shortened by one byte | 59 / `bd35b19a776b099a1677878ce5b33be46cad0246376eaad05d72c8256b40fc8e` | reject: invalid encoding | accept | changed |
| SegWit locktime shortened by one byte | 64 / `519398f5e8d66f0289df64152fd015e44d90a81abe9a63eabd0bbf146318096b` | reject: invalid encoding | accept | changed |
| SegWit witness/locktime tail shortened by two bytes | 63 / `bf1571d27b31a8aa8da38914b0db1ff04d6bb800e36c5ecab8ab94aae0fe580e` | reject: invalid encoding | accept | changed |
| Legacy trailing byte | 61 / `465a61cf13555b2b680a056fbc16928feca3e42c6e2d0fc90808162739ff01ca` | reject: invalid encoding | reject: extra bytes | - |
| Zero-input legacy form | 10 / `96eeff563b3135e3f77964e8c062328fd207c8bc9e754fc423abaf83eb3f1490` | accept | reject: invalid SegWit marker | - |
| Zero-input SegWit-style form | 12 / `13cc01f0c367c94ee08a6457b40982c7b82d66fb47e45803997adf52f2e9ce4b` | reject: invalid encoding | accept | changed to legacy |

The final row's displayed hash was independently checked from the exact raw bytes. The exact superfluous-witness reproducer is:

`010000000001010000000000000000000000000000000000000000000000000000000000000000ffffffff00ffffffff010100000000000000000000000000`

Embit's `Transaction.read_from` at `transaction.py:145-167` calls `int.from_bytes(stream.read(4), "little")` for version and locktime without checking that four bytes were returned, and it never checks that a SegWit transaction contains a nonempty witness stack. Its outer `parse` does reject trailing bytes, so the discrepancy is specifically acceptance of truncated fixed-width fields and superfluous witness data, not a general trailing-data issue. The two accepted malformed inputs reserialize differently, making the data-loss risk directly observable.

Zero-input behavior is a deliberate invalid-domain/API distinction, not a Core defect. Rust-bitcoin documents and implements the no-input exception in `primitives/src/transaction.rs:84-95` and `205-213`: it emits BIP141-style serialization to avoid ambiguity even though consensus-invalid zero-input transactions are not meaningful Core transactions. Its decoder skips the witness loop for that special case at `:528-542`. The bitcoinfuzz rust adapter explicitly skips empty-input transactions for this reason. Core's raw decoder accepting the legacy empty form is consistent with its existing `tx_no_inputs` test and later context-free `CheckTransaction` validation.

### Valid-domain and compact-block checks

A seeded Python differential harness generated 50 canonical transactions with one to four inputs and outputs, arbitrary scripts, empty and nonempty witness stacks, versions 1 through 3, and deterministic sequence/locktime values. Core and Embit agreed on acceptance, byte-for-byte reserialization, and txid in all 50 cases. Boundary scripts of lengths 252, 253, 254, 4096, and 16000 bytes also matched in legacy and SegWit forms. A first 65535-byte attempt was blocked by the host's argument-size limit before either parser ran and was not counted as evidence.

For BIP152, Core's `src/blockencodings.h:82-143` and `src/blockencodings.cpp:69-210` enforce the compact-block transaction-count/index bounds and reset partial reconstruction state on failure. Rust-bitcoin's `p2p/src/bip152.rs` uses the same six-byte short-ID and differential prefilled-index wire shape, rejects aggregate index overflow, and separates version-1 txid from version-2 wtxid construction in its block builder. The bitcoinfuzz adapter records the remaining layer difference: rust-bitcoin performs more context-free transaction validation during decode, while Core defers it to `CheckTransaction`, and the harness skips those prefilled cases before comparison. That is an explicit normalization, not a disagreement in Core's compact-block wire parser.

Corrected Core validation commands, with pre-created scratch `TMPDIR`s, passed:

- `test_bitcoin --run_test=transaction_tests --random=19705`: 18/18 cases, 23,817/23,817 assertions.
- `test_bitcoin --run_test=blockencodings_tests --random=19706`: 30/30 cases, 369/369 assertions.
- Targeted `transaction_tests/tx_no_inputs` and `blockencodings_tests/SimpleRoundTripTest`: both passed.

An initial suite attempt used non-existent temporary directories and was not treated as code evidence; the corrected runs above were clean. `git diff --check` passed after the journal-only edit.

### Classification and handoff

The superfluous-witness and short-fixed-field differences are report-ready external Embit parser findings at the pinned snapshot, corroborated by Core, btcd, and rust-bitcoin source behavior. They do not justify a Core change. The zero-input difference is documented intentional policy for an invalid consensus domain. The btcd stream decoder's lack of an EOF check is also not comparable to Core's hex/RPC decoder because its API decodes one length-framed wire message from an `io.Reader`.

No local Bitcoin Core source or permanent test change is justified in this cell. The journal is the handoff artifact; preserve the exact raw fixtures and external snapshot hashes for a future bindings/library campaign, but do not repeat the earlier CompactSize, PSBT, descriptor, or ADDRv2 cells. Next queue: fresh selector gate and the highest-risk unchecked protocol, crypto, database, or wrapper cell.

## Cycle 155 selection and gate

- Exact selector protocol: after the Cycle 154 close, the first fresh draw was `5`, already closed by the boundary-condition campaign. The required exact reroll was `55`, `alternative-implementation`.
- Dedicated branch: `uber-cycle-155-alternative-implementation-20260730`.
- Cycle start HEAD: `158909b90aa0a00fa4684a22aec3200e7830f62c`.
- `origin/master`: `67efced1fc83a0b7215cc1513e7c4754fee0f12f`.
- Merge-base: `a2aab6df97d9f3e1186e8c3fc57ad909cc8aef9b`.
- Start divergence from `git rev-list --left-right --count HEAD...origin/master`: `1094 42`.
- The tracked/index gate was clean and `git diff --check` passed. Existing untracked agent artifacts, `node_modules`, test cache files, and PID `777094` were preserved and excluded from the cycle.
- The selector hashes and the uber protocol hash matched the authoritative catalog. This cycle used the goal text from `agent-goals/goals.tsv` row 55 and the corresponding section in `agent-journal/reusable-continuous-agent-goals.md`.

The selected goal is to compare protocol, cryptographic, database, and wrapper behavior with related implementations, classify intentional policy differences, and prove local contract or compatibility defects. The audit is a continuing campaign, so the next cycle must start with a fresh gate and a fresh selector rather than treating this handoff as repository completion.

## Evidence sources and limits

External source snapshots were cloned outside the workspace so they could not affect the repository:

- bitcoinfuzz: `/data/my_storage/tmp/cycle155-bitcoinfuzz`, HEAD `0b02212bc6bf48643d36486acd4d0551df63ee3d`.
- rust-bitcoin: `/data/my_storage/tmp/cycle155-rust-bitcoin`, HEAD `607e8b2fe0d8f1ebe06923dbbc0ca6afdf00d1d1`.
- Embit: `/data/my_storage/tmp/cycle155-embit`, HEAD `fff7ffa43f6ce088c5ba22cb3877a122bf01dc96`.
- btcd: `/data/my_storage/tmp/cycle155-btcd`, HEAD `05585e037ba0690572208dbc46d121a49cc0c4c9`.

The host has no `go`, `rustc`, or `cargo`, and Docker is unavailable. Therefore the external comparison uses source-level contracts plus the pure-Python Embit runtime and Bitcoin Core's existing RPC/test binaries. The bitcoinfuzz build was not attempted as an oracle because its module build requires external language toolchains and submodules, and its Bitcoin Core module targets its own external Core tree rather than this checkout. External code and differential-harness comments were treated as leads, not as correctness oracles.

## CompactSize and PSBT boundary

### Hypothesis

An alternative implementation might accept a CompactSize encoding or PSBT length that Core rejects, or Core might have a permissive call path that disagrees with the network/persistence contract and creates a compatibility or resource-safety defect.

### Source contracts

- Core's `ReadCompactSize` in `src/serialize.h:328-364` rejects non-minimal 3-, 5-, and 9-byte encodings and, by default, rejects values above `MAX_SIZE`. Its comment explicitly distinguishes length prefixes from generic numeric fields.
- Core's PSBT `UnserializeFromVector` in `src/psbt.h:115-140` reads a CompactSize length, checks it against remaining stream data before allocating, and requires the nested reader to consume exactly the stated value.
- The only intentional generic numeric use found in the production serialization surface is `CAddress::nServices`: `src/protocol.h:461-469` uses `CompactSizeFormatter<false>`, matching a full-width service-bit field rather than a collection length.
- rust-bitcoin's `consensus_encoding/src/compact_size.rs` rejects non-minimal values and separates its default 32 MiB length-limited `CompactSizeDecoder` from the unbounded `CompactSizeU64Decoder` for generic numeric fields.
- btcd's `wire/common.go:487-550` rejects non-canonical `ReadVarInt` encodings at all three prefix widths.
- Embit's `src/embit/compact.py:30-45` decodes the prefix and payload but does not check minimality. Its reader also has no Core-equivalent length limit before callers use the result.

### Differential execution

A deterministic v0 PSBT containing a 60-byte unsigned transaction was sent through the running regtest Core RPC and Embit's parser. The canonical PSBT was accepted by both. Three independent mutations were then tested:

1. Encode the one-byte global key length `01` as `fd0100`.
2. Encode the 60-byte global value length `3c` as `fd3c00`.
3. Encode the unsigned transaction's one-byte input count `01` as `fd0100`.

`bitcoin-cli -datadir=/data/my_storage/tmp/cycle155-rpc.pTS5Dd -regtest -rpcport=18555 decodepsbt ...` rejected each mutated PSBT with `TX decode failed non-canonical ReadCompactSize(): iostream error`. `PSBT.parse()` from the pinned Embit snapshot accepted each one. The difference is reproducible from the exact `ReadCompactSize` and `compact.read_from` bodies and is an external-library canonicality/resource-boundary gap, not a Core compatibility defect: Core's serialization contract deliberately requires the canonical encoding.

The same fixture family also checked context-free transaction layering. With one empty-input map per transaction input and one empty-output map per transaction output, Core and Embit both decoded transactions with empty vout and duplicate inputs. Core also decoded negative-output and non-coinbase null-prevout transactions. Those are later rejected by Core's `CheckTransaction` in `src/consensus/tx_check.cpp`, not by the raw transaction deserializer. bitcoinfuzz's rust-bitcoin adapter comments document the opposite layering for rust-bitcoin, where most of these checks happen during decode and the harness skips those cases before comparing. This is an intentional API-layer distinction, not evidence that Core should reject context-free-invalid transactions during PSBT decoding.

### Verdict

Dismissed for Core. No permissive Core length path or missing canonicality check was found. The external Embit mismatch is retained as a prior-finding seed for a future binding/library audit, but changing Core to accept noncanonical PSBTs would conflict with its existing serialization tests and release history.

## Descriptor and key-wrapper matrix

### Hypothesis

Descriptor parsing, extended-key network selection, and address derivation could disagree between Core and a commonly used pure-Python wrapper at edge paths, especially around testnet prefixes, wildcard derivation, wrapped SegWit, and Taproot.

### Execution

Using seed bytes `00..0f`, Embit generated a regtest `tpub` at `m/84h/1h/0h`:

`tpubDDNRbZGvdA33cgpY5uy2mmphT7sK4uciRjcQScSd64S5KRyZDxHcPuzs24or84Hywugb2JbEEt2jWH8fduiN9cmZzkSj8sSSx6txXkhXyZs`

For indexes 0 through 2, Core's `getdescriptorinfo` plus `deriveaddresses` and Embit's `Descriptor.from_string(...).derive(i).address(network=NETWORKS["regtest"])` produced identical outputs for:

- `pkh(tpub.../0/*)`;
- `wpkh(tpub.../0/*)`;
- `sh(wpkh(tpub.../0/*))`; and
- `tr(tpub.../0/*)`.

The comparison also confirmed that Core rejects a mainnet `xpub` in the regtest descriptor context, while Embit selects the network from the address/key configuration. That is expected version-prefix policy, not an unexplained divergence. Core expands `combo(tpub.../0/*)` into nine legacy, native, and wrapped outputs; Embit's parser rejects `combo()` as unsupported. bitcoinfuzz's Embit module likewise excludes unsupported descriptor fragments such as `combo`, `thresh`, `raw`, `rawtr`, and `pk`, so this is a documented feature-surface difference.

### Verdict

Dismissed for Core. The supported descriptor/address matrix matched, and the remaining difference is a declared missing Embit feature rather than a Core contract defect.

## ADDRv2 protocol matrix

### Source comparison

- Core enforces `MAX_ADDRV2_SIZE == 512` before allocating in `src/netaddress.h:423-475`, validates fixed sizes for known BIP155 networks, consumes unknown network payloads, and materializes them as invalid addresses so the containing message can continue.
- rust-bitcoin's `p2p/src/address.rs` recognizes IPv4, IPv6, Tor v3, I2P, CJDNS, and unknown network IDs, rejects fixed-size and embedded-address violations, and applies the 512-byte limit after its bounded byte-vector decoder.
- btcd's `wire/netaddressv2.go` uses the same 512-byte maximum and fixed network sizes, but deliberately returns `ErrSkippedNetworkID` for I2P, CJDNS, unknown networks, and invalid OnionCat/IPv4-mapped IPv6 entries because those are not represented by btcd's peer manager.

The differences are consistent with the implementations' roles: Core and rust-bitcoin preserve more BIP155 address types, while btcd skips unsupported network IDs. Core's pre-allocation check is stricter than rust-bitcoin's post-decode rejection but does not make Core incompatible with the protocol; it is the safer local resource policy.

`/data/my_storage/tmp/cycle89-build/bin/test_bitcoin --run_test=net_tests --log_level=message --color_output=false` completed all 36 selected network test cases with `*** No errors detected`. The ADDRv2 vectors covered valid and truncated IPv4/IPv6/Tor v3/I2P/CJDNS cases, embedded IPv4/Tor v2, unknown IDs, the 513-byte boundary, and the 32 MiB CompactSize boundary.

### Verdict

Dismissed for Core. No protocol encoding mismatch, missing size bound, or local handling error was independently reproduced. The btcd exclusions and rust-bitcoin representation differences are intentional compatibility-policy differences.

## Cross-cycle prior-art and deduplication checks

The journal and repository history were searched for prior backend, database, release-differential, fuzz-engine, ADDRv2, descriptor, PSBT, and CompactSize campaigns. Existing cycles already cover SHA backend parity, LevelDB wrapper semantics, release behavior, and fuzz-engine comparisons. This cycle therefore targeted the new combination of external wrapper parsing plus protocol-layer classification and did not duplicate those fixes.

## Final cycle verdict and handoff

No local Bitcoin Core source or permanent test change is justified. The independent evidence forms were:

1. Direct source-contract comparison across Core, rust-bitcoin, btcd, bitcoinfuzz, and Embit.
2. Core RPC versus Embit execution for canonical/noncanonical PSBTs, invalid transaction layering, and four descriptor families.
3. Core's complete selected `net_tests` run covering ADDRv2 boundaries.

The scratch regtest node at `/data/my_storage/tmp/cycle155-rpc.pTS5Dd` was stopped after the probes. The exact scratch source snapshots remain outside the workspace for recurrence checks. Future revisits should require a new boundary, a newly observed alternative implementation, or executable Rust/Go differential tooling; they should not repeat the canonical CompactSize, supported descriptor, or current ADDRv2 cells.

Next queue: fresh selector gate, then the highest-risk unchecked protocol/crypto/database/wrapper cell from the selected goal's accumulated risk map. Keep this journal-only cycle separate from the next cycle's branch and commits.
