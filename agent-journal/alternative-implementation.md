# Alternative-implementation compatibility audit

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
