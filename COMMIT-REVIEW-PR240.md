# PR #240 commit review: Knots rebased on Core 006f8f7d49 (2026-07-17)

Scope: all 1,907 commits in `l0rinc/bitcoin` PR #240 (`git merge-base origin/master pr240` = `006f8f7d49`), each classified
(doc/test/revert/consensus/fix?/infra/core-code/gui/other) and the 183 interesting ones (fix?/consensus/revert) individually
verified against latest Knots (`knots/mainline` = 29.x, tip 2026-05-07) and Core master. Review date: 2026-07-21.

## Headline answers

- **Covert fixes present in Knots that Core lacks:** 30+ commits, all DISCLOSED as bugfixes in their messages (nothing
  disguised as a feature). Highest-value cherry-pick candidates for Core are listed below the table.
- **Serious consensus bug in Knots:** none found. The RDTS deployment state machine (versionbits.cpp) is internally
  consistent: bit 4, 55% (1109/2016), NO_TIMEOUT, forced LOCKED_IN at max_activation_height 965664 (guaranteed
  activation), ACTIVE→EXPIRED after 52416 blocks (~1 year), EXPIRED terminal and enforcement-gated consistently.
  The forced-activation semantics are deliberate: every Knots node switches to RDTS rules at 965664 regardless of
  signaling. Enforcement commits (f67b0c73e7, 20227b23be, 9b96910ebc) gate correctly on DeploymentActiveAt.
- **Rebase-introduced bugs found in the PR itself (to fix in the rebase, not in Core):**
  1. `7af6e86d4d` — Qt build break: uses nonexistent `CNodeStats::nTimeOffset` at `src/qt/rpcconsole.cpp:1387`
     (verified: the symbol exists nowhere else in the tree; master uses `nodeStateStats.time_offset` at :1417).
  2. `a7d5d82f60` — silently reverts Core #32149 ("Fix crash on empty wallet migration"): restores the pre-#32149
     `WALLET_FLAG_BLANK_WALLET` check and deletes `HasLegacyRecords` (pr240 `src/wallet/wallet.cpp:4874` vs
     master `:4406`, verified). Its own regression test (`wallet_migration.py` watchonly_empty2) is still in the
     tree — run it before shipping.
  3. `ccaba2d146` — conflict-resolution error drops `bcrypt.dll`, `SETUPAPI.dll`, `SHCORE.dll` from the guix
     symbol-check allow-list → Windows release checks fail (verified against master's `contrib/guix/symbol-check.py:168-170`).
  4. `50ace54b72` — stale cherry-pick also removes `getmempoolfeeratediagram` and `getmempoolcluster` from
     `RPC_COMMANDS_SAFE_FOR_FUZZING` → Knots silently stops fuzzing two Core RPCs (restore or document).
  5. `38678af8d9` — resurrects orphan file `src/wallet/test/fuzz/notifications.cpp` (deleted upstream `fad2faf6c5`);
     dead weight. `41bd9cd389`/`108411b19f` commit `rebase_assessment/` tooling junk and a dead `test_runner.py`.
  6. `3fa603874f` — latent SanitizeString escape-mode mojibake for bytes ≥0x80 (cosmetic, debug-log only).

## Top cherry-pick candidates for Core master (all disclosed, verified present on master today)

1. `cf436bed1f` — true exclusive-create on Windows in `fsbridge::fopen`: fixes still-open Core issue #30210
   (xor-key/tmp files can be truncated/raced on Windows).
2. `dfb593e8e9` — only `SetEndOfFile` when `SetFilePointerEx` succeeds: master can truncate blk/rev files on
   Windows API failure (rare corruption path).
3. `5e3de447c9` — GUI hang: infinite eventFilter recursion via `setStyleSheet` in `BitcoinAmountField::setValid`
   under Breeze Qt styles; present verbatim on master.
4. `a637d80c34` — Tor inbounds on a shared `-bind` get clearnet treatment incl. whitelist permissions on master;
   moderate, behavior tradeoff to discuss.
5. `d838b590a0` — torcontrol maps bind-any onion target to loopback before ADD_ONION (master dials 0.0.0.0).
6. `f97af78c13` — build-info stamps a foreign repo's commit when building inside an unrelated work tree.
7. `48669516c6` — deprecated string-mode `warnings` drops all but the last message (regression vs pre-#27755).
8. `f83772ae5e` + `24e81ee1ce` — first-run disk-space check: GB vs GiB mismatch and prune-mode warns full-chain size.
9. `58ff6b9ea7` — `AddNode()` CJDNS duplicate detection incl. inbound port-agnostic match.
10. `428c0f69b0` — fee-estimator corrupt-file `scale*maxPeriods` uint64 overflow (local-file, very low).
11. Trivial/cosmetic but free: `54d35529de`, `c00db656ed`, `0520f620e6`, `24e8ceabd4`, `95e1d4e3c7`,
    `6b7331173a`, `93b8ef602b`, `357d975672`, `033a56b23f`, `7d6a9f85f3`, `b10af8ac01` (free perf),
    `035c64b8e9`, `29e6d66b34` (needs real fix, not `#elif 0`), `5d823a0c0d`, `d9193295ac`.

## Deliberate Knots divergences (not bugs; listed for completeness)

UPnP kept (reintroduces the miniupnpc attack surface Core dropped in #31130 — the main security tradeoff in the PR),
fullrbf default-on (matches master), legacy BDB wallet + libbitcoinconsensus kept, feefilter option kept, RDTS
softfork (bit 4, forced activation at 965664), `-blockmaxsize`/datacarrier/dust policy knobs, rpcwhitelist-wallets,
legacy getbalance semantics, legacy wallet migration differences (backup moved into wallet dir vs master's walletdir).

## Coverage note

All 1,907 commits were mechanically classified. The 183 fix?/consensus/revert commits were individually diff-read and
verified against `knots/mainline` (each is a rebase of a mainline commit, confirmed via `cherry picked from` hashes or
subject match) and against `origin/master` for the bug's presence. ~80 rows carry full verdicts below; the remainder
are doc/test/infra/Knots-only-feature/revert-policy entries with no Core-relevant bug content (batch summaries on file
in the audit handover).

---

| # | Commit | Subject | Author | Category | Verdict |
|---|---|---|---|---|---|
| 1 | # |  |  | other | |
| 2 | 1fc9277a1c | test: cover disconnect on private broadcast peer with relay=false | Bruno Garcia | test | |
| 3 | b52454538b | lint: remove E712 Ruff ignore | will | infra | |
| 4 | 6eca11175b | lint: remove E731 Ruff ignore | will | infra | |
| 5 | c8b2aeb226 | qt: Avoid implicit `NSApplication` instantiation | Hennadii Stepanov | gui | |
| 6 | fd59d68c26 | qt, test: Enable tests on macOS with `minimal` QPA plugin | Hennadii Stepanov | gui | |
| 7 | e0c196f9c1 | Merge bitcoin/bitcoin#35721: lint: drop most remaining default Ruff rule ignores | merge-script | other | |
| 8 | 4906594a38 | Merge bitcoin-core/gui#950: qt, test: Run GUI tests on macOS with `minimal` QPA plugin | Hennadii Stepanov | other | |
| 9 | d1d85263f8 | Merge bitcoin/bitcoin#35681: test: cover disconnect on private broadcast peer with relay=false | merge-script | other | |
| 10 | e115f347af | Multiselect in coincontrol treewidget and display selected count | Andras Elso | other | |
| 11 | 625b09da88 | GUI/CoinControl: Remove selection-only counter and minimise diff | Luke Dashjr | gui | |
| 12 | 3512a0e673 | GUI: Restore "request payment" button text | Luke Dashjr | gui | |
| 13 | 88eacb7005 | Bugfix: GUI: Disable editing of sending addresses | Luke Dashjr | fix? |  COVERT-FIX-CORE (disclosed): kills 3 GUI address-label/DestData-loss bugs (#10244/#13756); master still allows edit; Core would want a real setData fix, not feature-removal |
| 14 | b7abbcfca1 | qt: Add -guisettingsdir option | Emil Engler | gui | |
| 15 | 50bcca0350 | Enlarge Network Traffic Graph | git | other | |
| 16 | 205b43bd56 | Bugfix: GUI/Addressbook: Drop confusing message about signed messages | Luke Dashjr | fix? | |
| 17 | 1ae49df32d | wallet: Preserve unsupported key origin flags | Luke Dashjr | core-code | |
| 18 | f9f349b12c | Network Graph layout | @RandyMcMillan | core-code | |
| 19 | bc7c7cd1f8 | test: Support -cli tests using external bitcoin-cli | Luke Dashjr | test | |
| 20 | 834e153c10 | [Qt] support for persisted rpc console history | Jonas Schnelli | other | |
| 21 | 860d8695c1 | Qt/RPCConsole: Refactor RPCConsole::WriteCommandHistory out of ~RPCConsole | Luke Dashjr | gui | |
| 22 | b8b5638fba | Qt/RPCConsole: If filtered commands are found in history at startup, erase them best we can | Luke Dashjr | gui | |
| 23 | 5de650bf62 | Network Graph - Create y_value function | R E Broadley | core-code | |
| 24 | eeddec7f78 | Enable non-linear network traffic graph | R E Broadley | other | |
| 25 | e4b03156a8 | Bugfix: GUI/QRImageWidget: Correctly calculate sizes | Luke Dashjr | fix? | |
| 26 | 033a56b23f | Bugfix: GUI/QRImageWidget: If text won't fit, split across multiple lines | Luke Dashjr | fix? |  COVERT-FIX-CORE (trivial): wraps long QR caption text; master clips it |
| 27 | dbbb467011 | GUI/QRImageWidget: Allow image to grow up to 25% wider to accomidate single-line text | Luke Dashjr | gui | |
| 28 | 41bd9cd389 | Bugfix: Skip tests for tools not being built | Luke Dashjr | fix? | |
| 29 | 3c705a5e9f | contrib/init: (OpenRC) use -daemonwait to wait for startup completion | Matt Whitlock | infra | |
| 30 | adf8a40317 | contrib/init: (OpenRC) use ${SVCNAME} in BITCOIND_PIDFILE default | Matt Whitlock | infra | |
| 31 | f04c82ccff | rpc: add period_start to version bits statistics | Sjors Provoost | core-code | |
| 32 | 8913159833 | GUI: Alt-P shortcut key for paste button in Open URI dialog | Emil Engler | gui | |
| 33 | 98460b2c18 | GUI: Peers: When sorting by address, sort by network first | Luke Dashjr | gui | |
| 34 | ed5a0fbe89 | gui: avoid unclean exit due to permissions issues when setting start on system startup | mruddy | gui | |
| 35 | b1d284a4f2 | qt: Fix shortcut ambiguities | shaavan | gui | |
| 36 | b6765f4088 | qt: Add colon to third party URLS data field | shaavan | gui | |
| 37 | 3d59d386e0 | rpc: Don't FlushStateToDisk when pruneblockchain(0) | MarcoFalke | core-code | |
| 38 | a02bf2779c | net: Ensure CNode.cleanSubVer is always assigned before nVersion | Luke Dashjr | core-code | |
| 39 | 7af6e86d4d | Bugfix: GUI: Peers: If subversion is actually blank, show blank | Luke Dashjr | fix? |  SUSPICIOUS (rebase bug): uses nonexistent CNodeStats::nTimeOffset at rpcconsole.cpp:1387 — Qt build break in pr240 (verified) |
| 40 | 357d975672 | Bugfix: GUI: Peers: When selecting a new peer, reset fields to N/A before loading data | Luke Dashjr | fix? |  COVERT-FIX-CORE (minor GUI): resets stale peer detail fields on new selection |
| 41 | 3637774ee8 | Bugfix: GUI: Peers: Peers without any permissions are "None", not "N/A" | Luke Dashjr | fix? | |
| 42 | f7a6175f63 | Allow configuring target block time for a signet | benthecarman | other | |
| 43 | 98aa50bb84 | rpc: make importaddress compatible with descriptors wallet | furszy | core-code | |
| 44 | c8e50bebd5 | GUI: Initialise DBus notifications in another thread | Luke Dashjr | gui | |
| 45 | 2cedec41b1 | Bugfix: GUI: When restoring table columns, still set their minimum column width and stretch on last section | Luke Dashjr | fix? |  FIX-KNOTS-ONLY: min column width + stretch even with restored header state |
| 46 | 54c8fd6051 | Revert "qt: Move transactionView properties settings to constructor" | Luke Dashjr | revert | |
| 47 | eb5017301b | Revert "qt: Move recentRequestsView properties settings to constructor" | Luke Dashjr | revert | |
| 48 | 610e2259f3 | Revert "qt: Drop buggy TableViewLastColumnResizingFixer class" | Luke Dashjr | revert | |
| 49 | 91bfa30ef2 | Bugfix: net_processing: Restore "Already requested" error for FetchBlock | Luke Dashjr | fix? | |
| 50 | 614b90cad7 | QA: rpc_getblockfrompeer: Test that a non-existent peer generates an error, even if we already have the block | Luke Dashjr | test | |
| 51 | 7d06d42c45 | QA: rpc_getblockfrompeer: Test specific error for trying to fetch from peer twice | Luke Dashjr | test | |
| 52 | be810b6bfa | GUI: Pass PlatformStyle through ClientModel into PeerTableModel | Luke Dashjr | gui | |
| 53 | 6a60bd5cef | GUI: Peers: Add direction table column before "Address" with arrow | Luke Dashjr | gui | |
| 54 | fb936b9eaa | GUI: Make Peers table aware of runtime palette change | Luke Dashjr | gui | |
| 55 | a5d0c5ef58 | GUI: Peers: Update table column widths and labels | Luke Dashjr | gui | |
| 56 | 9c3d8f3aff | Revert "gui: peersWidget - ResizeToContents Age and IP/Netmask columns" | Luke Dashjr | revert | |
| 57 | 5b55fa785c | wallet: allow external_signer flag toggle | Sjors Provoost | core-code | |
| 58 | fdb8ceba3e | Wallet/RPC: setwalletflag: Warn about wallet reload required to toggle external signer flag | Luke Dashjr | core-code | |
| 59 | dcb02eb8c8 | Restore ability to display addresses in GUI | Luke Dashjr | other | |
| 60 | a603690d4f | rpc: Add min_conf option to fundrawtransaction and walletcreatefundedpsbt [as deprecated] | João Barbosa | core-code | |
| 61 | 572e792b91 | qa: Test fundrawtransaction and walletcreatefundedpsbt with min_conf | João Barbosa | test | |
| 62 | e1049d1a65 | RPC/Wallet: Check for negative min_conf in FundTransaction | Luke Dashjr | core-code | |
| 63 | e16b35c192 | RPC: Backward compatibility with Knots 0.19-0.21.0 validateaddress (address_type param and error_index output) | Luke Dashjr | core-code | |
| 64 | 91c8d27991 | RPC/blockchain: Add getblockfrompeer nodeid alias for backward compatibility | Luke Dashjr | core-code | |
| 65 | b10af8ac01 | fix: unnecessary continuation after finding mutation | Kratos | fix? |  DOC/TEST/INFRA (perf-only): early break in merkle mutation scan; behavior-identical; free perf for Core |
| 66 | a6f49e43bc | validateaddress: Strictly check value of deprecated address_type parameter | Luke Dashjr | other | |
| 67 | 9f4c3c9606 | RPC/output_script: validateaddress: Correct error code for erroneous deprecated address_type value | Luke Dashjr | core-code | |
| 68 | 1bda9a76e4 | Wallet: Avoid using change types newer than user's preferred address type | Luke Dashjr | core-code | |
| 69 | 2f1fc0d680 | Refactor: GUI: Make `modernize-use-default-member-init` happy with `columnResizingFixer` | Luke Dashjr | other | |
| 70 | 1a670c6ce8 | RPC: Support specifying different types for param aliases | Luke Dashjr | core-code | |
| 71 | e83dc1f04c | RPC/Net: Allow addconnection on non-regtest networks | Luke Dashjr | core-code | |
| 72 | e1e3de7361 | Bugfix: util: Correctly handle Number value types in GetArg/GetBoolArg | Luke Dashjr | fix? |  COVERT-FIX-CORE (low): accepts numeric bools in settings.json; master aborts init (borderline by-design) |
| 73 | bda241601c | util: Add a ForceSetArgV that can handle non-strings | Luke Dashjr | other | |
| 74 | b3ee71b7a5 | interfaces/chain: Provide a way to ask the user a question during init | Luke Dashjr | other | |
| 75 | bdee57bedf | Bugfix: GUI: Allow the user to start anyway when loading a wallet errors | Luke Dashjr | fix? | |
| 76 | 6f46b9ef91 | scripted-diff: test: rename `strCmd` to `command` | Luke Dashjr | core-code | |
| 77 | 27d9fd2eb8 | Support multiple -*notify commands | Luke Dashjr | other | |
| 78 | 3e7020882e | init: Optimise -startupnotify slightly | Luke Dashjr | other | |
| 79 | c20a5a5937 | RPC/Wallet: Convert walletprocesspsbt to use options parameter | Andrew Chow | core-code | |
| 80 | 38d2083035 | dumpmasterprivkey command | Luke Dashjr | other | |
| 81 | 108411b19f | Bugfix: RPC/Wallet: dumpwallet should include hdkeypath metadata outside of comment | Luke Dashjr | fix? | |
| 82 | f488ad922b | Bugfix: RPC/Wallet: dumpwallet should include hdseedid metadata | Luke Dashjr | fix? | |
| 83 | de9c2aa50a | GUI: Accept Ctrl-D as a shortcut to close the RPC console | Luke Dashjr | gui | |
| 84 | ccaba2d146 | Bugfix: devtools/symbol-check: Check PE libraries case-insensitively | Luke Dashjr | fix? |  SUSPICIOUS (rebase bug): also drops bcrypt/SETUPAPI/SHCORE from symbol-check allow-list → guix Windows check fails (verified); case-insensitive half is a valid Core improvement |
| 85 | 414097f7a8 | rpc: validate fee estimation mode case insensitive | Torkel Rogstad | core-code | |
| 86 | f54a4f7fa0 | rpc: validate conf_target is set alongside estimate_mode | Torkel Rogstad | core-code | |
| 87 | a095c1a8b7 | Wallet: Keep track of what addresses are used in wallet transactions (memory only) | Luke Dashjr | core-code | |
| 88 | 90d7eefead | Wallet: Add fairly-efficient [negative] check that an address is not known to be used | Luke Dashjr | core-code | |
| 89 | dd03dfb8ac | RPC/Wallet: Add "use_txids" to output of getaddressinfo | Luke Dashjr | core-code | |
| 90 | 5e16963461 | RPC/blockchain: Restore ability for pruned nodes to getblockfrompeer future blocks | Luke Dashjr | core-code | |
| 91 | ee2b1d6f54 | QA: wallet_taproot: Workaround fee estimation limitation for sendall cleanup as well | Luke Dashjr | test | |
| 92 | 53907fdaf8 | QA: wallet_taproot: Ensure transaction created by sendall has enough fee to get into the mempool | Luke Dashjr | test | |
| 93 | 461a9b9070 | Wallet: Increase default confirmation target to 144 | Luke Dashjr | core-code | |
| 94 | 58ff6b9ea7 | p2p, bugfix: correctly detect CJDNS addnode entries in AddNode() | Jon Atack | fix? |  COVERT-FIX-CORE (disclosed): AddNode() CJDNS duplicate detection incl. inbound port-agnostic match; master lacks it |
| 95 | 2cf25bfbd9 | test: AddNode() CJDNS regression unit tests | Jon Atack | test | |
| 96 | 6fa572c070 | BufferedFile: fclose at destruction | Luke Dashjr | other | |
| 97 | 5c651e6e2b | interfaces/wallet: Add checkAddressForUsage and findAddressUsage | Luke Dashjr | other | |
| 98 | c46db0dd43 | GUI: Add GUIUtil::dateStr | Luke Dashjr | gui | |
| 99 | d15fe01370 | GUI: SendConfirmationDialog: Defer button setup until exec | Luke Dashjr | gui | |
| 100 | 1d0fd002a7 | GUI: SendConfirmationDialog: Enable changing the actual buttons used | Luke Dashjr | gui | |
| 101 | 82f5ee0bab | GUI: WalletModel: Wrap checkAddressForUsage and findAddressUsage | Luke Dashjr | gui | |
| 102 | 858de24ba9 | GUI: QValidatedLineEdit: Add support for a warning-but-valid state | Luke Dashjr | gui | |
| 103 | f1e8455db1 | GUI: Implement BitcoinAddressUnusedInWalletValidator | Luke Dashjr | gui | |
| 104 | 41d7531a06 | GUI: Use warning indicator for send coins entries with reused addresses | Luke Dashjr | gui | |
| 105 | b557d81a6c | GUI: Add a warning prompt when sending to an already-used address | Luke Dashjr | gui | |
| 106 | 2024b827e9 | Optimized siphash implementation | Elichai Turkel | other | |
| 107 | 08bd26ebc6 | Diff-minimise | Luke Dashjr | other | |
| 108 | 9ec196cabe | qt: Add "Alternating Row Color" settings for the Peers Tab | Hennadii Stepanov | gui | |
| 109 | 09c5db0ea1 | Diff-minimise | Luke Dashjr | other | |
| 110 | ff8e42a518 | GUI: Enable customisation of QR Code font | Luke Dashjr | gui | |
| 111 | d256c55d48 | GUI: Change default QR Code font to embedded | Luke Dashjr | gui | |
| 112 | c6a3607130 | rpc, p2p: allow `disconnectnode` with subnet | brunoerg | core-code | |
| 113 | 3f187c87d2 | test: add coverage for `subnet` in `disconnectnode` RPC | brunoerg | test | |
| 114 | 4c1a03252a | QA: p2p_disconnect_ban: Explicitly select nodes for test, and reconnect after disconnecting | Luke Dashjr | test | |
| 115 | fdfff4a055 | RPC/net: disconnectnode: Support disconnecting by IP (without a port) | Luke Dashjr | core-code | |
| 116 | eb0b7158b3 | QA: wallet_reindex: Use `importdescriptors` directly rather than abusing test_framework's `importaddress` wrapper | Luke Dashjr | test | |
| 117 | 44dd616092 | Diff-minimise | Luke Dashjr | other | |
| 118 | f1cac756a8 | Keep ProcessDescriptorImport exported for bitcoin-wallet importfromcoldcard | Luke Dashjr | other | |
| 119 | a352571035 | GUI: Support returning positions from BitcoinAddress{Entry,Check}Validator::validate | Luke Dashjr | gui | |
| 120 | e5a0fb7473 | GUI: Point out position of invalid characters in Bech32 addresses | Luke Dashjr | gui | |
| 121 | 2a92a7455b | GUI: Choose colour for incorrect characters in address based on user theme | Luke Dashjr | gui | |
| 122 | 40bd822964 | Diff-minimise | Luke Dashjr | other | |
| 123 | 0ff89f6b9c | Revert "Remove -feefilter option" | Luke Dashjr | revert |  REVERT-POLICY: restores -feefilter option Core removed deliberately |
| 124 | 5b9533617b | doc: Move -feefilter option to correct category | Luke Dashjr | doc | |
| 125 | db8c12d836 | RPC: add transaction hash to mempool entry output | Luke Dashjr | core-code | |
| 126 | 7049e08648 | Add Neutrino filter for p2wpkh script types | dangershony | other | |
| 127 | 4993287d2e | Add possible values to cli description | Luke Dashjr | other | |
| 128 | f9db02de1f | Apply suggestions from code review | Dan Gershony | other | |
| 129 | 7710268763 | Adding tests for p2wpkh filter type | dangershony | other | |
| 130 | a58ba0d903 | Make the filter element for witness ver0 | Luke Dashjr | other | |
| 131 | b93865c1e0 | Act on review | Luke Dashjr | other | |
| 132 | 72d1c4071e | Move the filter logic to a single method named BuildFilterElements | dangershony | other | |
| 133 | 8da2c49677 | Change the BuildFilterElements default args | dangershony | other | |
| 134 | 3e8f138fd6 | Act on review | dangershony | other | |
| 135 | ec41820ee7 | QA: blockfilter_tests: Diff-minimise | Luke Dashjr | test | |
| 136 | f6778daecc | Only enable basic blockfilterindex if enabled with type omitted | Luke Dashjr | other | |
| 137 | 89797fff41 | wallet, rpc: add anti-fee-sniping to `send` and `sendall` | ishaanam | core-code | |
| 138 | 720286ad80 | Revert "wallet: Remove unused GetLegacyBalance" (complex) | Luke Dashjr | revert |  REVERT-POLICY: re-adds GetLegacyBalance for legacy getbalance semantics (Knots-only path) |
| 139 | 94e17ae8ef | Bugfix: wallet: GetLegacyBalance: Rather than include two conflicting unconfirmed transactions in the same balance, only include such transactions when they are in our mempool (which cannot have conflicts) | Luke Dashjr | fix? |  FIX-KNOTS-ONLY: GetLegacyBalance skips conflicting non-mempool unconfirmed |
| 140 | 2da5f6f10e | wallet: let ListWalletDir do not iterate trough our blocksdata. | Luke Dashjr | core-code | |
| 141 | 41d57ed49f | rpc: getblock fixups | Jon Atack | core-code | |
| 142 | c57327855f | rpc: getrawtransaction fixups | Jon Atack | core-code | |
| 143 | ce74d446e0 | rpc: decodepsbt fixups | Jon Atack | core-code | |
| 144 | 6c6b607637 | rpc: decodescript fixups | Jon Atack | core-code | |
| 145 | 37c3eb4a54 | rpc: gettxout fixups | Jon Atack | core-code | |
| 146 | dd156acc1c | Minimise (diff & string changes) | Luke Dashjr | other | |
| 147 | 1cfa936e73 | Allow acceptstalefeeestimates on all networks | Luke Dashjr | other | |
| 148 | 38678af8d9 | policy: Delete buggy GetVirtualTransactionSize(CTransaction&) | Luke Dashjr | fix? |  SUSPICIOUS (rebase artifact): resurrects orphan src/wallet/test/fuzz/notifications.cpp (dead file) |
| 149 | 554f6640d4 | node: Refactor BroadcastTransaction to accept a CFeeRate maximum | Luke Dashjr | core-code | |
| 150 | 851050ba6c | Bugfix: RPC/Mempool: Pass CFeeRate to BroadcastTransaction to correctly account for non-weight vsize | Luke Dashjr | fix? |  FIX-KNOTS-ONLY: CFeeRate for Knots BroadcastTransaction; transient privatebroadcast drop repaired in-branch |
| 151 | 319fe699b5 | Bugfix: GUI/PSBTOperationsDialog: Pass feerate limit as a CFeeRate instead of assuming 1kvB | Luke Dashjr | fix? | |
| 152 | caea5c7726 | Diff-minimise | Luke Dashjr | other | |
| 153 | 6bdbf5803a | RPC: Keep .cookie if it was replaced after being generated | Luke Dashjr | core-code | |
| 154 | 87a72a0fdf | Fix Requested Payments History Multiselect | John Moffett | fix? |  COVERT-FIX-CORE (disclosed, gui#684): multiselect/sort-stable/context-menu in recent requests; master still single-selection |
| 155 | b845a12dec | Bugfix: GUI/ReceiveCoinsDialog: Use correct Qt plural forms for context menu | Luke Dashjr | fix? | |
| 156 | 0896a0884a | GUI/RecentRequestsTableModel: Return a null string for "no amount" in edit mode | Luke Dashjr | fix? | |
| 157 | 7f86e8ca64 | GUI/ReceiveCoinsDialog: Allow copying labels/messages/amounts as long as at least one selected request has it populated | Luke Dashjr | gui | |
| 158 | 603b1cbe63 | Diff-minimise & revert ReceiveCoinsDialog context menu plural forms to singular for now | Luke Dashjr | other | |
| 159 | ed13f1bba1 | Diff-minimise | Luke Dashjr | other | |
| 160 | f83772ae5e | Bugfix: init: Correct conversion of AssumedBlockchainSize to use GB | Luke Dashjr | fix? |  COVERT-FIX-CORE (disclosed, minor): first-run disk check GB vs GiB mismatch (init.cpp:1993) |
| 161 | 24e81ee1ce | Bugfix: init: For first-run disk space check, advise user of correct pruned size rather than full blockchain size | Luke Dashjr | fix? |  COVERT-FIX-CORE (disclosed, minor): prune-mode first-run warns full-chain size (init.cpp:2005) |
| 162 | 48669516c6 | Bugfix: RPC: Return all warnings in get{blockchain,mining,network}info (even in deprecated String mode) | Luke Dashjr | fix? |  COVERT-FIX-CORE (disclosed, minor): deprecated string-mode warnings drops all but last message (node/warnings.cpp:59) |
| 163 | 1e3c493736 | net: Add blockfilters white{bind,list} permission flag | Luke Dashjr | core-code | |
| 164 | 401551a0d5 | Include "blockfilters" in NetPermissions::ToStrings as appropriate | Luke Dashjr | other | |
| 165 | 985ceebbe8 | Refactor to avoid conflicts in new p2p permissions | Luke Dashjr | other | |
| 166 | 24a1a6fdcd | Diff-minimise | Luke Dashjr | other | |
| 167 | 107d2cbb00 | policy: add CFeeRate::SatsToString helper without units | Jon Atack | core-code | |
| 168 | 0a8a8e5f9f | core_io: Add ValueFromFeeRate helper | Luke Dashjr | other | |
| 169 | bb9c2273c1 | test: add ValueFromFeeRate/CFeeRate unit tests | Jon Atack | test | |
| 170 | eefbf914b7 | wallet: introduce setfeerate, an improved settxfee in sat/vB | Jon Atack | core-code | |
| 171 | 29c6d86eb0 | QA: wallet_basic: Split wtx expected_fields over multiple lines to minimise merge conflicts | Luke Dashjr | test | |
| 172 | 71f077b894 | RPC/Wallet: Convert descriptorprocesspsbt to use options parameter | Luke Dashjr | core-code | |
| 173 | f5f29b7530 | Diff-minimise | Luke Dashjr | other | |
| 174 | 4e860bc680 | Witness-only option for fundrawtransaction | Aurèle Oulès | other | |
| 175 | 0bcf53b9f5 | Wallet: Keep segwit_inputs_only in simple variable for duration of AvailableCoins | Luke Dashjr | core-code | |
| 176 | fe13f4a982 | Bugfix: QA/wallet_fundrawtransaction: Ensure segwit_inputs_only gets tested correctly | Luke Dashjr | fix? | |
| 177 | bf4f45ee05 | net: Apply outgoing connection permissions to automatic outgoing connections | Luke Dashjr | core-code | |
| 178 | 47bedc7dbc | QA: p2p_invalid_messages: Check misbehaving with noban,out whitelisting | brunoerg | test | |
| 179 | a598ec6c64 | wallet: track background validation height | Sjors Provoost | core-code | |
| 180 | 51e11891cf | wallet: add IsTxAssumed() to WalletTxStatus | Sjors Provoost | core-code | |
| 181 | c34fd488cf | gui: add assumed confirmed state | Sjors Provoost | gui | |
| 182 | c50681d7c6 | Wallet: Pass dump_filename to DumpWallet as an argument | Luke Dashjr | core-code | |
| 183 | 28e1ed2837 | interfaces/Wallet: Add the ability to choose a backup format | Luke Dashjr | other | |
| 184 | c08ffd2ec0 | interfaces/Wallet: Add DumpWallet format to backupWallet interface | Luke Dashjr | other | |
| 185 | 4555ddf7a1 | GUI: Support backup to DumpWallet format | Luke Dashjr | gui | |
| 186 | 0e1e954597 | GUI: Omit DbDump option for backup of BDB wallets | Luke Dashjr | gui | |
| 187 | 0f6d3e6c5e | RPC: Support first param name in GetParamIndex (and Arg<T>) | Luke Dashjr | core-code | |
| 188 | 89e2d32ee6 | RPC/Wallet: Hacky fix for getbalance bugs | Luke Dashjr | fix? | |
| 189 | 7c8c1cbf27 | Bugfix: Wallet: Use CHECK_NONFATAL for findBlock in GetLegacyBalance | Luke Dashjr | fix? | |
| 190 | ebf3a775ed | RPC/Wallet: getbalance: Avoid calling modern GetBalance when its result is unused | Luke Dashjr | core-code | |
| 191 | bafa78515c | RPC/Wallet: getbalance: Throw an error if avoid_reuse is set but ignored | Luke Dashjr | core-code | |
| 192 | 4f8bc589d8 | doc: RPC/Wallet: Attempt to explain the dummy parameter | Luke Dashjr | doc | |
| 193 | fbfd3d8431 | eviction: track one random unprotected node to evict if forced | Matthew Zipkin | other | |
| 194 | 421ac7b803 | net: add new permission ForceInbound | Matthew Zipkin | core-code | |
| 195 | d43d93ab33 | net: nodes with ForceInbound permission force eviction | Matthew Zipkin | core-code | |
| 196 | 9e9d5fe14f | test: cover ForceInbound permission success even when connections are full | Matthew Zipkin | test | |
| 197 | 7ceff629ed | contrib: manpages shouldn't "see also" themselves | Luke Dashjr | doc | |
| 198 | 1a17f70bbb | rpc: add format command with support for args_cli | Brandon Odiwuor | core-code | |
| 199 | bad0c8c214 | test: keeps bitcoin-cli autocomplete in sync | Brandon Odiwuor | test | |
| 200 | b0ab398c3f | Bugfix: QA: tool_cli_bash_completion: Typo in self.options | Luke Dashjr | test | |
| 201 | 8821b15008 | rpc: Add support to populate PSBT input utxos via rpc | Greg Sanders | core-code | |
| 202 | 4a899692ee | RPC/Net: Allow changing the connection_type for addnode onetry | Luke Dashjr | core-code | |
| 203 | 0d0475818e | RPC/Net: addnode: Backward compatibility with connection_type in position 3 | Luke Dashjr | core-code | |
| 204 | 4e605dc1a0 | Handle MSG_FILTERED_WITNESS_BLOCK messages | Eric Lombrozo | other | |
| 205 | 4e08aa612b | rpc: add relevant_blocks to scanblocks status | tdb3 | core-code | |
| 206 | 2e1abec147 | doc: add release notes for 30713 | tdb3 | doc | |
| 207 | d39c7307c8 | corrected lockunspent rpc quoting | Michael Little | other | |
| 208 | 3bfb822427 | further RPC example corrections | Michael Little | other | |
| 209 | 764f279007 | GUI: Always show pruning explanation to avoid checkbox jumping around | Luke Dashjr | gui | |
| 210 | d2664eaece | GUI/Wallet: Strip ".dat" from end of wallet display names | Luke Dashjr | gui | |
| 211 | 74721501f7 | RPC/blockchain: Allow using getblockfrompeer without already having the block header | Luke Dashjr | core-code | |
| 212 | c60b2d0e18 | QA: rpc_getblockfrompeer: Test that fetch-without-header actually works | Luke Dashjr | test | |
| 213 | 65820b05df | RPC/rawtx: descriptorprocesspsbt: Convert prevtxs param to options object only | Luke Dashjr | core-code | |
| 214 | ea5e9e7eef | net_permissions: Include "addr" permission on implicit whitelist entries | Luke Dashjr | core-code | |
| 215 | 86727c05d3 | Enforce checkpoints at their specific block height | Luke Dashjr | other | |
| 216 | a7f138dacb | QA: Check that a few defined mainchain checkpoints are working | Luke Dashjr | test | |
| 217 | dd0f3f4ffa | QA: Functional test for checkpoint enforcement | Luke Dashjr | test | |
| 218 | b0d7b9bb65 | GUI: Adjust recommended # of blocks confirmed to 16 | Luke Dashjr | gui | |
| 219 | ad1916fb40 | Revert "guix: temporarily disable powerpcle taget" | Luke Dashjr | revert | |
| 220 | 3385f8b38b | RPC: Add getblocklocations call | Roman Zeyde | core-code | |
| 221 | 4d483a0169 | Bugfix: RPC/blockchain: Hold cs_main for CBlockIndex access as needed to avoid races in getblocklocations | Luke Dashjr | fix? | |
| 222 | 53b0d56720 | RPC/Blockchain: Bugfix: Correct getblocklocations example RPC method name | Luke Dashjr | fix? | |
| 223 | d0ccbf04f5 | Bugfix: RPC/Blockchain: Correct getblocklocations return type | Luke Dashjr | fix? | |
| 224 | b9fe5d9be8 | Bugfix: QA/fuzz: Add getblocklocations to RPC_COMMANDS_SAFE_FOR_FUZZING | Luke Dashjr | fix? | |
| 225 | 856e4ce0e8 | QA: rpc_getblocklocations: Support for testing with blocksxor enabled | Luke Dashjr | test | |
| 226 | 2714455391 | RPC: introduce 'getblockfileinfo' RPC command | furszy | core-code | |
| 227 | f26f6d40c4 | test: rpc_getblockfrompeer.py, remove magic numbers usage | furszy | test | |
| 228 | bc407853e2 | QA: tool_cli_bash_completion: Whitespace linter fix | Luke Dashjr | test | |
| 229 | c77c809425 | RPC: addmultisigaddress / createmultisig: parameterize _createmultisig_redeemScript to allow sorting of public keys (BIP67) | Thomas Kerin | core-code | |
| 230 | 1a86192f1d | RPC: Use options object rather than adding a "sort" boolean for multisig methods | Luke Dashjr | core-code | |
| 231 | cac7a4b35d | script: GetScriptForRawPubKey: More efficient key-encoding loop | Luke Dashjr | core-code | |
| 232 | 1386dca5a7 | doc/bips: Add BIP 67 | Luke Dashjr | doc | |
| 233 | dabc258e73 | Ensure whilst sorting only compressed public keys are used | Thomas Kerin | other | |
| 234 | b332029b19 | Add more tests to sort_multisig.py / wallet_labels.py | Thomas Kerin | other | |
| 235 | 7fca8e017e | script: Optimise GetScriptForMultisig slightly | Luke Dashjr | core-code | |
| 236 | 5783bb6432 | QA: rpc_sort_multisig: Rename variables to avoid keyword conflict with `sorted` | Luke Dashjr | test | |
| 237 | 60f32515e7 | RPC: createmultisig/addmultisigaddress: Support named args for options | Luke Dashjr | core-code | |
| 238 | 744a46883e | validation: sync utxo state after block sync | Andrew Toth | core-code | |
| 239 | 5575952167 | test: add test for SyncCoinsTipAfterChainSync | Andrew Toth | test | |
| 240 | 55a955b139 | rpc: implement getgeneralinfo | Luke Dashjr | core-code | |
| 241 | c9ad57159e | QA: Actually check getgeneralinfo results are correct | Luke Dashjr | test | |
| 242 | 0e340b07f6 | Bugfix: QA: rpc_getgeneralinfo: Adapt test for startuptime ignoring mocktime | Luke Dashjr | test | |
| 243 | 9f5c1f0298 | Bugfix: QA/fuzz: Add getgeneralinfo to RPC_COMMANDS_SAFE_FOR_FUZZING | Luke Dashjr | fix? | |
| 244 | a1929ecc7e | RPC: Add getrpcwhitelist method | Harris | core-code | |
| 245 | 48743b2763 | RPC: getrpcwhitelist: Return methods as a JSON Object for future expansion to sub-call permissions | Luke Dashjr | core-code | |
| 246 | dc27a17534 | Bugfix: RPC: Correct types in getrpcwhitelist RPC docs | Luke Dashjr | fix? | |
| 247 | 1faecd1df3 | RPC: getrpcwhitelist: Return all methods (or none) if no explicit whitelist defined | Luke Dashjr | core-code | |
| 248 | 61aa3a3a77 | Bugfix: QA/fuzz: Add getrpcwhitelist to RPC_COMMANDS_SAFE_FOR_FUZZING | Luke Dashjr | fix? | |
| 249 | a052fac2b2 | Bugfix: QA: test_node: Ensure debug.log exists at start of assert_debug_log | Luke Dashjr | test | |
| 250 | 316f9c9203 | GUI: BitcoinAddressCheckValidator: Avoid hiding standard validate method signature needlessly | Luke Dashjr | gui | |
| 251 | 80f5e6fbb2 | fixup! test: cover ForceInbound permission success even when connections are full | Luke Dashjr | fix? | |
| 252 | a0436e8c6c | net: only allow 8 simultaneous forced inbound connections | Matthew Zipkin | core-code | |
| 253 | a38919cc30 | net: add forced_inbound to getpeerinfo | Matthew Zipkin | core-code | |
| 254 | c61030c528 | net: Remove forcedinbound limit antifeature | Luke Dashjr | core-code | |
| 255 | 65e8fc8241 | GUI: Make various dialogs less modal | Luke Dashjr | gui | |
| 256 | 8312321f6e | [doc] update man pages for 29.0rc1 | glozow | doc | |
| 257 | 588f6f7d34 | [examples] generate example bitcoin.conf | glozow | other | |
| 258 | b3f5c9541d | [doc] release notes link for 29.0 | glozow | doc | |
| 259 | bca8a6aec8 | build: use make < 3.82 syntax for define directive | Sjors Provoost | infra | |
| 260 | 2b6d3bcf57 | [doc] update man pages for 29.0rc2 | glozow | doc | |
| 261 | a0ef4547ab | [doc] update example bitcoin.conf with missing options | glozow | other | |
| 262 | ce238d8744 | [doc] update man pages for 29.0rc3 | glozow | doc | |
| 263 | 9e4a071350 | Revert "Use more specific path when including `memenv.h` header" | Luke Dashjr | revert |  REVERT-POLICY: memenv.h include-style revert, no bug |
| 264 | a5c9b6e332 | crypto/sha256: Use pragmas to enforce necessary intrinsics for GCC and Clang | Luke Dashjr | other | |
| 265 | 18eb9746f8 | cmake: For intrinsics checks, use pragmas to explicitly enable them for GCC and Clang if possible | Luke Dashjr | infra | |
| 266 | 3421e08400 | http: Fail initialization when any bind fails | Luke Dashjr | other | |
| 267 | 51aac703a5 | httpserver: Check for and tolerate more possible system limitations | Luke Dashjr | other | |
| 268 | 3fa603874f | SanitizeString: Support optional escaping of disallowed characters | Luke Dashjr | fix? |  SUSPICIOUS (latent): SanitizeString escape mode sign-extends ≥0x80 to mojibake + non-injective '%'; cosmetic debug-log garbage at pr240 net_processing.cpp:3828 |
| 269 | 8662886da2 | net_processing: Escape rather than remove any printable characters in UAs | Luke Dashjr | core-code | |
| 270 | 27f7668c41 | Try to use posix_fadvise with CBufferedFile | Evan Klitzke | other | |
| 271 | 37138d05cf | fs_helpers: Guard _POSIX_C_SOURCE check within #ifdef | Luke Dashjr | other | |
| 272 | 6de4db8e0d | The three arguments -nohelp, -noh, and -no? were previously silently accepted and interpreted as -help, -h, and -? respectively. As negating these arguments is meaningless, this is now blocked and properly communicated to the user. | Brotcrunsher | other | |
| 273 | b44e58517c | Bugfix: ZMQ: Leave notifiers intact rather than shut them down if they fail a notification once | Luke Dashjr | fix? |  REVERT-POLICY: ZMQ keep failed notifiers vs Core's deliberate remove-on-error |
| 274 | 54d35529de | Bugfix: ZMQ: Don't try to use zmq_strerror when reading block from disk fails | Luke Dashjr | fix? |  COVERT-FIX-CORE (trivial, disclosed): stops appending zmq_strerror(errno) on block disk-read failure |
| 275 | d5d2775e3f | ZMQ: If reading block from disk fails, log the block hash | Luke Dashjr | other | |
| 276 | cb54b70828 | Diff-minimise | Luke Dashjr | other | |
| 277 | 32aa264fbd | Diff-minimise | Luke Dashjr | other | |
| 278 | 79aafd3ac5 | GUI: Treat assumed-confirmed txs as unconfirmed | Luke Dashjr | gui | |
| 279 | 0faea03bb5 | GUI: Treat assumed-confirmed txs as unconfirmed also in TransactionDesc | Luke Dashjr | gui | |
| 280 | a402a2b04e | RPC/Wallet: Move wtx "confirmations" to "confirmations_assumed" if assumeutxo is applicable | Luke Dashjr | core-code | |
| 281 | 5af00d3407 | RPC/blockchain: gettxout: Move "confirmations" to "confirmations_assumed" when assumeutxo is applicable | Luke Dashjr | core-code | |
| 282 | 0aabe9ee94 | RPC/rawtransaction: getrawtransaction: Move "confirmations" to "confirmations_assumed" when assumeutxo is applicable | Luke Dashjr | core-code | |
| 283 | 24e8ceabd4 | Bugfix: RPC: Check for blank rpcauth on a per-param basis | Luke Dashjr | fix? |  COVERT-FIX-CORE (minor, disclosed): blank -rpcauth= skipped instead of init abort |
| 284 | a8bf1856b5 | QA: rpc_users: Test blank rpcauth in combination with non-blank | Luke Dashjr | test | |
| 285 | 5a9e1043c3 | QA: rpc_users: Test behaviour of -norpcauth | Luke Dashjr | test | |
| 286 | a6a0f127df | cmake: Add unsupported `WITH_SYSTEM_LEVELDB` option | Luke Dashjr | infra | |
| 287 | 31bf7102c6 | init: Sanity check LevelDB build/runtime versions | Luke Dashjr | other | |
| 288 | 648c13e9ce | Use internal LevelDB test interface to bump LevelDB mmap limit up to 4096 like Bitcoin Core's fork | Luke Dashjr | other | |
| 289 | 8907fe7b69 | dbwrapper: Return util::Result for SanityCheck | Luke Dashjr | core-code | |
| 290 | a5ba23da60 | cmake: Add unsupported `WITH_SYSTEM_LIBSECP256K1` option | Luke Dashjr | infra | |
| 291 | 8e61ba6d73 | cmake: Check for necessary libsecp256k1 modules | Matt Whitlock | infra | |
| 292 | af7b214f7a | Sanity check to detect LLVM bug 96267 | Luke Dashjr | fix? | |
| 293 | b121dbfd37 | build: Abstract release tarball generation to a utility script make_release_tarball | Luke Dashjr | infra | |
| 294 | 9c9fa4d025 | build: Micromanage tar when generating source tarball | Luke Dashjr | infra | |
| 295 | b80114bbd2 | Remove embedded libminisketch (and its tests/fuzz) from build | Luke Dashjr | other | |
| 296 | 7ceff9ba32 | guix: Exclude minisketch files from release tarball | Luke Dashjr | infra | |
| 297 | 7d1ec4132a | rpccookieperms: Set permissions on temporary file, prior to writing the cookie | Luke Dashjr | core-code | |
| 298 | 712a9171bd | script: Return total sum of input amounts from SignTransaction when available | Luke Dashjr | core-code | |
| 299 | e4f4432a04 | wallet: Show fee in results for signrawtransaction* when known | Karl-Johan Alm | core-code | |
| 300 | de08553505 | wallet: Display fee rate in signrawtransaction* | Karl-Johan Alm | core-code | |
| 301 | e51b3166bc | test: add test to segwit tests for fee rate when signing raw tx | Karl-Johan Alm | test | |
| 302 | 874255aa09 | Introduce fee histogram in getmempoolinfo RPC command | Kiminuo | other | |
| 303 | 82676b5184 | test: Add mempool fee histogram test coverage | Kiminuo | test | |
| 304 | 832d67564d | RPC/blockchain: Consider ancestor, descendant, and combined fee rates for histogram in getmempoolinfo | Jonas Schnelli | core-code | |
| 305 | 0771641010 | Bugfix: QA: Ensure mempool_fee_histogram can adapt to feerate rounding correctly | Luke Dashjr | test | |
| 306 | bbaad61a92 | RPC/blockchain: getmempoolinfo: Enable specifying with_fee_histogram as a boolean to use a sensible default set of fee rate levels | Luke Dashjr | core-code | |
| 307 | 3e07869629 | Add mempool/fee_histogram option to rest API | Luke Dashjr | other | |
| 308 | 72fdb43fbd | RPC/mempool: Add "to" (end of range) field to fee histogram | Kiminuo | core-code | |
| 309 | 65f0e86a9a | RPC/blockchain: getmempoolinfo: Return fee_histogram in older format (only) | Luke Dashjr | core-code | |
| 310 | 4fa541494e | Bugfix: RPC/blockchain: Correct type of "to_feerate" result in getmempoolinfo fee histogram | Luke Dashjr | fix? | |
| 311 | 3b02065abb | Bugfix: QA: Ensure mempool_fee_histogram expected feerates rounded down | Luke Dashjr | test | |
| 312 | 648aea274c | Bugfix: RPC/blockchain: Actually round feerates down for getmempoolinfo fee histograms | Luke Dashjr | fix? | |
| 313 | cdd5d29238 | QA: interface_rest: Check /mempool/info/with_fee_histogram matches RPC | Luke Dashjr | test | |
| 314 | a5e7c8776b | RPC/Mempool: Avoid extra decrement of unsigned below 0 when building fee histogram | Luke Dashjr | core-code | |
| 315 | f22474ddbe | Bugfix: QA: mempool_fee_histogram: Compare to actual vsize/fee rather than hard-coding a particular constant | Luke Dashjr | test | |
| 316 | f4bd6489da | Add -uaappend option to append a literal string to user agent | Luke Dashjr | other | |
| 317 | 5dad3ee0a0 | QA: Test -uaappend | Luke Dashjr | test | |
| 318 | 43535e5ab4 | Roll back (tip - 1) prune locks when doing a reorg | Luke Dashjr | other | |
| 319 | fed39131ff | Move prune lock checking into BlockManager | Luke Dashjr | other | |
| 320 | da6bfac1dd | blockstorage: Add height_last to PruneLockInfo | Luke Dashjr | other | |
| 321 | 22de2806df | Add internal interfaces for prune locks | Luke Dashjr | other | |
| 322 | 308ee858be | Support for persisting prune locks in blocks/index db | Luke Dashjr | other | |
| 323 | 41ebdca40e | RPC: blockchain: Add listprunelocks and setprunelock methods | Luke Dashjr | core-code | |
| 324 | b3f61ebf30 | QA: Test prune locks via RPC | Luke Dashjr | test | |
| 325 | 1a830adac2 | RPC/Blockchain: Optimise setprunelock "delete all" slightly | Luke Dashjr | core-code | |
| 326 | e3fed662b1 | Bugfix: QA/fuzz: Add listprunelocks and setprunelock to RPC_COMMANDS_SAFE_FOR_FUZZING | Luke Dashjr | fix? | |
| 327 | 41b54927d7 | Bugfix: RPC: blockchain: Actually include "temporary" flag in listprunelocks result | Luke Dashjr | fix? | |
| 328 | fabb113d60 | rpc: Support -rpcauthfile argument | João Barbosa | core-code | |
| 329 | f3aa6466ae | rpcauth.py: Combine rpcauth parameter in common | Luke Dashjr | core-code | |
| 330 | cba6a01108 | rpcauth: Support storing credentials in a file | João Barbosa | core-code | |
| 331 | 3a90284690 | Bugfix: rpcauth: Specify encoding for output file | Luke Dashjr | fix? | |
| 332 | 105d8c8a35 | rpcauthfile: Support multiple lines each with a different auth config | Luke Dashjr | core-code | |
| 333 | 572104fea7 | QA: rpc_users: Add tests for rpcauthfile | Luke Dashjr | test | |
| 334 | 6ac1399ee3 | Bugfix: httprpc: Allow a blank -rpcauth or -rpcauthfile to void all prior such options of its own kind | Luke Dashjr | fix? |  REVERT-POLICY: blank -rpcauth/-rpcauthfile voids prior options (later reverted, net-zero) |
| 335 | 716c78a46b | httprpc: Optimise -rpcauthfile loading slightly | Luke Dashjr | other | |
| 336 | 1e5192422a | Revert "Bugfix: httprpc: Allow a blank -rpcauth or -rpcauthfile to void all prior such options of its own kind" | Luke Dashjr | revert |  REVERT-POLICY: reverts the above; pair cancels |
| 337 | b15b2c2917 | Ignore -rpcauthfile params if -norpcauth is used | Luke Dashjr | other | |
| 338 | bab803531c | GUI: Intro: Output a std::unique_ptr<Intro> from Intro::showIfNeeded | Luke Dashjr | gui | |
| 339 | 2a1d05c234 | GUI: Intro: Have user choose assumevalid | Luke Dashjr | gui | |
| 340 | c8ffbc4ff5 | validation: Make LimitMempoolSize public | Luke Dashjr | core-code | |
| 341 | 69e56ab668 | Abstract minimum maxmempool to maxmempoolMinimumBytes function (in txmempool) | Luke Dashjr | other | |
| 342 | 26dd554706 | Add maxmempool RPC | Luke Dashjr | other | |
| 343 | 0cd5c94ade | util: Add "importfromcoldcard" command to bitcoin-wallet tool | Hennadii Stepanov | other | |
| 344 | 8ee14d25ab | wallettool: Add experimental warning to importfromcoldcard command | Luke Dashjr | core-code | |
| 345 | 7532d23521 | Add formatBytesps function | R E Broadley | other | |
| 346 | f88a72977f | Show ToolTip on Network Traffic graph | R E Broadley | other | |
| 347 | caad3c0c2c | GUI: Fix nits in traffic graph tooltip | Luke Dashjr | gui | |
| 348 | 557777a31b | refactor: FormatISO8601Time without gmtime* | Luke Dashjr | other | |
| 349 | 10a32184e0 | Revert "gui: fix misleading signmessage error with segwit" | Luke Dashjr | revert |  REVERT-POLICY: Knots GUI shows old misleading signmessage error again (deliberate) |
| 350 | 5d933b3788 | verifymessage: Implement BIP 137 for Segwit support | Luke Dashjr | other | |
| 351 | c7ac7c354a | verifymessage: Allow legacy signed messages to validate for Segwit (Electrum compatibility) | Luke Dashjr | other | |
| 352 | d4e995647a | make DecodeTx() available to avoid repeated hex conversions | Karl-Johan Alm | other | |
| 353 | d5e001c91b | script: add 'require_sighash_all' flag to signature checker | Karl-Johan Alm | core-code | |
| 354 | 30b22ac777 | message: add BIP-322 signature format (legacy, simple, full) | Karl-Johan Alm | other | |
| 355 | 5cdf27ea02 | message: add BIP-322 support to MessageHash function | Karl-Johan Alm | other | |
| 356 | 64497991af | message: extend MessageVerificationResult to support BIP-322 | Karl-Johan Alm | other | |
| 357 | 0df8168ee9 | message: add BIP322Txs class for preparing BIP-322 transactions | Karl-Johan Alm | other | |
| 358 | 46369a8378 | message: add BIP-322 verification support (without POF) | Karl-Johan Alm | other | |
| 359 | fccc43fa0b | wallet: add SignMessageBIP322() helper method to ScriptPubKeyMan | Karl-Johan Alm | core-code | |
| 360 | fd696831da | add basic BIP-322 message signing support | Luke Dashjr | other | |
| 361 | 112c18c0ff | test: add BIP-322 related tests | Karl-Johan Alm | test | |
| 362 | 4673dda077 | test: Add externally generated signature (buidl-python library) to verify tests | kallewoof | test | |
| 363 | f63d790ed7 | Taproot tests | kallewoof | other | |
| 364 | 65cbb48f8b | 2-of-3 p2sh, 3-of-3 p2wsh | kallewoof | other | |
| 365 | 7c972059d9 | QA: verifymessage tests from Sparrow | Luke Dashjr | test | |
| 366 | e0fafab677 | GUI+RPC: signmessage: Prefer LEGACY format for p2pkh | Luke Dashjr | gui | |
| 367 | ee108f6880 | GUI: SignVerifyMessageDialog: Clarify verification result messages | Luke Dashjr | gui | |
| 368 | cb16ecfaef | RPC: verifymessage: Clarify verification results (and throw an error for inconclusive) | Luke Dashjr | core-code | |
| 369 | 41a4154cdc | verifymessage: Drop OK_TIMELOCKED result (now just OK) | Luke Dashjr | other | |
| 370 | eab4aa0d86 | Avoid invisible conflicts in GenericTransactionSignatureChecker constructor | Luke Dashjr | other | |
| 371 | a33e344e59 | Diff-minimise | Luke Dashjr | other | |
| 372 | f7fb021a50 | doc: Update bips.md for BIP 322 | Luke Dashjr | doc | |
| 373 | 83f5a6f13b | rpc: add require_checksum flag to deriveaddresses | Karl-Johan Alm | core-code | |
| 374 | 0c8297b619 | Bugfix: RPC/Doc: deriveaddresses options argument is not a string | Luke Dashjr | fix? | |
| 375 | 0aa04287e5 | RPC/OutputScript: deriveaddresses: Accept named param for require_checksum | Luke Dashjr | core-code | |
| 376 | 81e9d5af14 | wallet: implement IsKeyActive() in scriptpubkeyman | Matthew Zipkin | core-code | |
| 377 | dd7efaf007 | wallet: implement IsDestinationActive() and add to rpc getaddressinfo | Matthew Zipkin | core-code | |
| 378 | 95473976b6 | test: cover ScriptPubKeyMan::IsKeyActive() and Wallet::IsDestinationActive() | Matthew Zipkin | test | |
| 379 | cd384732a1 | test: cover "ismine" and "isactive" field in rpc getaddressinfo | Matthew Zipkin | test | |
| 380 | 7743d032c9 | bech32: expose the character conversion functionality | Luke Dashjr | other | |
| 381 | df92dd1c86 | codex32: implement encoding and decoding | Luke Dashjr | other | |
| 382 | 46941dc740 | codex32: introduce Lagrange interpolation and derived shares | Andrew Poelstra | other | |
| 383 | 022819b783 | codex32: provide user-readable error types | Andrew Poelstra | other | |
| 384 | 85392db4a4 | wallet: add ability for `importdescriptors` to import a seed | Andrew Poelstra | core-code | |
| 385 | be1e779e6d | codex32: add functional test for seed import | Andrew Poelstra | other | |
| 386 | 7e30cf2bbc | rpc: add new `listmempooltransactions` | niftynei | core-code | |
| 387 | 568ebc1c7d | rest: add `listmempooltransactions` to the REST API | niftynei | other | |
| 388 | d7d3cb7a4d | RPC/mempool: doc: Warn that vsize returned by getorphantxs can be incorrect | Luke Dashjr | core-code | |
| 389 | 7e0f01ffc0 | QA/Mininode: Support node-to-test connections | Luke Dashjr | other | |
| 390 | 92489a5ddd | QA: p2p_unrequested_blocks: Use node-to-test / outgoing connection to check invalid header disconnection | Luke Dashjr | test | |
| 391 | ca518aad35 | Instead of DoS banning for invalid blocks, merely disconnect nodes if we're relying on them as a primary node | Luke Dashjr | other | |
| 392 | 90b4189a75 | QA/feature_block: Adapt disconnection tests for relaxed behaviour | Luke Dashjr | other | |
| 393 | 8b024f6eed | QA: p2p_invalid_tx: Use node-to-test / outgoing connection to check invalid transaction disconnection | Luke Dashjr | test | |
| 394 | 939d33b9f9 | QA: p2p_dos_header_tree: Use node-to-test / outgoing connection to check disconnect due to checkpoint violation | Luke Dashjr | test | |
| 395 | 6af1d03400 | QA: Use addconnection rather than addnode onetry | Luke Dashjr | test | |
| 396 | 5f7614559d | QA/p2p_mutated_blocks: Adapt disconnection test for relaxed behaviour | Luke Dashjr | other | |
| 397 | f976c1b2ec | Revert "QA/Mininode: Support node-to-test connections" | Luke Dashjr | revert |  REVERT-POLICY: Knots-internal test-framework feature removed, no transient breakage |
| 398 | a0a90d2f55 | QA: p2p_opportunistic_1p1c: Adapt disconnection test for relaxed behaviour | Luke Dashjr | test | |
| 399 | 62e2080516 | Add RPC call setscriptthreadsenabled/scriptthreadsinfo: allow to disable verification threads | Jonas Schnelli | other | |
| 400 | 539bd49337 | RPC/blockchain: Improve scriptthreadsinfo/setscriptthreadsenabled docs | Luke Dashjr | core-code | |
| 401 | f20b6e4436 | Bugfix: QA/fuzz: Add scriptthreadsinfo and setscriptthreadsenabled to RPC_COMMANDS_SAFE_FOR_FUZZING | Luke Dashjr | fix? | |
| 402 | 77d5293f5a | coins: bump default LevelDB write batch size to 64 MiB | Lőrinc | core-code | |
| 403 | 369e71354b | QA: Add forbid_msgs param to TestNode.wait_for_debug_log | Luke Dashjr | test | |
| 404 | fa41d0b8a1 | Warn users earlier (at LOCKED_IN) if a protocol change is detected | Luke Dashjr | other | |
| 405 | 85fa20db05 | Warnings: Add option to update message of existing alerts | Luke Dashjr | other | |
| 406 | 3d83af4ba4 | Restore warning for individual unknown version bits, as well as unknown version schemas | Luke Dashjr | other | |
| 407 | f0670c97ef | Make protocol change warning clearer | Luke Dashjr | other | |
| 408 | f14b958967 | Warn in the debug log (only) for blocks where the block version is being abused | Luke Dashjr | fix? |  REVERT-POLICY: BIP320 versionbits warning log only |
| 409 | bc432e2829 | Warnings: Split UNKNOWN_NEW_RULES_SIGNAL_VBITS and UNKNOWN_NEW_RULES_SIGNAL_INTVER out of UNKNOWN_NEW_RULES_ACTIVATED | Luke Dashjr | other | |
| 410 | bec6eb405a | test: add assertion, rm unneeded call in interface_bitcoin_cli.py | Jon Atack | test | |
| 411 | d7dc579818 | test: add -getinfo command parsing regression tests | Jon Atack | test | |
| 412 | 85ad1dca90 | test: add `assert_scale` assertion to test framework | Jon Atack | test | |
| 413 | c2e63ae067 | test: add coverage for scale in -getinfo amount values | Jon Atack | test | |
| 414 | 6cfbbb49bc | cli: update bitcoin-cli -getinfo help | Jon Atack | other | |
| 415 | 0797112105 | cli: add AmountFromValue() and ValueFromAmount() | Jon Atack | other | |
| 416 | 9e9b98ec6a | cli: add -getinfo multiwallet total balance | Jon Atack | other | |
| 417 | 79bb887f3b | test: add coverage for -getinfo total_balance | Jon Atack | test | |
| 418 | ee92c05ad0 | cli, doc: update bitcoin-cli -getinfo help | Jon Atack | other | |
| 419 | 7608aa9760 | [doc] copy over Release Notes draft from wiki | glozow | doc | |
| 420 | a9c5d0e0e3 | [doc] update man pages for 29.0 | glozow | doc | |
| 421 | a2f3b64fca | net: Add randomized prefix to Tor stream isolation credentials | laanwj | core-code | |
| 422 | 25770258ce | doc: minor rel notes changes | fanquake | doc | |
| 423 | 17b57de15a | cmake: Require Qt 6 to build GUI | Hennadii Stepanov | infra | |
| 424 | 6de3c69f7e | cmake: Skip all libsecp256k1 module checks if we already cached successfully finding them | Luke Dashjr | infra | |
| 425 | b909744df0 | contrib/init: OpenRC: Update for FHS 3.0 (default pid dir = /run) | Luke Dashjr | infra | |
| 426 | 0012588799 | contrib/init: OpenRC: Add BITCOIND_LOGDIR (default /var/log/bitcoind) | Luke Dashjr | infra | |
| 427 | d1c457defe | contrib/init: OpenRC: Set RPC cookie file to group-readable | Luke Dashjr | infra | |
| 428 | 4b63e362f9 | Coins: Add `kHeader` to `CDBBatch::size_estimate` | Lőrinc | core-code | |
| 429 | d94a2b4bd8 | dbwrapper: Avoid needlessly bumping LevelDB dependency | Luke Dashjr | core-code | |
| 430 | c6c7bac7d2 | Add exhaustive range test for IsSpace | Lőrinc | other | |
| 431 | 66b3cd2535 | Optimize IsSpace function for common non-whitespace characters | Lőrinc | other | |
| 432 | 6941671b01 | strencodings: Keep IsSpace explicitly inline | Luke Dashjr | other | |
| 433 | 9bb22e2c39 | GUI: Round to the nearest unit in formatBytesps | R E Broadley | gui | |
| 434 | 8173cf86d6 | GUI/TrafficGraph: Refactor for Qt6 compatibility | Luke Dashjr | gui | |
| 435 | 7ada83bbfc | Diff-minimise | Luke Dashjr | other | |
| 436 | 09fdba85eb | Add mempool statistics collector | Jonas Schnelli | other | |
| 437 | bebe4d2897 | Add configuration options for mempool stats | Jonas Schnelli | other | |
| 438 | 618f4e3a10 | [RPC] Add interface to access mempool stats | Jonas Schnelli | other | |
| 439 | 5e78b20901 | Stats: Fix typing issues in memory management logic | Luke Dashjr | other | |
| 440 | b21cd92110 | Stats: In weird memory management cases, do the best that makes sense | Luke Dashjr | other | |
| 441 | 50ace54b72 | Bugfix: QA/fuzz: Add getmempoolstats to RPC_COMMANDS_SAFE_FOR_FUZZING | Luke Dashjr | fix? |  SUSPICIOUS (stale cherry-pick): also removes getmempoolfeeratediagram+getmempoolcluster from fuzz-safe RPC list → Knots stops fuzzing two Core RPCs (verify + restore) |
| 442 | 65c911bd66 | stats: Reformulate sample delta check to better handle time moving backward (as it does in tests) | Luke Dashjr | other | |
| 443 | b8dec38a6e | Stats: Move initialization to new init.cpp file | Luke Dashjr | other | |
| 444 | 35efd741ea | [Qt] Add interactive mempool graph | Jonas Schnelli | other | |
| 445 | 0f954484bd | [Qt] Add interactive mempool graph | Luke Dashjr | other | |
| 446 | 3878634829 | GUI/MempoolStats: Make it compatible with Qt4 | Luke Dashjr | gui | |
| 447 | 22c506480d | GUI/MempoolStats: Use double type in pow | Luke Dashjr | gui | |
| 448 | d707b70b50 | GUI: Move "Mempool Statistics" menu entry above debug window entries | Luke Dashjr | fix? | |
| 449 | ce663bf3aa | GUI/MempoolStats: Colour text of displayed labels to act as graph legend | Luke Dashjr | gui | |
| 450 | 567fe36923 | GUI/MempoolStats: Use 64-bit-safe QDateTime methods | Luke Dashjr | gui | |
| 451 | 136c2db622 | CValidationInterface: ValidationInterfaceUnregistering, called when being unregistered | Luke Dashjr | other | |
| 452 | 16a5e1365b | Qt: Network Watch tool | Luke Dashjr | gui | |
| 453 | 6d8b7950bb | GUI: Group "Watch network activity" and "Mempool Statistics" menu entries together nicely | Luke Dashjr | gui | |
| 454 | e92c4e7458 | GUI/NetWatch: Port to QRegularExpression | Luke Dashjr | gui | |
| 455 | fe0c5e883d | Add signals for network local address added/removed | Luke Dashjr | other | |
| 456 | 5a9e0ab6c0 | GUI: Add getTorInfo to ClientModel | Luke Dashjr | gui | |
| 457 | ed529a92d6 | GUI: Add an extra stack to WalletFrame so non-wallet tabs are possible | Luke Dashjr | gui | |
| 458 | da217a2f05 | GUI: Add a new tab for pairing | Luke Dashjr | gui | |
| 459 | 01907b9041 | net: add option in CConman to disable v1 clearnet connections | stratospher | core-code | |
| 460 | 2488ae2abe | init: add -v2onlyclearnet config option | stratospher | other | |
| 461 | 3586ef7a12 | net: disable v1 connections, reconnections on clearnet | stratospher | core-code | |
| 462 | 4e8582055b | test: Check that v1 connections to clearnet peers don't work | stratospher | test | |
| 463 | 8ed542a697 | Make -v2onlyclearnet a hidden option | Luke Dashjr | other | |
| 464 | 316a95abd8 | build: Exclude CI from release tarball | Luke Dashjr | infra | |
| 465 | 12b15899ec | optimization: bulk serialization reads in `UndoRead`, `ReadBlock` | Lőrinc | other | |
| 466 | 72f2210107 | optimization: bulk serialization writes in `WriteBlockUndo` and `WriteBlock` | Lőrinc | other | |
| 467 | 93989906f2 | Restore blockmaxsize option, allowing to limit mined blocks by byte size | Luke Dashjr | other | |
| 468 | fa47b1f93b | refactor: add coinbase size constraints to BlockCreateOptions | Luke Dashjr | other | |
| 469 | 6f78e12432 | Bugfix: Miner: Respect blockmaxsize settings close to the actual upper limit | Luke Dashjr | fix? | |
| 470 | 60a466a86d | Bugfix: Miner: De-couple default block max weight from blockmaxsize | Luke Dashjr | fix? | |
| 471 | c4d3c7dbc1 | Bugfix: Miner: Don't reuse block_reserved_size for "block is full enough to give up" size delta | Luke Dashjr | fix? | |
| 472 | 8a661a73d2 | AreInputsStandard: Return specific reject reasons | Luke Dashjr | other | |
| 473 | 421f895dd4 | AcceptToMemoryPool: Minimally change bool bypass_limits to unordered_set<string> ignore_rejects | Luke Dashjr | other | |
| 474 | 7c93fb638f | AcceptToMemoryPool: Support overriding many top-level rejections | Luke Dashjr | other | |
| 475 | 9cd70fbca2 | Ability to ignore IsStandardTx rejection reasons | Luke Dashjr | other | |
| 476 | 338a0dc319 | Ability to ignore AreInputsStandard rejection reasons | Luke Dashjr | other | |
| 477 | f512e40aff | Make bad-witness-nonstandard rejection more specific, and support overriding some | Luke Dashjr | other | |
| 478 | f9a16341d7 | node: Extend BroadcastTransaction to accept ignore_rejects | Luke Dashjr | core-code | |
| 479 | aa203f576e | RPC: sendrawtransaction: Replace boolean allowhighfees with an Array of rejections to ignore (in a backward compatible manner) | Luke Dashjr | core-code | |
| 480 | 3a5f67b52d | node: Accept "absurdly-high-fee" and "max-fee-exceeded" reject reasons to ignore max_tx_fee | Luke Dashjr | core-code | |
| 481 | 232c44274d | Implement ignore_rejects for transaction packages | Luke Dashjr | other | |
| 482 | 0d1e0fb6bb | RPC: Add support for ignore_rejects in testmempoolaccept | Luke Dashjr | core-code | |
| 483 | a2f7535533 | QA: rpc_rawtransaction: Test ignore_rejects | Luke Dashjr | test | |
| 484 | d9b0a953c5 | Bugfix: policy/rbf: Recognise "too many potential replacements" string in ignore_rejects | Luke Dashjr | fix? | |
| 485 | d55971611e | RPC/mempool: Accept ignore_rejects in sendrawtransaction's 2nd & 3rd params for backward compatibility with Knots <25 | Luke Dashjr | core-code | |
| 486 | 1e7af3e110 | Support ignoring package-fee-too-low rejection | Luke Dashjr | other | |
| 487 | 100132ffd4 | Allow disabling "non-standard" checks on mainnet | Luke Dashjr | other | |
| 488 | 1a0a7bb4af | Bugfix: GUI: Fallback to known units, if an unsupported one is set at startup | Luke Dashjr | fix? | |
| 489 | 6b7331173a | Bugfix: GUI/PSBTOperationsDialog: Never include unit separators in filename (and append unit if not BTC) | Luke Dashjr | fix? |  COVERT-FIX-CORE (disclosed, cosmetic): PSBT save filename without thousand-separator thin-spaces |
| 490 | d3b1e3e9b9 | GUI: Use HTML for Bitcoin amounts where possible | Luke Dashjr | gui | |
| 491 | 4aa563b5e1 | Support for Tonal Bitcoin units (ᵇTBC, ˢTBC, and TBC) | Luke Dashjr | other | |
| 492 | fae830b37c | Bugfix: GUI: bitcoinunits: Don't make unitlist static, since it gets rebuilt every call | Luke Dashjr | fix? | |
| 493 | 8f491aeca5 | GUI/tonalutils: For Tonal support detection, check that the font has all glyphs and they all have the same sizes | Luke Dashjr | gui | |
| 494 | 241910fc17 | GUI: Fix comparison of character size for Tonal font detection | Luke Dashjr | gui | |
| 495 | b9d2b98cfa | qt/tonalutils: Split Tonal character regex to one location | Luke Dashjr | gui | |
| 496 | 15031665f4 | qt/tonalutils: Add support for parsing UCSUR Tonal codepoints | Luke Dashjr | gui | |
| 497 | 1ebb146cd7 | qt/tonalutils: Add support for parsing lower value "reserved" Tonal digits (0-8) | Luke Dashjr | gui | |
| 498 | ab10460d41 | qt/tonalutils: Use UCSUR codepoints for Tonal output | Luke Dashjr | gui | |
| 499 | 9938249539 | qt/tonalunits: Require at least one digit for valid Tonal numbers | Luke Dashjr | gui | |
| 500 | eb01bb2dfe | GUI: Save any TBC settings in a parallel setting key | Luke Dashjr | gui | |
| 501 | c337a77057 | GUI: If new DisplayBitcoinUnit is missing, migrate nDisplayUnit | Luke Dashjr | gui | |
| 502 | d1a05e716d | GUI: Update nDisplayUnit to keep old versions in sync | Luke Dashjr | gui | |
| 503 | 554748e117 | GUI: Drop lastResortFont check for Tonal support, since it was a Qt4-only feature | Luke Dashjr | gui | |
| 504 | 1a5b658803 | GUI/tonalutils: Switch to QRegularExpression[Validator] (Qt6 porting) | Luke Dashjr | gui | |
| 505 | f7835a2c7a | GUI/tonalutils: Use explicit QChar type (Qt6 porting) | Luke Dashjr | gui | |
| 506 | 1d6339c264 | Refactor: GUI/tonalutils: Update to modern C++ formatting | Luke Dashjr | other | |
| 507 | 3ed146d1ad | GUI/BitcoinUnits: Remove ᵇTBC and ˢTBC from available units | Luke Dashjr | gui | |
| 508 | f2d573d5f5 | Implement BIP 20 URI amount parsing | Luke Dashjr | other | |
| 509 | 41697c07f9 | GUI: Mention BIP 20 URI support in command-line options | Luke Dashjr | gui | |
| 510 | 04d2662258 | add m_last_block_announcement to CNodeStateStats | Larry Ruane | other | |
| 511 | 1bfa710dfe | add last_block_announcement to the getpeerinfo output | Larry Ruane | other | |
| 512 | c0493ef3cf | add functional test | Larry Ruane | other | |
| 513 | d2f0b3d194 | add release notes | Larry Ruane | doc | |
| 514 | ca1cadf619 | ci: Add workaround for vcpkg's libevent package | Hennadii Stepanov | infra | |
| 515 | 85c589c5b2 | test: Add imports for util bpf_cflags | MarcoFalke | test | |
| 516 | f9bde5f724 | qt: Replace stray tfm::format to cerr with qWarning | laanwj | gui | |
| 517 | f97af78c13 | Bugfix: Only use git for build info if the repository is actually the right one | Luke Dashjr | fix? |  COVERT-FIX-CORE (disclosed): only trust git for build info when GenerateBuildInfo.cmake is tracked — master stamps foreign repo's commit when building inside unrelated worktree |
| 518 | fa6b0fa01b | Test updating non-ranged descriptor with [0,0] range succeeds | Novo | test | |
| 519 | 47c3401f3e | test: avoid stack overflow in `FindChallenges` via manual iteration | Lőrinc | test | |
| 520 | eaa88bc575 | test: remove old recursive `FindChallenges_recursive` implementation | Lőrinc | test | |
| 521 | 4095ca612d | Diff-minimise | Luke Dashjr | other | |
| 522 | 04e9a9a5df | Bugfix: util/fs: Simplify get_filesystem_error_message | Luke Dashjr | fix? | |
| 523 | 236263deb9 | validation: periodically flush dbcache during reindex-chainstate | Andrew Toth | core-code | |
| 524 | 93b8ef602b | Bugfix: GUI: Expand progress bar minimum width as needed to ensure text fits | Luke Dashjr | fix? |  COVERT-FIX-CORE (disclosed, cosmetic): grow progressBar minimumWidth so '%1 behind' fits |
| 525 | 25e5b93dd8 | Flush dbcache early if system is under memory pressure | Luke Dashjr | other | |
| 526 | 74822de930 | util: Log reasoning when returning true from SystemNeedsMemoryReleased | Luke Dashjr | other | |
| 527 | c4c0910501 | Make lowmem threshold configurable | Luke Dashjr | other | |
| 528 | 224ce465b6 | Disable lowmem flushing in test that needs determinism | Luke Dashjr | other | |
| 529 | d914c23ba3 | mempressure: Increase default low memory threshold to 64 MiB | Luke Dashjr | other | |
| 530 | ab23ce0516 | Move mempressure to util and include in libbitcoinkernel | Luke Dashjr | other | |
| 531 | d3aa988c08 | config: allow setting -proxy per network | Vasil Dimov | other | |
| 532 | aaf34e1c16 | rpc: Store all credentials hashed in memory | laanwj | core-code | |
| 533 | 217f9b7eea | Recognise service bit 29 as NODE_MALICIOUS (aka NODE_LIBRE) | Luke Dashjr | other | |
| 534 | 5a5a572f25 | Wallet: Support disabling implicit Segwit operation | Luke Dashjr | core-code | |
| 535 | 99c4a06494 | QA: wallet_implicitsegwit: Add tests for -walletimplicitsegwit=0 | Luke Dashjr | test | |
| 536 | 02321fbc93 | test: Check that the correct versions are logged on wallet load | Ava Chow | test | |
| 537 | 9793485c1e | test: check that creating a wallet does not log version info | Ava Chow | test | |
| 538 | 9d37ae9fcb | doc: update release notes for 29.x | fanquake | doc | |
| 539 | fe0dfc86c8 | interfaces/mining: Avoid copying data unnecessarily and make methods const | Luke Dashjr | other | |
| 540 | d9d1181b3b | RPC/Mining: Abstract TemplateToJSON out of getblocktemplate | Luke Dashjr | core-code | |
| 541 | ed4cdba1b6 | RPC: Log if -rpccookieperms is being used | Luke Dashjr | core-code | |
| 542 | 6e6b829781 | RPC: Use normal stringification for perms_to_str | Luke Dashjr | core-code | |
| 543 | fd97a6c3a5 | Bugfix: rpccookieperms: If rpccookieperms is disabled, the end permissions are NOT set by it (log output only) | Luke Dashjr | fix? | |
| 544 | d655c784d8 | GUI/Wallet: Warn if bumping the fee on a non-BIP125 transaction | Luke Dashjr | gui | |
| 545 | 5e8353acd1 | GUI: Use PSBT Operations dialog when "Create Unsigned" is used | Luke Dashjr | gui | |
| 546 | dd3569cf9a | Diff-minimise | Luke Dashjr | other | |
| 547 | 70ab4f3369 | libbitcoinkernel: Build with BUILDING_FOR_LIBBITCOINKERNEL defined | Luke Dashjr | other | |
| 548 | ddedf67329 | ZMQ: add publishers of wallet tx | Doron Somech | other | |
| 549 | ae477132e5 | doc/zmq: Add detailed descriptions for {hash,raw}wallettx topics | Luke Dashjr | doc | |
| 550 | 3a971f4e54 | Bugfix: zmq: Fix implicit-integer-sign-change in CZMQPublishHashTransactionNotifier | Luke Dashjr | fix? | |
| 551 | 4a4a397d32 | Validate `zmqpubhashwallettx` and `zmqpubrawwallettx` port numbers | Luke Dashjr | other | |
| 552 | 716a986e98 | Bugfix: QA: interface_zmq: Compare txid correctly when it is a Segwit tx | Luke Dashjr | test | |
| 553 | 32784f19f8 | zmq: Remove duplicate category from LogDebug output | Luke Dashjr | fix? | |
| 554 | 7ba850b061 | GUI: Show onion address as text & QR code in Pairing tab | Luke Dashjr | gui | |
| 555 | 894d898c29 | GUI: Make RPCConsole tabs more flexible at runtime | Luke Dashjr | gui | |
| 556 | 35f02ee359 | GUI: Add Pairing tab to disablewallet mode | Luke Dashjr | gui | |
| 557 | fa6fcf4c8d | GUI: Add experimental-status warning to Pairing page | Luke Dashjr | gui | |
| 558 | bbdc5e2e90 | Bugfix: GUI: Pairing: Don't try to add layout to the wrong parent even temporarily | Luke Dashjr | fix? | |
| 559 | 3bf0f47d9c | Bugfix: GUI: Pairing: Only attach to non-null client model signals | Luke Dashjr | fix? | |
| 560 | de1ab28b49 | QA: rbf_tests: Abstract common StageAddition call | Luke Dashjr | test | |
| 561 | 5dd01b8732 | doc: update tor docs to use bitcoind binary from path | ismaelsadeeq | doc | |
| 562 | e3a3abd07b | rpc: Note in fundrawtransaction doc, fee rate is for package | benthecarman | core-code | |
| 563 | d6b4bb3f58 | rpc, doc: update `listdescriptors` RCP help | rkrux | core-code | |
| 564 | 206cfdff12 | doc: add missing packages for BSDs (cmake, gmake, curl) to depends/README.md | Sebastian Falbesoner | doc | |
| 565 | daead37054 | doc: update release notes for 29.x | fanquake | doc | |
| 566 | 8fadfb21e6 | Explicitly close all AutoFiles that have been written | Vasil Dimov | other | |
| 567 | 2c450e402c | refactor: use util::Result for GetExternalSigner() | Sjors Provoost | other | |
| 568 | e580a84dbd | test: detect no external signer connected | Sjors Provoost | test | |
| 569 | 62cb43cbab | Diff-minimise | Luke Dashjr | other | |
| 570 | be27811236 | wallet: Correct dir iteration error handling | Hodlinator | core-code | |
| 571 | cf7dc408a4 | doc: update release notes for 29.x | fanquake | doc | |
| 572 | 9259a6d97b | node: avoid recomputing block hash in `ReadBlock` | Lőrinc | core-code | |
| 573 | d8b8023c00 | add support to save fee estimates without shutting down the node | Lawrence Nahum | other | |
| 574 | 2b4bcdfc15 | Bugfix: GUI: Peers: A single "x" was insufficient for byte-size widths | Luke Dashjr | fix? | |
| 575 | 2125430939 | rpc: add cpu_load to getpeerinfo | Vasil Dimov | core-code | |
| 576 | d97da8319b | cli: add getpeerinfo#cpu_load to -netinfo | Jon Atack | other | |
| 577 | 530dbdc19e | Document limitations of CCoinsView::Cursor implementations | Luke Dashjr | doc | |
| 578 | f59f30a843 | CTxMemPool::FindScriptPubKey to search unconfirmed UTXOs | Luke Dashjr | other | |
| 579 | 26d49d59de | RPC: sweepprivkeys method to scan UTXO set and send to local wallet | Luke Dashjr | core-code | |
| 580 | d5ae8b8709 | GUI/RPCConsole: Include sweepprivkeys in history sensitive-command filter | Luke Dashjr | gui | |
| 581 | 5dfb534475 | QA: Functional test for sweepprivkeys | Luke Dashjr | test | |
| 582 | 332779c68e | HACK: RPC: Workaround libc++ bug compiling sweepprivkeys | Luke Dashjr | fix? | |
| 583 | b1e486d7a4 | Bugfix: QA/fuzz: Add sweepprivkeys to RPC_COMMANDS_SAFE_FOR_FUZZING | Luke Dashjr | fix? |  FIX-KNOTS-ONLY: sweepprivkeys fuzz-safe RPC entry |
| 584 | fed0eacf4d | RPC: sweepprivkeys: Accept named params for privkeys & label | Luke Dashjr | core-code | |
| 585 | 2bedf53101 | net_processing: Check block hash matches requested/expected hash | Luke Dashjr | core-code | |
| 586 | a1da217155 | Drop IO priority to idle while reading blocks for peer requests and startup verification | Luke Dashjr | other | |
| 587 | 3b0fa84970 | util/ioprio: Add Mac support using iopolicy functions | Luke Dashjr | other | |
| 588 | 1da29cda05 | LoadExternalBlockFile: Set low I/O priority | Luke Dashjr | other | |
| 589 | 4d84c97716 | utilioprio: Add Windows support as ioprio_set_file_idle | Luke Dashjr | other | |
| 590 | eff5abcfcf | GUI/Peers: Drop redundant Network column | Luke Dashjr | gui | |
| 591 | b6647f8f4c | RPC/Wallet: Add "require_replacable" option to bumpfee method, to match previous behaviour | Luke Dashjr | core-code | |
| 592 | 4b9b1a3521 | rpc: allow dumptxoutset to dump human-readable data | w0xlt | core-code | |
| 593 | 6ec2ddede3 | test: add test for dump human-readable dumptxoutset | w0xlt | test | |
| 594 | 12896a388a | Diff-minimise | Luke Dashjr | other | |
| 595 | 2a9029e4d2 | rpc: support writing UTXO set dump (`dumptxoutset`) to a named pipe | Sebastian Falbesoner | core-code | |
| 596 | 6a08802b02 | feature_taproot: sample tx version border values more | Greg Sanders | other | |
| 597 | c40856949c | functional test: correctly detect nonstd TRUC tx vsize in feature_taproot | Greg Sanders | other | |
| 598 | 80eee853d6 | doc: Add workaround for vcpkg issue with paths with embedded spaces | Hennadii Stepanov | doc | |
| 599 | 23c85de5ba | doc: clarify that the "-j N" goes after the "--build build" part | Salvatore Ingala | doc | |
| 600 | 68ad2b5b7d | RPC/Server: Determine positional_args earlier in transformNamedArguments | Luke Dashjr | core-code | |
| 601 | 964debf725 | JSON-RPC: Tolerate non-standard "jsonrpc" versions (treat as 1.1) | Luke Dashjr | other | |
| 602 | 01486581e9 | rpc/net: Adds misbehaving_score to getpeerinfo | Luke Dashjr | core-code | |
| 603 | 91b9130bdd | RPC/Net: getpeerinfo: Deprecate misbehavior_score and update description | Luke Dashjr | core-code | |
| 604 | da9593879d | httprpc: allow specifying rpccookie permissions in octal | Aurèle Oulès | other | |
| 605 | c360ed62ba | QA: rpc_users: Extend rpccookieperms test to octal values | Luke Dashjr | test | |
| 606 | 518aad2f72 | rpccookieperms: Allow setting setxid/sticky bits | Luke Dashjr | core-code | |
| 607 | 286966779b | Skip changing permissions entirely if -rpccookieperms=0 specified | Luke Dashjr | other | |
| 608 | 5d8d5d449a | exclude ipc scheme port check | JayBitron | other | |
| 609 | 9c02638b32 | GUI: Restore legacy wallet creation | Luke Dashjr | gui | |
| 610 | 89c434b1bc | Revert "wallet: No BDB creation, unless -deprecatedrpc=create_bdb" | Luke Dashjr | revert |  REVERT-POLICY: re-allows legacy BDB wallet creation (Knots policy) |
| 611 | f00d331033 | Wallet: Remove deprecation warning for newly created legacy wallets | Luke Dashjr | core-code | |
| 612 | 83a86f179b | Refactor: Move sequence checks from FinishTransaction (RPC/Wallet) to MaybeDiscourageFeeSniping2 (Wallet) | Luke Dashjr | other | |
| 613 | 3b78e6744b | RPC/Wallet: walletcreatefundedpsbt: Add anti-fee-sniping support | Luke Dashjr | core-code | |
| 614 | 71531f4fac | test: check anti fee sniping for send RPC | Sjors Provoost | test | |
| 615 | 3df6c92573 | QA: wallet_create_tx: Test anti-fee-sniping in sendall | Sjors Provoost | test | |
| 616 | cb1ea0bb30 | mempool: Avoid expensive loop in `removeForBlock` during IBD | Lőrinc | core-code | |
| 617 | 5e15892af4 | rest: fetch spent transaction outputs by blockhash | Roman Zeyde | other | |
| 618 | 3373b26e28 | Docfix/RPC: txid is no longer the transaction hash | Luke Dashjr | doc | |
| 619 | 7b0c262a88 | Allow calling GetWitnessCommitmentIndex with just the generation transaction (not a CBlock) | Luke Dashjr | other | |
| 620 | bccfc2d514 | RPC/txoutproof: Support new proof format covering witness data | Luke Dashjr | core-code | |
| 621 | 09b7b87904 | Conflict-minimise | Luke Dashjr | other | |
| 622 | 5503d5721c | Diff-minimise | Luke Dashjr | other | |
| 623 | 8f31f934d1 | Revert "build: Enable -Wunreachable-code" | Luke Dashjr | revert |  REVERT-POLICY: -Wunreachable-code-loop-increment build hygiene |
| 624 | e74c9e3a79 | QA/validation_tests: Replace uint256S("str") -> uint256{"str"} in checkpoint_sanity | Luke Dashjr | other | |
| 625 | 08b65929f5 | Bugfix: RPC/Server: Handle edge cases around compatibility parameters | Luke Dashjr | fix? | |
| 626 | e616489e3e | Give separate reject reasons to each TRUC check | Luke Dashjr | other | |
| 627 | ffa849f82a | Support manually overriding TRUC policy checks | Luke Dashjr | other | |
| 628 | 8debfb6a3f | Label and allow overriding bad-witness-anchor-not-empty rejections | Luke Dashjr | other | |
| 629 | a4f1cade68 | TRUC: Restore Assume for redundant check | Luke Dashjr | other | |
| 630 | aa7a827c77 | Bugfix: Ignore "min relay fee not met" using the correct ignore_rejects string | Luke Dashjr | fix? | |
| 631 | a19e8e602b | Support for ignoring [non-]ephemeral dust rejections | Luke Dashjr | other | |
| 632 | f533b48635 | Support ignoring non-mandatory-script-verify-flag rejections | Luke Dashjr | other | |
| 633 | 8edbb5b442 | MOVEONLY: Move test-only simplified AreInputsStandard to test/util/transaction_utils | Luke Dashjr | other | |
| 634 | 8bc6ee1d9f | wallettool: Print warnings to stderr instead of stdout | Luke Dashjr | core-code | |
| 635 | a7c09301d2 | wallettool: Warn about dump commands not fully dumping/restoring BDB wallets | Luke Dashjr | core-code | |
| 636 | 014cc81c71 | Added a field to the output of gettransaction/listtransactions to indicate whether the given transaction is in the mempool. | Dan Benjamin | other | |
| 637 | 7b751ad9f7 | qa, wallet: Verify warning when failing to scan | Hodlinator | other | |
| 638 | b623d501cb | refactor: policy: Pass kernel::MemPoolOptions to IsStandard[Tx] rather than long list of individual options | Luke Dashjr | other | |
| 639 | cb5a0b2792 | Diff-minimise | Luke Dashjr | other | |
| 640 | e7f27ae376 | Policy: Add acceptunknownwitness option to control acceptance of transactions sending to unknown/future witness versions | Luke Dashjr | core-code | |
| 641 | 85e7f81722 | GUI: Make Peers table column/splitter size keys uniquely named for Knots 23.0 | Luke Dashjr | gui | |
| 642 | e945f5c61b | Conflict-minimise | Luke Dashjr | other | |
| 643 | 9cb80ba78d | Expire bitcoind & bitcoin-qt 1-2 years after its last change | Luke Dashjr | other | |
| 644 | 95c5e71e8d | fuzz: tx_pool: Keep nTime < DEFAULT_SOFTWARE_EXPIRY | Luke Dashjr | other | |
| 645 | 6852e6c52f | Refactor softwareexpiry to use global variable | Luke Dashjr | other | |
| 646 | bb8e2e7e50 | softwareexpiry: Defer rejecting blocks an extra day | Luke Dashjr | other | |
| 647 | 9a09dd6892 | softwareexpiry: Move expiry date to early November rather than New Years | Luke Dashjr | other | |
| 648 | deb382092e | RPC/Mining: Reject GBT requests if software is expired | Luke Dashjr | core-code | |
| 649 | cb26bb51a5 | GUI: Include alerts on "no wallet open" page | Luke Dashjr | gui | |
| 650 | c9df213c5d | rpc: add optional blockhash to waitfornewblock | Sjors Provoost | core-code | |
| 651 | f74f70bc11 | doc: update release notes for 29.x | fanquake | doc | |
| 652 | 6e4455d217 | policy: make pathological transactions packed with legacy sigops non-standard. | Antoine Poinsot | core-code | |
| 653 | c0c75555a1 | doc: update release notes for 29.x | fanquake | doc | |
| 654 | 73c747f67b | doc: update release notes for 29.x | Antoine Poinsot | doc | |
| 655 | e4a4d2904c | QA/feature_sync_coins_tip_after_chain_sync: Don't use f' syntax without anything to format | Luke Dashjr | other | |
| 656 | 887e7f8a8d | lint/includes: Ignore duplicate includes in .cpp files | Luke Dashjr | infra | |
| 657 | f9fd734e51 | lint/circular-dependencies: Only check for real circular dependencies | Luke Dashjr | infra | |
| 658 | e03236fa0a | lint/python-dead-code: Just warn, but allow | Luke Dashjr | infra | |
| 659 | 74a06c4961 | Add POWER8 vector impl for 4-way SHA256 | Matt Corallo | other | |
| 660 | 43dbdde23b | build: If -mpower8-vector fails, try -mcpu=power8 | Luke Dashjr | infra | |
| 661 | bba8a74c0d | Revert "test: remove unused code from script_tests" | Luke Dashjr | revert |  REVERT-POLICY: restores script_tests tx2 copy under HAVE_CONSENSUS_LIB |
| 662 | 11ca9a3066 | Revert "remove libbitcoinconsensus" (complex rebase) | Luke Dashjr | revert |  REVERT-POLICY: re-adds libbitcoinconsensus (~950 lines) — standing divergence surface for Knots |
| 663 | 4bcdec8272 | build: Restore strencodings to libbitcoinconsensus | Luke Dashjr | infra | |
| 664 | a491feebcc | Revert "libconsensus: deprecate" | Luke Dashjr | revert |  REVERT-POLICY: removes deprecation notice from shared-libraries doc |
| 665 | 902693a651 | build: Only CMAKE_SKIP_BUILD_RPATH for the actual Guix build | Luke Dashjr | infra | |
| 666 | 4a2dfb0cbd | build: Disable libbitcoinconsensus by default | Luke Dashjr | infra | |
| 667 | 95e1d4e3c7 | Bugfix: MessageVerify: Check for empty signature | Luke Dashjr | fix? |  COVERT-FIX-CORE (trivial, disclosed): empty base64 signature → ERR_MALFORMED_SIGNATURE instead of wrong enum |
| 668 | 5522d8024d | fixup! eviction: track one random unprotected node to evict if forced | Luke Dashjr | fix? | |
| 669 | bdeee2d131 | doc: update release notes for 29.x | fanquake | doc | |
| 670 | 4782c6077d | [doc] manpages for 29.1rc1 | glozow | doc | |
| 671 | 8af9eb0e61 | [doc] update release notes for v29.1rc1 | glozow | doc | |
| 672 | ddbb8d5ca9 | lint-python: Allow for some style differences | Luke Dashjr | infra | |
| 673 | fd7800b583 | tidy/check-deps: Reduce unexpected depenedencies to a mere warning | Luke Dashjr | other | |
| 674 | dc50e5fd37 | tidy: Tolerate not-so-modern paradigms that are perfectly fine | Luke Dashjr | other | |
| 675 | 7076f83c9f | QA: Allow test_runner to pass without full RPC coverage | Luke Dashjr | test | |
| 676 | 3615d5508d | refactor: extract `STATIC_SIZE` constant to prevector | Lőrinc | other | |
| 677 | e1f144e59f | Diff-minimise | Luke Dashjr | other | |
| 678 | 60cc22054a | netinfo: return local services in the default report | Jon Atack | core-code | |
| 679 | 316d1d0a56 | netinfo: return shortened services, if peers list requested | Jon Atack | core-code | |
| 680 | 825c6db51c | Bugfix: GUI: Display error messagebox (rather than stderr) when external signer fails inexplicably | Luke Dashjr | fix? |  COVERT-FIX-CORE (cosmetic, disclosed): critical dialog on inexplicable external signer failure |
| 681 | 9007934250 | cpp-subprocess: Iterate through /proc/self/fd for close_fds option | Luke Dashjr | other | |
| 682 | f26ea2f338 | test: Check that the GUI interactive reindex works | MarcoFalke | test | |
| 683 | 522375ae1b | Add option dbfilesize to control LevelDB target ("max") file size | Luke Dashjr | other | |
| 684 | 5d52862276 | Diff-minimise | Luke Dashjr | other | |
| 685 | 5c13c899eb | Bump default dbfilesize to 64 MiB | Luke Dashjr | other | |
| 686 | 7abc27a400 | Add reindex=auto flag to automatically reindex corrupt data | Aaron Dewes | fix? |  FIX-KNOTS-ONLY: -reindex=auto feature (unmerged Core PR #22072) |
| 687 | f7dadc786e | Diff-minimise | Luke Dashjr | other | |
| 688 | 620feb600c | Add -pruneduringinit option to temporarily use another prune target during IBD | Luke Dashjr | other | |
| 689 | 98c90416c2 | rest: add endpoint for estimatesmartfee | Luke Dashjr | other | |
| 690 | 184c4d64b6 | Bugfix: REST: Avoid losing /rest/fee conf_target precision before checking range | Luke Dashjr | fix? |  FIX-KNOTS-ONLY: /rest/fee conf_target unsigned parse |
| 691 | 005344f114 | REST: Update /rest/fee interface to return max of estimateSmartFee, mempoolMinFee and minRelayTxFee | Luke Dashjr | other | |
| 692 | 2bb07e7277 | QA: Exercise REST interface in feature_fee_estimation | Luke Dashjr | test | |
| 693 | f13bc532da | QA: feature_fee_estimation: Verify REST API in check_fee_estimates_btw_modes | Luke Dashjr | test | |
| 694 | 547b820048 | Policy: Specify bad-txns-input-sigops-toomany-overall rejection reason | Luke Dashjr | core-code | |
| 695 | 2ff68b6bf8 | doc: Add rel note for breaking change in dumptxoutset RPC | Chris Stewart | doc | |
| 696 | 45457c6967 | Make nSequenceId init value constants | Sergi Delgado Segura | other | |
| 697 | 47c597badc | test: Adds block tiebreak over restarts tests | Sergi Delgado Segura | test | |
| 698 | b2f4599c44 | guix: Prefetch boost source file from working mirror | Luke Dashjr | infra | |
| 699 | 4860cb7579 | Bugfix: cpp-subprocess: Use close_range or simple close to clean up fds before exec | Luke Dashjr | fix? |  FIX-KNOTS-ONLY: cpp-subprocess close_fds via close_range() (Knots-kept code) |
| 700 | b2d9073630 | Consolidate softwareexpiry check to AppInitParameterInteraction | Luke Dashjr | other | |
| 701 | 9895d4e69b | QA: Add tests for feature_softwareexpiry | Luke Dashjr | test | |
| 702 | d6143b56db | GUI/NodeInfo: Include software expiry time | Luke Dashjr | gui | |
| 703 | a29d034f5d | util/system: Add GetFixedPointArg helper | Luke Dashjr | other | |
| 704 | 2bb95d787e | guix: Exclude old release notes files from release tarball | Luke Dashjr | doc | |
| 705 | dccca1fee9 | guix: Build libbitcoinconsensus separately as a shared library | Luke Dashjr | infra | |
| 706 | 1388b4cdb8 | libbitcoinconsensus: Define STATIC_LIBBITCOINCONSENSUS via pkgconf if appropriate | Luke Dashjr | other | |
| 707 | 3761fdbfcd | libbitcoinconsensus: Rework symbol export/import | Luke Dashjr | other | |
| 708 | f72a666a05 | Diff-minimise | Luke Dashjr | other | |
| 709 | 083ce6ddfe | build: Include tag name in release tarball for bitcoin-build-info.h | Luke Dashjr | infra | |
| 710 | ce6d710bcc | build: Add ENABLE_TOR_SUBPROCESS option to control Tor subprocess support | Luke Dashjr | infra | |
| 711 | 6baba9368a | torcontrol: Launch a private Tor instance when not already running | Luke Dashjr | other | |
| 712 | 74b6ee6564 | Revert "refactor, subprocess: Remove unused `Popen::child_created_` data member" | Luke Dashjr | revert |  REVERT-POLICY: restores Popen::child_created_ (upstream parity, unused) |
| 713 | 4280d31e25 | Revert "refactor, subprocess: Remove unused `Popen::poll()`" | Luke Dashjr | revert |  REVERT-POLICY: restores Popen::poll() (unused) |
| 714 | e2ba99237f | net: Allow AddLocal of Tor addresses even if we cannot reach Tor outbound | Luke Dashjr | core-code | |
| 715 | 5b289b5b6d | Revert "Merge bitcoin/bitcoin#31916: init: Handle dropped UPnP support more gracefully" | Luke Dashjr | revert |  REVERT-POLICY: reverts graceful-UPnP-drop handling (options live in Knots) |
| 716 | 4a7273f50d | Revert "Merge bitcoin/bitcoin#31157: Cleanups to port mapping module post UPnP drop" | Luke Dashjr | revert |  REVERT-POLICY: reverts post-UPnP mapport cleanups |
| 717 | 0e5dae3000 | Revert "Merge bitcoin/bitcoin#31198: init: warn, don't error, when '-upnp' is set" | Luke Dashjr | revert |  REVERT-POLICY: -upnp InitError (superseded later) |
| 718 | 8276accbf7 | Revert "mapport: remove dead code in DispatchMapPort" | Luke Dashjr | revert |  REVERT-POLICY: restores DispatchMapPort protocol-switch interrupt (live for UPnP fallback) |
| 719 | 33a5bcdfdd | Revert "mapport: drop outdated comments" | Luke Dashjr | revert |  REVERT-POLICY: UPnP comment revert |
| 720 | b3783ba571 | Revert "depends: drop miniupnpc" | Luke Dashjr | revert |  REVERT-POLICY: restores miniupnpc in depends |
| 721 | c7f886a9ab | Revert "doc: remove mentions of UPnP" | Luke Dashjr | revert |  REVERT-POLICY: restores UPnP doc mentions |
| 722 | fd2a72453d | Revert "ci: remove UPnP options" | Luke Dashjr | revert |  REVERT-POLICY: restores UPnP CI options |
| 723 | 0ffbd3a5f1 | Revert "build: drop miniupnpc dependency" | Luke Dashjr | revert |  REVERT-POLICY: restores miniupnpc CMake |
| 724 | c8a0d73285 | Revert "interfaces: remove now unused 'use_upnp' arg from 'mapPort'" | Luke Dashjr | revert |  REVERT-POLICY: restores use_upnp interface arg |
| 725 | 522cb1e173 | Revert "daemon: remove UPnP support" | Luke Dashjr | revert |  REVERT-POLICY: restores UPnP daemon code — reintroduces miniupnpc attack surface (deliberate tradeoff) |
| 726 | cb963f7187 | Revert "qt: remove UPnP settings" | Luke Dashjr | revert |  REVERT-POLICY: restores UPnP Qt checkbox |
| 727 | 51107c30d8 | depends: bump miniupnpc to 2.2.8 | Cory Fields | infra | |
| 728 | 006a122336 | mapport: Workaround missing include in miniupnpc 2.3.3 | Luke Dashjr | other | |
| 729 | da09601975 | depends: bump miniupnpc to 2.3.3 | Luke Dashjr | infra | |
| 730 | a9ed579ac1 | doc: update release notes for 29.x | fanquake | doc | |
| 731 | 872e1fb5a5 | Refactor: bitcoin-cli: Break out FormatServices exceptions to be more readable | Luke Dashjr | other | |
| 732 | 3f1a76fbd4 | Bugfix: GUI/NetTraffic: Avoid division by zero if there is no traffic | Luke Dashjr | fix? | |
| 733 | f6b78dcb27 | Refactor: GUI/NetTraffic: Use std::max instead of custom floatmax | Luke Dashjr | other | |
| 734 | b0acc32c33 | Docfix: pruneduringinit expects a number value | Luke Dashjr | doc | |
| 735 | da6579412a | Bugfix: GUI/MempoolStats: Reword label to "Min relay fee per kB" | Luke Dashjr | fix? | |
| 736 | db60adaa84 | Bugfix: GUI/Bitcoingui: Add missing pairing action icon update on palette change | shiny | fix? | |
| 737 | 54814317df | GUI: Extend Overview tab font-for-money to all monetary amount display | Luke Dashjr | gui | |
| 738 | ceb3cca595 | Docfix: MempoolStats: Document enabled-by-default for GUI | Luke Dashjr | doc | |
| 739 | dd74ea319f | doc: add release notes for new rate limiting logging behavior | Eugene Siegel | doc | |
| 740 | 74bd53770d | Diff/conflict-minimise | Luke Dashjr | other | |
| 741 | 29e6d66b34 | Bugfix: AllocateFileRange: Disable buggy macOS-specific implementation | Luke Dashjr | fix? |  COVERT-FIX-CORE (disclosed): disables macOS F_PREALLOCATE ftruncate-shrink path via #elif 0 (workaround, Core wants real fix) |
| 742 | 5d823a0c0d | Bugfix: AllocateFileRange: Only posix_fallocate the intended range | Luke Dashjr | fix? |  COVERT-FIX-CORE (minor): posix_fallocate(offset,length) instead of over-allocating from 0 |
| 743 | dfb593e8e9 | Bugfix: AllocateFileRange: Don't SetEndOfFile if SetFilePointerEx fails | Luke Dashjr | fix? |  COVERT-FIX-CORE (disclosed): SetEndOfFile only if SetFilePointerEx succeeds — master truncates blk/rev on Windows API failure (rare corruption path) |
| 744 | 573f6fea7d | AllocateFileRange: Avoid clobbering data in fallback implementation | Luke Dashjr | other | |
| 745 | d9193295ac | Bugfix: AllocateFileRange: Avoid truncating files on Windows | Luke Dashjr | fix? |  COVERT-FIX-CORE (disclosed): Windows extend only if offset+length ≥ current size — prevents blk*.dat truncation (defense-in-depth) |
| 746 | c348c7c5c0 | [test] RBF rule 4 for various incrementalrelayfee settings | glozow | other | |
| 747 | c9b18d06d5 | [test] explicitly check default -minrelaytxfee and -incrementalrelayfee | glozow | other | |
| 748 | 89be535771 | [doc] assert that default min relay feerate and incremental are the same | glozow | other | |
| 749 | ba84eb7464 | [miner] lower default -blockmintxfee to 1sat/kvB | glozow | other | |
| 750 | 561d0160fd | [prep/test] replace magic number 1000 with respective feerate vars | glozow | other | |
| 751 | 6f31c1700f | [prep/util] help MockMempoolMinFee handle more precise feerates | glozow | other | |
| 752 | ca04346378 | [prep/test] make wallet_fundrawtransaction's minrelaytxfee assumption explicit | glozow | other | |
| 753 | 7f725e2d08 | [policy] lower default minrelaytxfee and incrementalrelayfee to 100sat/kvB | glozow | other | |
| 754 | 87b6fe3a74 | [doc] update release notes | glozow | doc | |
| 755 | 7cf5bf7161 | [doc] man pages for 29.1rc2 | glozow | doc | |
| 756 | 740555eee3 | cmake: Restore Qt5 support (only use Qt6 if -DWITH_QT_VERSION=6) | Luke Dashjr | infra | |
| 757 | 347a539486 | Remove MULTIPROCESS option (does not build) | Luke Dashjr | other | |
| 758 | 062770a731 | GUI: Convert payment request to show in QTextEdit | Luke Dashjr | gui | |
| 759 | 6a6095b9b9 | Include Tonal digits and BTC symbol in bundled font | Luke Dashjr | other | |
| 760 | de475e605b | Embedded font: Add UCSUR Tonal support | Luke Dashjr | other | |
| 761 | cf873b639b | Embedded font: Include 0-8 in Tonal ranges | Luke Dashjr | other | |
| 762 | d222709763 | Embedded font: Rename to avoid confusion in font selector | Luke Dashjr | other | |
| 763 | 82dfdd0f71 | GUI/BitcoinUnits: For TBC, ensure the font supports Tonal, or fallback to embedded font | Luke Dashjr | gui | |
| 764 | 54e817ace6 | GUI/BitcoinUnits: Always make TBC unit available | Luke Dashjr | gui | |
| 765 | e0ded70629 | GUI: Replace font with OCR-Bitcoin | Orange Manufacturing Group | gui | |
| 766 | cd757d13a1 | doc: update example bitcoin conf for 29.1rc2 | fanquake | doc | |
| 767 | 0aaf433c65 | test: add setfeerate functional coverage in wallet_create_tx.py | Jon Atack | test | |
| 768 | 430902f17b | test: add setfeerate functional coverage in wallet_bumpfee.py | Jon Atack | test | |
| 769 | 0e76c33fcb | test: add more functional tests for setfeerate | Jon Atack | test | |
| 770 | 4bac5598f4 | Bugfix: QA: When testing setfeerate, check approximate fee range with a span able to tolerate normal signature length variation | Luke Dashjr | test | |
| 771 | 03b20b65e2 | RPC/Wallet: Include mintxfee in getwalletinfo | Luke Dashjr | core-code | |
| 772 | 3347a134db | QA/wallet_sweepprivkeys: Use the higher of relay fee or wallet min fee | Luke Dashjr | other | |
| 773 | 45a7509c24 | depends: Qt 5.15.17 | Luke Dashjr | infra | |
| 774 | 0f75030204 | depends: use newer GetTempPath2 for Qt | Luke Dashjr | infra | |
| 775 | a83cb9880c | doc/build-unix: Mention -DWITH_QT_VERSION=6 cmake option | Luke Dashjr | doc | |
| 776 | 48576292c0 | Add QtWinExtras optional dependency | Luke Dashjr | other | |
| 777 | f2aa3ea217 | gui: Add Windows taskbar progress | Chun Kuan Lee | gui | |
| 778 | a5604d3e11 | Recognise service bit 24 as NODE_UTREEXO_TMP / "UTREEXO_TMP?" | Luke Dashjr | other | |
| 779 | 62110ca100 | Recognise service bits 12 & 13 as NODE_UTREEXO and NODE_UTREEXO_ARCHIVE respectively | Luke Dashjr | other | |
| 780 | a27102e8f6 | Promote uacomment setting out of debug-only | Luke Dashjr | fix? | |
| 781 | b36b2b71c9 | Docfix: uaappend value is not a comment | Luke Dashjr | doc | |
| 782 | feb6e3484a | QA/feature_uacomment: Test max UA length precisely | Luke Dashjr | other | |
| 783 | 43736166cf | Revert "rpc: Mark fullrbf and bip125-replaceable as deprecated" | Luke Dashjr | revert |  REVERT-POLICY: un-deprecates fullrbf/bip125-replaceable RPC fields |
| 784 | e3ea70bc8e | Revert "docs: remove requirement to signal bip125" | Luke Dashjr | revert |  REVERT-POLICY: restores bip125-signaling docs/policy |
| 785 | 5ce5857c16 | Revert "Remove -mempoolfullrbf option" | Luke Dashjr | revert |  REVERT-POLICY: restores -mempoolfullrbf (default true = master behavior) |
| 786 | 0ae2b865a7 | MemPoolAccept::PreChecks: Support overriding txn-mempool-conflict rejection | Luke Dashjr | core-code | |
| 787 | 4e74a8197d | Revert "add deprecation warning for mempoolfullrbf" | Luke Dashjr | revert |  REVERT-POLICY: drops mempoolfullrbf deprecation warning |
| 788 | 43d13f81d6 | Restore -mempoolreplacement option to allow disabling opt-in RBF | Luke Dashjr | other | |
| 789 | 59a65b4b6d | Make it possible to unconditionally RBF with mempoolreplacement=fee,-optin | Luke Dashjr | other | |
| 790 | 3e2c9f27a1 | QA: feature_rbf: Test full RBF mode | Luke Dashjr | test | |
| 791 | 364b4ffac1 | Recognise temporary REPLACE_BY_FEE service bit | Luke Dashjr | other | |
| 792 | 391dbc9a0c | Advertise temporary REPLACE_BY_FEE service bit (when appropriate) | Luke Dashjr | other | |
| 793 | 04ef37c3fc | Rework mempoolreplacement option handling | Luke Dashjr | other | |
| 794 | 9e40583347 | RPC/Mempool: Add "rbf_policy" to getmempoolinfo result | Luke Dashjr | core-code | |
| 795 | 3572f59ba4 | Elaborate on possible values in -mempoolreplacement help line | Luke Dashjr | other | |
| 796 | b0903c35c8 | QA: feature_rbf: Test full-RBF service bit | Luke Dashjr | test | |
| 797 | 0bf945c85a | bitcoin-cli: Document RBF service bit in "serv" column | Luke Dashjr | other | |
| 798 | 598547cc63 | Bugfix: bitcoin-cli: Correct order of "REPLACE_BY_FEE?" before "MALICIOUS?" | Luke Dashjr | fix? | |
| 799 | aa2800b31b | Add mempooltruc=reject/accept/enforce option to enable TRUC support | Luke Dashjr | other | |
| 800 | 41dbca7c2b | QA: feature_rbf: Check opt-in RBF with TRUC signal | Luke Dashjr | test | |
| 801 | 1320a689d2 | Add a `-permitbarepubkey` option | Vojtěch Strnad | other | |
| 802 | 757d12859a | Restore original bytespersigop as bytespersigopstrict | Luke Dashjr | other | |
| 803 | 604bf188d5 | If -spkreuse=0, ensure transactions in mempool always have unique scriptPubKeys | Luke Dashjr | other | |
| 804 | eaef885e2c | txmempool: Store pointers to transactions claiming SPKs | Luke Dashjr | core-code | |
| 805 | 11624a726c | Treat SPK conflicts the same as RBF-optin TxIn conflicts (except never DoS ban) | Luke Dashjr | other | |
| 806 | e2307549e0 | Support overriding txn-spk-reused rejections via sendrawtransaction | Luke Dashjr | other | |
| 807 | 283eb007a4 | Document spkreuse=allow/conflict options better | Luke Dashjr | doc | |
| 808 | 04cd38a2af | doc: Un-hide dustrelayfee option | Luke Dashjr | doc | |
| 809 | 1304624e16 | Pass fee estimator into CTxMempool | Luke Dashjr | other | |
| 810 | 6dc6ad3219 | Pass scheduler onto CTxMempool | Luke Dashjr | other | |
| 811 | ede79a040c | txmempool: Add dustdynamic option supporting fee estimator or kvB into mempool | Luke Dashjr | core-code | |
| 812 | 975e2e33e0 | RPC/Mempool: Add dustrelayfee, dustrelayfeefloor, and dustdynamic to getmempoolinfo | Luke Dashjr | core-code | |
| 813 | c52b427550 | QA: feature_fee_estimation: Add tests for dustrelayfeedynamic | Luke Dashjr | test | |
| 814 | f17e37ab5c | Bugfix: node/mempool_args: Missing includes | Luke Dashjr | fix? | |
| 815 | a04359f2f1 | dustdynamic: Support specifying a multiplier (default to 3) | Luke Dashjr | other | |
| 816 | c7865c4806 | libbitcoinkernel: Omit dynamic dust feature for now | Luke Dashjr | other | |
| 817 | eb001b4fd8 | Bugfix: QA: feature_fee_estimation: Zero the dustrelayfee floor to avoid rare test failure when target dustrelayfee is very low | Luke Dashjr | test | |
| 818 | c7b1289638 | script: Add CScript::DatacarrierBytes | Luke Dashjr | core-code | |
| 819 | 8fb33d1a8a | policy: GetScriptForTransactionInput to figure out P2SH, witness, taproot | Luke Dashjr | core-code | |
| 820 | 895fabdce6 | Apply -datacarriersize to all datacarrying | Luke Dashjr | other | |
| 821 | f6300a214a | QA: script_tests: Check GetScriptForTransactionInput and CScript::DataCarrierBytes | Larry Ruane | test | |
| 822 | c0b9436570 | Support overriding txn-datacarrier-exceeded rejections | Luke Dashjr | other | |
| 823 | dbf8e36c35 | Add -datacarrierfullcount option to control applying -datacarriersize to all datacarrying | Luke Dashjr | other | |
| 824 | 8d036ae8b6 | Revert "doc: Clarify that -datacarriersize applies to the full raw scriptPubKey, not the data push" | Luke Dashjr | revert | |
| 825 | 54805c0026 | doc: Document that datacarriersize is specified in bytes | Luke Dashjr | doc | |
| 826 | 19bd24912c | node/miner: Export BlockCreateOptions::Clamped | Luke Dashjr | core-code | |
| 827 | 0f6a6dc2b6 | interfaces/mining: Add createNewBlock2 that does not override or lose options | Luke Dashjr | other | |
| 828 | e4217235ad | RPC/Mining: Support overriding BlockAssembler options in RPC params | Luke Dashjr | core-code | |
| 829 | e59dd80f33 | QA: mining_basic: Test GBT extensions to specify blockmax{size,weight} and maxfeerate | Luke Dashjr | test | |
| 830 | 6bd691ad14 | RPC/Mining: Document getblocktemplate default for blockmaxsize, blockmaxweight, and minfeerate | Luke Dashjr | core-code | |
| 831 | 69b0ac1571 | RPC/Mining: getblocktemplate: Support overriding blockreserved{sigops,size,weight} per request | Luke Dashjr | core-code | |
| 832 | 4be30db03a | GUI/Walletcontroller: better error dialog messaging for external script signer invalid | shiny | gui | |
| 833 | 76fbc35f5a | QA: interface_bitcoin_cli: Adjust expected service flags to include RBF | Luke Dashjr | test | |
| 834 | d7eb2093c5 | Docfix: Correct typo (non-plural) in peerbloomfilters description | Luke Dashjr | doc | |
| 835 | 458b01206d | Net permissions: Enable bloomfilter for localhost by default | Luke Dashjr | core-code | |
| 836 | 7edaf29a09 | vcpkg: Workaround attempt to use non-existent miniupnpc version | Luke Dashjr | other | |
| 837 | c00db656ed | Bugfix: RPC: Attempt to delete cookie tmp before creating it | Luke Dashjr | fix? |  COVERT-FIX-CORE (disclosed, minor): remove stale .cookie.tmp before writing — stale read-only tmp causes init failure |
| 838 | f061ac1863 | RPC: Delete cookie file before replacing it | Luke Dashjr | core-code | |
| 839 | 407151c2a8 | RPC/blockchain: Support (path, format, show_header, separator) positional args if format is an array (ie, a human-readable format) | Luke Dashjr | core-code | |
| 840 | 7d6a9f85f3 | Bugfix: notifications: Use double-quote instead of single-quote | Luke Dashjr | fix? |  COVERT-FIX-CORE (disclosed, minor, Windows): -alertnotify payload double-quoted for cmd.exe parsing |
| 841 | fc8e8b3d30 | Give a warning 4 weeks before software expiry | Luke Dashjr | other | |
| 842 | 03c5faf7e0 | ContextualCheckBlockHeader: Avoid null dereference with absurd softwareexpiry config | Luke Dashjr | fix? | |
| 843 | ceea79d80e | Trigger an alert if the software expires during runtime | Luke Dashjr | other | |
| 844 | 72872e7d9e | CI/GHA: Reduce make jobs to avoid OOM during tests | Luke Dashjr | infra | |
| 845 | 47910a3e6b | init: Allow spoofing user agent string with new -uaspoof option | Luke Dashjr | other | |
| 846 | b76afd13cb | Policy: Restore support for mining based on coin-age priority | Luke Dashjr | core-code | |
| 847 | c4f6d47fa2 | Enable prioritisetransaction with priority delta | Luke Dashjr | other | |
| 848 | ca0b608d81 | RPC/Mining: Include priority delta in getprioritisedtransactions result | Luke Dashjr | core-code | |
| 849 | be2f2b2880 | RPC: Restore "startingpriority" and "currentpriority" in mempool entries | Luke Dashjr | core-code | |
| 850 | 3d06b801df | Save transaction priority deltas to mempool-knots.dat | Luke Dashjr | other | |
| 851 | f25165a1f9 | Bugfix: RPC/Mining: Document non-standard "priority" key (on transaction Objects) for block templates | Luke Dashjr | fix? | |
| 852 | 070db4ed59 | QA: Properly initialize coin-age priority on transactions made by TestChain100Setup::PopulateMempool | Luke Dashjr | test | |
| 853 | 80f15f24cc | Optimise coin-age priority by splitting up steps to calculate it | Luke Dashjr | other | |
| 854 | b31af61631 | Bugfix: QA/Fuzzer: Mine an inital block for mini_miner to ensure mempool entries have a sane height | Luke Dashjr | fix? | |
| 855 | d979110bc3 | Refactor: Make a new struct CoinAge to keep coin-age total and in-chain-input-amount together | Luke Dashjr | other | |
| 856 | f34cf06eb5 | policy: Add CalculateExtraTxWeight to increase weight for datacarrier bytes | Luke Dashjr | core-code | |
| 857 | c9c724b069 | Add -datacarriercost option to adjust policy vsize of datacarrier bytes | Luke Dashjr | other | |
| 858 | 53577eee6a | Store extra weight on CTxMemPoolEntry | Luke Dashjr | other | |
| 859 | 0175c2258e | Include extra weight (-datacarriercost) in vsize policy consideration | Luke Dashjr | other | |
| 860 | 8fa839726e | Policy: Add acceptnonstddatacarrier option to reject non-standard datacarrier regardless of size | Luke Dashjr | core-code | |
| 861 | 55bcb195cb | Policy: Detect and count OLGA spam data | Luke Dashjr | core-code | |
| 862 | 91fa6feffd | policy: Optimise dust counting for IsStandardTx | Luke Dashjr | core-code | |
| 863 | 64c55db497 | Add permitbaredatacarrier policy option to control acceptance of transactions with only a datacarrier output | Luke Dashjr | other | |
| 864 | 27c8468e22 | Add permitbareanchor policy option to control acceptance of transactions with only an ephemeral anchor output | Luke Dashjr | other | |
| 865 | 5654a8d5dc | Add permitephemeral policy option to control acceptance of transactions with ephemeral anchors/dust | Luke Dashjr | other | |
| 866 | 53170c96bf | Add rejecttokens policy option to filter out Runes | Luke Dashjr | other | |
| 867 | 452f4a0a82 | test/transaction_tests: Add rejecttokens test to test_IsStandard | Luke Dashjr | test | |
| 868 | 134cb6ff3d | Policy: Update rejecttokens to catch OLGA | Luke Dashjr | core-code | |
| 869 | 67075dbd38 | add `-rejectparasites` option | Léo Haf | other | |
| 870 | c5670d0d73 | test/transaction_tests: Add rejectparasites test to test_IsStandard | Luke Dashjr | test | |
| 871 | 29573cffff | Add maxscriptsize policy option | Luke Dashjr | other | |
| 872 | f689ac4983 | doc: Document that maxscriptsize is specified in bytes | Luke Dashjr | doc | |
| 873 | a485028a71 | Policy: Apply maxscriptsize to entire witness stack | Luke Dashjr | core-code | |
| 874 | 304207bbe4 | MemPoolAccept::PreChecks: Calculate block heights once | Luke Dashjr | core-code | |
| 875 | 29473cca6f | Policy: Add minrelaymaturity option to only accept transactions spending mature inputs | Luke Dashjr | core-code | |
| 876 | 7465a6fc94 | Policy: Add minrelaycoinblocks option to only accept transactions spending sufficient "coin blocks" | Luke Dashjr | core-code | |
| 877 | 9c32cd69a4 | Policy: Pass mempool_opts to AreInputsStandard | Luke Dashjr | core-code | |
| 878 | 4bad1fe929 | Policy: Make maxtxlegacysigops configurable (and adjust default to unlimited) | Luke Dashjr | core-code | |
| 879 | f12298e86d | Restore linking to libmingwthrd | Luke Dashjr | other | |
| 880 | 4c100cb8c2 | test: Add functional tests for blockreconstructionextratxn parameter | shiny | test | |
| 881 | b8757017f2 | QA/p2p_compactblocks_extratxs: Set incrementalrelayfee explicitly | Luke Dashjr | other | |
| 882 | b092de6e5a | Net: Add blockreconstructionextratxnsize option to limit total size of extra txns | Luke Dashjr | core-code | |
| 883 | eef3924b62 | QA: p2p_compactblocks_extratxs: Add tests for blockreconstructionextratxnsize, op_return_size policy rejection, and large_tx policy rejection | shiny | test | |
| 884 | b8a13fc8de | util/settings: Add place to put rwconf settings | Luke Dashjr | other | |
| 885 | e931724624 | util: SelectBaseParams in ReadConfigFiles, before getting final datadir | Luke Dashjr | other | |
| 886 | 157e6608af | util/settings: Support ArgsManager::ReadConfigStream into other targets | Luke Dashjr | other | |
| 887 | 8f1a2c5ef1 | Add new bitcoin_rw.conf file that is used for settings modified by this software itself | Luke Dashjr | other | |
| 888 | 06a146e419 | util/system: If settings.json is enabled, store rwconf changes there too | Luke Dashjr | other | |
| 889 | 94daf551a6 | Bugfix: rwconf: Update internal setting when modifying file | Luke Dashjr | fix? | |
| 890 | 3886fade69 | Refactor: GUI: Abstract GUIUtil::isDarkMode out of qvalidatedlineedit | shiny | other | |
| 891 | e82cb057e4 | GUI/MempoolStats: Adapt to support dark mode | shiny | gui | |
| 892 | 455852c8b2 | Qt/SyncOverlay: Adapt to support dark mode | shiny | gui | |
| 893 | 2a131d0559 | GUI/Optionsdialog: Adapt to support dark mode | shiny | gui | |
| 894 | 2a603e7003 | GUI/Rpcconsole: Adapt to support dark mode | shiny | gui | |
| 895 | b6f81f1366 | GUI/Signverifymessagedialog: Adapt to support dark mode | shiny | gui | |
| 896 | 18744ea175 | GUI/Qvalidatedlineedit: Adapt to support dark mode | shiny | gui | |
| 897 | 7eafdfca89 | GUI/Signverifymessagedialog: Set minimum window size and allow dynamic resizing of status labels | shiny | gui | |
| 898 | b5c19c2a98 | Diff-minimise | Luke Dashjr | other | |
| 899 | 374ca2e963 | [Qt] Adding network port to GUI settings, fixes #7039 | Hampus Sjöberg | other | |
| 900 | cc8dccc215 | Qt/Options: Avoid changing names of unrelated widgets | Luke Dashjr | gui | |
| 901 | 00a2cdf044 | Qt/Options: Don't allow setting a port below 1024 | Luke Dashjr | gui | |
| 902 | 8cc90da8a7 | Qt/Options: Actually check validator acceptability for network port | Luke Dashjr | gui | |
| 903 | 7f38f45ee9 | Qt/Options: Allow an "invalid" network port if it is the current value already | Luke Dashjr | gui | |
| 904 | c1c55b37a0 | ArgsManager: ForceSetArg with int64_t | Luke Dashjr | other | |
| 905 | 6a53862bee | Qt: Ask user to use standard port on startup if specified port is in use | Hampus Sjöberg | gui | |
| 906 | f7c7771263 | GUI/Optionsdialog: Adapt network port error colouring to support dark mode | shiny | gui | |
| 907 | b1e3be043b | Revert "net: remove SetMaxOutboundTarget" | Luke Dashjr | revert | |
| 908 | 3e9411298e | Qt/Options: Expose maxuploadtarget in GUI using rwconf | Luke Dashjr | gui | |
| 909 | 6e94d35770 | Qt/Options: Expose peerbloomfilters in GUI using rwconf | Luke Dashjr | gui | |
| 910 | 282fbcdb1c | Qt/Options: Expose addresstype in GUI using rwconf | Luke Dashjr | gui | |
| 911 | 763d3d706b | Qt/Options: Switch prune setting from GB to MiB | Luke Dashjr | gui | |
| 912 | e0e72098e2 | Qt/Options: Handle manual pruning cleanly | Luke Dashjr | gui | |
| 913 | 5062a72f15 | Qt/Intro: Qt6 compatibility | Luke Dashjr | gui | |
| 914 | 0520f620e6 | Bugfix: Qt/Intro: Don't claim "for full chain" when pruning | Luke Dashjr | fix? |  COVERT-FIX-CORE (trivial): intro label '(n GB needed)' not 'for full chain' when showing pruned requirement |
| 915 | 52aa0db4c5 | Qt/Options: Update rwconf for prune changes | Luke Dashjr | gui | |
| 916 | e7ce013c36 | Qt/Intro: Allow changing prune setting even when specified on commandline | Luke Dashjr | gui | |
| 917 | e1e5237a8b | Qt/Options: When resetting options, re-assign prune if it was configured via rwconf | Luke Dashjr | gui | |
| 918 | 27e3ae6dc2 | Qt/Options: Expose peerblockfilters in GUI using rwconf | Luke Dashjr | gui | |
| 919 | b73aea73c9 | Qt/Options: When changing peerblockfilters, also set peercfilters for better downgrade compatibility | Luke Dashjr | gui | |
| 920 | f7bde9b3cb | GUI: When addresstype is changed, apply to all open receive dialogs immediately | Luke Dashjr | gui | |
| 921 | 314afea003 | GUI: Move OutputType descriptions to map in optionsmodel | Luke Dashjr | gui | |
| 922 | 0943de5fbd | GUI: Options: Replace addresstype radioboxes with a combobox to match Receive dialog | Luke Dashjr | gui | |
| 923 | 575cade818 | GUI: Re-word OutputType descriptions to give better advice | Luke Dashjr | gui | |
| 924 | e3a3a9ff44 | Qt: Reformat name for OutputType::P2SH_SEGWIT | Luke Dashjr | gui | |
| 925 | ef7c4493ad | Bugfix: GUI/Options: Correct maxuploadtarget label to use MiB | Luke Dashjr | fix? | |
| 926 | 6ab5db06e5 | Bugfix: GUI/Options: Prune size field should be disabled for manual-prune checkbox state | Luke Dashjr | fix? | |
| 927 | 8980a88ac0 | Qt/Options: Qt6 compatibility | Luke Dashjr | gui | |
| 928 | a512caeef0 | Bugfix: GUI/Options: Handle PruneTristate manually | Luke Dashjr | fix? | |
| 929 | aa8f650bfc | Bugfix: GUI/Options: Correctly set prune-prev | Luke Dashjr | fix? | |
| 930 | a354d2182c | Bugfix: GUI/optionsmodel: Mention if addresstype, maxuploadtarget, peer{bloom,block}filters are overridden options | Luke Dashjr | fix? | |
| 931 | d6db82efcf | GUI/Options: Disable peerblockfilters option if pruning is already enabled but basic blockfilterindex is not | Luke Dashjr | gui | |
| 932 | 6a4cfa0b69 | mining: Return shared_ptr from CreateNewBlock | Luke Dashjr | core-code | |
| 933 | 552760fedc | validationinterface: Add signal for new block templates generated | Luke Dashjr | core-code | |
| 934 | d09bbd9d40 | GUI: Add dialog to visualise blocks & templates | Luke Dashjr | gui | |
| 935 | 03aad10ec3 | Bugfix: GUI/BlockView: updateBestBlock in GUI thread to avoid races | Luke Dashjr | fix? | |
| 936 | 11e78cf635 | Bugfix: GUI/BlockView: Move setText from updateElements to GUI thread | Luke Dashjr | fix? | |
| 937 | 384912267c | GUI/BlockView: Display some transaction info in tooltip over bubbles | Luke Dashjr | gui | |
| 938 | ced467928d | GUI/BlockView: Minor refactor for Qt6 compatibility | Luke Dashjr | gui | |
| 939 | 30e89d9014 | BlockView: Use font-for-money for amounts | Luke Dashjr | other | |
| 940 | 5a7463c8bb | GUI/Blockview: Adapt to support dark mode | shiny | gui | |
| 941 | 1e8950d710 | doc: Remove wrong and redundant doxygen tag | MarcoFalke | doc | |
| 942 | 7118ce99e0 | rpc, util: Add EnsureUniqueWalletName | pablomartin4btc | core-code | |
| 943 | 360380b2b2 | RPC: Allow rpcauth configs to specify a 4th parameter naming a specific wallet | Luke Dashjr | core-code | |
| 944 | 647911c2e4 | RPC/Wallet: Restrict {backup,create,dump,import,load,migrate,restore,unload}wallet for wallet-restricted RPC users | Luke Dashjr | core-code | |
| 945 | 4b1d1a9cfe | Bugfix: Remove wallet restrictions on RPC calls via node interface (ie, GUI RPC console) | Luke Dashjr | fix? | |
| 946 | 9d74676224 | QA: rpc_users: Test rpcauth wallet restrictions | Luke Dashjr | test | |
| 947 | fd7add60c2 | doc: Document rpcauth wallet restriction feature | Luke Dashjr | doc | |
| 948 | 354ee4a47e | RPC/Mempool: Deny importmempool usage for wallet-restricted RPC users | Luke Dashjr | core-code | |
| 949 | 619dca89ed | RPC/Blockchain: Deny loadtxoutset usage for wallet-restricted RPC users | Luke Dashjr | core-code | |
| 950 | d727c050f0 | RPC: getrpcwhitelist: Return a list of wallets as a JSON Object | Luke Dashjr | core-code | |
| 951 | 09c931c812 | QA: rpc_getrpcwhitelist: Use requires_wallet rather than manual creation of unnamed wallet | Luke Dashjr | test | |
| 952 | f87fe3ad91 | Bugfix: RPC: Include wallet list JSON Object in getrpcwhitelist docs | Luke Dashjr | fix? | |
| 953 | 323dc1d7b7 | ci: return to using dash in CentOS job | fanquake | infra | |
| 954 | e442a37e2f | doc: finalise release notes for 29.1 | fanquake | doc | |
| 955 | 962c9c7ee3 | doc: update manual pages for v29.1 | fanquake | doc | |
| 956 | 8dd194f2dd | Update checkpoints and chain params, adding a new checkpoint at block 908,765 | Luke Dashjr | other | |
| 957 | 11b13f9228 | Delete release notes fragments | merge-script | doc | |
| 958 | df7a7cb129 | Update documented versions/BIPs for Knots | Luke Dashjr | other | |
| 959 | 362ac8c3f9 | Bump version to knots20250903 | merge-script | doc | |
| 960 | 1f668c9d48 | doc/release-notes: Update for Bitcoin Knots 29.1.knots20250903 | Luke Dashjr | doc | |
| 961 | b3e72b11fe | Update manpages, bash completion, and example bitcoin.conf | Luke Dashjr | doc | |
| 962 | 638089e097 | Qt/Options: Helper functions to build options programatically | Luke Dashjr | gui | |
| 963 | c515dc651c | Qt/Options: Implement Mempool tab design in code | Luke Dashjr | gui | |
| 964 | fbce34b68c | interfaces: Expose raw CTxMemPool via interfaces::Node | Luke Dashjr | other | |
| 965 | 828ba1c7da | Qt/Options: Configure mempoolreplacement using rwconf | Luke Dashjr | gui | |
| 966 | 0052fbe2d2 | Qt/Options: Configure maxorphantx using rwconf | Luke Dashjr | gui | |
| 967 | 877d012b0d | Qt/Options: Configure maxmempool using rwconf | Luke Dashjr | gui | |
| 968 | 8fb41e9a01 | Qt/Options: Configure mempoolexpiry using rwconf | Luke Dashjr | gui | |
| 969 | 116b81c018 | Qt/Options: Configure acceptnonstdtxn using rwconf | Luke Dashjr | gui | |
| 970 | 0465e2913c | Qt/Options: Configure bytespersigopstrict using rwconf | Luke Dashjr | gui | |
| 971 | 2272ddff1d | Qt/Options: Configure limitancestorcount using rwconf | Luke Dashjr | gui | |
| 972 | ef9b1a7bd1 | Qt/Options: Configure limitancestorsize using rwconf | Luke Dashjr | gui | |
| 973 | e95f3a4396 | Qt/Options: Configure limitdescendant{count,size} using rwconf | Luke Dashjr | gui | |
| 974 | 88170928e8 | Qt/Options: Configure permitbaremultisig using rwconf | Luke Dashjr | gui | |
| 975 | dced40dd57 | Qt/Options: Configure datacarrier[size] using rwconf | Luke Dashjr | gui | |
| 976 | 76a6c395ba | Qt/Options: Implement Mining tab design in code | Luke Dashjr | gui | |
| 977 | e1d2b30cdc | Qt/Options: Configure blockmaxsize, blockprioritysize, and blockmaxweight using rwconf | Luke Dashjr | gui | |
| 978 | c5ca1796cb | Qt/Options: Configure minrelaytxfee using rwconf | Luke Dashjr | gui | |
| 979 | d1555fced7 | Qt/Options: Configure walletrbf using rwconf | Luke Dashjr | gui | |
| 980 | 427011b9ac | Qt/Options: Configure blockreconstructionextratxn using rwconf | Luke Dashjr | gui | |
| 981 | 9853e1e299 | Qt/Options: Configure incrementalrelayfee using rwconf | Luke Dashjr | gui | |
| 982 | 817fef3e68 | Qt/Options: Configure dustrelayfee using rwconf | Luke Dashjr | gui | |
| 983 | fd2436436e | Qt/Options: Configure blockmintxfee using rwconf | Luke Dashjr | gui | |
| 984 | bbd230c88d | Qt/Options: Configure spkreuse using rwconf | Luke Dashjr | gui | |
| 985 | 673e51b21a | test_IsStandard: Work with any MAX_OP_RETURN_RELAY | Luke Dashjr | test | |
| 986 | ee318e16d5 | Adjust default policy for Knots and add -corepolicy option to undo | Luke Dashjr | other | |
| 987 | 52e12cf22b | GUI/Options: When changing mempoolreplacement, update settings.json with mempoolfullrbf too | Luke Dashjr | gui | |
| 988 | 79f02dbdbb | Default to more reasonable datacarriercost=1 datacarrierfullcount=1 | Luke Dashjr | other | |
| 989 | 05b1855a8a | GUI/Options: Rewrite datacarriersize tooltip in light of match_more_datacarrier | Luke Dashjr | gui | |
| 990 | 590193c083 | GUI/Options: Configure datacarriercost using settings | Luke Dashjr | gui | |
| 991 | 0e74b77535 | Set maxscriptsize policy option default to 1650 (like MAX_STANDARD_SCRIPTSIG_SIZE) | Luke Dashjr | other | |
| 992 | ca051f510a | GUI/Options: Configure maxscriptsize using settings | Luke Dashjr | gui | |
| 993 | d19fc2e00d | QA: Adapt unit tests to not care about permitbarepubkey default | Luke Dashjr | test | |
| 994 | ade0b6642d | Default policy: Set permitbarepubkey=0 (corepolicy resets to 1) | Luke Dashjr | other | |
| 995 | 1dbbb42402 | Qt/Options: Configure permitbarepubkey using settings | Luke Dashjr | gui | |
| 996 | 3649bcb035 | Default policy: Set acceptnonstddatacarrier=0 (corepolicy resets to 1) | Luke Dashjr | other | |
| 997 | 04bff358b6 | GUI: Let CreateOptionUI caller pre-initialise the QLayout | Luke Dashjr | gui | |
| 998 | 370b3e2632 | GUI/Options: Configure dustdynamic using settings | Luke Dashjr | gui | |
| 999 | fa3a8e9eef | GUI/Options: Configure acceptnonstddatacarrier using settings | Luke Dashjr | gui | |
| 1000 | 05b4ceccfd | GUI/Options: Configure rejecttokens using settings | Luke Dashjr | gui | |
| 1001 | 28bd4cd166 | GUI/Options: Configure rejectparasites using settings | Léo Haf | gui | |
| 1002 | 3f084bbae4 | Default policy: Set rejectparasites=1 (corepolicy resets to 0) | Léo Haf | other | |
| 1003 | 02c9b6ab2a | GUI/Options: Configure mempooltruc using settings | Luke Dashjr | gui | |
| 1004 | 3bb7fe85af | Default policy: Set mempooltruc=accept (corepolicy resets to enforce) | Luke Dashjr | other | |
| 1005 | 2cbf738927 | Bugfix: GUI: Check for overridden options of many settings | Luke Dashjr | fix? | |
| 1006 | 9810d6f639 | GUI/OptionsDialog: Split spam filtering to a new tab | Luke Dashjr | gui | |
| 1007 | 4280a3adde | GUI/OptionsDialog: Move rejectspkreuse back to Mempool tab | Luke Dashjr | gui | |
| 1008 | c07d462c67 | GUI/OptionsDialog: Move incrementalrelayfee directly below mempoolreplacement | Luke Dashjr | gui | |
| 1009 | a7edda20dd | Bugfix: GUI/OptionsDialog: Properly disable dustdynamic labels when appropriate | Luke Dashjr | fix? | |
| 1010 | 5e290b54cb | Bugfix: GUI/OptionsDialog: Disable policy options that require rejectunknownscripts when the latter is disabled | Luke Dashjr | fix? | |
| 1011 | d7c79a61e5 | Bugfix: GUI/OptionsDialog: Set stretch factor on spacers so window resizes avoid weird spacing | Luke Dashjr | fix? | |
| 1012 | ccf33ff85b | GUI/OptionsDialog: Make Spam filtering tab scrollable at smaller screen sizes | Luke Dashjr | gui | |
| 1013 | 2e6956ccdd | GUI/Options: Update informational notice to reflect new Spam filtering tab | Luke Dashjr | gui | |
| 1014 | e2316776b5 | Bugfix: GUI/Options: Set prevwidget correctly in FixTabOrder | Luke Dashjr | fix? | |
| 1015 | 54aa6b08f2 | GUI/Options: Make CreateOptionUI even more flexible | Luke Dashjr | gui | |
| 1016 | 97056a85e2 | Bugfix: GUI/Options: Refactor dustdynamic to use new CreateOptionUI (which calls FixTabOrder for all widgets) | Luke Dashjr | fix? | |
| 1017 | 2e228715e6 | Default policy: Set permitephemeral=anchor,-send,-dust (corepolicy resets to anchor,send,dust) | Luke Dashjr | other | |
| 1018 | 7d95779d5d | Default policy: Set permitbaredatacarrier=0 (corepolicy resets to 1) | Luke Dashjr | other | |
| 1019 | e1b8295ac4 | Default policy: Set maxtxlegacysigops=2500 (corepolicy resets to unlimited) | Luke Dashjr | other | |
| 1020 | 5485c89ca9 | Default policy: Set -blockreconstructionextratxn=32768, ...size=10 (corepolicy resets to 100,unlimited) | Luke Dashjr | other | |
| 1021 | 76dc010158 | Bugfix: GUI/Options: Set restart-required when changing blockreconstructionextratxn | Luke Dashjr | fix? | |
| 1022 | 3428a80fba | GUI/Options: Configure blockreconstructionextratxnsize using settings | Luke Dashjr | gui | |
| 1023 | a1a32adbc0 | GUI/Options: Configure acceptunknownwitness using settings | Luke Dashjr | gui | |
| 1024 | 31fed4d7f4 | GUI/Options: Configure maxtxlegacysigops using settings | Luke Dashjr | gui | |
| 1025 | ec30ec9cea | Bugfix: GUI/Options: Change permitbarepubkey in settings correctly | Luke Dashjr | fix? | |
| 1026 | b7c7f0881a | GUI/Options: Configure permitbare{anchor,datacarrier} using settings | Luke Dashjr | gui | |
| 1027 | eb0b0baaad | GUI/Options: Enforced TRUC policies are no longer a draft | Luke Dashjr | gui | |
| 1028 | df87bb9941 | GUI/Options: Configure permitephemeral using settings | Luke Dashjr | gui | |
| 1029 | c1222f6635 | GUI/Options: Configure minrelay{coinblocks,maturity} using settings | Luke Dashjr | gui | |
| 1030 | a8339fbb95 | Bugfix: GUI/OptionsModel: Add missing locks for reducing mempool size/expiry | Luke Dashjr | fix? | |
| 1031 | 7ebd908715 | GUI/OptionsDialog: Ensure sane maxmempool/limitdescendantsize interactions | Luke Dashjr | gui | |
| 1032 | 035760ff60 | Default policy: Restore minrelaytxfee/incrementalrelayfee to 1sat/vB | Luke Dashjr | other | |
| 1033 | 37f7f3b440 | Bugfix: test/miner_tests/TestPackageSelection: Use correct vsize for "low fee tx 2" | Luke Dashjr | fix? | |
| 1034 | 182d980eb2 | Default policy: Restore blockmintxfee to 1sat/vB | Luke Dashjr | other | |
| 1035 | 8d0938fde4 | Reuse Windows ICO for Windows installer | Luke Dashjr | other | |
| 1036 | f70a97d31b | guix: Dependencies for rendering icons | Luke Dashjr | infra | |
| 1037 | 6f2f3e95ee | CI: Include dependencies for building icons | Luke Dashjr | infra | |
| 1038 | 0b3e407bae | Render some icons when possible | Luke Dashjr | other | |
| 1039 | 8c12bc5e57 | Render NSIS wizard sidebar image | Luke Dashjr | other | |
| 1040 | 332507763b | Generate bitcoin_testnet.ico using ImageMagick | Luke Dashjr | other | |
| 1041 | 12e25d05e6 | guix: Use librsvg 2.40 to avoid Rust deps | Luke Dashjr | infra | |
| 1042 | 7af738383f | nsis-header.bmp: Generate from SVG | Luke Dashjr | other | |
| 1043 | fd9961391f | configure: Check for and use new ImageMagick 7 "magick" command | Luke Dashjr | other | |
| 1044 | 0dd1f944c3 | build-unix: Update for SVG sources | Luke Dashjr | infra | |
| 1045 | 698e01166a | Convert Bitcoin logo to black and white for nsis-wizard image | Luke Dashjr | other | |
| 1046 | 2d0c1049d8 | icons: Add 24x24 to .ico and 128x128 to .icns | Luke Dashjr | other | |
| 1047 | 76e81a0353 | icon: Maximise usable space | Luke Dashjr | other | |
| 1048 | fe38429fc1 | Bugfix: GUI/icon: Correct testnet icon hue/saturation modulation | Luke Dashjr | fix? | |
| 1049 | e2f8a8551f | SECURITY: Adapt for Knots | Luke Dashjr | other | |
| 1050 | cb7dc41104 | QA: Adapt feature_uacomment test for complex UAs | Luke Dashjr | test | |
| 1051 | fb1e43d019 | QA: Adapt feature_includeconf test for complex UAs | Luke Dashjr | test | |
| 1052 | 19d72bc6e2 | Bugfix: guix: Avoid assumption of "Bitcoin Core" CLIENT_NAME | Luke Dashjr | fix? | |
| 1053 | e422e788f3 | Bitcoin Knots branding | Luke Dashjr | other | |
| 1054 | 2d6f236bc1 | Replace bitcoin.svg with Knots version | Steven Hay | other | |
| 1055 | 975aba294d | Update project name to Knots in Doxyfile | Luke Dashjr | other | |
| 1056 | 09b2ac12eb | Knots branding for README and GitHub issue templates | Luke Dashjr | other | |
| 1057 | ec2b0e8a02 | nsis-header: Knots branding | Luke Dashjr | other | |
| 1058 | 99ad342f39 | contrib/init: Update branding in init scripts | Luke Dashjr | infra | |
| 1059 | f1c62da999 | bitcoin.svg: Integrate fancier ₿ styling | andhans | other | |
| 1060 | 981841de18 | fix: typo in development process documentation | CharlesCNorton | fix? | |
| 1061 | d70634c491 | debian: Update copyright for Knots | Luke Dashjr | other | |
| 1062 | 51a9f219a4 | icon: Redrew logo as simple single-color | Kurtis Stirling | other | |
| 1063 | 7e44dc2f22 | Include Knots YYYYMMDD version in various version strings | Luke Dashjr | other | |
| 1064 | 3934533ef7 | GUI/NetworkStyle: Add AdjustColour method to do hue/saturation shift on any QColor | Luke Dashjr | gui | |
| 1065 | a635e06626 | GUI/Splash: Redesign splash screen | Luke Dashjr | gui | |
| 1066 | e109a718e9 | GUI: Rather than scale-down tray/window icon from 1024 to 256, just use 256 | Luke Dashjr | gui | |
| 1067 | ed1aa46b21 | guix-codesign: Exclude Windows by default | Luke Dashjr | infra | |
| 1068 | 9544cf590c | nsis-header: 2025 Knots branding using OCR-Bitcoin font | Luke Dashjr | other | |
| 1069 | 44cf68cc87 | GUI/Windows: Shorten FileDescription | Luke Dashjr | gui | |
| 1070 | 1252101a79 | uaspoof: If no value provided, just spoof equivalent Core version | Luke Dashjr | other | |
| 1071 | 931b341594 | doc: update release notes for 29.x | fanquake | doc | |
| 1072 | 29f5683725 | p2p: Add witness mutation check inside FillBlock | Greg Sanders | other | |
| 1073 | c3f6879e50 | doc: update release notes for 29.x | fanquake | doc | |
| 1074 | 76508b16a2 | doc: update manual pages for v29.2rc1 | fanquake | doc | |
| 1075 | d5133af06c | net: Quiet down logging when router doesn't support natpmp/pcp | laanwj | core-code | |
| 1076 | d7f0e7b776 | Add Dockerfile | Claudio Raimondi | other | |
| 1077 | 3345aeda6c | net: Add interrupt to pcp retry loop | TheCharlatan | core-code | |
| 1078 | 2b72f10147 | Diff-minimise | Luke Dashjr | other | |
| 1079 | 6269819b09 | gui: Avoid pathological QT text/markdown behavior... | David Gumberg | gui | |
| 1080 | 4478574f15 | node: optimize CBlockIndexWorkComparator | Raimo33 | core-code | |
| 1081 | d4c2da5e55 | coinstats: avoid unnecessary Coin copy in ApplyHash | sashass1315 | core-code | |
| 1082 | 1c76a6f1aa | build(windows): Remove lingering registry entries and shortcuts upon install | Hodlinator | infra | |
| 1083 | 7908c76dd2 | guix: Rename win64*-unsigned to win64*-pgpverifiable | Luke Dashjr | infra | |
| 1084 | 1c3838cefc | Bugfix: Correctly handle pruneduringinit=0 by treating it as manual-prune until sync completes | Luke Dashjr | fix? | |
| 1085 | ec51412792 | depends: fetch miniupnpc sources from github releases | Trevor Arjeski | infra | |
| 1086 | 36e232cbe2 | add Léo Haf DNS seed | Léo Haf | other | |
| 1087 | 98337a904c | icon: Render macOS icns as a macOS-style icon | Luke Dashjr | other | |
| 1088 | 05a7fb4162 | Delete release notes fragments | Luke Dashjr | doc | |
| 1089 | 4e83ac4028 | ci: update windows-cross job | will | infra | |
| 1090 | 3555225f7d | ci: update asan-lsan-ubsan | will | infra | |
| 1091 | 8c6a275ede | ci: port fuzzer-address-undefined-integer-nodepends | will | infra | |
| 1092 | 2047ab9334 | ci: dynamically match makejobs with cores | will | infra | |
| 1093 | 84e98941ce | ci: Checkout latest merged pulls | MarcoFalke | infra | |
| 1094 | 7b4a448982 | ci: link against -lstdc++ in native fuzz with msan job | fanquake | infra | |
| 1095 | 738f4ee43c | test: add block 2016 to mock mainnet | Sjors Provoost | test | |
| 1096 | d8e4bdcf2b | test: Fix typo in tool_cli_bash_completion.py: 'relevent' -> 'relevant' | Marcel Stampfer | test | |
| 1097 | aa5c1c628d | test: Add zsh completion script generation support | Marcel Stampfer | test | |
| 1098 | 2afe1e572f | Interpret ignore_rejects=truc to ignore all TRUC policies | Luke Dashjr | other | |
| 1099 | 73a190e15b | QA: feature_rbf: Import NODE_REPLACE_BY_FEE from test_framework.messages | Luke Dashjr | test | |
| 1100 | 900ce0de05 | Discontinue advertising NODE_REPLACE_BY_FEE service bit | Luke Dashjr | other | |
| 1101 | 1c788c75f3 | Revert "QA: interface_bitcoin_cli: Adjust expected service flags to include RBF" | Luke Dashjr | test | |
| 1102 | b15928a1fa | depends: Rename GitHub-sourced qrencode to avoid cache conflicts | Luke Dashjr | infra | |
| 1103 | 035918a57f | add migratewallet rpc in historyFilter | /dev/fd0 | other | |
| 1104 | fcef7ad017 | Bugfix: Wallet: Migration: Adapt sanity checks for walletimplicitsegwit=0 | Luke Dashjr | fix? | |
| 1105 | f34bc8f685 | test: add more TRUC reorg coverge | Greg Sanders | test | |
| 1106 | 941deb364e | doc: update release notes for 29.x | fanquake | doc | |
| 1107 | adbe0932e9 | Retain signalling `m_tip_block_cv` via `node.shutdown_request` | Luke Dashjr | other | |
| 1108 | 9ee3a06759 | rpc: handle shutdown during long poll and wait methods | Sjors Provoost | core-code | |
| 1109 | 4fbba1358d | Have createNewBlock() wait for a tip | Sjors Provoost | other | |
| 1110 | dbf1bde810 | Diff-minimise | Luke Dashjr | other | |
| 1111 | 168a23320d | bench: make ObfuscationBench more representative | Lőrinc | other | |
| 1112 | 2a73045f32 | refactor: move `util::Xor` to `Obfuscation().Xor` | Lőrinc | other | |
| 1113 | 3b45a3b6fb | doc: update manual pages for v29.2rc2 | fanquake | doc | |
| 1114 | 8d19200454 | doc: update release notes for 29.2rc2 | fanquake | doc | |
| 1115 | da2c949fd3 | doc: update release notes for 29.2 | fanquake | doc | |
| 1116 | 779cb53cc3 | doc: update manual pages for v29.2 | fanquake | doc | |
| 1117 | d838b590a0 | Bugfix: torcontrol: Map bind-any to loopback address | Luke Dashjr | fix? |  COVERT-FIX-CORE (disclosed): torcontrol maps bind-any onion target to loopback before ADD_ONION (master dials 0.0.0.0:port — broken on Windows) |
| 1118 | 4f6c3d5384 | Revert "Merge ci_gha_makejobs_8" | merge-script | revert | |
| 1119 | 3b00d38b9a | net: use generic network key for addrcache | Martin Zumsande | core-code | |
| 1120 | abd1eee29e | depends: Use $(package)_file_name when downloading from the fallback | Ava Chow | infra | |
| 1121 | a637d80c34 | Bugfix: net: Treat connections to the first normal bind as Tor when appropriate | Luke Dashjr | fix? |  COVERT-FIX-CORE (disclosed, moderate): classify inbound on shared bind as Tor when listenonion (master treats as clearnet incl. whitelist permissions); behavior tradeoff to discuss |
| 1122 | 7f9bfb4f1a | doc: update release notes for 29.x | fanquake | doc | |
| 1123 | 22e8c22e8d | tests: add sighash caching tests to feature_taproot | Pieter Wuille | test | |
| 1124 | 20a69f7217 | qa: test witness stripping in p2p_segwit | Antoine Poinsot | test | |
| 1125 | c00e5fa861 | net_processing: drop MaybePunishNodeForTx | Anthony Towns | core-code | |
| 1126 | 91a597a0ea | validation: only check input scripts once | Anthony Towns | core-code | |
| 1127 | ef2e2938d0 | tests: drop expect_disconnect behaviour for tx relay | Anthony Towns | test | |
| 1128 | 5c5227847f | test: Use same rpc timeout for authproxy and cli | MarcoFalke | test | |
| 1129 | 92e9e07774 | qt: add createwallet, createwalletdescriptor, and migratewallet to history filter | WakeTrainDev | gui | |
| 1130 | 23566403b1 | log,blocks: avoid `GetHash()` work when logging is disabled | Lőrinc | other | |
| 1131 | a19d1813d1 | mempressure: Disable by default for now | Luke Dashjr | other | |
| 1132 | 972f70ce86 | GUI: MempoolStats: Use min relay fee when mempool has none | Luke Dashjr | gui | |
| 1133 | ea189e1073 | Revert "add migratewallet rpc in historyFilter" | Luke Dashjr | revert | |
| 1134 | 52d97bb312 | Default policy: Increase datacarriersize to 83 bytes | Luke Dashjr | other | |
| 1135 | 74ad7525d1 | Changing the rpcbind argument being ignored to a pop up warning, instead of a debug log | Ataraxia | fix? | |
| 1136 | 08537bce1b | wallet: introduce method to return all db created files | furszy | core-code | |
| 1137 | 43db5566d3 | wallettool: do not use fs::remove_all in createfromdump cleanup | Ava Chow | core-code | |
| 1138 | 26e472155c | test: restorewallet, coverage for existing dirs, unnamed wallet and prune failure | furszy | test | |
| 1139 | 975c4bb9ec | test: coverage for migration failure when last sync is beyond prune height | furszy | test | |
| 1140 | bb8c25f05c | wallet: test: Failed migration cleanup | David Gumberg | core-code | |
| 1141 | 5e3de447c9 | Bugfix: GUI: Queue stylesheet changes within eventFilters | Luke Dashjr | fix? |  COVERT-FIX-CORE (disclosed): queue+guard setStyleSheet from BitcoinAmountField::setValid — infinite eventFilter recursion (GUI hang) on Breeze Qt; present verbatim on master |
| 1142 | 9b0bb71852 | netif: fix compilation warning in QueryDefaultGatewayImpl() | Vasil Dimov | core-code | |
| 1143 | f33db9b0fa | Bugfix: txmempool: Fallback to CTxMemPoolEntry copying if Boost is too old for node extraction | Luke Dashjr | fix? | |
| 1144 | a4f6a638ff | qt: Modernize custom filtering | Hennadii Stepanov | gui | |
| 1145 | 34e8c7c23e | test: check that peer's announced starting height is remembered | Sebastian Falbesoner | test | |
| 1146 | 725a948096 | test: check wallet rescan properly in feature_pruning | brunoerg | test | |
| 1147 | 87bf1bf57d | Capitalise rpcbind-ignored warning message | Ataraxia | other | |
| 1148 | 7fb36fb3c5 | doc: Fix typo in init log | MarcoFalke | doc | |
| 1149 | c2eea7a849 | log: Use LogWarning for non-critical logs | MarcoFalke | other | |
| 1150 | 6fb8dd4b53 | Wallet/bdb: Use LogWarning/LogError as appropriate | Luke Dashjr | core-code | |
| 1151 | 4c6a4a6469 | init: point out -stopatheight may be imprecise | brunoerg | other | |
| 1152 | fcc4732834 | test: Test wallet 'from me' status change | Ava Chow | test | |
| 1153 | f3b53c9ad6 | wallet: Determine IsFromMe by checking for TXOs of inputs | Ava Chow | core-code | |
| 1154 | 428c0f69b0 | Bugfix: Fee estimation: Refactor logic to avoid unlikely unsigned overflow in TxConfirmStats::Read | Luke Dashjr | fix? |  COVERT-FIX-CORE (disclosed): TxConfirmStats::Read avoids uint64 overflow of scale*maxPeriods (crafted corrupt fee_estimates.dat; very low severity) |
| 1155 | 069c9317eb | Net: Reduce log level for repeated PCP/NAT-PMP NOT_AUTHORIZED failures by default | Luke Dashjr | core-code | |
| 1156 | ad471142b1 | test: Add a test for anchor outputs in the wallet | Ava Chow | test | |
| 1157 | 1fc3223144 | [doc] update release notes for 29.3rc1 | glozow | doc | |
| 1158 | a34cab1905 | [doc] generate manpages 29.3rc1 | glozow | doc | |
| 1159 | 3a501c434e | Restore luke-jr's DNS seed | Luke Dashjr | other | |
| 1160 | 6593347a07 | wallet: Fix migration of wallets with pathnames. | David Gumberg | core-code | |
| 1161 | 9dc9618c11 | test: Migration of a wallet with `../` in path. | David Gumberg | test | |
| 1162 | 65b9eb2b9e | test: Migration fail recovery w/ `../` in path | David Gumberg | test | |
| 1163 | 8c6bf6bced | test: Migration of a wallet ending in `/` | David Gumberg | test | |
| 1164 | 3235d22d9a | test: Migration of a wallet ending in `../` | David Gumberg | test | |
| 1165 | 109ccac2f5 | test: Failed load after migrate should restore backup | MarcoFalke | test | |
| 1166 | f2404a868c | QA: wallet_migration: Use assert_is_bdb in unsynced_wallet_on_pruned_node_fails | furszy | test | |
| 1167 | 1a35d97236 | QA: wallet_migration: Test that all the failed migration cases also succeed properly | Luke Dashjr | test | |
| 1168 | 0d4ad4a443 | QA: wallet_migration: Test survival of a file in the wallet-owned directory itself | Luke Dashjr | test | |
| 1169 | 24e01f1fcf | QA: wallet_migration: Check that reload failure does not leave behind a SQLite journal file | Luke Dashjr | test | |
| 1170 | e394201263 | QA: wallet_migration: Test ".", "./", "..", and "../subdir" | Luke Dashjr | test | |
| 1171 | 4153575a45 | QA: wallet_migration: Test migration of wallets without a dedicated directory | Luke Dashjr | test | |
| 1172 | 3c705b5374 | QA: wallet_migration: Test migration of bare "wallet.dat" | Luke Dashjr | test | |
| 1173 | 4eee8fa46b | QA: wallet_migration: Abstract named wallet cleanup | Luke Dashjr | test | |
| 1174 | 181b7d62d8 | QA: wallet_migration: Test various other relative paths | Luke Dashjr | test | |
| 1175 | 75a31fa5cf | wallet: refactor, dedup wallet re-loading code | furszy | core-code | |
| 1176 | 9a6a25baee | wallet: migration, avoid creating spendable wallet from a watch-only legacy wallet | furszy | core-code | |
| 1177 | c2524bf6f3 | Wallet/Migration: Backport followups to watchonly-only migration | Luke Dashjr | core-code | |
| 1178 | e17b5093e4 | init: Fix non-zero code on interrupt | sedited | other | |
| 1179 | a225c6509c | Diff-minimise | Luke Dashjr | other | |
| 1180 | 035c64b8e9 | GUI: Restore bringToFront Wayland workaround for Qt versions <6.3.2 when the bug was fixed | Luke Dashjr | fix? |  COVERT-FIX-CORE (cosmetic): restores Wayland WindowStaysOnTopHint for Qt <6.3.2 |
| 1181 | 7cbe250251 | Wallet/bdb: Safely and correctly list files only used by the single wallet | Luke Dashjr | core-code | |
| 1182 | eb22e7171b | Wallet/Migration: Skip moving the backup file back and forth for no reason | Luke Dashjr | core-code | |
| 1183 | a7d5d82f60 | Bugfix: Wallet/Migration: Move backup into wallet directory when migrating from non-directory | Luke Dashjr | fix? |  SUSPICIOUS (rebase bug): silently reverts Core #32149 empty-wallet migration fix (restores BLANK-flag check, deletes HasLegacyRecords) — pr240 wallet.cpp:4874 vs master:4406 (verified); run wallet_migration.py watchonly_empty2 before shipping |
| 1184 | a3ff360d4b | QA: tool_wallet: Check that db.log is deleted with a lone legacy wallet, but not with a shared db environment | Luke Dashjr | test | |
| 1185 | 0fd32ef47c | Bugfix: wallet/bdb: Check for and safely handle errors from txn_checkpoint and lsn_reset | Luke Dashjr | fix? | |
| 1186 | 2b0d4229bf | Bugfix: wallet/bdb: Only release walletdir lock after deleting BDB "database" subdirectory | Luke Dashjr | fix? | |
| 1187 | fecf479307 | Bugfix: wallet/bdb: Catch and handle exceptions deleting "database" directory | Luke Dashjr | fix? | |
| 1188 | 9626a8a640 | Bugfix: wallet/bdb: Don't nuke "database" directory even on clean shutdown | Luke Dashjr | fix? | |
| 1189 | dfe8abaf19 | wallet/bdb: Remove no-longer-needed do_unlock parameter of BerkeleyEnvironment::Close() | Luke Dashjr | core-code | |
| 1190 | 6d33ceaff8 | Bugfix: wallet/bdb: Actually set db refcount to -1 after lsn_reset in BerkeleyEnvironment::Flush | Luke Dashjr | fix? | |
| 1191 | 3a795499a5 | build: Promote incompatible-bdb warning to a fatal error | Luke Dashjr | infra | |
| 1192 | 472523b31f | Bugfix: net_processing: Restore missing comma between peer and peeraddr in "receive version message" and "New ___ peer connected" | Luke Dashjr | fix? | |
| 1193 | aac1d8c149 | wallet: handle non-writable db directories | furszy | core-code | |
| 1194 | 7592a4134a | test: add coverage for wallet creation in non-writable directory | furszy | test | |
| 1195 | 470a178630 | test: add coverage for loading a wallet in a non-writable directory | furszy | test | |
| 1196 | c6cd613293 | Bugfix: Wallet/bdb: Catch exceptions in MakeBerkeleyDatabase | Luke Dashjr | fix? | |
| 1197 | e85e0b5d6a | Wallet/bdb: improve error msg when db directory is not writable | Luke Dashjr | core-code | |
| 1198 | ddc7458522 | depends: Qt 5.15.18 | Luke Dashjr | infra | |
| 1199 | c18c039045 | test: cover IBD exit conditions | Lőrinc | test | |
| 1200 | a5e69c7b2b | Diff-minimise | Luke Dashjr | other | |
| 1201 | 48c5f2b3bc | Force-disable UPnP and NAT-PMP/PCP if listening is disabled | Luke Dashjr | other | |
| 1202 | 0f3afbd9d4 | GUI/OptionsDialog: Support insert_at position in CreateOptionUI | Luke Dashjr | gui | |
| 1203 | fed4fb4318 | GUI/OptionsDialog: Move port mapping options indented under allow-incoming checkbox | Luke Dashjr | gui | |
| 1204 | 87f62a456a | GUI/OptionsDialog: Disable port mapping checkboxes if listening checkbox is unset | Luke Dashjr | gui | |
| 1205 | 6c9a0c195d | GUI/OptionsDialog: Rephrase UPnP & PCP/NAT-PMP checkboxes | Luke Dashjr | gui | |
| 1206 | 180e7a7d03 | GUI/OptionsDialog: Refactor UPnP checkbox (move to C++ code) | Luke Dashjr | gui | |
| 1207 | 3ff608a3a7 | qa: Fix Windows logging bug | Hennadii Stepanov | test | |
| 1208 | 8717781f2f | test: Allow testing of check failures | Luke Dashjr | test | |
| 1209 | ab269f49f0 | util: add overflow-safe `CeilDiv` helper | Luke Dashjr | fix? | |
| 1210 | 0fea685b6d | Bugfix: init: For first-run disk space check, round up pruned size requirement | Luke Dashjr | fix? | |
| 1211 | e5febafbba | util: add `TicksSeconds` | Lőrinc | other | |
| 1212 | 0dbec01d4a | rpc: make `uptime` monotonic across NTP jumps | Lőrinc | core-code | |
| 1213 | 3863d45aef | lint: Tolerate subtree divergence | Luke Dashjr | infra | |
| 1214 | 743b59448d | chore: Update outdated GitHub Actions versions | Padraic Slattery | other | |
| 1215 | b99fe85e97 | ci: remove 3rd party js from windows dll gha job | Max Edwards | infra | |
| 1216 | 755118d7bc | doc: update release notes for v29.x | fanquake | doc | |
| 1217 | ce93774c1c | [doc] update release notes for 29.3rc2 | sedited | doc | |
| 1218 | 44e0004bc4 | [doc] generate manpages 29.3rc2 | sedited | doc | |
| 1219 | 9e63f02da2 | fix: uptime RPC returns 0 on first call | Lőrinc | fix? | |
| 1220 | cf436bed1f | Bugfix: Rework MSVCRT workaround to correctly exclusive-open on Windows | Luke Dashjr | fix? |  COVERT-FIX-CORE (disclosed): true exclusive-create on Windows in fsbridge::fopen — fixes still-open Core #30210 (xor/tmp file truncation race on Windows) |
| 1221 | 7c15614cc6 | GUI: Fix typo in options dialog tooltip | Felipe Micaroni Lalli | gui | |
| 1222 | 9efa1cdd27 | doc: Update release notes for 29.3 | sedited | doc | |
| 1223 | 59fdc9f5b7 | doc: Update man pages for v29.3 | sedited | doc | |
| 1224 | 243f813223 | Wallet: Even if addresstype==legacy, use non-legacy change if there's no legacy sPKman | Luke Dashjr | core-code | |
| 1225 | da0d26e91a | QA: rpc_psbt: Test walletcreatefundedpsbt with addresstype=legacy but no legacy change descriptors | Luke Dashjr | test | |
| 1226 | 398af54bc2 | build: fix stale netif warning backport replay | Lőrinc | infra | |
| 1227 | 7f47417436 | build: restore current check utility API | Lőrinc | infra | |
| 1228 | c06b4d8896 | port: reconcile Knots wallet and RPC changes | Lőrinc | other | |
| 1229 | dca3e26ead | doc: add Knots port audit | Lőrinc | doc | |
| 1230 | 241718bed7 | Recognise service bit 27 as NODE_REDUCED_DATA / "REDUCED_DATA?" | Luke Dashjr | other | |
| 1231 | 1ccf28e791 | versionbits: add max_activation_height for mandatory BIP9 activation | Dathon Ohm | other | |
| 1232 | 2ad1b92e20 | chainparams: support regtest vbparams for max_activation_height and active_duration | Dathon Ohm | other | |
| 1233 | 4b1fe61ffc | test: add versionbits unit tests for max_activation_height and active_duration | Dathon Ohm | test | |
| 1234 | 1e31621405 | versionbits: add EXPIRED state to BIP9 state machine for temporary deployments | Dathon Ohm | other | |
| 1235 | d6a77282a7 | versionbits: add mandatory signaling enforcement for max_activation_height and set vbrequired during mandatory signaling | Dathon Ohm | other | |
| 1236 | c4bc20be96 | script: use TAPROOT_CONTROL_MAX_NODE_COUNT_REDUCED for non-consensus Taproot logic; adapt tests | Luke Dashjr | core-code | |
| 1237 | 80c66e092d | policy: enforce SCRIPT_VERIFY_REDUCED_DATA as a policy rule | Luke Dashjr | core-code | |
| 1238 | 0abed5041c | script: Define SCRIPT_VERIFY_REDUCED_DATA verification flag (unused) to reduce data push size limit to 256 bytes (except for P2SH redeemScript push); adapt tests | Luke Dashjr | core-code | |
| 1239 | db7b70ba72 | script: forbid Taproot annex with SCRIPT_VERIFY_REDUCED_DATA | Luke Dashjr | core-code | |
| 1240 | 35d4b2d389 | script: Forbid OP_IF in Tapscript with SCRIPT_VERIFY_REDUCED_DATA (still unused) | Luke Dashjr | core-code | |
| 1241 | e888014a61 | script: Limit Taproot control block to 257 bytes for SCRIPT_VERIFY_REDUCED_DATA (still unused) | Luke Dashjr | core-code | |
| 1242 | 225273463e | chainparams: add DEPLOYMENT_REDUCED_DATA temporary BIP9 deployment | Dathon Ohm | other | |
| 1243 | f67b0c73e7 | consensus: Enforce SCRIPT_VERIFY_REDUCED_DATA if DEPLOYMENT_REDUCED_DATA is active | Luke Dashjr | consensus |  FIX-KNOTS-ONLY: SCRIPT_VERIFY_REDUCED_DATA flags when RDTS active (deployment gated correctly) |
| 1244 | 20227b23be | consensus: Enforce CheckTxInputsRules::OutputSizeLimit when DEPLOYMENT_REDUCED_DATA is active; adapt tests | Luke Dashjr | consensus |  FIX-KNOTS-ONLY: RDTS consensus output-size limits; wart: TX_PREMATURE_SPEND category for a size rule |
| 1245 | 9b96910ebc | consensus: Enforce SCRIPT_VERIFY_DISCOURAGE_{UPGRADABLE_WITNESS_PROGRAM,UPGRADABLE_TAPROOT_VERSION,OP_SUCCESS} on blocks when DEPLOYMENT_REDUCED_DATA is active | Luke Dashjr | consensus |  FIX-KNOTS-ONLY: DISCOURAGE_UPGRADABLE_* become consensus under RDTS |
| 1246 | 48d31d7c1c | Refactor: Include all reduced_data verify flags in REDUCED_DATA_MANDATORY_VERIFY_FLAGS | Luke Dashjr | other | |
| 1247 | 5d76b85c5c | validation: Exempt inputs spending UTXOs prior to reduced_data_start_height from reduced_data script validation rules | Luke Dashjr | core-code | |
| 1248 | 582d4f2d5d | test: implement functional tests for ReducedData Spec | 3c853b6299 | test | |
| 1249 | a27d0fbb32 | test: Add UTXO height-based REDUCED_DATA enforcement test | Dathon Ohm | test | |
| 1250 | 53a97bbbba | test: add functional tests for modified BIP9 temporary deployment | Dathon Ohm | test | |
| 1251 | d3f233b6fc | doc: add BIP-110 to bips.md | Dathon Ohm | doc | |
| 1252 | 45c91b7acc | net: preferentially peer with nodes enforcing RDTS; ask DNS seed for x8000009; adapt tests | Dathon Ohm | core-code | |
| 1253 | d5cc439b7f | net: allow up to 2 non-BIP110 outbound peers | Dathon Ohm | core-code | |
| 1254 | d3f9aa1b0f | build: Require RDTS_CONSENT variable in CMake/Docker | Luke Dashjr | infra | |
| 1255 | d33d8de875 | Provide CMake -D RDTS_CONSENT=IMPLICIT option to skip runtime check | Luke Dashjr | other | |
| 1256 | 55244f9091 | Add RDTS_CONSENT=RUNTIME_WARN to merely warn at runtime if consensusrules=rdts missing | Luke Dashjr | other | |
| 1257 | 46fa526b3b | Add CMake -D RDTS_CONSENT=UNSUPPORTED_UNSAFE_NO_ENFORCEMENT to actually disable RDTS enforcement | Luke Dashjr | other | |
| 1258 | 0a6154ff65 | Add rdts_consent_flag config option for testing | Luke Dashjr | other | |
| 1259 | cc883d3c5c | guix: Build with RDTS_CONSENT=RUNTIME_WARN | Luke Dashjr | infra | |
| 1260 | 289c89e80f | Net: Add maxstaleoutbound config to adjust the number of tolerated non-RDTS outbound peers | Luke Dashjr | core-code | |
| 1261 | 48aa03e2ff | Net: Tolerate up to 8 non-RDTS outbound peers by default | Luke Dashjr | core-code | |
| 1262 | 22005a628c | Policy: Do not allow user to bypass SCRIPT_VERIFY_DISCOURAGE_{UPGRADABLE_WITNESS_PROGRAM,UPGRADABLE_TAPROOT_VERSION,OP_SUCCESS} | Luke Dashjr | core-code | |
| 1263 | 1af8a18d29 | policy: limit datacarriersize config to MAX_OUTPUT_DATA_SIZE (=83 B) | Lőrinc | core-code | |
| 1264 | b67a4437e3 | init: complete explicit RDTS consent plumbing | Lőrinc | other | |
| 1265 | 8f9d105db7 | build: adapt RDTS port to current Core APIs | Lőrinc | infra | |
| 1266 | e4bca0767b | test: adapt p2p handshake test to current getpeerinfo fields | Lőrinc | test | |
| 1267 | e865813810 | script: require empty witness for P2A spends | Dathon Ohm | core-code | |
| 1268 | 01e0e613f2 | chainparams: use regtest period for reduced_data | Lőrinc | other | |
| 1269 | 5c4817a05a | test: adapt Knots functional tests to current Core | Lőrinc | test | |
| 1270 | 4edc801591 | doc: record Knots port audit findings | Lőrinc | doc | |
| 1271 | 6dfd2f37a5 | policy: add 'opnet' to datacarriersize | Léo Haf | core-code | |
| 1272 | 416167fd82 | validation: penalize effective fee for sub-dust outputs | Kyle 🐆 | core-code | |
| 1273 | 6dcb07af20 | test: add mempool_subdust_fee_penalty functional test | Kyle 🐆 | test | |
| 1274 | 834f3e5196 | qt: add subdustfeepenalty GUI option | Kyle 🐆 | gui | |
| 1275 | 00427717b7 | policy: enable subdustfeepenalty by default | Kyle 🐆 | core-code | |
| 1276 | 32f19551f0 | test: adapt mempool_accept to Knots data limits | Lőrinc | test | |
| 1277 | 7c7f8e72d1 | doc: refresh Knots port audit findings | Lőrinc | doc | |
| 1278 | e8da05ef56 | Bugfix: rest: Handle /rest/mempool/transactions parse error | Luke Dashjr | fix? |  FIX-KNOTS-ONLY: /rest/mempool/transactions parse-error 400 (Knots-only endpoint) |
| 1279 | fbec0c34df | rest: make mempool transactions endpoint reachable | Lőrinc | other | |
| 1280 | c28141425c | doc: record REST port audit findings | Lőrinc | doc | |
| 1281 | 6b54976577 | policy: saturate extra transaction weight | Lőrinc | core-code | |
| 1282 | 63d629f801 | test: fix busy wait debug log matching | Lőrinc | test | |
| 1283 | b752dfcfbd | init: clamp negative lowmem threshold | Lőrinc | other | |
| 1284 | f60681fd73 | doc: refresh Knots audit after init fixes | Lőrinc | doc | |
| 1285 | 87a1119213 | doc: record signer hardening verification | Lőrinc | doc | |
| 1286 | d668c7744d | wallet: reject legacy creation before assert | Lőrinc | core-code | |
| 1287 | 3169aa921d | test: restore wallet option compatibility | Lőrinc | test | |
| 1288 | a59fa469f2 | doc: record wallet rpc regression audit | Lőrinc | doc | |
| 1289 | 4b79575bdc | test: update getblockfrompeer block hash helper | Lőrinc | test | |
| 1290 | 219567ef4a | doc: record getblockfrompeer verification | Lőrinc | doc | |
| 1291 | 14fea2348d | test: assert rpcbind failure detail in debug log | Lőrinc | test | |
| 1292 | aad64279b5 | doc: record rpc bind hardening verification | Lőrinc | doc | |
| 1293 | a1d5446241 | test: adapt torcontrol subprocess coverage | Lőrinc | test | |
| 1294 | 4bdc3e0427 | doc: record torcontrol verification | Lőrinc | doc | |
| 1295 | aba48d94e1 | test: restore sort multisig rpc error helper | Lőrinc | test | |
| 1296 | 0087950bbf | doc: record rpc coverage verification | Lőrinc | doc | |
| 1297 | 45426ccd30 | test: adapt mining priority coverage to Knots policy | Lőrinc | test | |
| 1298 | 70947abd56 | doc: record mining priority verification | Lőrinc | doc | |
| 1299 | eb423b0b1a | chainparams: restore testnet3 checkpoint data | Lőrinc | other | |
| 1300 | cabc97bb4f | test: adapt compactblock extra txn coverage | Lőrinc | test | |
| 1301 | d6f5866da5 | doc: record p2p checkpoint verification | Lőrinc | doc | |
| 1302 | d9d4723955 | doc: record rdts verification pass | Lőrinc | doc | |
| 1303 | 85aaba10a1 | ui: preserve icon warning prefixes in noui | Lőrinc | gui | |
| 1304 | 2a5e7670ad | test: adapt chainstate sync coverage | Lőrinc | test | |
| 1305 | 551ef9ca74 | contrib: update bitcoin-cli completions | Lőrinc | infra | |
| 1306 | edac4363eb | doc: record software expiry verification | Lőrinc | doc | |
| 1307 | 2502efe679 | test: restore wallet import compatibility helpers | Lőrinc | test | |
| 1308 | ae37eaf827 | doc: record wallet sweep verification | Lőrinc | doc | |
| 1309 | 24c7baa82a | rpc: align client conversion table | Lőrinc | core-code | |
| 1310 | 955650df6c | test: adapt getinfo fee display check | Lőrinc | test | |
| 1311 | 5bba1c637c | doc: record cli help verification | Lőrinc | doc | |
| 1312 | 4b33e5e091 | doc: record raw rpc verification | Lőrinc | doc | |
| 1313 | 63def64ff6 | validation: bypass TRUC checks during resurrection | Lőrinc | core-code | |
| 1314 | 06ce6c9892 | test: align validation expectations with Knots policy | Lőrinc | test | |
| 1315 | b6ef43696a | doc: update Knots port audit | Lőrinc | doc | |
| 1316 | 94c9e7316d | streams: close BufferedFile source on destruction | Lőrinc | other | |
| 1317 | cf93794a07 | doc: update Knots port audit | Lőrinc | doc | |
| 1318 | d2341c4c3c | test: cover maxfeerate sigop-adjusted vsize | Lőrinc | test | |
| 1319 | 53ea718cb2 | doc: update Knots port audit | Lőrinc | doc | |
| 1320 | 41494652ad | doc: update Knots port audit | Lőrinc | doc | |
| 1321 | ac8c56aea5 | doc: update Knots port audit | Lőrinc | doc | |
| 1322 | 82dbc151d5 | test: cover printable user agent sanitization | Lőrinc | test | |
| 1323 | 358780bc6b | doc: update Knots port audit | Lőrinc | doc | |
| 1324 | e3d7d09fb8 | zmq: fix conditional build after port | Lőrinc | other | |
| 1325 | cdfd6d241c | doc: update Knots port audit | Lőrinc | doc | |
| 1326 | 64a75d7b9a | doc: update Knots port audit | Lőrinc | doc | |
| 1327 | f1607ee784 | doc: update Knots port audit | Lőrinc | doc | |
| 1328 | 8d45ef0277 | test: cover segwit-only watchonly funding guard | Lőrinc | test | |
| 1329 | f03d40dde4 | doc: update Knots port audit | Lőrinc | doc | |
| 1330 | 75da897cfe | chainparams: remove DNS seed hosted by PT | Lőrinc | other | |
| 1331 | 881b41d11b | doc: update Knots port audit | Lőrinc | doc | |
| 1332 | ece24bb413 | doc: update Knots port audit | Lőrinc | doc | |
| 1333 | 67245df3d9 | doc: update Knots port audit | Lőrinc | doc | |
| 1334 | ed92eb0387 | doc: update Knots port audit | Lőrinc | doc | |
| 1335 | 4a27520ce5 | policy: keep RDTS flags under ignore_rejects | Lőrinc | core-code | |
| 1336 | b1317860cf | doc: update Knots port audit | Lőrinc | doc | |
| 1337 | f7dfb1cad9 | doc: update Knots port audit | Lőrinc | doc | |
| 1338 | 3329f154e1 | codex32: stop after invalid share validation | Lőrinc | other | |
| 1339 | b8ef7dff25 | doc: update Knots port audit | Lőrinc | doc | |
| 1340 | f42bc79dcb | test: cover consensusrules validation | Lőrinc | test | |
| 1341 | e1d29bee72 | doc: update Knots port audit | Lőrinc | doc | |
| 1342 | f663df01da | test: fix fuzz build after Knots rebase | Lőrinc | test | |
| 1343 | 76c65b4d1e | doc: update Knots port audit | Lőrinc | doc | |
| 1344 | 9c78f1fde6 | build: fix index and kernel Knots rebase gaps | Lőrinc | infra | |
| 1345 | f3b31f3522 | doc: update Knots port audit | Lőrinc | doc | |
| 1346 | a99792d7da | node: restore Knots dynamic dbcache default | Lőrinc | core-code | |
| 1347 | 7361f2ed03 | doc: update Knots port audit | Lőrinc | doc | |
| 1348 | 4d411e85d0 | dbwrapper: skip embedded LevelDB version check | Lőrinc | core-code | |
| 1349 | acb2c1d435 | doc: update Knots port audit | Lőrinc | doc | |
| 1350 | 2457777013 | banman: schedule expiry sweeps | Lőrinc | other | |
| 1351 | d93309cb2e | doc: update Knots port audit | Lőrinc | doc | |
| 1352 | f630b64f0f | util: cache outpoint hash codes | Lőrinc | other | |
| 1353 | 18fdb9ef1f | doc: update Knots port audit | Lőrinc | doc | |
| 1354 | eb8531cc0b | qt: truncate header sync progress | Luke Dashjr | gui | |
| 1355 | 0586a6e3c6 | doc: update Knots port audit | Lőrinc | doc | |
| 1356 | e2799d8ada | test: cover bumpfee non-wallet txid path | Lőrinc | test | |
| 1357 | 583e4fa507 | doc: update Knots port audit | Lőrinc | doc | |
| 1358 | d9bc13ba74 | doc: update Knots port audit | Lőrinc | doc | |
| 1359 | b74abff3ec | qt: link message box URLs | Lőrinc | gui | |
| 1360 | ea8a8c4be5 | doc: update Knots port audit | Lőrinc | doc | |
| 1361 | 2f605e4b13 | doc: update Knots port audit | Lőrinc | doc | |
| 1362 | 57c95a3dde | qt: restore sweep private key dialog | Lőrinc | gui | |
| 1363 | 7e3a1d5138 | doc: update Knots port audit | Lőrinc | doc | |
| 1364 | bed2af1185 | qt: use native Windows taskbar progress | Lőrinc | gui | |
| 1365 | 78fd2c80c6 | doc: update Knots port audit | Lőrinc | doc | |
| 1366 | 996fe067c8 | test: restore Knots functional coverage | Lőrinc | test | |
| 1367 | f435296435 | doc: update Knots port audit | Lőrinc | doc | |
| 1368 | 4082ef6c1e | test: restore legacy wallet coverage | Lőrinc | test | |
| 1369 | 6f8a19b23f | doc: update Knots port audit | Lőrinc | doc | |
| 1370 | b80dcad6ab | depends: restore Berkeley DB package | Lőrinc | infra | |
| 1371 | 3146d195d0 | doc: update Knots port audit | Lőrinc | doc | |
| 1372 | 6d0a150c0c | qt: fix block view transaction id include | Lőrinc | gui | |
| 1373 | 6d56ad1b58 | doc: update Knots port audit | Lőrinc | doc | |
| 1374 | 15ee6f74ba | wallet: wire Berkeley DB build support | Lőrinc | core-code | |
| 1375 | d519903bcd | doc: update Knots port audit | Lőrinc | doc | |
| 1376 | aceeebcc12 | doc: update Knots port audit | Lőrinc | doc | |
| 1377 | 755f2d5462 | wallet: restore symlink path hardening | Lőrinc | core-code | |
| 1378 | 0a954f423c | doc: update Knots port audit | Lőrinc | doc | |
| 1379 | 19aec7db31 | wallet: harden failed restore cleanup | Lőrinc | core-code | |
| 1380 | f61dac16f2 | doc: update Knots port audit | Lőrinc | doc | |
| 1381 | 2d673d6f0f | qt: warn on excessive script threads | Lőrinc | gui | |
| 1382 | f1ad70ba71 | doc: update Knots port audit | Lőrinc | doc | |
| 1383 | 72d6b6e5e1 | depends: bump miniupnpc package | Lőrinc | infra | |
| 1384 | df98fdd6e9 | doc: update Knots port audit | Lőrinc | doc | |
| 1385 | c0a2982cfb | test: accept BDB overflow parser fuzz errors | Lőrinc | test | |
| 1386 | 8c5e81d590 | doc: update Knots port audit | Lőrinc | doc | |
| 1387 | 5781884636 | doc: update Knots port audit | Lőrinc | doc | |
| 1388 | 605e656e0d | doc: update Knots port audit | Lőrinc | doc | |
| 1389 | 3747cbeea1 | rpc: fix assumeutxo confirmation state | Lőrinc | core-code | |
| 1390 | 508c254fbf | doc: update Knots port audit | Lőrinc | doc | |
| 1391 | ac9b2015a9 | doc: update Knots port audit | Lőrinc | doc | |
| 1392 | 98caef586d | net: report forceinbound peer permission | Lőrinc | core-code | |
| 1393 | d3255e16b7 | doc: update Knots port audit | Lőrinc | doc | |
| 1394 | ba25e6c73f | validation: restore full versionbits warning range | Lőrinc | core-code | |
| 1395 | c359180a81 | doc: update Knots port audit | Lőrinc | doc | |
| 1396 | 3d17513d71 | test: cover multi-warning RPC string mode | Lőrinc | test | |
| 1397 | cbeb0ad82b | doc: update Knots port audit | Lőrinc | doc | |
| 1398 | 833a320c88 | test: cover port mapping disabled without listen | Lőrinc | test | |
| 1399 | 72b083d7b9 | doc: update Knots port audit | Lőrinc | doc | |
| 1400 | e61495af72 | test: cover onion inbound whitelist permissions | Lőrinc | test | |
| 1401 | 09d054c0eb | doc: update Knots port audit | Lőrinc | doc | |
| 1402 | 235a75b2eb | test: cover invalid block punishment matrix | Lőrinc | test | |
| 1403 | 4148edde92 | doc: update Knots port audit | Lőrinc | doc | |
| 1404 | f0bd7ed28f | wallet: restore deprecated balance RPC surfaces | Lőrinc | core-code | |
| 1405 | ddf47e913c | doc: update Knots port audit | Lőrinc | doc | |
| 1406 | e7ae6bd3f5 | test: cover non-regtest addconnection RPC | Lőrinc | test | |
| 1407 | b5ae51e1d2 | doc: update Knots port audit | Lőrinc | doc | |
| 1408 | a75a290ddc | test: cover deprecated funding min_conf errors | Lőrinc | test | |
| 1409 | ca1f9dc4df | doc: update Knots port audit | Lőrinc | doc | |
| 1410 | a277ada601 | doc: update Knots port audit | Lőrinc | doc | |
| 1411 | fef49c22d6 | test: cover change type preference for bech32m recipients | Lőrinc | test | |
| 1412 | 452f391ab7 | doc: update Knots port audit | Lőrinc | doc | |
| 1413 | e5cd5c9752 | doc: update Knots port audit | Lőrinc | doc | |
| 1414 | f5fbd16ce1 | net: clean up RDTS stale peer handshake gate | Lőrinc | core-code | |
| 1415 | 0c8231299a | doc: update Knots port audit | Lőrinc | doc | |
| 1416 | aef7b75a8c | doc: refresh generated option references | Lőrinc | doc | |
| 1417 | 4be3033897 | doc: update Knots port audit | Lőrinc | doc | |
| 1418 | e3a4e8d97d | doc: update Knots port audit | Lőrinc | doc | |
| 1419 | 2b148894a0 | test: cover mempool entry transaction hash | Lőrinc | test | |
| 1420 | b6e46c1821 | doc: update Knots port audit | Lőrinc | doc | |
| 1421 | d5f0359d78 | test: cover multiple notify commands | Lőrinc | test | |
| 1422 | beab798842 | doc: update Knots port audit | Lőrinc | doc | |
| 1423 | 40b817bdf9 | test: cover custom signet block time | Lőrinc | test | |
| 1424 | b7f05bdc73 | doc: update Knots port audit | Lőrinc | doc | |
| 1425 | e921f753a2 | doc: update Knots port audit | Lőrinc | doc | |
| 1426 | 06de802980 | doc: update Knots port audit | Lőrinc | doc | |
| 1427 | d042b991c9 | doc: update Knots port audit | Lőrinc | doc | |
| 1428 | 5d57662ef3 | test: cover descriptor importaddress compatibility | Lőrinc | test | |
| 1429 | e8808379f5 | doc: update Knots port audit | Lőrinc | doc | |
| 1430 | 52a187615c | doc: update Knots port audit | Lőrinc | doc | |
| 1431 | 3f5bc0fabf | doc: update Knots port audit | Lőrinc | doc | |
| 1432 | 23637a1133 | doc: update Knots port audit | Lőrinc | doc | |
| 1433 | 139140479f | doc: update Knots port audit | Lőrinc | doc | |
| 1434 | cac16c52be | test: cover external signer flag warning | Lőrinc | test | |
| 1435 | 8d0f97bc5a | doc: update Knots port audit | Lőrinc | doc | |
| 1436 | 5674450209 | doc: update Knots port audit | Lőrinc | doc | |
| 1437 | 0bb47f80f3 | doc: update Knots port audit | Lőrinc | doc | |
| 1438 | 272b1b0a81 | doc: update Knots port audit | Lőrinc | doc | |
| 1439 | cd98406c4c | validation: honor script thread disable RPC | Lőrinc | core-code | |
| 1440 | 2f8e333278 | doc: update Knots port audit | Lőrinc | doc | |
| 1441 | a70d7f712c | test: cover mainnet acceptnonstdtxn option | Lőrinc | test | |
| 1442 | f168b4f1de | doc: update Knots port audit | Lőrinc | doc | |
| 1443 | f606a880b8 | test: cover runtime maxmempool RPC | Lőrinc | test | |
| 1444 | 881b2b7ec7 | doc: update Knots port audit | Lőrinc | doc | |
| 1445 | ebc03c12ec | test: cover listmempooltransactions output | Lőrinc | test | |
| 1446 | 4bf2a4e560 | doc: update Knots port audit | Lőrinc | doc | |
| 1447 | 446ecb8c89 | test: cover deriveaddresses checksum option | Lőrinc | test | |
| 1448 | 246b9b7481 | doc: update Knots port audit | Lőrinc | doc | |
| 1449 | cf4e7c65ac | test: restore keypool isactive coverage setup | Lőrinc | test | |
| 1450 | 23d1463e1e | doc: update Knots port audit | Lőrinc | doc | |
| 1451 | e53013bee9 | test: cover wallet BIP322 message signing | Lőrinc | test | |
| 1452 | 2333eb25bc | doc: update Knots port audit | Lőrinc | doc | |
| 1453 | 3969291399 | test: cover blockfilters peer permission | Lőrinc | test | |
| 1454 | e71086a5e6 | doc: update Knots port audit | Lőrinc | doc | |
| 1455 | 138931c232 | test: cover fee estimator corrupt scale guard | Lőrinc | test | |
| 1456 | adeb8edaf8 | doc: update Knots port audit | Lőrinc | doc | |
| 1457 | 163bd69623 | test: cover subprocess close_fds behavior | Lőrinc | test | |
| 1458 | cd79243cf2 | doc: update Knots port audit | Lőrinc | doc | |
| 1459 | 8648f64a1d | test: cover port mapping disabled without listening | Lőrinc | test | |
| 1460 | 5abf34dc20 | doc: update Knots port audit | Lőrinc | doc | |
| 1461 | bb631a91bf | test: cover empty blockfile info lookup | Lőrinc | test | |
| 1462 | 0ed21fa2cb | doc: update Knots port audit | Lőrinc | doc | |
| 1463 | d7ad0e243c | test: cover signer fingerprint redaction | Lőrinc | test | |
| 1464 | bcc86be0b7 | doc: update Knots port audit | Lőrinc | doc | |
| 1465 | be80b733ae | test: cover preserving replaced rpc cookie | Lőrinc | test | |
| 1466 | ec7ddb3aec | doc: update Knots port audit | Lőrinc | doc | |
| 1467 | 6c28ee36ed | net: escape peer user agent in version log | Lőrinc | core-code | |
| 1468 | 282dbd5ca6 | doc: update Knots port audit | Lőrinc | doc | |
| 1469 | 388194f45c | doc: update Knots port audit | Lőrinc | doc | |
| 1470 | af892ccdf7 | test: cover CJDNS addnode duplicate RPC | Lőrinc | test | |
| 1471 | 569de5726c | doc: update Knots port audit | Lőrinc | doc | |
| 1472 | 4bdcf32c85 | doc: update Knots port audit | Lőrinc | doc | |
| 1473 | dcf4a27daa | test: cover AllocateFileRange preserving bytes | Lőrinc | test | |
| 1474 | ef3863d5e0 | doc: update Knots port audit | Lőrinc | doc | |
| 1475 | dd25eec953 | doc: update Knots port audit | Lőrinc | doc | |
| 1476 | 1047cb0969 | doc: update Knots port audit | Lőrinc | doc | |
| 1477 | 1e03e8cdd9 | doc: clarify pruning impact on wallet sync | Memetic Money | doc | |
| 1478 | 18371c160d | contrib/debian/copyright: Update for 2026 | Luke Dashjr | infra | |
| 1479 | 14fa164846 | build: Workaround incompatibilities with Boost 1.91 | Luke Dashjr | infra | |
| 1480 | 58a0dc0055 | doc: update Knots port audit | Lőrinc | doc | |
| 1481 | 4e8ae4623c | guix: add -Wl,--icf=safe to darwin build | fanquake | infra | |
| 1482 | d27e07ecc9 | doc: update Knots port audit | Lőrinc | doc | |
| 1483 | 4da3be0a2e | test: cover I2P session-create error redaction | Lőrinc | test | |
| 1484 | 164e0a2d62 | doc: update Knots port audit | Lőrinc | doc | |
| 1485 | ac2972bb36 | doc: update Knots port audit | Lőrinc | doc | |
| 1486 | c8b1e694f5 | feat(qt): add /clearhistory command | Kyle Santiago | other | |
| 1487 | 0f98fb5729 | qt: Expand sync progress bar in status bar | SpectrGen | gui | |
| 1488 | ab42494d15 | qt: fix transaction filter modernization rebase | Lőrinc | gui | |
| 1489 | b83cd8cc9c | qt: drop duplicated RPC console text edit class | Lőrinc | gui | |
| 1490 | 7917addda8 | test: cover RPC console clear history command | Lőrinc | test | |
| 1491 | 42456ad2a2 | test: initialize rw config path in init test | Lőrinc | test | |
| 1492 | faad43bef9 | rpc: convert dumptxoutset separator named arg | Lőrinc | core-code | |
| 1493 | 0e0a14535e | test: adapt RBF diagram coverage to txgraph | Lőrinc | test | |
| 1494 | 0885c78c0f | doc: update Knots port audit | Lőrinc | doc | |
| 1495 | ad9e29cd84 | test: cover filtered witness block relay | Lőrinc | test | |
| 1496 | d76634dbe2 | test: cover feefilter disable option | Lőrinc | test | |
| 1497 | 4c8f713d00 | doc: update Knots port audit | Lőrinc | doc | |
| 1498 | 980544801a | rpc: restore scanblocks invalid action error | Lőrinc | core-code | |
| 1499 | 621852143f | doc: update Knots port audit | Lőrinc | doc | |
| 1500 | 7238bd9d5e | rpc: align getblock help fixups | Lőrinc | core-code | |
| 1501 | 62329ff677 | doc: update Knots port audit | Lőrinc | doc | |
| 1502 | 5e926aade2 | test: cover legacy addnode connection type | Lőrinc | test | |
| 1503 | c15478f3c2 | doc: update Knots port audit | Lőrinc | doc | |
| 1504 | dd28d6e98b | doc: update Knots port audit | Lőrinc | doc | |
| 1505 | 8bf7a382d1 | doc: update Knots port audit | Lőrinc | doc | |
| 1506 | dccd41c246 | doc: update Knots port audit | Lőrinc | doc | |
| 1507 | a8074d66fb | test: cover signrawtransactionwithkey fee result | Lőrinc | test | |
| 1508 | d2d5cef6ec | doc: update Knots port audit | Lőrinc | doc | |
| 1509 | 3279019a63 | doc: update Knots port audit | Lőrinc | doc | |
| 1510 | 47ac83819a | doc: update Knots port audit | Lőrinc | doc | |
| 1511 | 7ad059ffce | doc: update Knots port audit | Lőrinc | doc | |
| 1512 | 9df20d0ddb | test: cover walletdir node directory skip | Lőrinc | test | |
| 1513 | 55277c2068 | doc: update Knots port audit | Lőrinc | doc | |
| 1514 | 43e9968c50 | net: guard repeated compact blocktxn fills | Lőrinc | core-code | |
| 1515 | 3bbb0d3976 | doc: update Knots port audit | Lőrinc | doc | |
| 1516 | 5d83e4079d | test: cover BDB final-page LSN check | Lőrinc | test | |
| 1517 | dafb79ea8f | doc: update Knots port audit | Lőrinc | doc | |
| 1518 | a86a1f0cb7 | test: cover automatic outbound whitelist permissions | Lőrinc | test | |
| 1519 | bfc9ccd582 | doc: update Knots port audit | Lőrinc | doc | |
| 1520 | 7daa50bac3 | wallet: handle malformed Coldcard imports | Lőrinc | core-code | |
| 1521 | fe05786755 | doc: update Knots port audit | Lőrinc | doc | |
| 1522 | d428a65be1 | doc: update Knots port audit | Lőrinc | doc | |
| 1523 | 0d8d017d29 | test: cover legacy key metadata flags | Lőrinc | test | |
| 1524 | e5260a6e18 | doc: update Knots port audit | Lőrinc | doc | |
| 1525 | f69ff1f05b | doc: update Knots port audit | Lőrinc | doc | |
| 1526 | 5038f23d0c | test: make sigop maxfeerate check method-isolated | Lőrinc | test | |
| 1527 | b3a8377c39 | doc: update Knots port audit | Lőrinc | doc | |
| 1528 | 6f3bfb1de7 | doc: update Knots port audit | Lőrinc | doc | |
| 1529 | 22d26b0b23 | doc: update Knots port audit | Lőrinc | doc | |
| 1530 | 3e4360372c | test: cover prune lock RPC persistence | Lőrinc | test | |
| 1531 | f17a3f6839 | doc: update Knots port audit | Lőrinc | doc | |
| 1532 | c7c720bf3e | test: cover inbound invalid block punishment | Lőrinc | test | |
| 1533 | f8a4c8c20b | doc: update Knots port audit | Lőrinc | doc | |
| 1534 | d04a0ccbee | doc: update Knots port audit | Lőrinc | doc | |
| 1535 | 3c30452330 | test: cover v2onlyclearnet startup guard | Lőrinc | test | |
| 1536 | 1392b93fd1 | doc: update Knots port audit | Lőrinc | doc | |
| 1537 | 73dd5e5a06 | doc: update Knots port audit | Lőrinc | doc | |
| 1538 | 1450c16e1d | doc: update Knots port audit | Lőrinc | doc | |
| 1539 | 4abc4fbdde | doc: update Knots port audit | Lőrinc | doc | |
| 1540 | 0af61a0bb5 | doc: update Knots port audit | Lőrinc | doc | |
| 1541 | d215264835 | doc: update Knots port audit | Lőrinc | doc | |
| 1542 | 30a7123ea5 | doc: update Knots port audit | Lőrinc | doc | |
| 1543 | dab2afe5a9 | doc: update Knots port audit | Lőrinc | doc | |
| 1544 | 5350fb2ec6 | doc: update Knots port audit | Lőrinc | doc | |
| 1545 | 2409c25d27 | doc: update Knots port audit | Lőrinc | doc | |
| 1546 | e4dc2bc6ec | doc: update Knots port audit | Lőrinc | doc | |
| 1547 | 81fb0daedb | doc: update Knots port audit | Lőrinc | doc | |
| 1548 | 651493373b | doc: update Knots port audit | Lőrinc | doc | |
| 1549 | 42c3a47971 | doc: update Knots port audit | Lőrinc | doc | |
| 1550 | e6812f8f72 | doc: update Knots port audit | Lőrinc | doc | |
| 1551 | e5a3746e6f | doc: update Knots port audit | Lőrinc | doc | |
| 1552 | 99c5a9d1af | doc: update Knots port audit | Lőrinc | doc | |
| 1553 | ff48abcc26 | doc: update Knots port audit | Lőrinc | doc | |
| 1554 | 49d3c8a483 | doc: update Knots port audit | Lőrinc | doc | |
| 1555 | c72cf399ed | doc: update Knots port audit | Lőrinc | doc | |
| 1556 | 5f45459cf2 | doc: update Knots port audit | Lőrinc | doc | |
| 1557 | 569d0a86b4 | test: cover PCP explicit not-authorized warnings | Lőrinc | test | |
| 1558 | 600c0a6a24 | doc: update Knots port audit | Lőrinc | doc | |
| 1559 | f7d90a51b8 | doc: update Knots port audit | Lőrinc | doc | |
| 1560 | 05ef46ca08 | doc: update Knots port audit | Lőrinc | doc | |
| 1561 | 12ba7f0345 | test: avoid RPC worker starvation in shutdown test | Lőrinc | test | |
| 1562 | 68241b7ca0 | doc: update Knots port audit | Lőrinc | doc | |
| 1563 | e124bae81c | doc: update Knots port audit | Lőrinc | doc | |
| 1564 | 2d903fe87d | doc: update Knots port audit | Lőrinc | doc | |
| 1565 | f094b04ff5 | test: cover default blockfilter index selection | Lőrinc | test | |
| 1566 | 7079d07985 | doc: update Knots port audit | Lőrinc | doc | |
| 1567 | b2f29d6393 | test: restore proxy p2p port helper import | Lőrinc | test | |
| 1568 | af9d1c5c66 | doc: update Knots port audit | Lőrinc | doc | |
| 1569 | 221515a6d1 | doc: update Knots port audit | Lőrinc | doc | |
| 1570 | f19e81ca94 | coins: use Knots db batch default | Lőrinc | core-code | |
| 1571 | 083335b1ed | doc: update Knots port audit | Lőrinc | doc | |
| 1572 | ab6406fee3 | doc: update Knots port audit | Lőrinc | doc | |
| 1573 | 7d0c4c6fb0 | streams: uncache buffered files on close | Lőrinc | other | |
| 1574 | 3935884840 | doc: update Knots port audit | Lőrinc | doc | |
| 1575 | e67d53d397 | doc: update Knots port audit | Lőrinc | doc | |
| 1576 | 9561282fca | test: cover wallet-restricted rpc method guards | Lőrinc | test | |
| 1577 | ac06f8d0c1 | doc: update Knots port audit | Lőrinc | doc | |
| 1578 | 28e35851f3 | test: cover spkreuse replacement conflict | Lőrinc | test | |
| 1579 | c96556a7cf | doc: update Knots port audit | Lőrinc | doc | |
| 1580 | f38f998e5a | test: cover network chainparams sync | Lőrinc | test | |
| 1581 | 9ed97fb6ad | doc: update Knots port audit | Lőrinc | doc | |
| 1582 | f0a05af380 | test: cover automatic reindex recovery | Lőrinc | test | |
| 1583 | b6b5cccca5 | doc: update Knots port audit | Lőrinc | doc | |
| 1584 | ccbe187945 | test: cover latest mainnet checkpoint | Lőrinc | test | |
| 1585 | c0251892b6 | doc: update Knots port audit | Lőrinc | doc | |
| 1586 | 0fabdef322 | policy: preserve Knots RBF replacement limit | Lőrinc | core-code | |
| 1587 | e72bc8e123 | doc: update Knots port audit | Lőrinc | doc | |
| 1588 | 261f57d478 | test: cover raw block expected-hash mismatch | Lőrinc | test | |
| 1589 | 5f54e6ab41 | doc: update Knots port audit | Lőrinc | doc | |
| 1590 | 8da5b22044 | test: cover BIP322 sighash-all guard | Lőrinc | test | |
| 1591 | e60cf06c1c | doc: update Knots port audit | Lőrinc | doc | |
| 1592 | 1314e33fc2 | test: restore chain tiebreak disk coverage | Lőrinc | test | |
| 1593 | e54512a795 | doc: update Knots port audit | Lőrinc | doc | |
| 1594 | f401339111 | doc: update Knots port audit | Lőrinc | doc | |
| 1595 | 6f962b8fd1 | doc: update Knots port audit | Lőrinc | doc | |
| 1596 | 20632564e1 | validation: align inline block script error label | Lőrinc | core-code | |
| 1597 | cfaf5d1ddc | doc: update Knots port audit | Lőrinc | doc | |
| 1598 | 6743f685de | doc: update Knots port audit | Lőrinc | doc | |
| 1599 | c7339acb3b | test: align segwit block failure labels | Lőrinc | test | |
| 1600 | 3094c94f5b | doc: update Knots port audit | Lőrinc | doc | |
| 1601 | 48a9a49b78 | doc: update Knots port audit | Lőrinc | doc | |
| 1602 | cc7779ca48 | doc: update Knots port audit | Lőrinc | doc | |
| 1603 | 6b6ed871ce | doc: update Knots port audit | Lőrinc | doc | |
| 1604 | 6e60db6371 | doc: update Knots port audit | Lőrinc | doc | |
| 1605 | 71f5e0879b | doc: update Knots port audit | Lőrinc | doc | |
| 1606 | f51a3998d7 | wallet: preserve spendable anchor selection | Lőrinc | core-code | |
| 1607 | d0f1bf1693 | doc: update Knots port audit | Lőrinc | doc | |
| 1608 | 1572f63b9e | test: adapt mining basic to Knots mining policy | Lőrinc | test | |
| 1609 | b9786490fd | doc: update Knots port audit | Lőrinc | doc | |
| 1610 | b5c63b7ce9 | doc: update Knots port audit | Lőrinc | doc | |
| 1611 | d2309998e4 | doc: update Knots port audit | Lőrinc | doc | |
| 1612 | 935f98e065 | doc: update Knots port audit | Lőrinc | doc | |
| 1613 | d7fcab38cd | doc: update Knots port audit | Lőrinc | doc | |
| 1614 | 0ab501c214 | doc: update Knots port audit | Lőrinc | doc | |
| 1615 | f99d97d42e | mempool: tidy removeForBlock empty-state guard | Lőrinc | core-code | |
| 1616 | a21d22d203 | doc: update Knots port audit | Lőrinc | doc | |
| 1617 | 20ec296add | doc: update Knots port audit | Lőrinc | doc | |
| 1618 | f6bab2c7b7 | doc: update Knots port audit | Lőrinc | doc | |
| 1619 | 7b98951c09 | test: cover block storage space warning units | Lőrinc | test | |
| 1620 | 74d667eeea | doc: update Knots port audit | Lőrinc | doc | |
| 1621 | 1bb6dc2a68 | test: cover low-memory chainstate flushing | Lőrinc | test | |
| 1622 | 0fa13b0d8b | doc: update Knots port audit | Lőrinc | doc | |
| 1623 | 0188a5aae2 | test: cover file advice helpers | Lőrinc | test | |
| 1624 | f247ec94ae | doc: update Knots port audit | Lőrinc | doc | |
| 1625 | 49660e781b | doc: update Knots port audit | Lőrinc | doc | |
| 1626 | 10dd03279b | doc: update Knots port audit | Lőrinc | doc | |
| 1627 | 1f9142a34c | doc: update Knots port audit | Lőrinc | doc | |
| 1628 | ea8d208679 | test: cover RDTS witness expiry | Lőrinc | test | |
| 1629 | 9fa16d992b | doc: update Knots port audit | Lőrinc | doc | |
| 1630 | aa259b2f40 | test: cover Tor local address reachability | Lőrinc | test | |
| 1631 | b29c5aab08 | doc: update Knots port audit | Lőrinc | doc | |
| 1632 | 3b31910af5 | test: cover local-only bloom filter default | Lőrinc | test | |
| 1633 | 01ea496244 | doc: update Knots port audit | Lőrinc | doc | |
| 1634 | d2f8679ec0 | doc: update Knots port audit | Lőrinc | doc | |
| 1635 | 6127a0ce59 | doc: update Knots port audit | Lőrinc | doc | |
| 1636 | a01bd4b168 | doc: update Knots port audit | Lőrinc | doc | |
| 1637 | fcdb84a6ef | test: cover minrelay age policy | Lőrinc | test | |
| 1638 | c186fb189e | doc: update Knots port audit | Lőrinc | doc | |
| 1639 | ceff055aaa | policy: reject negative minrelay age options | Lőrinc | core-code | |
| 1640 | d2ebcf320b | doc: update Knots port audit | Lőrinc | doc | |
| 1641 | 715914f248 | test: cover fee histogram below lowest floor | Lőrinc | test | |
| 1642 | 9d5ec48e69 | doc: update Knots port audit | Lőrinc | doc | |
| 1643 | 8379bbc4d2 | test: cover ForceInbound no-cap behavior | Lőrinc | test | |
| 1644 | 0c23dbeb29 | doc: update Knots port audit | Lőrinc | doc | |
| 1645 | fa51a3ef11 | test: cover compact block witness mutation fallback | Lőrinc | test | |
| 1646 | b5cceb88d8 | doc: update Knots port audit | Lőrinc | doc | |
| 1647 | 2e6d0f0d08 | doc: refresh legacy sigop audit | Lőrinc | doc | |
| 1648 | 7c1abc40b0 | doc: document CVE-2025 validation hardening | Lőrinc | doc | |
| 1649 | f846b3f38b | test: cover change type age preference | Lőrinc | test | |
| 1650 | f8d1421731 | doc: document change type preference guard | Lőrinc | doc | |
| 1651 | 1d0a6a3ad9 | doc: document JSON-RPC version tolerance | Lőrinc | doc | |
| 1652 | 9146e8b04d | doc: document getpeerinfo misbehavior score | Lőrinc | doc | |
| 1653 | c295adfb5c | test: cover default wallet confirm target | Lőrinc | test | |
| 1654 | 71c1da16db | doc: document default wallet confirm target | Lőrinc | doc | |
| 1655 | 33bc0c9c0e | test: cover getblockfrompeer nodeid alias | Lőrinc | test | |
| 1656 | 127881d7dd | doc: document getblockfrompeer nodeid alias | Lőrinc | doc | |
| 1657 | 9338b5915d | doc: document RPC cookie permission compatibility | Lőrinc | doc | |
| 1658 | 9b7a16d738 | doc: document ZMQ IPC URI compatibility | Lőrinc | doc | |
| 1659 | 28fd8514d2 | doc: document legacy wallet creation gap | Lőrinc | doc | |
| 1660 | aa915f80d0 | test: cover maxscriptsize policy | Lőrinc | test | |
| 1661 | 8deb2498f6 | doc: document maxscriptsize policy | Lőrinc | doc | |
| 1662 | eecbb7696f | test: cover reject filter policy | Lőrinc | test | |
| 1663 | 897e14c0e1 | doc: document reject filter policy | Lőrinc | doc | |
| 1664 | f20c3445aa | test: cover bytespersigopstrict policy | Lőrinc | test | |
| 1665 | 441719ecd5 | doc: document bytespersigopstrict policy | Lőrinc | doc | |
| 1666 | 0be9d3b580 | test: cover bare pubkey output policy | Lőrinc | test | |
| 1667 | 056ef6a91e | doc: document bare pubkey policy interaction | Lőrinc | doc | |
| 1668 | 660d32f2c0 | test: cover datacarrier cost accounting | Lőrinc | test | |
| 1669 | 9b8ee858de | doc: document datacarrier cost accounting | Lőrinc | doc | |
| 1670 | db33e4270c | test: cover permitephemeral option modes | Lőrinc | test | |
| 1671 | 5f1c8a7879 | doc: document permitephemeral policy modes | Lőrinc | doc | |
| 1672 | e66399ebc7 | doc: document dynamic dust policy | Lőrinc | doc | |
| 1673 | 647fa736e1 | fix: sync staged modified fees with txgraph | Lőrinc | fix? |  FIX-KNOTS-ONLY: ChangeSet::UpdateModifiedFee syncs sub-dust penalty to txgraph |
| 1674 | 060ee95f8b | doc: document subdust fee penalty | Lőrinc | doc | |
| 1675 | 50edddd6bf | doc: document legacy mempool persistence | Lőrinc | doc | |
| 1676 | db948ffe7d | test: cover corepolicy truc option interaction | Lőrinc | test | |
| 1677 | 797d08e62a | doc: document truc policy modes | Lőrinc | doc | |
| 1678 | ae73f5cf89 | test: cover datacarrier policy modes | Lőrinc | test | |
| 1679 | 64bb4f5fba | doc: document datacarrier policy modes | Lőrinc | doc | |
| 1680 | f2d510ac1b | test: cover unknown witness output policy | Lőrinc | test | |
| 1681 | da52880aa3 | doc: document unknown witness policy | Lőrinc | doc | |
| 1682 | 593056155b | test: cover max orphan transaction limit | Lőrinc | test | |
| 1683 | db5a3e27ab | doc: document max orphan transaction limit | Lőrinc | doc | |
| 1684 | cab0317e6f | test: cover uaspoof boolean mode | Lőrinc | test | |
| 1685 | b16b667cc2 | doc: document user-agent spoof controls | Lőrinc | doc | |
| 1686 | d5d0135764 | test: cover mempool stats RPC | Lőrinc | test | |
| 1687 | 31dbd184ac | doc: document mempool stats RPC | Lőrinc | doc | |
| 1688 | 7ae9aece71 | doc: document wallet ZMQ publishers | Lőrinc | doc | |
| 1689 | 8539447ac4 | test: cover RBF policy option interactions | Lőrinc | test | |
| 1690 | 362b49dffd | doc: document RBF policy options | Lőrinc | doc | |
| 1691 | d31665bf3a | test: cover mutable rw config persistence | Lőrinc | test | |
| 1692 | 504f5e73e1 | doc: document database file size option | Lőrinc | doc | |
| 1693 | 298c075d28 | wallet: restore database flush controls | Lőrinc | core-code | |
| 1694 | 6a21178886 | doc: document wallet database flush controls | Lőrinc | doc | |
| 1695 | 8530e142d3 | doc: refresh addnode inbound crash repro | Lőrinc | doc | |
| 1696 | 70c8ff28f9 | doc: document negated help rejection | Lőrinc | doc | |
| 1697 | 7281de1ba8 | test: cover witness txoutproof duplicate padding | Lőrinc | test | |
| 1698 | aa1278443c | doc: document spenttxouts REST parity | Lőrinc | doc | |
| 1699 | a355966e40 | doc: refresh software expiry GBT evidence | Lőrinc | doc | |
| 1700 | 49c12d8efd | doc: refresh ignore rejects policy evidence | Lőrinc | doc | |
| 1701 | 9dc1392e29 | doc: document Tor subprocess evidence | Lőrinc | doc | |
| 1702 | e9451c077b | doc: refresh RPC bind hardening evidence | Lőrinc | doc | |
| 1703 | a6796c5d89 | doc: refresh peer subversion ordering evidence | Lőrinc | doc | |
| 1704 | 5fcef78c05 | doc: refresh RPC cookie auth evidence | Lőrinc | doc | |
| 1705 | 6b254459af | doc: refresh v2-only clearnet evidence | Lőrinc | doc | |
| 1706 | ffaaf0bc00 | doc: refresh compactblock extra txn evidence | Lőrinc | doc | |
| 1707 | 1f5352709e | doc: refresh max orphan transaction evidence | Lőrinc | doc | |
| 1708 | 2c8f02ce50 | doc: refresh minrelay age evidence | Lőrinc | doc | |
| 1709 | d57b9a3466 | doc: refresh unknown witness policy evidence | Lőrinc | doc | |
| 1710 | 343c008554 | doc: refresh CJDNS addnode evidence | Lőrinc | doc | |
| 1711 | 60a36e6311 | doc: refresh versionbits warning evidence | Lőrinc | doc | |
| 1712 | 1763c2455b | doc: refresh RPC warning evidence | Lőrinc | doc | |
| 1713 | 63f2580dcd | doc: refresh DNS seed evidence | Lőrinc | doc | |
| 1714 | 56b8577672 | doc: refresh user-agent escaping evidence | Lőrinc | doc | |
| 1715 | 0e0fb83a03 | doc: refresh user-agent option evidence | Lőrinc | doc | |
| 1716 | beae681257 | doc: refresh rw config evidence | Lőrinc | doc | |
| 1717 | 39393c14bd | doc: refresh mempool stats evidence | Lőrinc | doc | |
| 1718 | 153b002db7 | doc: refresh low-memory flush evidence | Lőrinc | doc | |
| 1719 | 09b6c9aec0 | doc: refresh buffered file advice evidence | Lőrinc | doc | |
| 1720 | 33663f5a33 | doc: refresh prune lock evidence | Lőrinc | doc | |
| 1721 | bc7cfd6f7f | doc: refresh wallet symlink evidence | Lőrinc | doc | |
| 1722 | 8d84001e00 | doc: refresh walletdir skip evidence | Lőrinc | doc | |
| 1723 | c4e8587577 | doc: refresh wallet cleanup evidence | Lőrinc | doc | |
| 1724 | ca71893c2c | subprocess: close fds in RunCommandParseJSON | Lőrinc | other | |
| 1725 | 651c7f4958 | doc: refresh subprocess close-fds evidence | Lőrinc | doc | |
| 1726 | 5723eaeb6a | doc: refresh port mapping evidence | Lőrinc | doc | |
| 1727 | dcb38a918b | doc: refresh proxy alias evidence | Lőrinc | doc | |
| 1728 | 7415684508 | doc: refresh tor local address evidence | Lőrinc | doc | |
| 1729 | 0d2277bd94 | doc: refresh pcp warning evidence | Lőrinc | doc | |
| 1730 | e3c6985e0b | doc: refresh rpc bind evidence | Lőrinc | doc | |
| 1731 | 63573734ca | doc: refresh invalid block punishment evidence | Lőrinc | doc | |
| 1732 | 1c22764189 | doc: refresh forceinbound evidence | Lőrinc | doc | |
| 1733 | 9f16bf03ee | doc: refresh outbound whitelist evidence | Lőrinc | doc | |
| 1734 | 046784dbba | doc: refresh implicit addr evidence | Lőrinc | doc | |
| 1735 | 862b5ff34d | doc: refresh compact block extra tx evidence | Lőrinc | doc | |
| 1736 | e6a5ab5cb4 | doc: refresh maxorphantx evidence | Lőrinc | doc | |
| 1737 | 3fcd14bbbe | doc: refresh acceptunknownwitness evidence | Lőrinc | doc | |
| 1738 | a58627fc2f | doc: refresh minrelay evidence | Lőrinc | doc | |
| 1739 | d6835a0880 | doc: refresh cjdns addnode evidence | Lőrinc | doc | |
| 1740 | 9ab8150d0f | doc: refresh v2-only clearnet evidence | Lőrinc | doc | |
| 1741 | 010d21f0f3 | doc: refresh versionbits warning evidence | Lőrinc | doc | |
| 1742 | 6e9574c0e4 | doc: refresh rpc warning evidence | Lőrinc | doc | |
| 1743 | 189bc69b83 | doc: refresh dns seed evidence | Lőrinc | doc | |
| 1744 | ca24d69716 | doc: refresh user agent escaping evidence | Lőrinc | doc | |
| 1745 | 3fb246966a | doc: refresh user agent spoof evidence | Lőrinc | doc | |
| 1746 | f42541b350 | doc: refresh rw config evidence | Lőrinc | doc | |
| 1747 | ff2624f04e | doc: refresh mempool stats evidence | Lőrinc | doc | |
| 1748 | 5fa258f8d6 | doc: refresh zmq evidence | Lőrinc | doc | |
| 1749 | 0f3da8bdf4 | doc: refresh fee estimator evidence | Lőrinc | doc | |
| 1750 | 4fee58cf4f | doc: refresh block file info evidence | Lőrinc | doc | |
| 1751 | d59e4d64a5 | doc: refresh fee histogram evidence | Lőrinc | doc | |
| 1752 | 6ad305fe18 | test: refresh segwit-only wallet funding coverage | Lőrinc | test | |
| 1753 | 3009865a91 | doc: refresh bdb evidence | Lőrinc | doc | |
| 1754 | 846e42cb50 | doc: refresh wallet psbt evidence | Lőrinc | doc | |
| 1755 | 1dbddc90f9 | doc: refresh getblockfrompeer evidence | Lőrinc | doc | |
| 1756 | 8c1fc150e7 | doc: refresh rdts consensus evidence | Lőrinc | doc | |
| 1757 | 31530b8723 | doc: refresh invalid block punishment evidence | Lőrinc | doc | |
| 1758 | b3322c1262 | doc: refresh rpc bind evidence | Lőrinc | doc | |
| 1759 | 12b12d7134 | doc: refresh rpc auth evidence | Lőrinc | doc | |
| 1760 | 7ef3cdcc20 | doc: refresh legacy wallet bdb evidence | Lőrinc | doc | |
| 1761 | 3d57208d44 | wallet: restore legacy bdb wallet creation | Lőrinc | core-code | |
| 1762 | 64ac50034d | doc: refresh legacy wallet restore evidence | Lőrinc | doc | |
| 1763 | 955233ff17 | wallet: restore legacy import rpc coverage | Lőrinc | test | |
| 1764 | 0261b82310 | wallet: finish Knots legacy port fixes | Lőrinc | core-code | |
| 1765 | 95d9907f28 | doc: classify inherited Core hardening misses | Lőrinc | doc | |
| 1766 | 94eb7446e6 | test: cover Knots rpc arg alias hardening | Lőrinc | test | |
| 1767 | 16d17ae71a | doc: classify Tor control hardening misses | Lőrinc | doc | |
| 1768 | aeee9e8cfa | doc: classify wallet migration hardening misses | Lőrinc | doc | |
| 1769 | e42f6a29fc | doc: map inherited runtime hardening fixes | Lőrinc | doc | |
| 1770 | 3ee35c208d | doc: map foundational RDTS divergence commits | Lőrinc | doc | |
| 1771 | b40c714602 | wallet: align IsFromMe with Core txo tracking | Lőrinc | core-code | |
| 1772 | 764931a17d | test: cover Knots rpcbind warning visibility | Lőrinc | test | |
| 1773 | 8c0cce7a0a | doc: classify build info provenance hardening | Lőrinc | doc | |
| 1774 | d2090ece58 | doc: map filesystem error helper equivalence | Lőrinc | doc | |
| 1775 | 662e3a2ec1 | doc: map wallet directory scan fix | Lőrinc | doc | |
| 1776 | 01a86e5774 | doc: classify wallet tool warning streams | Lőrinc | doc | |
| 1777 | c782404452 | test: cover exclusive fopen behavior | Lőrinc | test | |
| 1778 | b8c846a6fc | doc: classify getrpcwhitelist surface | Lőrinc | doc | |
| 1779 | 49a60435c2 | test: reconcile Knots fee estimation coverage | Lőrinc | test | |
| 1780 | 3baba049d5 | test: cover BDB overflow parser guards | Lőrinc | test | |
| 1781 | 1cdaf1274c | wallet: restore witness sweepprivkeys support | Lőrinc | core-code | |
| 1782 | b032d31243 | wallet: restore legacy upgrade RPC coverage | Lőrinc | test | |
| 1783 | 69d12d681f | doc: record wallet dump BDB verification | Lőrinc | doc | |
| 1784 | 845a774791 | doc: record wallet funding minconf verification | Lőrinc | doc | |
| 1785 | ffbf21b8c6 | doc: record uptime RPC verification | Lőrinc | doc | |
| 1786 | 90eef9b933 | doc: record RPC auth hashing verification | Lőrinc | doc | |
| 1787 | d62842f7d6 | doc: record Core-equivalent runtime hardening | Lőrinc | doc | |
| 1788 | bb9e912b81 | doc: classify txmempool Boost fallback | Lőrinc | doc | |
| 1789 | 1699696203 | doc: classify wallet migration safety fixes | Lőrinc | doc | |
| 1790 | 8362b0fae4 | doc: classify mining priority audit misses | Lőrinc | doc | |
| 1791 | 5cf91f8b9c | doc: classify RPC example corrections | Lőrinc | doc | |
| 1792 | 5951267122 | doc: classify Knots RPC fuzz coverage | Lőrinc | doc | |
| 1793 | 709dab0c25 | doc: classify mempool fee histogram schema | Lőrinc | doc | |
| 1794 | f4524c3f02 | doc: classify dustdynamic help correction | Lőrinc | doc | |
| 1795 | be7b6e6ae6 | doc: trace GBT blockmaxsize help fix | Lőrinc | doc | |
| 1796 | eb4a480225 | doc: trace software expiry warning | Lőrinc | doc | |
| 1797 | e0dba85b3d | doc: trace LevelDB file size initialization | Lőrinc | doc | |
| 1798 | e6da3fcc7d | doc: trace ellswift overflow hardening | Lőrinc | doc | |
| 1799 | 31493d68b0 | test: restore merkle mutation return invariant | Lőrinc | test | |
| 1800 | 8c3b9c6fe5 | doc: trace BIP322 sighash checker guard | Lőrinc | doc | |
| 1801 | 992816c8a0 | doc: classify DecodeTx and orphan vsize notes | Lőrinc | doc | |
| 1802 | 2f4763408b | doc: trace BDB non-writable directory handling | Lőrinc | doc | |
| 1803 | 3717882f67 | doc: trace wallet load and dbcache hardening | Lőrinc | doc | |
| 1804 | 7feea290cf | doc: trace RPC cookie permission follow-ups | Lőrinc | doc | |
| 1805 | 0ca1322b9a | doc: trace post-IBD coins tip sync | Lőrinc | doc | |
| 1806 | 44a573e970 | doc: trace CJDNS added-node regression | Lőrinc | doc | |
| 1807 | ac74c122e2 | doc: trace noban outbound header regression | Lőrinc | doc | |
| 1808 | e460b6792e | doc: trace wallet min_conf compatibility | Lőrinc | doc | |
| 1809 | b95f98b6aa | doc: trace forced inbound peer reporting | Lőrinc | doc | |
| 1810 | e00ba9e9fd | doc: trace BIP67 multisig sorting | Lőrinc | doc | |
| 1811 | 9d72c91be4 | doc: trace CLI completion format RPC | Lőrinc | doc | |
| 1812 | 3c4f352c3e | doc: trace getgeneralinfo RPC | Lőrinc | doc | |
| 1813 | 648ed97a15 | doc: trace GBT block assembly overrides | Lőrinc | doc | |
| 1814 | e944808038 | doc: trace mempool fee histogram lineage | Lőrinc | doc | |
| 1815 | 5ad4868b4b | doc: trace PSBT legacy change regression | Lőrinc | doc | |
| 1816 | 056e24d4eb | test: cover getwalletinfo mintxfee | Lőrinc | test | |
| 1817 | d0ae5741b5 | doc: trace bumpfee replaceability guard | Lőrinc | doc | |
| 1818 | db3cc6ada7 | doc: trace send anti-fee-sniping convergence | Lőrinc | doc | |
| 1819 | 77afb98fde | doc: trace PSBT bounds assert fix | Lőrinc | doc | |
| 1820 | b4cd630f5c | doc: trace feebumper combined-fee crash fix | Lőrinc | doc | |
| 1821 | 60c84cf8b3 | doc: trace wallet double-disconnect crash fix | Lőrinc | doc | |
| 1822 | 4a1b099a9f | doc: trace v2 reconnect UAF fix | Lőrinc | doc | |
| 1823 | 79c3c2d2bc | test: cover pruneblockchain zero no-op | Lőrinc | test | |
| 1824 | 24dbef244d | doc: trace reachable network RPC ordering | Lőrinc | doc | |
| 1825 | a969348712 | test: cover maxmempool parser cap | Lőrinc | test | |
| 1826 | e2b5c34f71 | doc: trace UTXO snapshot file ownership | Lőrinc | doc | |
| 1827 | 45eebba625 | test: cover rpcuser password warning | Lőrinc | test | |
| 1828 | 9b187678a9 | test: cover sendall unsolvable utxos | Lőrinc | test | |
| 1829 | 10737ad1e3 | doc: trace miner overflow lineage | Lőrinc | doc | |
| 1830 | 8011bf56ef | doc: trace tip wait shutdown wakeups | Lőrinc | doc | |
| 1831 | b1007eccdc | doc: trace getbalance legacy fixes | Lőrinc | doc | |
| 1832 | 72da967be4 | doc: trace long-poll shutdown handling | Lőrinc | doc | |
| 1833 | c643b9be07 | test: cover prune lock height ranges | Lőrinc | test | |
| 1834 | f35dad81a3 | doc: trace HTTP bind socket lineage | Lőrinc | doc | |
| 1835 | 9a2f8b35a7 | doc: trace LevelDB build sanity lineage | Lőrinc | doc | |
| 1836 | c00ed897bb | doc: trace standardness reject reasons | Lőrinc | doc | |
| 1837 | 2229fd7ee7 | doc: trace block location RPC lineage | Lőrinc | doc | |
| 1838 | cd91141f58 | doc: trace addnode connection type crash | Lőrinc | doc | |
| 1839 | 105d8d5687 | doc: trace AutoFile close hardening | Lőrinc | doc | |
| 1840 | 1663cd2e7d | crypto: restore Knots SipHash byte writer | Lőrinc | other | |
| 1841 | 08f4f34ff2 | test: cover forced non-string args | Lőrinc | test | |
| 1842 | ead1b76a19 | test: cover lowmem default disabled | Lőrinc | test | |
| 1843 | 37b2de3d9a | doc: trace v0 block filter support | Lőrinc | doc | |
| 1844 | f2c92e1363 | doc: classify block-times test coverage | Lőrinc | doc | |
| 1845 | 868332742a | qt: restore request payment address-book text | Lőrinc | gui | |
| 1846 | 769b8fd001 | rpc: restore peer starting height reporting | Lőrinc | core-code | |
| 1847 | 78e89c009e | doc: trace rest fee endpoint follow-ups | Lőrinc | doc | |
| 1848 | 5f8322bde3 | validation: restore locked-in versionbits warning | Lőrinc | core-code | |
| 1849 | a422d35031 | doc: trace assumeutxo confirmation status | Lőrinc | doc | |
| 1850 | 70b77ebe16 | doc: trace mining template interfaces | Lőrinc | doc | |
| 1851 | ba69591a3f | doc: trace Knots security reporting policy | Lőrinc | doc | |
| 1852 | aabe6fee4c | test: cover low-height software expiry guard | Lőrinc | test | |
| 1853 | a9c7ec0a2c | doc: trace getblockfrompeer race hardening | Lőrinc | doc | |
| 1854 | 928bd497d8 | doc: trace pruned reorg candidate hardening | Lőrinc | doc | |
| 1855 | 57d7574574 | doc: trace pruned startup unlinked-block hardening | Lőrinc | doc | |
| 1856 | c5e0c38920 | doc: trace settings write-failure hardening | Lőrinc | doc | |
| 1857 | 0b4a083aee | doc: trace HTTP worker race hardening | Lőrinc | doc | |
| 1858 | 2819f733d7 | p2p: restore txref compact extra transactions | Lőrinc | other | |
| 1859 | 6a6917c140 | doc: trace compact extra transaction hardening | Lőrinc | doc | |
| 1860 | 5a2c6ea39b | mempool: restore txref randomized transaction cache | Lőrinc | core-code | |
| 1861 | 9aca5733cd | doc: trace randomized mempool cache hardening | Lőrinc | doc | |
| 1862 | f74169c632 | doc: trace refreshed Core init hardening | Lőrinc | doc | |
| 1863 | 8d08ce1095 | doc: clarify compact block cache audit attribution | Lőrinc | doc | |
| 1864 | b7c94c7afa | doc: record Core topology rebase | Lőrinc | doc | |
| 1865 | fabf333257 | test: restore ismine flag assertions | Lőrinc | test | |
| 1866 | 23ad19c356 | test: restore functional runner coverage | Lőrinc | test | |
| 1867 | 4a71ce500a | test: restore functional framework helpers | Lőrinc | test | |
| 1868 | 955b3697fb | test: restore opportunistic package relay coverage | Lőrinc | test | |
| 1869 | 81152cf477 | test: adapt mempool reorg padding policy | Lőrinc | test | |
| 1870 | 7aec61b582 | test: adapt wallet v3 coverage for Knots TRUC policy | Lőrinc | test | |
| 1871 | 646b23bcc6 | test: adapt orphan handling padding policy | Lőrinc | test | |
| 1872 | 77b1acd166 | test: adapt assumevalid invalid block tolerance | Lőrinc | test | |
| 1873 | 700499ccab | test: reset RPC host for ignored bind warning | Lőrinc | test | |
| 1874 | b9fede67b6 | test: restore 1p1c network minfee setup | Lőrinc | test | |
| 1875 | 02fd42826f | test: restore descriptor import wrappers | Lőrinc | test | |
| 1876 | c069474bfc | test: restore Knots package limit coverage | Lőrinc | test | |
| 1877 | 1ca22d0e09 | test: adapt blocksxor padding policy | Lőrinc | test | |
| 1878 | c742ce53b6 | test: restore alternate mainnet block hashes | Lőrinc | test | |
| 1879 | a641f309d3 | test: adapt reindex init error prefix | Lőrinc | test | |
| 1880 | c4dad02c87 | rpc: preserve private broadcast routing | Lőrinc | core-code | |
| 1881 | 56c648dd82 | test: skip bitcoin wrapper test when unbuilt | Lőrinc | test | |
| 1882 | 93854a6f6a | validation: stage RBF removals before cluster check | Lőrinc | core-code | |
| 1883 | 6cb7ecda8d | test: adapt presegwit init error prefix | Lőrinc | test | |
| 1884 | a800a8d66a | doc: correct compact block port hashes | Lőrinc | doc | |
| 1885 | e3e28d4fb5 | rpc: restore wallet conversion metadata | Lőrinc | core-code | |
| 1886 | a79969d89e | test: adapt testshell wallet warning | Lőrinc | test | |
| 1887 | 3438047cfc | test: cover package RBF replacement entry bound | Lőrinc | test | |
| 1888 | b4d00abc40 | doc: record final verification run | Lőrinc | doc | |
| 1889 | b6887a3f3d | cmake: link Boost headers to affected targets | Hennadii Stepanov | infra | |
| 1890 | b45b08ac25 | bench: update benchmark registrations | MarcoFalke | other | |
| 1891 | f712b19ead | bench: update Knots benchmark inputs | Luke Dashjr | other | |
| 1892 | ab1d246087 | util: preserve existing bytes in fallback allocation | Luke Dashjr | other | |
| 1893 | 15430da76d | wallet: reject anchor outputs during import | Greg Sanders | core-code | |
| 1894 | 0ecc38f7b1 | wallet: let mock database create migration backups | Lőrinc | core-code | |
| 1895 | 38cb951d26 | test: remove invalid wallet-directory case | Lőrinc | test | |
| 1896 | 4c17f33e70 | wallet: normalize paths before listing wallets | Lőrinc | core-code | |
| 1897 | a569e49f45 | test: match signer fingerprint error text | Lőrinc | test | |
| 1898 | 52b20a5e1f | test: solicit compact blocks in extra txn tests | David Gumberg | test | |
| 1899 | 5feafff1d3 | test: use outbound peer for compact block punishment | Luke Dashjr | test | |
| 1900 | dd489cce8d | chainstate: use current Kernel logging options | Luke Dashjr | other | |
| 1901 | a0608fe30b | net: use current port mapping log level | Luke Dashjr | core-code | |
| 1902 | de594d5822 | index: name V0 block filter thread | Luke Dashjr | other | |
| 1903 | 9dc8ee89e2 | common: annotate fixed-point argument access | Luke Dashjr | other | |
| 1904 | 4b9d460092 | common: lock config state updates | Lőrinc | other | |
| 1905 | 23eff0805b | mining: annotate priority delta access | Luke Dashjr | core-code | |
| 1906 | 1294fd8cab | test: annotate TRUC helper mempool access | Luke Dashjr | test | |
| 1907 | 58b54ab036 | test: adapt mempool size-limit setup | Lőrinc | test | |
| 1908 | 3a1d07d172 | test: use I2P for local address filtering | Lőrinc | test | |
