# Review of the last 2000 non-merge commits merged to bitcoin/bitcoin

Range: 075e7f4218 (2026-07) .. 0672e727bf (2025-11-21), 2000 non-merge commits on origin/master.
Method: 58 parallel review agents (50 batches x 40 + 8 gap batches), every commit's diff read;
docs/CI/build/test/GUI commits skimmed, everything in consensus/p2p/coins/script/serialization/
wallet/crypto/rpc/http/kernel paths deep-read. Every non-trivial finding was re-verified against
current origin/master for liveness. Coverage cross-checked: 2000/2000 commits.

## LIVE findings (need an urgent fix PR)

| commit | author | severity | finding |
|---|---|---|---|
| df44afdc9 | Peter Zafonte | 🟡 | kernel C-API: btck_block_tree_entry_get_ancestor guards out-of-range height only with assert(ancestor) — debug abort; release wraps null which the C++ wrapper derefs (bitcoinkernel.cpp:952) |
| eae193e75 | Fabian Jahr | 🟡 | torcontrol: reconnect backoff only applies when Connect() fails — accept-then-close flap causes zero-delay busy reconnect loop burning a core (torcontrol.cpp ThreadControl) |
| d23641564 | Fabian Jahr | 🟡 | rpc: gettxspendingprevout erases from prevouts_to_process vector inside the resolution loop while holding mempool.cs — O(n^2) memmove, minutes-long validation stall via ~8.5MB authenticated request (rpc/mempool.cpp:1026) |
| af0da2fce | David Gumberg | 🟡 | crypto(latent): AES256CBC classes own raw iv pointer with implicit copy ops — double-free if ever copied; no copies today (crypto/aes.h:52) |
| d53852be3 | David Gumberg | 🟡 | crypto(latent): same for AES256Encrypt/Decrypt ctx pointer (crypto/aes.h:23) |
| e0463b4e8 | Sjors Provoost | 🟡 | rpc: getblock/REST hit CHECK_NONFATAL(witness_stack.size()==1) on stored mutated-but-PoW-valid blocks (bad-witness-nonce-size) — spurious 'internal bug', contained (rpc/blockchain.cpp coinbaseTxToJSON) |
| 6244212a5 | Fabian Jahr | 🟡 | init: -asmap=<path> silently ignored when the relative path starts with a digit (GetBoolArg parses it as true → embedded asmap used); hits canonical asmap-data filenames (init.cpp:1611) |

## Found but already fixed on master (recorded, no action)

| commit | status |
|---|---|
| ae5485fa0 | fixed intra-PR by 9a1587251 (EncryptMasterKey div-by-zero when elapsed==0) |
| d511adb66 | fixed by 00d22328b0 (IPC coinbase bad-cb-length at heights ≤16) |
| 14f99cfe5 | fixed by e67a676df9 (uptime RPC 0 on first call) |
| ed945a685 | fixed by f9273f01db (listdescriptors CHECK_NONFATAL on keyless desc) |

## Full table (2000 commits)

| commit | author | area | verdict | issue |
|---|---|---|---|---|
| 075e7f421 | rustaceanrob | net | 🟢 | none — defaulted `AddressPosition::operator==`, correct |
| fa0c8337a | MarcoFalke | test | 🟢 | none (gap-swept) |
| faa50c08b | MarcoFalke | refactor | 🟢 | none (gap-swept) |
| e446ea09c | Hennadii Stepanov | depends, | 🟢 | none (gap-swept) |
| f3f302150 | Hennadii Stepanov | ci | 🟢 | none (gap-swept) |
| a7e980af3 | Hennadii Stepanov | build | 🟢 | none (gap-swept) |
| faada35f9 | MarcoFalke | fuzz | 🟢 | none (gap-swept) |
| cf0f2aeae | Martin Zumsande | p2p | 🟢 | none (gap-swept) |
| fae067ec4 | MarcoFalke | fuzz | 🟢 | none (gap-swept) |
| fd59d68c2 | hebasto | qt/test | 🟢 | none — re-enables macOS minimal-QPA tests after crash fix |
| c8b2aeb22 | hebasto | qt | 🟢 | none — uses `NSApp` + nil check instead of implicit instantiation |
| 51d36dfd0 | Hennadii Stepanov | qt | 🟢 | none (gap-swept) |
| 0b0785daa | fanquake | guix | 🟢 | none (gap-swept) |
| 008a3e29c | fanquake | guix | 🟢 | none (gap-swept) |
| a3b5dc057 | Hennadii Stepanov | cmake | 🟢 | none (gap-swept) |
| 3bfdcbd7e | Lőrinc | coins | 🟢 | none (gap-swept) |
| 2beab9489 | Lőrinc | coins | 🟢 | none (gap-swept) |
| 7ff55cc65 | Lőrinc | bench | 🟢 | none (gap-swept) |
| 3aea85411 | Pieter Wuille | test | 🟢 | none (gap-swept) |
| a0ccd4ad1 | Pieter Wuille | crypto | 🟢 | none (gap-swept) |
| c2d7931b5 | Pieter Wuille | crypto | 🟢 | none (gap-swept) |
| 25bfca06d | Lőrinc | refactor | 🟢 | none (gap-swept) |
| af50ba850 | Lőrinc | test | 🟢 | none (gap-swept) |
| 6ee05c4b1 | bgarcia | test | 🟢 | none — BnB incomplete-result unit test |
| 6aa5d8d94 | Lőrinc | blockencodings | 🟢 | none (gap-swept) |
| be4e64d9e | Lőrinc | test | 🟢 | none (gap-swept) |
| 7298281ba | Anthony Towns | bitcoin-util | 🟢 | none (gap-swept) |
| afab8d422 | hebasto | fuzz | 🟢 | none — removes redundant `Serialize` overload in deserialize.cpp |
| efa7f8c14 | hebasto | fuzz | 🟢 | none — removes unused fuzz helper overload |
| 6eca11175 | will | lint | 🟢 | none — E731, lambdas→local funcs in functional tests |
| b52454538 | will | lint | 🟢 | none — E712, `== False`→`is False`, semantics preserved |
| 240d5f743 | marcofleon | fuzz | 🟢 | none — drops unneeded mutexes; single-threaded-per-process claim holds |
| 226e6388b | Hennadii Stepanov | depends | 🟢 | none (gap-swept) |
| fab8eeed8 | MarcoFalke | fuzz | 🟢 | none — whitespace-only clang-format |
| fa0d777ce | MarcoFalke | fuzz | 🟢 | none — clang-format config for LIMITED_WHILE |
| fa1a9bde5 | MarcoFalke | fuzz | 🟢 | none — drops empty-vector workaround; upstream ubsan fix is in the paired subtree pull |
| fa55385ab | MarcoFalke | fuzz | 🟢 | none — style: LIMITED_WHILE in ipc fuzz |
| 6d5f75392 | MarcoFalke | ipc | 🟢 | none — libmultiprocess subtree: memcpy→ranges::copy ubsan fix + tests + version bump |
| 900a7785b | fanquake | ci | 🟢 | none — Ubuntu 26.04 lint image |
| 058a73a0a | fanquake | ci | 🟢 | none — uv 0.11 pin |
| 14f4ddcb5 | fanquake | ci | 🟢 | none |
| b0e095115 | fanquake | ci | 🟢 | none |
| db35b9238 | Ryan Ofsky | ipc | 🟢 | none |
| a2e4cd7ad | fanquake | depends | 🟢 | none |
| 7508ac319 | Pablo Martin | bench | 🟢 | none |
| 63c5f9d22 | rustaceanrob | test | 🟢 | none |
| 5d57f2cef | woltx | test | 🟢 | none — coins-cache sizing test coverage |
| 1fc9277a1 | bgarcia | test | 🟢 | none — private-broadcast relay=false functional test |
| 9b2b3f4ec | fanquake | doc | 🟢 | none |
| a8223bb4e | pseudoramdom | wallet | 🟢 | none |
| fad5809cb | MarcoFalke | doc | 🟢 | none |
| 629df81e4 | Pieter Wuille | doc | 🟢 | none |
| d164a0434 | Lőrinc | node | 🟢 | none |
| d90806398 | Pieter Wuille | chainparams | 🟢 | none |
| e9ed898a0 | Martin Zumsande | validation | 🟢 | none |
| 3679f1ecf | Martin Zumsande | index | 🟢 | none |
| 65735728a | Martin Zumsande | index | 🟢 | none |
| 09c06960c | Martin Zumsande | validation | 🟢 | none |
| 13c02b546 | Martin Zumsande | test | 🟢 | none |
| cd2a4bc51 | rustaceanrob | test | 🟢 | none |
| c43b7a111 | fanquake | ci | 🟢 | none |
| 699c21aea | fanquake | depends | 🟢 | none |
| 777d23f25 | Pablo Martin | test | 🟢 | none |
| d1e7f8c98 | Pablo Martin | wallet | 🟢 | none |
| ee43743f1 | Pablo Martin | wallet | 🟢 | none |
| 443179a9e | Ava Chow | doc | 🟢 | none |
| 707d0ded8 | Ryan Ofsky | ipc | 🟢 | none |
| fabafd91f | MarcoFalke | refactor | 🟢 | none |
| 6667dc4ee | pzafonte | kernel | 🟢 | none — `btck_transaction_input_get_script_sig` writer-callback API, correct; empty scriptSig safe (CScript prevector data() valid) |
| e6de3a2d3 | pzafonte | kernel | 🟢 | none — witness-stack C-API; `assert(index < size)` matches existing convention and asserts stay on in release |
| d24d3cbad | frankomosh | fuzz | 🟢 | none — new p2p_private_broadcast harness, skimmed |
| fa615bd16 | MarcoFalke | refactor | 🟢 | none |
| 8ac222484 | stickies-v | p2p | 🟢 | none |
| 191bdcba2 | stickies-v | test | 🟢 | none |
| 7ad311be1 | stickies-v | test | 🟢 | none |
| 82a02a2a2 | stickies-v | rpc | 🟢 | none |
| 3ae3a94f2 | Pol Espinasa | wallet | 🟢 | none — fixes prioritisetransaction-induced negative bump-fee-discount assert; guard live on master (`src/wallet/spend.cpp:800`) |
| dab7f2c98 | will | test | 🟢 | none — -externalip/-onlynet functional coverage |
| 657a5aa3f | will | test | 🟢 | none — unit coverage for the same |
| 8c87e32bd | will | net | 🟢 | none — -externalip bypasses -onlynet; scoped to explicit user config only |
| f4af02e82 | will | net | 🟢 | none — plumbs `add_even_if_unreachable` defaulting false |
| 1a3cbf1bd | Lőrinc | net | 🟢 | none |
| 2b6e767d9 | fanquake | doc | 🟢 | none |
| b0735336e | Daniela Brozzoni | p2p | 🟢 | none |
| c1313b199 | ismaelsadeeq | init | 🟢 | none |
| eccb04a32 | Hennadii Stepanov | refactor | 🟢 | none |
| 22ac4ad94 | fanquake | ci | 🟢 | none |
| 495f43f7b | fanquake | ci | 🟢 | none |
| 244739db9 | fanquake | depends | 🟢 | none |
| 6b0a90730 | Ryan Ofsky | ipc | 🟢 | none |
| 2bab6bc73 | Hennadii Stepanov | refactor | 🟢 | none |
| 91b5c8a07 | Hennadii Stepanov | refactor | 🟢 | none |
| 56701ff6d | Hennadii Stepanov | doc | 🟢 | none |
| 22c328d38 | Hennadii Stepanov | refactor | 🟢 | none |
| 6d0ea4cf5 | Andrew Toth | doc | 🟢 | none |
| a2b1c8690 | Andrew Toth | index | 🟢 | none |
| 9f3e42722 | marcofleon | fuzz | 🟢 | none |
| 68cb7840d | Pablo Martin | doc | 🟢 | none |
| cddbad325 | Pablo Martin | doc | 🟢 | none |
| 31abaa264 | will | doc | 🟢 | none |
| a15bdc059 | Pol Espinasa | doc | 🟢 | none |
| a38807640 | Ava Chow | test | 🟢 | none |
| d053e3e5c | Ava Chow | wallet | 🟢 | none |
| 444878efe | Ava Chow | wallet | 🟢 | none |
| f9273f01d | Ava Chow | wallet | 🟢 | none |
| a1c83789a | Ava Chow | wallet | 🟢 | none |
| 1e996640e | Ava Chow | wallet | 🟢 | none |
| d2ee9227d | Ava Chow | descriptor | 🟢 | none |
| fb1d152c2 | fanquake | depends | 🟢 | none |
| 4c9de7d5b | Kyle 🐆 | signer | 🟢 | none — rejects non-8-hex fingerprints from signer enumerate, correct hardening |
| 47bbed052 | fanquake | ci | 🟢 | none |
| 9a25bc398 | fanquake | ci | 🟢 | none |
| 113402286 | Lőrinc | doc | 🟢 | none |
| ce0687828 | Lőrinc | index | 🟢 | none |
| a99148d57 | sedited | test | 🟢 | none |
| 5b4fd284f | sedited | kernel | 🟢 | none |
| 037ad7707 | Enoch Azariah | fuzz | 🟢 | none |
| 91586f701 | Sebastian Falbesoner | test | 🟢 | none |
| 771200ca4 | Sebastian Falbesoner | test | 🟢 | none |
| 4e29de719 | Gregory Sanders | p2p | 🟢 | none |
| cbf8c107c | Greg Sanders | p2p | 🟢 | none |
| 5aea3d037 | Gregory Sanders | p2p | 🟢 | none |
| dc1c17c08 | Andrew Toth | doc | 🟢 | none |
| 0e1093718 | Andrew Toth | fuzz | 🟢 | none |
| ce610a6ff | Andrew Toth | fuzz | 🟢 | none |
| 760fb22dc | Andrew Toth | test | 🟢 | none |
| d69a3b20d | Andrew Toth | doc | 🟢 | none |
| ab2a37923 | Andrew Toth | coins | 🟢 | none |
| fdf283036 | Andrew Toth | coins | 🟢 | none |
| ede11b831 | Andrew Toth | validation | 🟢 | none |
| f82043af5 | Andrew Toth | coins | 🟢 | none |
| 5bf1c3200 | Andrew Toth | validation | 🟢 | none |
| a318f4325 | ekzyis | bitcoin-util | 🟢 | none |
| 256482ab5 | optout | validation | 🟢 | none |
| f595daf1d | Matthew Zipkin | test | 🟢 | none |
| b98b10c07 | Matthew Zipkin | test | 🟢 | none |
| 922b08d37 | Matthew Zipkin | test | 🟢 | none |
| 73da2a8a5 | Matthew Zipkin | http | 🟢 | none |
| d64ea1582 | fanquake | ci | 🟢 | none |
| 5404b6207 | fanquake | depends | 🟢 | none |
| fed3cf6f0 | Ava Chow | wallet | 🟢 | none |
| 4f8823e8e | Ava Chow | wallet | 🟢 | none |
| a0e5e3001 | Hao Xu | fuzz | 🟢 | none |
| 41ceea400 | Hennadii Stepanov | scripted-diff | 🟢 | none |
| f395acdee | Hennadii Stepanov | scripted-diff | 🟢 | none |
| 7ac25c917 | Hennadii Stepanov | util | 🟢 | none |
| bbbbab86a | MarcoFalke | ci | 🟢 | none |
| 095596ddf | Lőrinc | log | 🟢 | none |
| b6b1d0653 | Lőrinc | nanobench | 🟢 | none |
| 829255c8b | Hennadii Stepanov | cmake | 🟢 | none |
| 5c55606da | Hennadii Stepanov | depends | 🟢 | none |
| 402ba10b2 | Hennadii Stepanov | cmake | 🟢 | none |
| 8ebfff0f8 | Sjors Provoost | doc | 🟢 | none |
| fb8a10388 | Hennadii Stepanov | doc | 🟢 | none |
| 5884f5a4f | Sjors Provoost | wallet | 🟢 | none |
| 58560c281 | fanquake | ci | 🟢 | none |
| b36730a3e | TheCharlatan | kernel | 🟢 | none |
| bfdbf513f | TheCharlatan | kernel | 🟢 | none |
| a9a1d92a1 | TheCharlatan | build | 🟢 | none |
| 9e62e4b1f | Pol Espinasa | test | 🟢 | none |
| 9b2ef8175 | Pablo Martin | doc | 🟢 | none |
| a2b0bfcd8 | Ava Chow | wallet | 🟢 | none |
| cb99864c9 | Ava Chow | wallet | 🟢 | none |
| 98d5cdae6 | Ava Chow | wallet | 🟢 | none |
| 7ef8a6efc | Ava Chow | wallet | 🟢 | none |
| 2155e913d | Ava Chow | wallet | 🟢 | none |
| c6ba98dcc | Ava Chow | wallet | 🟢 | none |
| 00abb174a | Ava Chow | wallet | 🟢 | none |
| 1a219a37a | Ava Chow | wallet | 🟢 | none |
| 72db4accb | Lőrinc | coins | 🟢 | none — drops null checks; `CCoinsViewDB::Cursor()` uses make_unique, never null (verified `src/txdb.cpp:240`) |
| 3d2f2d8de | Lőrinc | coins | 🟢 | none — UTXO-stats view by reference; all 5 master cursor sites use `CoinsDB()` directly |
| 35aedb282 | Lőrinc | coins | 🟢 | none — removes base-view `Cursor()`; no remaining callers via base class |
| c6fbe2f66 | Lőrinc | coins | 🟢 | none — narrows cursor users to `CCoinsViewDB*` |
| 3765b428d | Ryan Ofsky | logging | 🟢 | none |
| 703a671fb | Lőrinc | fuzz | 🟢 | none |
| 0868c85fd | Lőrinc | refactor | 🟢 | none |
| 6fa413229 | Hao Xu | fuzz | 🟢 | none |
| 8791c4764 | rkrux | test | 🟢 | none |
| 89ceafafb | rkrux | test | 🟢 | none |
| bbfffcab5 | rkrux | test | 🟢 | none |
| 2ab6e590f | rkrux | test | 🟢 | none |
| 9e2011872 | rkrux | test | 🟢 | none |
| 06af0cddb | rkrux | test | 🟢 | none |
| 4100fac20 | rkrux | test | 🟢 | none |
| ff3f6def9 | rkrux | test | 🟢 | none |
| 003f2a01f | rkrux | test | 🟢 | none |
| f988e6d6e | rkrux | test | 🟢 | none |
| 146b3adfa | fanquake | doc | 🟢 | none |
| 96d7f55f1 | fanquake | vcpkg | 🟢 | none |
| 0443943dc | fanquake | ci | 🟢 | none |
| a0ca249f3 | fanquake | depends | 🟢 | none |
| 35d2d0679 | fanquake | cmake | 🟢 | none |
| 0cdd817a8 | Pol Espinasa | test | 🟢 | none |
| 517d37ce3 | Pol Espinasa | test | 🟢 | none |
| b98dd63da | Pol Espinasa | rpc | 🟢 | none |
| 4acd063ba | Pol Espinasa | wallet | 🟢 | none |
| 1a3cfdf1b | Bruno Garcia | fuzz | 🟢 | none |
| c507fb306 | Bruno Garcia | fuzz | 🟢 | none |
| 4a6fce43e | Bruno Garcia | fuzz | 🟢 | none |
| 48df0939e | marcofleon | fuzz | 🟢 | none |
| a4c3b003f | marcofleon | fuzz | 🟢 | none |
| 336f5a738 | Pol Espinasa | wallet | 🟢 | none |
| 61020b36c | Matthew Zipkin | doc | 🟢 | none |
| 39e9099da | Matthew Zipkin | logging | 🟢 | none |
| 8c1eea077 | Matthew Zipkin | http | 🟢 | none |
| e427c227f | Matthew Zipkin | fuzz | 🟢 | none |
| 21c7542cf | Matthew Zipkin | http | 🟢 | none |
| cbb8d1fb3 | Matthew Zipkin | http | 🟢 | none |
| e5f242eef | Matthew Zipkin | http | 🟢 | none |
| 2ca645c2e | Matthew Zipkin | refactor | 🟢 | none |
| fec6b6bca | Matthew Zipkin | refactor | 🟢 | none |
| f946ff5a0 | Matthew Zipkin | http | 🟢 | none |
| dd11b5e01 | Matthew Zipkin | http | 🟢 | none |
| 7ee7df988 | Matthew Zipkin | http | 🟢 | none |
| 5ef1b80a0 | Matthew Zipkin | http | 🟢 | none |
| a69bb9e1e | Matthew Zipkin | http | 🟢 | none |
| cdf71998e | Matthew Zipkin | http | 🟢 | none |
| 6734bcdef | Matthew Zipkin | http | 🟢 | none |
| 80e1cfe5a | Matthew Zipkin | http | 🟢 | none |
| 3c5226ab9 | Matthew Zipkin | http | 🟢 | none |
| 4ef4ebdc0 | Matthew Zipkin | http | 🟢 | none |
| a85286c5c | Matthew Zipkin | http | 🟢 | none |
| 5a3aa1af2 | Matthew Zipkin | http | 🟢 | none |
| f5bc01894 | Matthew Zipkin | http | 🟢 | none |
| 9463e9878 | Matthew Zipkin | http | 🟢 | none |
| ad50aa4a0 | Matthew Zipkin | http | 🟢 | none |
| 68b5d289d | Matthew Zipkin | http | 🟢 | none |
| 89c54ae4c | Matthew Zipkin | http | 🟢 | none |
| 5aa3629b4 | Matthew Zipkin | util | 🟢 | none |
| 0cdbb191b | Matthew Zipkin | util | 🟢 | none |
| 881d4b6c7 | Matthew Zipkin | test | 🟢 | none |
| 9e6546c51 | Lőrinc | test | 🟢 | none |
| b552f1713 | will | ci | 🟢 | none |
| d2a03d50a | rkrux | test | 🟢 | none |
| afdb37808 | rkrux | test | 🟢 | none |
| 55e3a57f2 | Hodlinator | qa | 🟢 | none |
| 9c5dd2926 | David Gumberg | p2p | 🟢 | none |
| bf9884f4e | David Gumberg | p2p | 🟢 | none |
| 92cea63c7 | David Gumberg | test | 🟢 | none |
| e845e2634 | David Gumberg | test | 🟢 | none |
| 831359171 | David Gumberg | p2p | 🟢 | none |
| 44f377a71 | David Gumberg | refactor | 🟢 | none |
| 25457a327 | David Gumberg | test | 🟢 | none |
| 8f0354995 | fanquake | depends | 🟢 | none |
| fab287426 | MarcoFalke | lint | 🟢 | none |
| 1f3f0a4e2 | fanquake | secp256k1 | 🟢 | none |
| 855a3fee8 | Hao Xu | scripted-diff | 🟢 | none |
| abc33ff04 | brunoerg | test | 🟢 | none |
| 2d0dce0af | brunoerg | net | 🟢 | none |
| f963f2b67 | Pablo Martin | argsman | 🟢 | none |
| 0654511e1 | Shrey | util | 🟢 | none |
| 744d49501 | Max Edwards | ci | 🟢 | none |
| 2ee4fafa3 | kevkevinpal | test | 🟢 | none |
| 08b7c61fc | Vasil Dimov | p2p | 🟢 | none |
| 2a36d6a56 | Hodlinator | lint | 🟢 | none |
| 9a8ef9b0a | Vasil Dimov | test | 🟢 | none |
| eb3208364 | Vasil Dimov | test | 🟢 | none |
| fae482b4e | MarcoFalke | ci | 🟢 | none |
| 32df86f1d | Pablo Martin | argsman | 🟢 | none |
| 75929b11e | woltx | doc | 🟢 | none (gap-swept) |
| ed75d70fd | w0xlt | refactor | 🟢 | none (gap-swept) |
| cbaa1696f | w0xlt | mining | 🟢 | none (gap-swept) |
| 83f3bc002 | w0xlt | mining | 🟢 | none (gap-swept) |
| 4dbaa7cc6 | rkrux | test | 🟢 | none |
| 55a4c946f | brunoerg | test | 🟢 | none |
| d3e40af25 | Lőrinc | index | 🟢 | none |
| d69c46292 | Lőrinc | util | 🟢 | none |
| 41e531c4a | Lőrinc | util | 🟢 | none |
| 58cc2a045 | Hao Xu | test | 🟢 | none |
| 059edf190 | Hennadii Stepanov | guix | 🟢 | none |
| 2d86083fd | Hennadii Stepanov | guix | 🟢 | none |
| d92a20b31 | Hennadii Stepanov | iwyu | 🟢 | none |
| 9fae7e988 | Sebastian Falbesoner | test | 🟢 | none |
| 17353f9d9 | Sjors Provoost | ci | 🟢 | none |
| 406c2348d | stringintech | rpc | 🟢 | none |
| d186c390f | fanquake | depends | 🟢 | none |
| 2447385f4 | Renato Britto | rpc | 🟢 | none |
| 7a8511800 | Renato Britto | rpc | 🟢 | none |
| 88e2a6ae8 | Renato Britto | rpc | 🟢 | none |
| a9f9e7d17 | Renato Britto | rpc | 🟢 | none |
| 8a615a880 | Renato Britto | rpc | 🟢 | none |
| 372ac283a | Renato Britto | rpc | 🟢 | none |
| 0380a1c46 | Renato Britto | rpc | 🟢 | none |
| 946feb3f1 | Lőrinc | test | 🟢 | none |
| b3371029d | kevkevinpal | doc | 🟢 | none |
| 9bfdde74b | fanquake | guix | 🟢 | none |
| 77772e7a3 | MarcoFalke | refactor | 🟢 | none |
| fa45783d5 | MarcoFalke | refactor | 🟢 | none |
| fa4903db8 | MarcoFalke | refactor | 🟢 | none |
| fa1bc1fe5 | MarcoFalke | test | 🟢 | none |
| fa86e5dba | MarcoFalke | refactor | 🟢 | none |
| fa4badc0f | MarcoFalke | refactor | 🟢 | none |
| faad9d643 | MarcoFalke | refactor | 🟢 | none |
| 2fe34808f | Sjors Provoost | wallet | 🟢 | none |
| 394e473d4 | Lőrinc | coins | 🟢 | none |
| aa021b26f | Lőrinc | validation | 🟢 | none |
| bd5a32f7d | woltx | doc | 🟢 | none |
| 7131c8293 | woltx | doc | 🟢 | none |
| 4fdd4d8d2 | woltx | doc | 🟢 | none |
| fab92257f | woltx | doc | 🟢 | none |
| b10889d10 | Lőrinc | coins | 🟢 | none |
| df9eb72b1 | Greg Sanders | test | 🟢 | none |
| 3be1115ad | fanquake | ci | 🟢 | none |
| 2818a171c | Pol Espinasa | test | 🟢 | none |
| bc30e9516 | Pol Espinasa | wallet | 🟢 | none |
| b83a999b1 | Thomas | btcsignals | 🟢 | none |
| d6359937b | stratospher | validation | 🟢 | none |
| 0852925bd | stratospher | test | 🟢 | none |
| ca4a38028 | stratospher | test | 🟢 | none |
| c787b3b99 | stratospher | validation | 🟢 | none |
| 526aae376 | woltx | fuzz | 🟢 | none |
| 472b950b7 | Hodlinator | qa | 🟢 | none |
| 1ce9e2623 | Andrew Toth | fuzz | 🟢 | none |
| ed11dd6a5 | Pol Espinasa | test | 🟢 | none |
| d90d7f0a5 | Pol Espinasa | test | 🟢 | none |
| ad388bf25 | Pol Espinasa | test | 🟢 | none |
| ddceb4e60 | kevkevinpal | test | 🟢 | none |
| 6751a323c | Hennadii Stepanov | iwyu | 🟢 | none |
| fab52281f | MarcoFalke | refactor | 🟢 | none |
| fa4774d03 | MarcoFalke | ci | 🟢 | none |
| fa1414a36 | MarcoFalke | ci | 🟢 | none |
| 359680b74 | Eugene Siegel | net | 🟢 | none |
| fa03852e9 | MarcoFalke | test | 🟢 | none |
| fa3716c43 | MarcoFalke | test | 🟢 | none |
| fae9623c8 | MarcoFalke | test | 🟢 | none |
| a6ed29d6c | Hennadii Stepanov | bench | 🟢 | none |
| 087f02c92 | fanquake | ci | 🟢 | none |
| 6d47f7cc6 | fanquake | ci | 🟢 | none |
| fba713a28 | Hennadii Stepanov | scripted-diff | 🟢 | none |
| 84d07e471 | Pol Espinasa | test | 🟢 | none |
| ec6cf49b9 | sedited | blockstorage | 🟢 | none |
| 35a814a04 | MarcoFalke | test | 🟢 | none |
| 55e402ffe | seduless | scripted-diff | 🟢 | none |
| 1e9546fcf | seduless | test | 🟢 | none |
| 758fea59a | seduless | test | 🟢 | none |
| 7c2ec3949 | seduless | test | 🟢 | none |
| 0bfc5e4ff | Pol Espinasa | test | 🟢 | none |
| fdc9fc1df | Pol Espinasa | test | 🟢 | none |
| 7b821ef9b | Pol Espinasa | rpc | 🟢 | none |
| 5deb053a7 | Sebastian Falbesoner | fuzz | 🟢 | none |
| 19b32a2e1 | Hao Xu | fuzz | 🟢 | none |
| f6bdbcf79 | rustaceanrob | lint | 🟢 | none |
| b2fbd5b5d | fanquake | ci | 🟢 | none |
| 44fc3a290 | Renato Britto | rpc | 🟢 | none |
| 2cf2b22ff | Hennadii Stepanov | depends | 🟢 | none |
| da74ff9ca | Fabian Jahr | test | 🟢 | none |
| 01b8a117d | Anthony Towns | test | 🟢 | none |
| 6a129983c | Anthony Towns | p2p | 🟢 | none |
| 3210fc477 | Anthony Towns | net | 🟢 | none |
| 5b65e3127 | rkrux | test | 🟢 | none |
| 94ed45427 | Anthony Towns | serialize | 🟢 | none |
| 1b3f776eb | Anthony Towns | serialize | 🟢 | none |
| 54de023a7 | fanquake | guix | 🟢 | none |
| f42226d52 | Hodlinator | qa | 🟢 | none |
| 659671ac3 | Hodlinator | qa | 🟢 | none |
| d0b76c7f3 | Hodlinator | rpc | 🟢 | none |
| 43ca54ca0 | Hodlinator | test | 🟢 | none |
| b5e91e946 | Hodlinator | wallet | 🟢 | none |
| 2189a6f5f | codeabysss | p2p | 🟢 | none |
| 82901981b | will | ci | 🟢 | none |
| 4a6d1458b | fanquake | ci | 🟢 | none |
| c21b58e26 | fanquake | ci | 🟢 | none |
| de92208c2 | Ava Chow | wallet | 🟢 | none |
| a54ec373a | Hennadii Stepanov | depends | 🟢 | none |
| 7735c1348 | Michael Dietz | test | 🟢 | none |
| 8cb8653a2 | Andrew Toth | fuzz | 🟢 | none |
| 6609088fe | Andrew Toth | fuzz | 🟢 | none |
| 726e196ef | will | ci | 🟢 | none |
| 4731049ba | Sanjana2906 | build | 🟢 | none |
| c8b8c275f | xyzconstant | test | 🟢 | none |
| 107d4178d | Antoine Poinsot | versionbits | 🟢 | none |
| 94e3ac0b2 | Antoine Poinsot | doc | 🟢 | none |
| 1d5240574 | Antoine Poinsot | qa | 🟢 | none |
| f802edf57 | Anthony Towns | versionbits | 🟢 | none |
| 2ce4ae7d8 | will | ci | 🟢 | none |
| bf0d257c1 | Eugene Siegel | net | 🟢 | none |
| 5a3756d15 | Vasil Dimov | test | 🟢 | none |
| 9c1fcaca5 | rkrux | wallet | 🟢 | none |
| 570a62764 | stringintech | kernel | 🟢 | none |
| ac0926098 | rkrux | test | 🟢 | none |
| ab35a028e | Vasil Dimov | test | 🟢 | none |
| 2333be9cb | Vasil Dimov | test | 🟢 | none |
| 2ffa81fac | Vasil Dimov | test | 🟢 | none |
| 6c525c2ec | rkrux | wallet | 🟢 | none |
| 8877eec72 | rkrux | wallet | 🟢 | none |
| 618394251 | Brandon Odiwuor | iwyu | 🟢 | none |
| 5700a61b7 | fanquake | ci | 🟢 | none |
| 265563bf7 | fanquake | doc | 🟢 | none |
| b84762656 | Sjors Provoost | test | 🟢 | none |
| f4e643cb1 | Sjors Provoost | test | 🟢 | none |
| 280ce6a0a | Sjors Provoost | miner | 🟢 | none |
| 65bd3164f | Sjors Provoost | mining | 🟢 | none |
| 978e7216e | Sjors Provoost | test | 🟢 | none |
| a5859edef | Bruno Garcia | fuzz | 🟢 | none |
| 4b84c9125 | Bruno Garcia | fuzz | 🟢 | none |
| fa787043f | MarcoFalke | doc | 🟢 | none |
| 978481844 | Sjors Provoost | mining | 🟢 | none |
| d282ae688 | Sjors Provoost | mining | 🟢 | none |
| 0d5e4d471 | Sjors Provoost | test | 🟢 | none |
| f16b3613c | Sjors Provoost | ipc | 🟢 | none |
| 0f466e109 | Sjors Provoost | mempool | 🟢 | none |
| b86c1c443 | furszy | test | 🟢 | none |
| fd44d48b2 | furszy | wallet | 🟢 | none |
| fa51f37f1 | MarcoFalke | doc | 🟢 | none |
| 53388773a | Hennadii Stepanov | guix | 🟢 | none |
| 62cf7bc53 | Hennadii Stepanov | guix | 🟢 | none |
| 5d46429e3 | Hennadii Stepanov | guix | 🟢 | none |
| cab65ea9c | Hennadii Stepanov | guix | 🟢 | none |
| faa9d4345 | Hennadii Stepanov | guix | 🟢 | none |
| 6b59fd6b8 | Hennadii Stepanov | guix | 🟢 | none |
| d4c69a722 | Hennadii Stepanov | guix | 🟢 | none |
| fad585b6e | MarcoFalke | test | 🟢 | none |
| faf147551 | MarcoFalke | ci | 🟢 | none |
| fac27d702 | MarcoFalke | test | 🟢 | none |
| fa09de8b6 | MarcoFalke | test | 🟢 | none |
| f701cd159 | rkrux | doc | 🟢 | none |
| 7bc39e3d0 | rkrux | wallet | 🟢 | none |
| 2cbbcb565 | rkrux | wallet | 🟢 | none |
| 307134bd7 | rkrux | wallet | 🟢 | none |
| 3ec550d16 | rkrux | wallet | 🟢 | none |
| a52ea9bff | rkrux | wallet | 🟢 | none |
| a9ac680af | fanquake | build | 🟢 | none |
| 4d58c3271 | fanquake | build | 🟢 | none |
| 58cdb5c2e | fanquake | leveldb | 🟢 | none |
| 4bdd46ace | will | ci | 🟢 | none |
| fab5733f5 | MarcoFalke | doc | 🟢 | none |
| fa98d4495 | MarcoFalke | ci | 🟢 | none |
| faf7e3897 | MarcoFalke | ci | 🟢 | none |
| 42330922d | rkrux | wallet | 🟢 | none |
| 8cb6e405d | rkrux | wallet | 🟢 | none |
| 0ee94b2fe | rkrux | wallet | 🟢 | none |
| 5e833e068 | rkrux | wallet | 🟢 | none |
| a2a2b1745 | rkrux | wallet | 🟢 | none |
| c17cc76a1 | will | test | 🟢 | none |
| fad4f417d | MarcoFalke | test | 🟢 | none |
| 51dd90fb5 | Hodlinator | refactor | 🟢 | none |
| d846444d0 | Hennadii Stepanov | guix | 🟢 | none |
| 0b9e10ad4 | Hennadii Stepanov | guix | 🟢 | none |
| 3962138cc | woltx | test | 🟢 | none |
| 5b60f69e4 | woltx | mining | 🟢 | none |
| 813b4a80d | w0xlt | refactor | 🟢 | none |
| a3fe455a9 | rkrux | wallet | 🟢 | none |
| fa2469381 | MarcoFalke | test | 🟢 | none |
| fa8d4d5c3 | MarcoFalke | test | 🟢 | none |
| faf0f848e | MarcoFalke | test | 🟢 | none |
| faf993ee4 | MarcoFalke | test | 🟢 | none |
| 801e3bfe3 | Antoine Poinsot | chainparams | 🟢 | none |
| 4995c00a9 | Antoine Poinsot | chainparams | 🟢 | none |
| 0774eaaf0 | Hodlinator | util | 🟢 | none |
| 32d072a49 | Vasil Dimov | doc | 🟢 | none |
| d01b461f7 | Vasil Dimov | net | 🟢 | none |
| fd230f942 | Vasil Dimov | net | 🟢 | none |
| a815e3e26 | Hodlinator | rpc | 🟢 | none |
| 7be0d6fa1 | rkrux | test | 🟢 | none |
| 779f44468 | rkrux | test | 🟢 | none |
| 2e9fdcc6d | Guillermo Fernandes | doc | 🟢 | none |
| fa4fc8c1d | MarcoFalke | test | 🟢 | none |
| 5faf2ad88 | rkrux | doc | 🟢 | none |
| aba24a9b6 | rkrux | wallet | 🟢 | none |
| c03107acf | willcl-ark | ci | 🟢 | none |
| 3e8e21b2e | Lőrinc | txgraph | 🟢 | none — copy-not-move primitives in CommitStaging |
| d9f94aa88 | Lőrinc | rpc | 🟢 | none — copy-not-move enum types in RPCArg/RPCResult |
| b67baed4e | Lőrinc | coins | 🟢 | none — `EmplaceCoinInternalDANGER` takes outpoint by const ref; mechanical, all call sites updated |
| 1e5d3b4f0 | Sjors Provoost | doc | 🟢 | none |
| 0317f5202 | Sjors Provoost | ci | 🟢 | none |
| 8c58f6357 | Sjors Provoost | refactor | 🟢 | none |
| 3bb6498fb | Sjors Provoost | mining | 🟢 | none |
| 4637cd157 | Sjors Provoost | mining | 🟢 | none |
| 8daac1d6e | Sjors Provoost | mining | 🟢 | none |
| 128da7c3f | Sjors Provoost | miner | 🟢 | none |
| fa81e51ea | Sjors Provoost | mining | 🟢 | none |
| 020166080 | Sjors Provoost | mining | 🟢 | none |
| 44082bea4 | Sjors Provoost | interfaces | 🟢 | none |
| d4368e059 | Sjors Provoost | move-only | 🟢 | none |
| 6aeb1fbea | Sjors Provoost | test | 🟢 | none |
| 63b23ea1e | Sjors Provoost | test | 🟢 | none |
| 24750f8b3 | Sjors Provoost | test | 🟢 | none |
| 63ee9cd15 | Sjors Provoost | test | 🟢 | none |
| faf02674b | MarcoFalke | refactor | 🟢 | none |
| fae376caf | MarcoFalke | refactor | 🟢 | none |
| fa00c7c7a | MarcoFalke | test | 🟢 | none |
| fa8f25118 | MarcoFalke | refactor | 🟢 | none |
| fa3ae6c7d | MarcoFalke | test | 🟢 | none |
| fa2a3683d | MarcoFalke | test | 🟢 | none |
| 18c1cc65e | stickies-v | kernel | 🟢 | none |
| 7209eb779 | Ryan Ofsky | test | 🟢 | none |
| e4f1e4310 | Hennadii Stepanov | ci | 🟢 | none |
| fa99a3cca | MarcoFalke | ci | 🟢 | none |
| d61053d97 | Fabian Jahr | build | 🟢 | none |
| 798d051c8 | Fabian Jahr | cli | 🟢 | none |
| ca5483a66 | Antoine Poinsot | qa | 🟢 | none |
| 97f7cc023 | rkrux | wallet | 🟢 | none |
| c4a7613e6 | rkrux | wallet | 🟢 | none |
| 131fa570b | optout | test | 🟢 | none |
| f05b1a353 | optout | rpc | 🟢 | none |
| 97d08d62b | Pol Espinasa | refactor | 🟢 | none |
| 98f706c69 | Antoine Poinsot | qa | 🟢 | none |
| ac9aa71b7 | Greg Sanders | mempool | 🟢 | none |
| df7ed5f35 | Antoine Poinsot | chainparams | 🟢 | none |
| b63ef20d5 | Andrew Toth | test | 🟢 | none |
| 32169c385 | Andrew Toth | dbwrapper | 🟢 | none |
| 8d390c93f | Andrew Toth | dbwrapper | 🟢 | none |
| 376e7ef07 | Fabian Jahr | util | 🟢 | none |
| 5d562430d | Fabian Jahr | netbase | 🟢 | none |
| fa37c6a52 | MarcoFalke | test | 🟢 | none |
| a988ac592 | Fabian Jahr | cli | 🟢 | none |
| ae73b69b5 | ismaelsadeeq | test | 🟢 | none |
| 0358c26d4 | w0xlt | kernel | 🟢 | none |
| fa8909888 | MarcoFalke | test | 🟢 | none |
| 88d9bc5aa | yuvicc | kernel | 🟢 | none |
| 8ce84321c | nervana21 | musig | 🟢 | none |
| da769855d | w0xlt | test | 🟢 | none |
| 3f5b3c7a8 | w0xlt | psbt | 🟢 | none |
| 6189335f6 | csjones | kernel | 🟢 | none |
| c471c5085 | Fabian Jahr | common | 🟢 | none |
| 9687ef1bd | Fabian Jahr | ci | 🟢 | none |
| 02b2c4110 | Anthony Towns | logging | 🟢 | none |
| 57d7495fe | Anthony Towns | iwyu | 🟢 | none |
| 611878b46 | Anthony Towns | scripted-diff | 🟢 | none |
| 34332dba2 | Anthony Towns | logging | 🟢 | none |
| abea304dd | Anthony Towns | logging | 🟢 | none |
| 58113e583 | Anthony Towns | logging | 🟢 | none |
| f69d1ae56 | Anthony Towns | logging | 🟢 | none |
| 72e92d67d | Anthony Towns | logging | 🟢 | none |
| faf6afd99 | MarcoFalke | doc | 🟢 | none |
| fa514caad | MarcoFalke | doc | 🟢 | none |
| fa0202f31 | MarcoFalke | doc | 🟢 | none |
| fa37606c6 | MarcoFalke | doc | 🟢 | none |
| fa9c2ddea | MarcoFalke | doc | 🟢 | none |
| a154c05d4 | Hennadii Stepanov | cmake | 🟢 | none |
| 096bb0b5c | 0xb10c | bench | 🟢 | none |
| b6c367044 | takeshikurosawaa | i2p | 🟢 | none |
| 1c500b170 | Vasil Dimov | test | 🟢 | none |
| fa2afba28 | MarcoFalke | p2p | 🟢 | none |
| 3381855e5 | Danny van Heumen | doc | 🟢 | none |
| 3142e5f8c | Hennadii Stepanov | doc | 🟢 | none |
| b71cd5c16 | Hennadii Stepanov | cmake | 🟢 | none |
| fe941938e | Hennadii Stepanov | cmake | 🟢 | none |
| 9a2cced23 | Hennadii Stepanov | cmake | 🟢 | none |
| 004a7e3cf | marcofleon | fuzz | 🟢 | none |
| 7777a92a9 | MarcoFalke | ci | 🟢 | none |
| fac6c4270 | MarcoFalke | ci | 🟢 | none |
| fa3875982 | MarcoFalke | ci | 🟢 | none |
| 90eda67bb | Pol Espinasa | test | 🟢 | none |
| fa3d7ce11 | MarcoFalke | doc | 🟢 | none |
| 2ef6679c2 | Ava Chow | test | 🟢 | none |
| 8544537f4 | Sjors Provoost | mining | 🟢 | none |
| 58eeab790 | Sjors Provoost | mining | 🟢 | none |
| 00d22328b | Sjors Provoost | mining | 🟢 | none |
| 801d36f55 | Eugene Siegel | fuzz | 🟢 | none |
| ca93ab808 | junbyjun1238 | doc | 🟢 | none |
| 605ff3740 | Sjors Provoost | test | 🟢 | none |
| 1966621b7 | Sjors Provoost | test | 🟢 | none |
| 8ba5f68b1 | Sebastian Falbesoner | key | 🟢 | none |
| d087f266f | Sebastian Falbesoner | key | 🟢 | none |
| f36d89f43 | w0xlt | key | 🟢 | none |
| 0065f354a | ImMike | doc | 🟢 | none |
| 81348576c | rkrux | psbt | 🟢 | none |
| fa9c91967 | MarcoFalke | refactor | 🟢 | none |
| cd8d3bd93 | Lőrinc | wallet | 🟢 | none |
| fa9b01ade | MarcoFalke | ci | 🟢 | none |
| 21a1380c1 | Thomas | key | 🟢 | none |
| bb05986c0 | Ava Chow | musig | 🟢 | none |
| 3f44f9aef | marcofleon | test | 🟢 | none |
| 7e19ce200 | b-l-u-e | rpc | 🟢 | none (gap-swept) |
| f24a7b5f7 | Sjors Provoost | doc | 🟢 | none |
| b3a3f8834 | Thomas | crypto | 🟢 | none |
| 4defc466a | Hennadii Stepanov | cmake | 🟢 | none |
| 888857c55 | MarcoFalke | refactor | 🟢 | none |
| d9b57eeca | Philips Ucheolisah .N | doc | 🟢 | none |
| e7d4a7e3f | taboger | doc | 🟢 | none |
| fad61896e | MarcoFalke | ci | 🟢 | none |
| e2b0984f9 | Lőrinc | wallet | 🟢 | none |
| d5adb9d09 | Matthew Zipkin | doc | 🟢 | none |
| fa864b937 | MarcoFalke | rpc | 🟢 | none |
| 9f7a2293c | Hennadii Stepanov | depends | 🟢 | none |
| 086894098 | will | cmake | 🟢 | none |
| 0651a1fc1 | dergoegge | doc | 🟢 | none |
| ac58e6c53 | Lőrinc | test | 🟢 | none |
| 0429c503f | Murch | bench | 🟢 | none |
| ec1eefda7 | Murch | bench | 🟢 | none |
| e6c4ffb95 | Murch | bench | 🟢 | none |
| 1ed799fb2 | Antoine Poinsot | validation | 🟢 | none |
| 371eac806 | frankomosh | fuzz | 🟢 | none |
| 08c3c37d1 | Ryan Ofsky | bitcoin-cli | 🟢 | none |
| 5a2e35921 | Sebastian van Staa | node | 🟢 | none |
| 11c9ef92a | Lőrinc | ci | 🟢 | none |
| 86718e458 | jrakibi | crypto | 🟢 | none |
| bfbf1a7ef | w0xlt | kernel | 🟢 | none |
| d28179bac | Hennadii Stepanov | util | 🟢 | none |
| 11713c9fa | Vasil Dimov | net | 🟢 | none |
| aec4fa2de | Vasil Dimov | net | 🟢 | none |
| 8a739a551 | Enoch Azariah | build | 🟢 | none |
| eed7af666 | David Gumberg | doc | 🟢 | none |
| 3d7f0e4ed | Ava Chow | wallet | 🟢 | none |
| a39cc16b4 | Ava Chow | doc | 🟢 | none |
| 89b9a01b4 | Ava Chow | wallet | 🟢 | none |
| 35bbee637 | Ava Chow | wallet | 🟢 | none |
| f3f8bcbd1 | Ava Chow | wallet | 🟢 | none |
| 9fa4076b2 | w0xlt | test | 🟢 | none |
| 1660c1823 | Ava Chow | doc | 🟢 | none |
| 470e52a5f | Antoine Poinsot | fuzz | 🟢 | none |
| 5bd0579c0 | Ava Chow | test | 🟢 | none |
| b8b6e7f0c | Ava Chow | test | 🟢 | none |
| 0bc1c2e50 | Ava Chow | test | 🟢 | none |
| e0e4dbdeb | Ava Chow | psbt | 🟢 | none |
| bcc1dca77 | Ava Chow | rpc | 🟢 | none |
| ab38c3019 | Ava Chow | psbt | 🟢 | none |
| 93e339e29 | Ava Chow | psbt | 🟢 | none |
| b39c86ae6 | Ava Chow | rpc | 🟢 | none |
| dcc9a3c8d | Ava Chow | rpc | 🟢 | none |
| 5770dbd39 | Ava Chow | psbt | 🟢 | none |
| 863cf47b3 | Ava Chow | test | 🟢 | none |
| 925161eaf | Ava Chow | psbt | 🟢 | none |
| d9cf658ee | Ava Chow | rpc | 🟢 | none |
| 3da0e1601 | Ava Chow | psbt | 🟢 | none |
| c568624ff | Ava Chow | psbt | 🟢 | none |
| 092de4f1f | Ava Chow | psbt | 🟢 | none |
| 1d1ae6f0c | Ava Chow | wallet | 🟢 | none |
| 82bc280de | Ava Chow | test | 🟢 | none |
| 80c29bc6f | Ava Chow | descriptor | 🟢 | none |
| 82c9fe317 | Ava Chow | psbt | 🟢 | none |
| 95897507e | Ava Chow | psbt | 🟢 | none |
| 1b7d323a7 | Ava Chow | psbt | 🟢 | none |
| 543d3e1cd | Ava Chow | psbt | 🟢 | none |
| c01c7f068 | Ava Chow | psbt | 🟢 | none |
| 9671aa08c | Ava Chow | psbt | 🟢 | none |
| 990b084f1 | Ava Chow | psbt | 🟢 | none |
| 7eacc21ff | Ava Chow | psbt | 🟢 | none |
| f926c326b | Ava Chow | gui | 🟢 | none |
| 1e2d146b4 | Ava Chow | psbt | 🟢 | none |
| 88384180d | Ava Chow | test | 🟢 | none |
| 001877500 | David Gumberg | test | 🟢 | none |
| 0cb884e6d | Ava Chow | psbt | 🟢 | none |
| 57820c472 | Ava Chow | bench | 🟢 | none |
| 9a7604fd2 | Ava Chow | bench | 🟢 | none |
| 426a94e7b | Ava Chow | bench | 🟢 | none |
| d672455d2 | Ava Chow | bench | 🟢 | none |
| 61412ef88 | Ava Chow | bench | 🟢 | none |
| 451fdd26a | David Gumberg | wallet | 🟢 | none |
| 32946e029 | Ava Chow | wallet | 🟢 | none |
| e20aaff70 | Ava Chow | wallet | 🟢 | none |
| aa4f7823a | Ava Chow | wallet | 🟢 | none |
| 6538f6913 | Ava Chow | fuzz | 🟢 | none |
| 8be5ee554 | David Gumberg | test | 🟢 | none |
| 80b0c2599 | Ava Chow | wallet | 🟢 | none |
| f713fd172 | David Gumberg | wallet | 🟢 | none |
| cd912c4e1 | Ava Chow | wallet | 🟢 | none |
| 0301c758e | Ava Chow | wallet | 🟢 | none |
| 2424e5283 | will | lint | 🟢 | none |
| 5fefa5a65 | Sjors Provoost | ci | 🟢 | none |
| fd15b55c2 | will | lint | 🟢 | none |
| 5f4d3383d | will | lint | 🟢 | none |
| a53b81ce4 | will | lint | 🟢 | none |
| 2b0dc0d22 | Ava Chow | wallet | 🟢 | none |
| d084bc88b | Lőrinc | doc | 🟢 | none |
| 7c7cec456 | Lőrinc | ci | 🟢 | none |
| 75cf9708a | Vasil Dimov | ci | 🟢 | none |
| 1b93983bf | Vasil Dimov | test | 🟢 | none |
| 0bc9d354d | Cory Fields | multi_index | 🟢 | none (fixes boost ≥1.91 build) |
| eab72d14d | Sjors Provoost | sign | 🟢 | none (refactor, SignOptions struct) |
| 5ed41752c | Sjors Provoost | sign | 🟢 | none (refactor) |
| dc4a5d127 | Sjors Provoost | sign | 🟢 | none (refactor) |
| 032223f40 | Lőrinc | dbwrapper | 🟢 | none |
| 7403c0f90 | Lőrinc | dbwrapper | 🟢 | none |
| cb1ab0a71 | Lőrinc | test | 🟢 | none |
| 31ce729b2 | Lőrinc | streams | 🟢 | none |
| 0e4b0bace | marcofleon | validation | 🟢 | none |
| c8d688f41 | Eugene Siegel | fuzz | 🟢 | none |
| d0333bfe9 | Eugene Siegel | fuzz | 🟢 | none |
| 3c58efe2a | Eugene Siegel | fuzz | 🟢 | none |
| 651622432 | Eugene Siegel | fuzz | 🟢 | none |
| 8c9a3fd0e | Eugene Siegel | net | 🟢 | none |
| 6cd480f62 | Eugene Siegel | fuzz | 🟢 | none |
| 1d6696374 | ViniciusCestarii | log | 🟢 | none |
| a9301cfa0 | Sjors Provoost | refactor | 🟢 | none |
| 47d68cd98 | Sjors Provoost | ci | 🟢 | none |
| e2ef54b8b | Hennadii Stepanov | cmake | 🟢 | none |
| 6d86184a8 | Adam Andrews | rpc | 🟢 | none |
| 08925d5ee | furszy | test | 🟢 | none |
| 0218966c0 | furszy | test | 🟢 | none |
| bc0090f1d | furszy | wallet | 🟢 | none (fixes crash on non-writable db dir) |
| ef21e2929 | Max Edwards | doc | 🟢 | none |
| fa43da21f | MarcoFalke | util | 🟢 | none (u64 math can't overflow realistically) |
| fa5801762 | MarcoFalke | util | 🟢 | none (_MiB/_GiB now u64, consteval; call sites safe) |
| a1e534bbf | rkrux | doc | 🟢 | none |
| a49bc1e24 | Pol Espinasa | ci | 🟢 | none |
| fa204100e | MarcoFalke | streams | 🟢 | none (size() replaces truncating in_avail()) |
| fa5ab0220 | MarcoFalke | net | 🟢 | none (verified move-only) |
| 8deed0df0 | rkrux | doc | 🟢 | none |
| 1a85ca1df | rkrux | rpc | 🟢 | none (rpcdeprecate, opt-out via -deprecatedrpc) |
| f89d18c3b | rkrux | rpc | 🟢 | none (rpcdeprecate) |
| e6430b277 | Lőrinc | bench | 🟢 | none |
| ba0078e3b | Lőrinc | bench | 🟢 | none |
| b8b7f896e | Lőrinc | bench | 🟢 | none |
| 904c0d07b | Anthony Towns | util | 🟢 | none |
| 735b25519 | Sjors Provoost | support | 🟢 | none |
| 8ab4b9fc8 | Sjors Provoost | init | 🟢 | none |
| 4afbabdce | Sjors Provoost | init | 🟢 | none |
| fa9ddb01c | MarcoFalke | test | 🟢 | none |
| 14547eb48 | Thomas | kernel | 🟢 | none (fixes Handle self-move double-free) |
| c95968f78 | fanquake | doc | 🟢 | none |
| 89af67d79 | Anthony Towns | test | 🟢 | none |
| 92df78585 | Anthony Towns | test | 🟢 | none |
| 33c8090be | Anthony Towns | argsman | 🟢 | none |
| 186354a0d | Anthony Towns | bitcoin-wallet | 🟢 | none |
| d21e82b7d | Anthony Towns | argsman | 🟢 | none |
| 4abc0c2e0 | Antoine Poinsot | doc | 🟢 | none |
| 2a90b6132 | Fabian Jahr | doc | 🟢 | none |
| 8cb2d926b | Fabian Jahr | rpc | 🟢 | none (exportasmap: authenticated RPC file write, same precedent as dumptxoutset) |
| 13c8df4d5 | Lőrinc | bench | 🟢 | none |
| 2529f2555 | Lőrinc | streams | 🟢 | none |
| b8eb6c208 | Lőrinc | bench | 🟢 | none |
| 61d678a6e | Lőrinc | streams | 🟢 | none |
| 5de2f97a0 | Lőrinc | dbwrapper | 🟢 | none (SpanReader reads borrowed key bytes in place; no lifetime issue) |
| f0e498af5 | Lőrinc | test | 🟢 | none |
| dfe5d6a81 | frankomosh | fuzz | 🟢 | none |
| c9d858223 | Bruno Garcia | fuzz | 🟢 | none |
| dc84a3101 | rkrux | wallet | 🟢 | none |
| 94845df07 | rkrux | wallet | 🟢 | none |
| 6e796e1f4 | rkrux | wallet | 🟢 | none |
| 54e4c0be8 | rkrux | wallet | 🟢 | none |
| af0ee28eb | Lőrinc | util | 🟢 | none |
| b3edd30aa | Lőrinc | util | 🟢 | none |
| 7c75244ad | optout | validation | 🟢 | none |
| c5eb283bc | optout | chain | 🟢 | none |
| 20b58e281 | optout | chain | 🟢 | none |
| fe2d6e25e | optout | chain | 🟢 | none |
| db56bcd69 | optout | test | 🟢 | none |
| 8333abdd9 | optout | test | 🟢 | none |
| 3bf3b6d59 | takeshikurosawaa | net | 🟢 | none |
| b2debc927 | takeshikurosawaa | net | 🟢 | none (itself a fix: stops logging SOCKS5 credentials) |
| 9fe5896a4 | David Gumberg | tor | 🟢 | none (itself a fix: torcontrol line-count OOM) |
| 8b68287bf | David Gumberg | test | 🟢 | none |
| c5ec2d531 | stickies-v | logging | 🟢 | none |
| 3b92ec203 | stickies-v | logging | 🟢 | none |
| 07b9b13b4 | Alexander Wiederin | doc | 🟢 | none |
| f49a2afd9 | Matthew Zipkin | test | 🟢 | none |
| ba6287a44 | Alexander Wiederin | kernel | 🟢 | none |
| df44afdc9 | Peter Zafonte | - | 🟡 | LIVE: kernel C-API: btck_block_tree_entry_get_ancestor guards out-of-range height only with assert(ancestor) — debug abort; release wraps null which the C++ wrapper derefs (bitcoinkernel.cpp:952) |
| 8115001cd | stickies-v | logging | 🟢 | none |
| b414913c7 | stickies-v | logging | 🟢 | none |
| 8a55b1775 | stickies-v | util | 🟢 | none |
| b02d6b056 | fanquake | ci | 🟢 | none |
| 655a39ee1 | fanquake | ci | 🟢 | none |
| bdc8e496d | Guillermo Fernandes | doc | 🟢 | none |
| ab5889796 | David Gumberg | tor | 🟢 | none |
| b2de59d48 | Ava Chow | wallet | 🟢 | none |
| dc3a2b9c3 | Ava Chow | wallet | 🟢 | none |
| fa16bc53d | MarcoFalke | test | 🟢 | none |
| fab352053 | MarcoFalke | test | 🟢 | none |
| fad6deb3c | MarcoFalke | test | 🟢 | none |
| fa5eb74b9 | MarcoFalke | test | 🟢 | none |
| c54f37c1b | Torkel Rogstad | rpc/cli | 🟢 | none |
| 4d040b7d6 | fanquake | doc | 🟢 | none |
| fa02eb87d | MarcoFalke | test | 🟢 | none |
| 49895b9cb | Sebastian Falbesoner | build | 🟢 | none |
| 577a3e74c | yuvicc | test | 🟢 | none |
| 1ad551281 | yuvicc | kernel | 🟢 | none |
| 86662623e | yuvicc | streams | 🟢 | none |
| fbffe8a64 | David Gumberg | bench | 🟢 | none |
| 616ee6fe7 | Sebastian Falbesoner | bench | 🟢 | none |
| 6ac49373a | optout | test | 🟢 | none |
| 09c0e3778 | Hennadii Stepanov | ci | 🟢 | none |
| fa1015bbc | MarcoFalke | net | 🟢 | none |
| ea893cff0 | ArvinFarrelP | doc | 🟢 | none |
| 701bc2dc0 | Torkel Rogstad | contrib | 🟢 | none |
| 7249b376a | Murch | wallet | 🟢 | none |
| 520429186 | Murch | wallet | 🟢 | none |
| ba1807b98 | Murch | coinselection | 🟢 | none |
| fa226ab90 | Murch | coinselection | 🟢 | none |
| 7ecea1dc5 | Murch | coinselection | 🟢 | none |
| 3ca0f3616 | Murch | coinselection | 🟢 | none |
| 2e7373983 | Murch | coinselection | 🟢 | none |
| eff9e798b | Murch | coinselection | 🟢 | none |
| dc9309108 | Hennadii Stepanov | ci | 🟢 | none |
| 88bbf2ad3 | Hennadii Stepanov | ci | 🟢 | none |
| 1ed1a1240 | Vasil Dimov | net_processing | 🟢 | none (itself a privacy improvement) |
| dfd54c959 | fanquake | subtree | 🟢 | none (secp256k1 subtree sync 57315a6985..7262adb4b4) |
| 8783cc805 | Lőrinc | coins | 🟢 | none |
| 86296f276 | Lőrinc | coins | 🟢 | none |
| b637566c8 | Lőrinc | coins | 🟢 | none |
| 90c635c01 | Lőrinc | fuzz | 🟢 | none |
| a9f92e349 | Lőrinc | coins | 🟢 | none |
| 38a99f334 | Lőrinc | coins | 🟢 | none |
| 06172ef0d | Lőrinc | coins | 🟢 | none |
| 0e712b381 | Matthew Zipkin | test-util | 🟢 | none |
| 3de02abf3 | Matthew Zipkin | util | 🟢 | none |
| b0ca40061 | Matthew Zipkin | util | 🟢 | none |
| 817209929 | Matthew Zipkin | util | 🟢 | none |
| ba01b00d4 | stratospher | validation | 🟢 | none |
| aa0eef735 | stratospher | test | 🟢 | none |
| 1b0b3e2c2 | stratospher | validation | 🟢 | none (invariant verified on master) |
| d06dabf26 | Sebastian van Staa | node | 🟢 | none |
| c74c6cfd8 | Hennadii Stepanov | ci | 🟢 | none |
| 858a0a9c9 | Murch | test | 🟢 | none |
| fe9f53bf0 | Murch | test | 🟢 | none |
| 2840f041c | Murch | test | 🟢 | none |
| 64ab97466 | Murch | test | 🟢 | none |
| 65900f8dc | Murch | test | 🟢 | none |
| b555a0b78 | Alfonso Roman Zubeldia | test | 🟢 | none |
| 2af003ae3 | Hodlinator | test | 🟢 | none |
| 9ee77701d | Hodlinator | test | 🟢 | none |
| 7a6d21098 | Sjors Provoost | subtree | 🟢 | none (libmultiprocess sync) |
| 422ca211e | Matthew Zipkin | test | 🟢 | none |
| 3fd68a95e | Lőrinc | test | 🟢 | none |
| 301b1d7b1 | Lőrinc | test | 🟢 | none |
| 06a4176c4 | Lőrinc | test | 🟢 | none |
| d9a3cf20a | Lőrinc | test | 🟢 | none |
| 23c06d4e6 | Lőrinc | test | 🟢 | none |
| dcd90fbe5 | Lőrinc | test | 🟢 | none |
| 4f4516e3f | Lőrinc | test | 🟢 | none |
| 76a5570b3 | Lőrinc | test | 🟢 | none |
| 0c1a07e89 | Matthew Zipkin | test | 🟢 | none |
| f06de5c1e | Matthew Zipkin | test | 🟢 | none |
| 1950da94f | Lőrinc | test | 🟢 | none |
| 7236a0550 | Lőrinc | test | 🟢 | none |
| 19e99be01 | fanquake | guix | 🟢 | none |
| 47b7a9f66 | fanquake | guix | 🟢 | none |
| 851152e42 | ismaelsadeeq | validation | 🟢 | none |
| 1401011f7 | Fabian Jahr | test | 🟢 | none |
| 84c1f3207 | Fabian Jahr | test | 🟢 | none |
| 7dff9ec29 | Fabian Jahr | test | 🟢 | none |
| 569383356 | Fabian Jahr | test | 🟢 | none |
| 4117b92e6 | Fabian Jahr | tor/fuzz | 🟢 | none (itself a fix: empty AUTHCHALLENGE reply deref) |
| b1869e9a2 | Fabian Jahr | tor | 🟢 | none |
| eae193e75 | Fabian Jahr | - | 🟡 | LIVE: torcontrol: reconnect backoff only applies when Connect() fails — accept-then-close flap causes zero-delay busy reconnect loop burning a core (torcontrol.cpp ThreadControl) |
| 242b0ebb5 | Cory Fields | signals | 🟢 | none |
| b12f43a0a | Cory Fields | depends | 🟢 | none |
| a4b160798 | Cory Fields | lint/doc | 🟢 | none |
| 375397ebd | Cory Fields | build | 🟢 | none |
| 091736a15 | Cory Fields | signals | 🟢 | none |
| 9958f4fe4 | Cory Fields | build | 🟢 | none |
| 34eabd77a | Cory Fields | signals | 🟢 | none |
| e60a0b9a2 | Cory Fields | signals | 🟢 | none |
| 63c68e2a3 | Cory Fields | test | 🟢 | none |
| d517fa0a9 | Eugene Siegel | rpc | 🟢 | none (itself a fix: init-order fiasco) |
| f1e14dfbe | fanquake | depends | 🟢 | none |
| 5603ae0ff | Pol Espinasa | test | 🟢 | none |
| fc736013a | Fabian Jahr | rpc | 🟢 | none |
| d0fd71894 | Fabian Jahr | test | 🟢 | none |
| ab9463efa | Fabian Jahr | test | 🟢 | none |
| 49d5e835a | Fabian Jahr | rpc | 🟢 | none |
| fe58eb985 | Fabian Jahr | blockstorage | 🟢 | none |
| 037ea2c71 | Ava Chow | wallet | 🟢 | none |
| 59484e2fd | Ava Chow | wallet | 🟢 | none |
| b69f989dc | Ava Chow | bench | 🟢 | none |
| e7d67c9fd | Ava Chow | test | 🟢 | none |
| 964eafb71 | Ava Chow | bench | 🟢 | none |
| facaeb9c7 | MarcoFalke | doc | 🟢 | none |
| fc9987dfc | w0xlt | doc | 🟢 | none |
| 7abf6f6fb | Alexander Wiederin | doc | 🟢 | none |
| 91cd0e3aa | Bruno Garcia | fuzz | 🟢 | none |
| 75608547b | Alexander Wiederin | kernel | 🟢 | none |
| fa955af61 | MarcoFalke | lint | 🟢 | none |
| fedeff7f2 | deadmanoz | crypto | 🟢 | none |
| 257769a7c | Hodlinator | test | 🟢 | none |
| 20a94c152 | Hodlinator | cli/rpc | 🟢 | none |
| 84c3f8d32 | Hodlinator | rpc | 🟢 | none |
| fa244b984 | MarcoFalke | net | 🟢 | none |
| fa2605b20 | MarcoFalke | net | 🟢 | none |
| 12c3c3f81 | Sebastian Falbesoner | test | 🟢 | none |
| 8b49e2dd4 | Hennadii Stepanov | build | 🟢 | none |
| 6953363be | Hennadii Stepanov | build | 🟢 | none |
| eb750d277 | Hennadii Stepanov | build | 🟢 | none |
| 5fa689881 | ismaelsadeeq | policy | 🟢 | none (comment only) |
| 3aeccb7d7 | Hennadii Stepanov | depends | 🟢 | none |
| 325f743ee | Hennadii Stepanov | guix | 🟢 | none |
| fabab69e9 | MarcoFalke | univalue | 🟢 | none |
| fa0a09441 | MarcoFalke | wallet/rpc | 🟢 | none |
| fafb0c4cb | MarcoFalke | logging | 🟢 | none |
| 890a09b1e | Ava Chow | fuzz | 🟢 | none |
| fae807ed2 | MarcoFalke | test | 🟢 | none |
| fab277264 | MarcoFalke | test | 🟢 | none |
| fa21edddb | MarcoFalke | test | 🟢 | none |
| faa404e11 | MarcoFalke | test | 🟢 | none |
| 2478a15ef | Ryan Ofsky | subtree | 🟢 | none (libmultiprocess sync) |
| fa644e625 | MarcoFalke | net | 🟢 | none |
| 333316f6b | MarcoFalke | doc | 🟢 | none |
| fa54fb012 | MarcoFalke | gui | 🟢 | none |
| fab88884b | MarcoFalke | net | 🟢 | none |
| facfce37f | MarcoFalke | util | 🟢 | none |
| fa41e072b | MarcoFalke | net | 🟢 | none |
| 261d22945 | Hodlinator | test | 🟢 | none |
| 3dcdb2b9b | David Gumberg | test | 🟢 | none |
| 6664e41e5 | David Gumberg | test | 🟢 | none |
| d28c98924 | David Gumberg | test | 🟢 | none |
| fa1ebde1a | MarcoFalke | fuzz | 🟢 | none |
| 325afe664 | Mccalabrese | net | 🟢 | none |
| 1438165b1 | Sjors Provoost | wallet | 🟢 | none (comment removal) |
| fabbfec3b | MarcoFalke | fuzz | 🟢 | none |
| f89967463 | Bruno Garcia | test | 🟢 | none |
| 758f208cc | Sjors Provoost | contrib | 🟢 | none |
| 5a81d73a8 | Anthony Towns | rpc | 🟢 | none (scripted) |
| 4e789299a | Anthony Towns | rpc | 🟢 | none (scripted rename) |
| 3e089038a | fanquake | doc | 🟢 | none |
| 0d1301b47 | David Gumberg | test | 🟢 | none |
| 8bfb422de | David Gumberg | test | 🟢 | none |
| a7e4a59d6 | David Gumberg | test | 🟢 | none |
| 6b99a3e4f | w0xlt | doc | 🟢 | none |
| 0587c5609 | w0xlt | kernel | 🟢 | none |
| 3129d4a69 | Hennadii Stepanov | ci | 🟢 | none |
| 71f827c3c | w0xlt | kernel | 🟢 | none |
| 999d18ab1 | Mccalabrese | net | 🟢 | none |
| 25f69d970 | Pol Espinasa | doc | 🟢 | none |
| af629821c | Pol Espinasa | test | 🟢 | none |
| a3d6f32a3 | Pol Espinasa | rpc | 🟢 | none |
| 5b2e4c4a8 | Pol Espinasa | validation | 🟢 | none |
| 3e5dc6103 | rkrux | rpc | 🟢 | none |
| fa73ed467 | MarcoFalke | net | 🟢 | none |
| 65379bb8d | fanquake | ci | 🟢 | none |
| f44191f16 | fanquake | depends | 🟢 | none |
| 7f7018738 | fanquake | depends | 🟢 | none |
| 6464f1408 | fanquake | depends | 🟢 | none |
| 8444efbd4 | Fabian Jahr | tor | 🟢 | none |
| 6bcb60354 | Fabian Jahr | tor | 🟢 | none |
| a36591d19 | Fabian Jahr | tor | 🟢 | none |
| 8d2f06853 | Anthony Towns | sync | 🟢 | none |
| cbc231ed8 | Anthony Towns | logging | 🟢 | none (scripted) |
| f808786f4 | Anthony Towns | logging | 🟢 | none |
| e196cf26e | Anthony Towns | util | 🟢 | none |
| 0b489886f | Hennadii Stepanov | ci | 🟢 | none |
| 0026b330c | furszy | wallet | 🟢 | none (itself a fix: amount computed as boolean) |
| 313655992 | HouseOfHufflepuff | doc | 🟢 | none |
| 2d5cedfe1 | Hennadii Stepanov | ci | 🟢 | none |
| ad75b147b | Enoch Azariah | test | 🟢 | none |
| e7a918b69 | Enoch Azariah | test | 🟢 | none |
| 63684d692 | Enoch Azariah | test | 🟢 | none |
| 4ada575d6 | Enoch Azariah | test | 🟢 | none |
| 0fe6fccec | Hennadii Stepanov | doc | 🟢 | none |
| cfa3b10d5 | Hennadii Stepanov | iwyu | 🟢 | none (pragma comment) |
| 015bea05e | Hennadii Stepanov | iwyu | 🟢 | none (pragma comments) |
| 48bfcfede | Hennadii Stepanov | iwyu | 🟢 | none (pragma comments) |
| 179abb387 | Hennadii Stepanov | sync | 🟢 | none (header move-only) |
| 9aa5b3c3a | Hennadii Stepanov | ci | 🟢 | none |
| 65882fa68 | Hennadii Stepanov | ci | 🟢 | none |
| 7a9304f88 | Hennadii Stepanov | depends | 🟢 | none |
| faad08e59 | MarcoFalke | test | 🟢 | none |
| fa8fe0941 | MarcoFalke | fuzz | 🟢 | none |
| fa9f434df | MarcoFalke | test | 🟢 | none |
| faaea7895 | MarcoFalke | net | 🟢 | none (time type refactor, equivalent) |
| 3333c5023 | MarcoFalke | net | 🟢 | none (token bucket refactor, negative diff still clamped) |
| b14f2c76a | kevkevinpal | test | 🟢 | none |
| 6d2952c3c | Hennadii Stepanov | serialize | 🟢 | none (missing include) |
| 6072a2a6a | furszy | wallet | 🟢 | none (itself a fix: feebumper bad_optional_access crash) |
| 658e68f95 | Hennadii Stepanov | test | 🟢 | none (scripted rename) |
| 445143bfc | Renato Britto | doc | 🟢 | none |
| fa71c6e84 | MarcoFalke | ci | 🟢 | none |
| fa30951af | MarcoFalke | test | 🟢 | none |
| faf71d6cb | MarcoFalke | test | 🟢 | none |
| 99996f6c0 | MarcoFalke | test | 🟢 | none |
| 578525d31 | Hennadii Stepanov | depends | 🟢 | none |
| d6f680b42 | sedited | validation | 🟢 | none (move into BlockDisconnected; pblock unused after move) |
| 4d02d2b31 | sedited | validation | 🟢 | none (connected_blocks consumed per-iteration, then destroyed) |
| 8b0fb64c0 | sedited | validation | 🟢 | none (event enqueue refactor, rvalue-enforced) |
| 498b6eb6b | Daniel Pfeifer | build | 🟢 | none (SQLite3 target alias handles old CMake) |
| fa70b9eba | MarcoFalke | ci | 🟢 | none |
| faf3ef4ee | MarcoFalke | ci | 🟢 | none |
| 9f28120a5 | Sebastian Falbesoner | kernel | 🟢 | none (nSequence getter) |
| 6b64b181d | Sebastian Falbesoner | kernel | 🟢 | none (nLockTime getter) |
| 2104282dd | Chandra Pratap | fuzz | 🟢 | none |
| 43b09b993 | Chandra Pratap | fuzz | 🟢 | none |
| 3293e9a61 | fanquake | guix | 🟢 | none |
| 978023fd9 | fanquake | guix | 🟢 | none |
| ab9a98b1e | fanquake | guix | 🟢 | none |
| 2276426bb | fanquake | guix | 🟢 | none |
| feea2a850 | fanquake | ci | 🟢 | none |
| a7524f57b | fanquake | guix | 🟢 | none |
| 2bf97e813 | fanquake | guix | 🟢 | none |
| dc0ddab38 | fanquake | guix | 🟢 | none |
| 31eb46f05 | fanquake | guix | 🟢 | none |
| 0f323e107 | fanquake | guix | 🟢 | none |
| db3c25cfa | Hao Xu | index | 🟢 | none |
| 551875360 | will | ci | 🟢 | none |
| 79467e3ec | Cory Fields | sync | 🟢 | none (ContendedLock instantiations cover mutex+recursive_mutex) |
| f55c891a6 | fanquake | lint | 🟢 | none |
| 8864917d8 | fanquake | lint | 🟢 | none |
| ecefc1292 | fanquake | lint | 🟢 | none |
| ee8c22eb6 | fanquake | contrib | 🟢 | none |
| fa55723b8 | MarcoFalke | net | 🟢 | none (move-only extraction) |
| 92287ae75 | Novo | test | 🟢 | none |
| 04e211837 | fanquake | lint | 🟢 | none |
| 685a44c60 | frankomosh | fuzz | 🟢 | none |
| fadf901fd | MarcoFalke | rpc | 🟢 | none (elision → print_elision; type checks now run) |
| bde35d61f | fanquake | depends | 🟢 | none |
| ac1ccc5bd | Pol Espinasa | build | 🟢 | none |
| 9f273f1c1 | Pol Espinasa | build | 🟢 | none |
| fa050da98 | MarcoFalke | test | 🟢 | none |
| a1f22a0a6 | Hennadii Stepanov | test | 🟢 | none |
| fa9168ffc | MarcoFalke | test | 🟢 | none |
| fa4ec13b4 | MarcoFalke | build | 🟢 | none (warning flag + leveldb exclusion) |
| fa2670bd4 | MarcoFalke | refactor | 🟢 | none (exhaustive-switch style, no behavior change) |
| c2732146d | ViniciusCestarii | doc | 🟢 | none |
| faea12ecd | MarcoFalke | test | 🟢 | none |
| fa90b2143 | MarcoFalke | test | 🟢 | none |
| fa6b05c96 | MarcoFalke | test | 🟢 | none |
| fa7bac94d | MarcoFalke | test | 🟢 | none |
| fa388a358 | MarcoFalke | test | 🟢 | none |
| fa803710e | MarcoFalke | test | 🟢 | none |
| fab5072ce | MarcoFalke | ci | 🟢 | none |
| d23641564 | Fabian Jahr | - | 🟡 | LIVE: rpc: gettxspendingprevout erases from prevouts_to_process vector inside the resolution loop while holding mempool.cs — O(n^2) memmove, minutes-long validation stall via ~8.5MB authenticated request (rpc/mempool.cpp:1026) |
| 9a968ad35 | will | ci | 🟢 | none (gap-swept) |
| faae981d3 | MarcoFalke | lint | 🟢 | none (gap-swept) |
| 9085dee47 | David Gumberg | mempool | 🟢 | none (annotation only) |
| 32325d177 | Anthony Towns | test | 🟢 | none |
| 25e063d95 | Anthony Towns | wallet | 🟢 | none |
| ec4ec91d5 | Sebastian Falbesoner | kernel | 🟢 | none (gap-swept) |
| b19caeea0 | Sebastian Falbesoner | doc | 🟢 | none (gap-swept) |
| 61a5460d0 | Sebastian Falbesoner | test | 🟢 | none (gap-swept) |
| 2e8072edb | Sebastian Falbesoner | rpc | 🟢 | none (gap-swept) |
| 745ad941d | Sebastian Falbesoner | p2p | 🟢 | none (gap-swept) |
| b267efcda | Sebastian Falbesoner | rpc, | 🟢 | none (gap-swept) |
| fadaa7db3 | MarcoFalke | ci | 🟢 | none (gap-swept) |
| 4c07cf87e | will | doc | 🟢 | none (gap-swept) |
| fa4d5891b | MarcoFalke | rpc | 🟢 | none (TxDocOptions refactor) |
| fa8250e96 | MarcoFalke | rpc | 🟢 | none (RPCResultOptions refactor) |
| d03e3be24 | fanquake | ci | 🟢 | none |
| 66d80d57b | fanquake | macdeploy | 🟢 | none |
| ab137cbfe | fanquake | macdeploy | 🟢 | none |
| af0da2fce | David Gumberg | - | 🟡 | LIVE: crypto(latent): AES256CBC classes own raw iv pointer with implicit copy ops — double-free if ever copied; no copies today (crypto/aes.h:52) |
| d53852be3 | David Gumberg | - | 🟡 | LIVE: crypto(latent): same for AES256Encrypt/Decrypt ctx pointer (crypto/aes.h:23) |
| 8c6fedaa8 | David Gumberg | build | 🟢 | none |
| 51ac1abf6 | David Gumberg | bench | 🟢 | none |
| 9a1587251 | David Gumberg | wallet | 🟢 | none (fixes div-by-zero from ae5485fa0d) |
| ae5485fa0 | David Gumberg | - | 🟡 | fixed intra-PR by 9a1587251 (EncryptMasterKey div-by-zero when elapsed==0) |
| 98fcd7af2 | David Gumberg | wallet | 🟢 | none |
| f3bf63ec4 | w0xlt | kernel | 🟢 | none — fix: cursor+best-block acquired atomically under cs_main; live |
| 5e77072fa | w0xlt | rpc | 🟢 | none — fix: gettxoutsetinfo stale-pindex race (#34263); master coherent |
| 79571b918 | Andrew Toth | threadpool | 🟢 | none (gap-swept) |
| fa270fdac | MarcoFalke | net | 🟢 | none |
| faeac1a93 | MarcoFalke | net | 🟢 | none |
| d67c8ed78 | Pieter Wuille | clusterlin | 🟢 | none (gap-swept) |
| 0690a5d0f | Fabian Jahr | Update | 🟢 | none (gap-swept) |
| 20fb7618b | w0xlt | args | 🟢 | none |
| 22b40f34f | w0xlt | args | 🟢 | none (RecursiveMutex→Mutex; recursion eliminated in prior commits, enforced by lock annotations) |
| 3a16ec858 | w0xlt | test | 🟢 | none |
| 70b51fef7 | w0xlt | args | 🟢 | none |
| 7d61e03c7 | w0xlt | args | 🟢 | none |
| 74f71c505 | Sjors Provoost | consensus | 🟢 | none (taproot BIP9 removal compile-safe; GBT keeps "taproot" rule; MinBIP9WarningHeight bumped) |
| 6b20ad84e | fanquake | doc | 🟢 | none (gap-swept) |
| b50376881 | Eugene Siegel | fuzz | 🟢 | none (gap-swept) |
| ddf2a064d | Hennadii Stepanov | init | 🟢 | none (-debuglogfile compat) |
| c08f0c3c2 | fanquake | ci | 🟢 | none (gap-swept) |
| 9f3752c43 | fanquake | ci | 🟢 | none (gap-swept) |
| fadb77169 | MarcoFalke | test | 🟢 | none |
| 3a83715c2 | fanquake | depends | 🟢 | none (gap-swept) |
| eeeeb2a0b | MarcoFalke | fuzz | 🟢 | none |
| fa4fae622 | MarcoFalke | test | 🟢 | none |
| d21afb297 | Hennadii Stepanov | qt | 🟢 | none (gap-swept) |
| fa0587a30 | MarcoFalke | test | 🟢 | none (gap-swept) |
| 46189fd52 | rkrux | doc | 🟢 | none (gap-swept) |
| be6d24ec2 | Hodlinator | guix | 🟢 | none (gap-swept) |
| 69baddc91 | Martin Zumsande | validation | 🟢 | none |
| 501a3dd4a | Sebastian Falbesoner | walletdb | 🟢 | none (gap-swept) |
| 8825051e0 | Lőrinc | bench | 🟢 | none |
| 83b8528dd | Lőrinc | bench | 🟢 | none |
| 55d37546f | Luke Dashjr | gui | 🟢 | none |
| 48b952cbb | Ava Chow | build | 🟢 | none (gap-swept) |
| 1b3d58f12 | Ava Chow | docs | 🟢 | none (gap-swept) |
| b7cf2f87d | Ava Chow | docs | 🟢 | none (gap-swept) |
| 0ebc6891e | fanquake | depends | 🟢 | none (gap-swept) |
| 168997e9b | fanquake | depends | 🟢 | none (gap-swept) |
| 8bc62ce17 | Fabian Jahr | doc | 🟢 | none (gap-swept) |
| 57bfa864f | stratospher | test | 🟢 | none |
| 7ee8c0abc | stratospher | test | 🟢 | none |
| ecb5ce6e7 | stratospher | test | 🟢 | none |
| 15c488949 | furszy | index | 🟢 | none (gap-swept) |
| f8b9595aa | Fabian Jahr | test | 🟢 | none (gap-swept) |
| 9316d9624 | David Gumberg | test | 🟢 | none |
| cbdb891de | Ava Chow | test | 🟢 | none (gap-swept) |
| 2db5c049b | Ava Chow | test | 🟢 | none (gap-swept) |
| 39668f1ee | CaesarCoder | contrib | 🟢 | none |
| 32debfa1e | Bruno Garcia | fuzz | 🟢 | none |
| a1074d852 | Fabian Jahr | index, | 🟢 | none (gap-swept) |
| 2e041b490 | Hao Xu | init | 🟢 | none (help text) |
| 2f8f2e900 | stickies-v | validation | 🟢 | none (gap-swept) |
| 20ae9b98e | marcofleon | Extend | 🟢 | none (gap-swept) |
| 854a6d5a9 | marcofleon | validation | 🟢 | none (gap-swept) |
| fa79098ce | MarcoFalke | test | 🟢 | none (gap-swept) |
| 9249e6089 | marcofleon | validation | 🟢 | none (gap-swept) |
| d76ec4de1 | Antoine Poinsot | fuzz | 🟢 | none — adds PSBT ser/deser roundtrip assertion |
| faa68ed4b | MarcoFalke | test | 🟢 | none (gap-swept) |
| bff8a7a80 | kevkevinpal | subprocess | 🟢 | none (gap-swept) |
| 89386e700 | sedited | kernel | 🟢 | none (gap-swept) |
| 2678abe90 | Daniel Pfeifer | prevector | 🟢 | none (gap-swept) |
| 0a6724aaa | Hennadii Stepanov | doc | 🟢 | none (gap-swept) |
| 473e5f8ef | Hennadii Stepanov | qt | 🟢 | none (gap-swept) |
| 3cb4d6066 | Cory Fields | qt | 🟢 | none (gap-swept) |
| d7e972a90 | Cory Fields | qt | 🟢 | none (gap-swept) |
| 19693a8c9 | Hennadii Stepanov | depends | 🟢 | none (gap-swept) |
| c55584575 | Hennadii Stepanov | cmake | 🟢 | none (gap-swept) |
| 5e35a9069 | fanquake | script | 🟢 | none — removes 4-line clang-tidy NOLINT comment only, no code change |
| 4089682f5 | fanquake | ci | 🟢 | none — Clang 22 in tidy task |
| 7ea076f99 | fanquake | tidy | 🟢 | none — replaces deprecated ClangTidy header |
| eb17f29aa | fanquake | cmake | 🟢 | none — makes clang-tidy a required dependency |
| b83de7f28 | stickies-v | validation | 🟢 | none (gap-swept) |
| d8f4e7caf | ismaelsadeeq | doc | 🟢 | none |
| 248c175e3 | ismaelsadeeq | test | 🟢 | none |
| d2716e9e5 | ismaelsadeeq | policy | 🟢 | none (AreInputsStandardness rename+reasons; semantics preserved) |
| 1c1de334e | Ryan Ofsky | test | 🟢 | none — fixes interface_ipc.py test race |
| 2a7a4f608 | Hennadii Stepanov | depends | 🟢 | none (gap-swept) |
| a61907e5d | Lőrinc | doc | 🟢 | none (gap-swept) |
| 5c005363a | rkrux | test | 🟢 | none (gap-swept) |
| 04d951574 | rkrux | test | 🟢 | none (gap-swept) |
| b87a1c27c | Ryan Ofsky | doc | 🟢 | none — dependencies.md IPC docs |
| f580cc7e9 | nervana21 | doc | 🟢 | none (gap-swept) |
| 4ae9a10ad | Andrew Toth | doc | 🟢 | none (gap-swept) |
| 37d49f5de | Pieter Wuille | doc | 🟢 | none (gap-swept) |
| 771f7642b | Pieter Wuille | doc | 🟢 | none (gap-swept) |
| 708b84999 | Pieter Wuille | doc | 🟢 | none (gap-swept) |
| 8f2a869a1 | Pieter Wuille | doc | 🟢 | none (gap-swept) |
| 65a8b6c2e | Pieter Wuille | doc | 🟢 | none (gap-swept) |
| c510d126e | Andrew Toth | doc | 🟢 | none (gap-swept) |
| 027cac852 | Andrew Toth | qt | 🟢 | none (gap-swept) |
| 5b34f2518 | Andrew Toth | dbcache | 🟢 | none (gap-swept) |
| 44feab23a | Weixie Cui | script | 🟢 | none (gap-swept) |
| 4565cff72 | Ryan Ofsky | gui | 🟢 | none |
| fbea576c2 | Ryan Ofsky | test | 🟢 | none |
| 0448a19b1 | Ryan Ofsky | ipc | 🟢 | none |
| 8d614bfa4 | Ryan Ofsky | ipc/cli | 🟢 | none |
| 6a5483489 | Ryan Ofsky | ipc | 🟢 | none |
| df76891a3 | Ryan Ofsky | http/rpc | 🟢 | none |
| 3cd1cd3ad | Ryan Ofsky | ipc | 🟢 | none |
| 779e7825d | brunoerg | fuzz | 🟢 | none |
| 98e8af4bb | Ava Chow | wallet | 🟢 | none — fix: drains validation queue on wallet unload (see note) |
| 52992ebe1 | Ava Chow | interfaces | 🟢 | none — adds Chain::waitForNotifications() |
| fa9d0623a | MarcoFalke | doc | 🟢 | none (gap-swept) |
| 1aa78cdab | Pieter Wuille | clusterlin | 🟢 | none |
| 747da2536 | Pieter Wuille | feefrac | 🟢 | none |
| fa48f8c86 | MarcoFalke | test | 🟢 | none |
| 73e385311 | Martin Zumsande | test | 🟢 | none (gap-swept) |
| fab51e470 | MarcoFalke | test/ci | 🟢 | none — valgrind.supp relocation |
| fa9cf81d3 | MarcoFalke | test | 🟢 | none — path resolve() fix |
| fa18be2f2 | MarcoFalke | test | 🟢 | none — typo |
| fac932698 | MarcoFalke | ci | 🟢 | none |
| c1361fc42 | MarcoFalke | netif | 🟢 | none — FreeBSD 15 sign-compare fix; `recv_result` is verified `>= 0` before the unsigned cast, `NLMSG_NEXT` still mutates the signed original; LIVE |
| 8834e4e86 | Max Edwards | test | 🟢 | none |
| fa7612f25 | MarcoFalke | ci | 🟢 | none — script_assets download for Windows CI |
| 7777a1330 | MarcoFalke | test | 🟢 | none — move print into util, no behavior change |
| faf96286c | MarcoFalke | test | 🟢 | none — move-only download_from_url refactor |
| 17a079c2f | will | ci | 🟢 | none |
| 408d5b12e | pinheadmz | test | 🟢 | none |
| 9dc653b3b | furszy | test | 🟢 | none |
| ce2a984ee | l0rinc | test | 🟢 | none |
| d9c6769d0 | furszy | test | 🟢 | none |
| dbbb780af | Hodlinator | test | 🟢 | none |
| 3b7cbcafc | seduless | test | 🟢 | none |
| ca101a231 | furszy | test | 🟢 | none |
| bf2c607aa | furszy | threadpool | 🟢 | none — Stop() now drains queue via `ProcessTask()` loop; `Submit()` rejects via `m_interrupt`, so queue is finite; LIVE on master with later self-join assert added |
| e88d27443 | furszy | test | 🟢 | none |
| fa0cc1c5a | MarcoFalke | doc | 🟢 | none — removes outdated comment |
| 44538f8ad | Ava Chow | kernel | 🟢 | none (gap-swept) |
| 58c2e23fc | Ava Chow | kernel | 🟢 | none (gap-swept) |
| cf261b071 | Ava Chow | kernel | 🟢 | none (gap-swept) |
| 8eaf1d26d | Ava Chow | kernel | 🟢 | none (gap-swept) |
| 5ca0c5551 | Ava Chow | kernel | 🟢 | none (gap-swept) |
| fec58229f | Ava Chow | contrib | 🟢 | none (gap-swept) |
| 27fbdb009 | Ava Chow | makeseeds | 🟢 | none (gap-swept) |
| 982883a1b | Ava Chow | makeseeds | 🟢 | none (gap-swept) |
| faa70ca76 | MarcoFalke | doc | 🟢 | none |
| b7ca3bf06 | Ryan Ofsky | ipc | 🟢 | none — libmultiprocess subtree sync, itself fixes a theoretical cancel_mutex race; IPC not in release builds |
| 8fe91f371 | Ryan Ofsky | test | 🟢 | none — interface_ipc.py updates for libmultiprocess#240 |
| 33fbaed31 | Greg Sanders | policy | 🟢 | none — skips `CheckEphemeralSpends` only when `m_bypass_limits`, which master confirms is set solely for reorg resurrection (src/validation.cpp:324) |
| a49f97ff4 | sedited | net | 🟢 | none (itself a privacy improvement) |
| 8cd4a4363 | furszy | threadpool | 🟢 | none — Start-Stop race fix keeps `m_interrupt` set until workers joined; residual Stop+Stop reset race not reachable in node code (single controller thread); LIVE |
| 9ff1e82e7 | l0rinc | test | 🟢 | none |
| 744d47fce | Pieter Wuille | clusterlin | 🟢 | none — trained cost model; work remains bounded by max_cost budget |
| 4eefdfc5b | Pieter Wuille | clusterlin | 🟢 | none — cost rescale; `ACCEPTABLE_COST=75'000` still on master and consistent |
| bbc8f1e0a | Ryan Ofsky | ipc/init | 🟢 | none — fixes `m_node.chainman` assert crash reachable via IPC mining on early startup; LIVE; related cv-wake hole later fixed by c1313b199f |
| a7cabf92e | Ryan Ofsky | init | 🟢 | none — single-instance KernelNotifications refactor; LIVE |
| ecc9a84f8 | Pieter Wuille | clusterlin | 🟢 | none — pure iters→cost rename, verified faithful line-by-line |
| 9e7129df2 | Pieter Wuille | clusterlin | 🟢 | none — CostModel abstraction, arithmetic verified identical to naive model |
| e8f8b74a4 | furszy | test | 🟢 | none |
| c8e332cb3 | Ryan Ofsky | init | 🟢 | none — pure refactor |
| fa5d47885 | MarcoFalke | test | 🟢 | none |
| fa29fb72c | MarcoFalke | test | 🟢 | none |
| fa03fbf7e | MarcoFalke | test | 🟢 | none |
| a28eedb8c | fanquake | ci | 🟢 | none |
| 3281824ec | Lőrinc | fuzz | 🟢 | none — harness now respects FRESH/DIRTY invariants so real `BatchWrite` bugs surface |
| 780f46063 | Lőrinc | fuzz | 🟢 | none — `PeekCoin` recurses to base view on master (src/coins.cpp:40), so derived `possible_overwrite` is sound |
| d7e0d510f | Lőrinc | fuzz | 🟢 | none — same overwrite-contract fix for `AddCoins` |
| b8fa6f0f7 | Lőrinc | coins | 🟢 | none — `Assume(TrySub(...))` hardening; verified master's `Assume` always evaluates its argument (src/util/check.h:128), so accounting still happens in release builds |
| ac3bea07c | furszy | test | 🟢 | none |
| da7f70a53 | Vasil Dimov | test | 🟢 | none — I2P port 0 in p2p_private_broadcast.py |
| a8ebcfd34 | Vasil Dimov | test | 🟢 | none — deflakes p2p_private_broadcast.py ordering |
| 45133c589 | Lőrinc | doc | 🟢 | none |
| e5f061350 | sedited | net_processing | 🟢 | none — skips txdownloadman bloom-filter updates during IBD; `IsInitialBlockDownload()` is an atomic cached latch (thread-safe from validation callback); `BlockDisconnected` resets the whole filter so no asymmetry; re-request cost documented in test; LIVE |
| ce8b69289 | Lőrinc | test | 🟢 | none |
| c462e54f9 | Vasil Dimov | test | 🟢 | none |
| 371056630 | Vasil Dimov | test | 🟢 | none |
| 38a7a6712 | Hennadii Stepanov | build | 🟢 | none — fixes Ninja+CMake≥4.0.5 Darwin cross-configure break |
| fcaec2544 | Sjors Provoost | doc | 🟢 | none |
| 1e82fa498 | Sjors Provoost | mining | 🟢 | none — interrupt flag plumbing reuses existing `InterruptWait` pattern; acknowledged stray-flag quirk is benign (self-resets on next wait) |
| a11297a90 | Sjors Provoost | mining | 🟢 | none — `BlocksAheadOfTip` correct under cs_main; cooldown bounded at 20s/iteration, interruptible; RPC/regtest paths opt out |
| a9e59f7d9 | Greg Sanders | rpc | 🟢 | none — `DoWork(0)` is an O(1) queue-empty check; no RPC amplification |
| a3fb3dd55 | Greg Sanders | mempool | 🟢 | none — logging only |
| 3feabb203 | fanquake | leveldb | 🟢 | none — drops unreferenced c.cc/dumpfile.cc/histogram.cc from build |
| c86bce597 | fanquake | guix | 🟢 | none |
| ef987683d | Hennadii Stepanov | qt | 🟢 | none — translation refresh |
| 3574905ce | will | ci | 🟢 | none — revert of Sequoia SHA1-key workaround |
| fa4424fd9 | MarcoFalke | test | 🟢 | none |
| faed837f2 | MarcoFalke | test | 🟢 | none |
| 0b96b9c60 | sstone | Minimize | 🟢 | none (gap-swept) |
| 6202acd28 | brunoerg | test | 🟢 | none |
| e0463b4e8 | Sjors Provoost | - | 🟡 | LIVE: rpc: getblock/REST hit CHECK_NONFATAL(witness_stack.size()==1) on stored mutated-but-PoW-valid blocks (bad-witness-nonce-size) — spurious 'internal bug', contained (rpc/blockchain.cpp coinbaseTxToJSON) |
| c68e3d2c5 | Vasil Dimov | doc | 🟢 | none |
| 4bae84c94 | Vasil Dimov | doc | 🟢 | none |
| 4c6798a3d | Vasil Dimov | tor | 🟢 | none (PoW defenses w/ syntax-error fallback to no-PoW) |
| fb993f760 | Vasil Dimov | tor | 🟢 | none (constants dedup) |
| 3d82ec5bd | sstone | Add | 🟢 | none (gap-swept) |
| 111864ac3 | Hodlinator | qa | 🟢 | none |
| c2e28d455 | Hodlinator | ci | 🟢 | none |
| 850a80c19 | Hodlinator | qa | 🟢 | none |
| fb803e3c7 | Hodlinator | qa | 🟢 | none |
| ed43ce57c | Hodlinator | qa | 🟢 | none |
| 64a098a9b | Hodlinator | qa | 🟢 | none |
| bb1aff7ed | Hodlinator | qa | 🟢 | none |
| d1a4ddb58 | Hodlinator | qa | 🟢 | none |
| c811e4736 | Hodlinator | qa | 🟢 | none |
| 73cf85891 | Hodlinator | qa | 🟢 | none |
| 726b3663c | furszy | http | 🟢 | none (fixes unclean shutdown; shared_ptr lifetime handled) |
| fa6af8563 | MarcoFalke | util | 🟢 | none — sanitizer-cast, no behavior change; LIVE |
| fa692974a | MarcoFalke | util | 🟢 | none — fixes real uninitialized-termios UB in `SetStdinEcho` on non-tty stdin; LIVE |
| 97e7e7943 | Hennadii Stepanov | test | 🟢 | none (gap-swept) |
| a4324ce09 | Hennadii Stepanov | test | 🟢 | none (gap-swept) |
| fa36adeb7 | MarcoFalke | ci | 🟢 | none |
| fae31b1e2 | MarcoFalke | ci | 🟢 | none |
| 59d24bd5d | furszy | threadpool | 🟢 | none |
| fb3e1bf9c | stratospher | test | 🟢 | none |
| 29740c06a | stratospher | validation | 🟢 | none (disk migration of CHILD flag in b5b2956bd runs before all VALID-only checks) |
| b5b2956bd | stratospher | validation | 🟢 | none |
| 37bc20785 | stratospher | validation | 🟢 | none |
| 120c631e1 | stratospher | refactor | 🟢 | none |
| fa4cb96bd | MarcoFalke | test | 🟢 | none |
| c2fcf2506 | Pieter Wuille | clusterlin | 🟢 | none (reachable-set recomputation provably equivalent; fuzz SanityCheck coverage) |
| d90f98ab4 | Pieter Wuille | clusterlin | 🟢 | none |
| b684f954b | Pieter Wuille | clusterlin | 🟢 | none |
| 1daa600c1 | Pieter Wuille | clusterlin | 🟢 | none |
| 63b06d552 | Pieter Wuille | clusterlin | 🟢 | none |
| ae16485aa | Pieter Wuille | clusterlin | 🟢 | none (self-merge precondition ⇔ reachable overlap, MergeChunks Assume holds) |
| 3221f1a07 | Pieter Wuille | clusterlin | 🟢 | none |
| 7194de3f7 | Pieter Wuille | clusterlin | 🟢 | none |
| 6f898dbb8 | Pieter Wuille | clusterlin | 🟢 | none |
| dcf458ffb | Pieter Wuille | clusterlin | 🟢 | none |
| cbd684a47 | Pieter Wuille | clusterlin | 🟢 | none |
| b75574a65 | Pieter Wuille | clusterlin | 🟢 | none |
| 73cbd15d4 | Pieter Wuille | clusterlin | 🟢 | none |
| 7c6f63a8a | Pieter Wuille | clusterlin | 🟢 | none |
| 20e2f3e96 | Pieter Wuille | clusterlin | 🟢 | none (scripted rename) |
| 268fcb6a5 | Pieter Wuille | clusterlin | 🟢 | none (adds asserts only) |
| d69c9f56e | Pieter Wuille | clusterlin | 🟢 | none |
| f66fa69ce | Pieter Wuille | clusterlin | 🟢 | none |
| 900e45977 | Pieter Wuille | clusterlin | 🟢 | none |
| 666b37970 | Pieter Wuille | clusterlin | 🟢 | none (itself a type fix; overflow unreachable at n≤64) |
| 231dd04b8 | will | build | 🟢 | none (gap-swept) |
| fa48d4216 | MarcoFalke | test | 🟢 | none |
| fa626bd14 | MarcoFalke | util | 🟢 | none (same whitespace-split semantics, no shell) |
| fd06157d1 | Fabian Jahr | test | 🟢 | none (gap-swept) |
| 3d7ab7ecb | Fabian Jahr | rpc, | 🟢 | none (gap-swept) |
| 312919c9d | Fabian Jahr | test | 🟢 | none (gap-swept) |
| a9a3b29dd | Fabian Jahr | index | 🟢 | none (gap-swept) |
| fafdb8f63 | MarcoFalke | ci | 🟢 | none (gap-swept) |
| fab73e213 | MarcoFalke | ci | 🟢 | none (gap-swept) |
| 62e378584 | fanquake | guix | 🟢 | none |
| badcf1c68 | fanquake | guix | 🟢 | none |
| 746d8cddc | Hennadii Stepanov | gui | 🟢 | none |
| f611d3bda | brunoerg | addrman | 🟢 | none |
| fa5672dca | MarcoFalke | gui | 🟢 | none |
| d159b1039 | janb84 | doc | 🟢 | none |
| 353c660be | Lőrinc | bench | 🟢 | none |
| afea2af13 | ANAVHEOBA | net | 🟢 | none |
| fac3ecaf6 | MarcoFalke | rpc | 🟢 | none (int-range saturation is an improvement) |
| faee36f63 | MarcoFalke | util | 🟢 | none (out-of-range JSON-number settings for new int32 args can throw at startup; local-config-only, pre-existing class) |
| 22335474d | Lőrinc | net | 🟢 | none (gap-swept) |
| e55ea534f | Lőrinc | test | 🟢 | none (gap-swept) |
| 736b17c0f | Lőrinc | log | 🟢 | none (gap-swept) |
| 9cf82bed3 | Lőrinc | log | 🟢 | none (gap-swept) |
| bb00fd214 | b-l-u-e | test | 🟢 | none |
| 4f19508ae | b-l-u-e | test | 🟢 | none |
| b8827ce61 | b-l-u-e | net | 🟢 | none (itself a fix: Discover() with -bind=0.0.0.0) |
| 6df4a045f | Hennadii Stepanov | gui | 🟢 | none |
| 80dc4359b | Hennadii Stepanov | cmake | 🟢 | none (gap-swept) |
| 211111b80 | MarcoFalke | test | 🟢 | none |
| 24f93c9af | Pol Espinasa | doc | 🟢 | none |
| 331a5279d | Pol Espinasa | wallet | 🟢 | none (planned deprecation removal; all m_pay_tx_fee refs cleaned) |
| eafd530d2 | Cory Fields | kernel | 🟢 | none (gap-swept) |
| 24c3b4701 | Cory Fields | build | 🟢 | none (gap-swept) |
| cae6d895f | Andrew Toth | fuzz | 🟢 | none |
| 86eda88c8 | Andrew Toth | fuzz | 🟢 | none |
| 89824fb27 | Andrew Toth | fuzz | 🟢 | none |
| 73e99a596 | Andrew Toth | coins | 🟢 | none (overlay flushed only on successful ConnectBlock; integration test included) |
| 67c0d1798 | Andrew Toth | coins | 🟢 | none |
| 69b01af0e | Andrew Toth | coins | 🟢 | none (PeekCoin read-through correct incl. spent/FRESH handling) |
| f700609e8 | Ryan Ofsky | doc | 🟢 | none (gap-swept) |
| 79c934b51 | Hennadii Stepanov | cmake | 🟢 | none (gap-swept) |
| fa90d44a2 | MarcoFalke | test | 🟢 | none |
| 9453c1536 | Sjors Provoost | ipc | 🟢 | none (gap-swept) |
| 70de5cc2d | Sjors Provoost | ipc | 🟢 | none (gap-swept) |
| 2278f017a | Ryan Ofsky | ipc | 🟢 | none (gap-swept) |
| c6638fa7c | Ryan Ofsky | ipc | 🟢 | none (gap-swept) |
| a4603ac77 | Ryan Ofsky | ipc | 🟢 | none (gap-swept) |
| ff995b50c | Ryan Ofsky | ipc | 🟢 | none (gap-swept) |
| b970cdf20 | Ryan Ofsky | test | 🟢 | none (gap-swept) |
| df53a3e5e | Ryan Ofsky | rpc | 🟢 | none (gap-swept) |
| 2a1d0db79 | Andrew Toth | doc | 🟢 | none |
| c3378be10 | Andrew Toth | test | 🟢 | none |
| 557260ca1 | Andrew Toth | rpc | 🟢 | none (NumToOpenSub saturates at 0; auth-only RPC) |
| 15dff452e | Andrew Toth | test | 🟢 | none |
| 996f20c18 | Andrew Toth | rpc | 🟢 | none (read-only RPC) |
| 5e6498254 | Andrew Toth | net | 🟢 | none |
| a067ca341 | glozow | [doc] | 🟢 | none (gap-swept) |
| 02d047fd5 | Lőrinc | refactor | 🟢 | none (gap-swept) |
| 50cf6838e | SomberNight | wallet | 🟢 | none (gap-swept) |
| c1355493e | ismaelsadeeq | fees | 🟢 | none (call sites consistently migrated; verified on master) |
| c413cf12c | willcl-ark | ci | 🟢 | none (gap-swept) |
| 922ebf96e | ismaelsadeeq | refactor | 🟢 | none (move-only) |
| d29bc5e6d | sedited | doc | 🟢 | none (gap-swept) |
| 452c74395 | Hennadii Stepanov | refactor | 🟢 | none (gap-swept) |
| 7164a0cab | Hennadii Stepanov | build | 🟢 | none (gap-swept) |
| 2ccfdb582 | Cory Fields | build | 🟢 | none (gap-swept) |
| f7be5fb8f | glozow | wallet | 🟢 | none — pure variable rename in `getTransactionAncestry` call sites |
| fa8c89511 | MarcoFalke | Fixup | 🟢 | none (gap-swept) |
| f8d2f30bf | Hennadii Stepanov | ci | 🟢 | none (gap-swept) |
| 573bb542b | Andrew Toth | net | 🟢 | none |
| fa13b1323 | MarcoFalke | ci | 🟢 | none (gap-swept) |
| fa2719ab1 | MarcoFalke | ci | 🟢 | none (gap-swept) |
| fa99ba5f1 | MarcoFalke | ci | 🟢 | none (gap-swept) |
| fa4a1cab6 | MarcoFalke | ci | 🟢 | none (gap-swept) |
| 111110868 | MarcoFalke | ci | 🟢 | none (gap-swept) |
| fac9c7bd6 | MarcoFalke | ci | 🟢 | none (gap-swept) |
| 39e3295c7 | Hodlinator | test | 🟢 | none |
| faf738946 | MarcoFalke | ci | 🟢 | none (gap-swept) |
| fa674d55d | MarcoFalke | ci | 🟢 | none (gap-swept) |
| 6f113cb18 | Pieter Wuille | txgraph | 🟢 | none (gap-swept) |
| 0a3351947 | Pieter Wuille | txgraph | 🟢 | none (gap-swept) |
| fba004a3d | Pieter Wuille | txgraph | 🟢 | none (gap-swept) |
| 941c432a4 | Pieter Wuille | txgraph | 🟢 | none (gap-swept) |
| 39d0052cb | Pieter Wuille | clusterlin | 🟢 | none (gap-swept) |
| 8bfbba320 | Pieter Wuille | txgraph | 🟢 | none (gap-swept) |
| e0bc73ba9 | Pieter Wuille | clusterlin | 🟢 | none (gap-swept) |
| 6c1bcb2c7 | Pieter Wuille | txgraph | 🟢 | none (gap-swept) |
| 7427c7d09 | Pieter Wuille | txgraph | 🟢 | none (gap-swept) |
| 3ddafceb9 | Pieter Wuille | txgraph | 🟢 | none (gap-swept) |
| 5f36e0ff1 | Lőrinc | rpc | 🟢 | none (itself a fix: getblockstats UTXO overhead) |
| 76190489e | Lőrinc | coins | 🟢 | none (value-identical packing; fixes theoretical shift UB) |
| 1f309d1aa | Lőrinc | coins | 🟢 | none |
| 18f11695c | stratospher | validation | 🟢 | none |
| d339884f1 | Sebastian Falbesoner | bench | 🟢 | none (gap-swept) |
| fa90277d2 | MarcoFalke | ci | 🟢 | none — ubuntu-slim runners |
| fa9627af9 | MarcoFalke | ci | 🟢 | none — cmake preset toolchain for Windows CI |
| fa3f89aca | MarcoFalke | ci | 🟢 | none — check_manifests in ci-windows.py |
| 1111079a1 | MarcoFalke | ci | 🟢 | none — run_tests step in ci-windows.py |
| afb1bc120 | Pieter Wuille | validation | 🟢 | none (dirty-count accounting verified against all mutation paths incl. Uncache/BatchWrite FRESH erase) |
| b413491a1 | Pieter Wuille | coins | 🟢 | none |
| 7e52b1b94 | Lőrinc | fuzz | 🟢 | none |
| fe0b1513a | Hao Xu | test | 🟢 | none — txgraph staging test |
| ef253a9d3 | Hao Xu | test | 🟢 | none — txgraph block builder tests |
| 4a1ac31e9 | Hao Xu | test | 🟢 | none — txgraph GetWorstMainChunk test |
| b623fab1b | Sjors Provoost | mining | 🟢 | none (gap-swept) |
| d3e49528d | Sjors Provoost | mining | 🟢 | none (gap-swept) |
| 418b7995d | Sjors Provoost | test | 🟢 | none (gap-swept) |
| 4c0d4f6f9 | furszy | wallet | 🟢 | none — bundle createTransaction outputs into Result; GUI call site correctly updated (`change_pos.value_or(-1)` cast well-defined under C++20); no later fixes on master |
| e2c3ec9bf | furszy | wallet | 🟢 | none — move CreatedTransactionResult to types.h |
| 45372175c | furszy | gui | 🟢 | none — drop AmountWithFeeExceedsBalance special case (wallet now reports error) |
| b73a62f66 | Ava Chow | test | 🟢 | none — fix feature_assumevalid.py flake |
| 633d18311 | Sjors Provoost | test | 🟢 | none (gap-swept) |
| 52ccd9215 | Sjors Provoost | test | 🟢 | none (gap-swept) |
| 4e49fa2a6 | Sjors Provoost | test | 🟢 | none (gap-swept) |
| 01a1ae889 | Sjors Provoost | test | 🟢 | none (gap-swept) |
| 576f89202 | hebasto | translations | 🟢 | none — bitcoin_en.xlf update |
| 4b9f5beaf | hebasto | translations | 🟢 | none — Transifex slug 31.x |
| fa6801366 | MarcoFalke | rpc | 🟢 | none — remove integral casts; UniValue integral template ctor (`setInt(int64_t/uint64_t)`) handles all pushed types safely; series continued by take 3 (fa864b937e), no bug fix |
| d79249d27 | fanquake | ci | 🟢 | none (gap-swept) |
| 48161f6a0 | stratospher | wallet | 🟢 | none (gap-swept) |
| b7fa609ed | stratospher | wallet | 🟢 | none (gap-swept) |
| 7819da2c1 | stratospher | walllet | 🟢 | none (gap-swept) |
| fa561682c | MarcoFalke | ci | 🟢 | none — prepare_tests step |
| fa3e607c6 | MarcoFalke | ci | 🟢 | none — verbose Windows CI failure output |
| 4444808dd | MarcoFalke | ci | 🟢 | none — build step |
| fabdd4e82 | MarcoFalke | ci | 🟢 | none — Windows CI into script |
| fa88ac3f4 | MarcoFalke | doc | 🟢 | none — clang-tidy config comment |
| fa0677d13 | MarcoFalke | refactor | 🟢 | none (gap-swept) |
| e5474079f | stratospher | wallet | 🟢 | none (gap-swept) |
| d8ea921d0 | stratospher | wallet | 🟢 | none (gap-swept) |
| 7072d825e | stratospher | wallet | 🟢 | none (gap-swept) |
| fefa3be78 | stratospher | wallet | 🟢 | none (gap-swept) |
| 322c4ec44 | fanquake | build | 🟢 | none (gap-swept) |
| edc297805 | Cory Fields | signals | 🟢 | none |
| 9ade3929a | Cory Fields | signals | 🟢 | none |
| 037e58b57 | Cory Fields | signals | 🟢 | none |
| 65134c7e5 | Hennadii Stepanov | depends | 🟢 | none (gap-swept) |
| 94a692b6a | Hennadii Stepanov | cmake | 🟢 | none (gap-swept) |
| b5375c44e | Hennadii Stepanov | depends | 🟢 | none (gap-swept) |
| d73378ffc | Hennadii Stepanov | cmake | 🟢 | none (gap-swept) |
| c6a6435ce | rkrux | wallet | 🟢 | none (gap-swept) |
| 61039d72a | rkrux | wallet | 🟢 | none (gap-swept) |
| 5af5e8764 | Hodlinator | test | 🟢 | none |
| fd7c494c6 | Hodlinator | miniscript | 🟢 | none (comment) |
| 8966352df | ismaelsadeeq | doc | 🟢 | none |
| 704a09fe7 | ismaelsadeeq | test | 🟢 | none |
| 243e48cf4 | ismaelsadeeq | fees | 🟢 | none (old files rejected by version check before dummy field is missed) |
| fc4fbda42 | ismaelsadeeq | fees | 🟢 | none |
| b54dedcc8 | ismaelsadeeq | fees | 🟢 | none (bucket-range change paired with file-version bump 149900→309900) |
| 2cb7e99de | Vasil Dimov | test | 🟢 | none (gap-swept) |
| 91b7c874e | Vasil Dimov | test | 🟢 | none (gap-swept) |
| 42ee31e80 | jayvaliya | doc | 🟢 | none (gap-swept) |
| 54d039305 | Murch | FUZZ | 🟢 | none (gap-swept) |
| eb510f867 | Lőrinc | ci | 🟢 | none |
| 04c4d7100 | Lőrinc | ci | 🟢 | none |
| b65a3d800 | Hennadii Stepanov | iwyu | 🟢 | none (gap-swept) |
| a50d0b672 | Cory Fields | build | 🟢 | none (gap-swept) |
| fad9dd1a8 | MarcoFalke | test | 🟢 | none (gap-swept) |
| fabb58d42 | MarcoFalke | test | 🟢 | none (gap-swept) |
| fa51594c5 | MarcoFalke | refactor | 🟢 | none (gap-swept) |
| fa33acec8 | MarcoFalke | Revert | 🟢 | none (gap-swept) |
| 24699fec8 | Fabian Jahr | doc | 🟢 | none |
| bab085d28 | Fabian Jahr | ci | 🟢 | none |
| e53934422 | Fabian Jahr | doc | 🟢 | none |
| 6244212a5 | Fabian Jahr | - | 🟡 | LIVE: init: -asmap=<path> silently ignored when the relative path starts with a digit (GetBoolArg parses it as true → embedded asmap used); hits canonical asmap-data filenames (init.cpp:1611) |
| 6202b50fb | Fabian Jahr | build | 🟢 | none |
| 634cd60dc | Fabian Jahr | build | 🟢 | none (binary asmap blob, verified by CheckStandardAsmap at runtime) |
| faa4ab113 | MarcoFalke | ci | 🟢 | none (gap-swept) |
| dd93362a1 | Sebastian Falbesoner | bench | 🟢 | none (gap-swept) |
| 02b5f6078 | ismaelsadeeq | fees | 🟢 | none (gap-swept) |
| faa016af5 | MarcoFalke | refactor | 🟢 | none (gap-swept) |
| fad7d86d8 | MarcoFalke | ci | 🟢 | none (gap-swept) |
| 2f2952c5f | MarcoFalke | Squashed | 🟢 | none (gap-swept) |
| 7528d1879 | will | ci | 🟢 | none (gap-swept) |
| 4b53cbd69 | Ava Chow | test | 🟢 | none (gap-swept) |
| ec0f47b15 | Ava Chow | miniscript | 🟢 | none (gap-swept) |
| 6fd780d4f | Ava Chow | descriptors | 🟢 | none (gap-swept) |
| b12281bd8 | Ava Chow | miniscript | 🟢 | none (gap-swept) |
| ce4c66eb7 | Ava Chow | test | 🟢 | none (gap-swept) |
| 2150153f3 | Cory Fields | build | 🟢 | none |
| fd5e9d990 | Cory Fields | signals | 🟢 | none |
| 8c0331838 | Lőrinc | consensus | 🟢 | none — comment-only: GetValueOut precondition; claim verified accurate (CheckBlock→CheckTransaction precedes ConnectBlock; PreChecks calls CheckTransaction) |
| 82ef92c8d | Lőrinc | consensus | 🟢 | none — comment-only: why bad-txns-fee-outofrange unreachable; check deliberately kept in place |
| fad3eb395 | MarcoFalke | refactor | 🟢 | none (gap-swept) |
| fa06e2676 | MarcoFalke | refactor | 🟢 | none (gap-swept) |
| fabd4d2e2 | MarcoFalke | refactor | 🟢 | none (gap-swept) |
| 37cc2a2d9 | stickies-v | logging | 🟢 | none — include-only refactor to util/log.h (incl. dbwrapper.cpp Level alias, equivalent) |
| bb8e9e7c4 | stickies-v | logging | 🟢 | none — formatting moved to util/log.h; only change is an extra strprintf when category enabled but output disabled (documented perf impact); args were always evaluated, so no behavior change |
| 001f0a428 | stickies-v | logging | 🟢 | none — move-only macros |
| 94c0adf4e | stickies-v | logging | 🟢 | none — move levels; BCLog::Level kept as alias |
| 56d113cab | stickies-v | logging | 🟢 | none — move-only categories |
| f5233f7e9 | stickies-v | logging | 🟢 | none — move-only SourceLocation |
| fa20bc2ec | MarcoFalke | refactor | 🟢 | none (gap-swept) |
| fa879db73 | MarcoFalke | test | 🟢 | none (gap-swept) |
| d40571319 | fanquake | ci | 🟢 | none (gap-swept) |
| 1cee0e4cd | fanquake | ci | 🟢 | none (gap-swept) |
| 9d4c9b003 | fanquake | Squashed | 🟢 | none (gap-swept) |
| 3c8f5e48f | will | ci | 🟢 | none |
| da51b5e4d | Hodlinator | miniscript | 🟢 | none (move ctor arg) |
| 580e9eefe | will | ci | 🟢 | none (gap-swept) |
| 67696b207 | Vasil Dimov | net | 🟢 | none — log-only; `conn_type` is a plain enum, `ConnectionTypeAsString` total, verified `src/net.cpp:399` on master |
| 881ab4fc8 | furszy | support | 🟢 | none (gap-swept) |
| 232a2bce9 | Lőrinc | test | 🟢 | none — GetValueOut out-of-range unit test |
| aa87aae14 | Lőrinc | test | 🟢 | none — CheckTxInputs MoneyRange unit tests |
| 1bf384222 | Hennadii Stepanov | ci | 🟢 | none |
| dfb936460 | b-l-u-e | fuzz | 🟢 | none |
| 1f8f7d477 | optout | Change | 🟢 | none (gap-swept) |
| 38fd85c67 | furszy | http | 🟢 | none (gap-swept) |
| c323f882e | TheCharlatan | fuzz | 🟢 | none (gap-swept) |
| c528dd5f8 | furszy | util | 🟢 | none (gap-swept) |
| 07af50f78 | Hennadii Stepanov | util | 🟢 | none |
| 4dfb6eef7 | billymcbip | test | 🟢 | none (gap-swept) |
| 884978f38 | billymcbip | test | 🟢 | none (gap-swept) |
| 527e8ca7b | billymcbip | test | 🟢 | none (gap-swept) |
| 516be10bb | Hennadii Stepanov | wallet | 🟢 | none |
| bbbb78a4f | MarcoFalke | ci | 🟢 | none (gap-swept) |
| 2222dadab | MarcoFalke | ci | 🟢 | none (gap-swept) |
| 9c839aa9e | Hennadii Stepanov | iwyu | 🟢 | none |
| 91824646c | Hennadii Stepanov | iwyu | 🟢 | none |
| 37de7d191 | Hennadii Stepanov | iwyu | 🟢 | none |
| fad2876ec | MarcoFalke | ci | 🟢 | none |
| e67a676df | Lőrinc | fix | 🟢 | none (gap-swept) |
| a89e1618d | fanquake | contrib | 🟢 | none |
| 57a778ed2 | fanquake | depends | 🟢 | none |
| 3e0fd0e4d | Andrew Toth | refactor | 🟢 | none |
| 44b4ee194 | Andrew Toth | validation | 🟢 | none |
| 8fb604323 | Andrew Toth | coins | 🟢 | none — RAII ResetGuard + unit/fuzz coverage |
| 041758f5e | Andrew Toth | coins | 🟢 | none — `SetNull()` ≡ `SetBestBlock(ZERO)`, equivalent |
| 8dd9200fc | Andrew Toth | coins | 🟢 | none — protected `Reset()`, correct semantics |
| efcbf7944 | Hennadii Stepanov | ci | 🟢 | none |
| d3e681bc0 | marcofleon | fuzz | 🟢 | none |
| eeb4d2814 | Lőrinc | validation | 🟢 | none |
| facb2aab2 | MarcoFalke | test | 🟢 | none |
| 6354b4fd7 | furszy | tests | 🟢 | none (gap-swept) |
| 45930a794 | furszy | http-server | 🟢 | none (gap-swept) |
| 4fab35cf8 | Antoine Poinsot | miniscript | 🟢 | none |
| 51abf7d15 | Antoine Poinsot | script | 🟢 | none |
| f2b8acc0e | glozow | remove | 🟢 | none (gap-swept) |
| faba426b3 | MarcoFalke | lint | 🟢 | none (gap-swept) |
| 1111fff91 | MarcoFalke | lint | 🟢 | none (gap-swept) |
| fad042235 | MarcoFalke | refactor | 🟢 | none |
| c8abac994 | ci | ci | 🟢 | none |
| fafdae46f | MarcoFalke | test | 🟢 | none |
| 3400db804 | yancy | doc | 🟢 | none |
| ddae1b4ef | fanquake | ci | 🟢 | none |
| fa43897c1 | MarcoFalke | doc | 🟢 | none (gap-swept) |
| bbbba0fd4 | MarcoFalke | scripted-diff | 🟢 | none (gap-swept) |
| fac541546 | MarcoFalke | refactor | 🟢 | none (gap-swept) |
| fac529188 | MarcoFalke | refactor | 🟢 | none (gap-swept) |
| fa376095a | MarcoFalke | refactor | 🟢 | none (gap-swept) |
| fada83801 | MarcoFalke | refactor | 🟢 | none (gap-swept) |
| fa80cd3ce | MarcoFalke | test | 🟢 | none (gap-swept) |
| d511adb66 | Sjors Provoost | - | 🟡 | fixed by 00d22328b0 (IPC coinbase bad-cb-length at heights ≤16) |
| bf3b5d6d0 | Sjors Provoost | test | 🟢 | none (gap-swept) |
| 78df9003d | Anthony Towns | [doc] | 🟢 | none (gap-swept) |
| e77039208 | Bruno Garcia | test | 🟢 | none |
| 5aeaa71c7 | will | lint | 🟢 | none |
| c17a2adb8 | will | lint | 🟢 | none |
| fa9c92d7b | MarcoFalke | log | 🟢 | none |
| 2fccbea3c | fanquake | secp256k1 | 🟢 | none |
| fa2e1b85d | MarcoFalke | build | 🟢 | none |
| fa06cd4ba | MarcoFalke | doc | 🟢 | none |
| ab649ce45 | janb84 | guix | 🟢 | none |
| 905dfdee8 | fanquake | test | 🟢 | none |
| 1f60ca360 | Martin Zumsande | wallet | 🟢 | none (fixes removeprunedfunds conflicting-tx bug) |
| 7d9e1a810 | stringintech | test | 🟢 | none |
| 3bd98b450 | joaonevess | refactor | 🟢 | none |
| a73a3ec55 | Lőrinc | doc | 🟢 | none |
| eeee3755f | MarcoFalke | fuzz | 🟢 | none |
| 5cd57943b | Yash Bhutwala | test | 🟢 | none (gap-swept) |
| 1b36bf0c5 | Hennadii Stepanov | build | 🟢 | none |
| 9f2b338bc | Hennadii Stepanov | build | 🟢 | none |
| fa15a8d2d | MarcoFalke | doc | 🟢 | none |
| be2b48b9f | fanquake | test | 🟢 | none |
| 43c528aba | rkrux | test | 🟢 | none |
| 6e3a0afc2 | rkrux | wallet | 🟢 | none (itself a fix: gethdkeys map::at throw) |
| 40735450c | Suhas Daftuar | mempool | 🟢 | none |
| 1a8494d16 | Suhas Daftuar | mempool | 🟢 | none |
| db2effaca | David Gumberg | scripted-diff | 🟢 | none (gap-swept) |
| 27e021ebc | David Gumberg | wallet | 🟢 | none (gap-swept) |
| d8bec61be | David Gumberg | wallet | 🟢 | none (gap-swept) |
| f35acc893 | David Gumberg | refactor | 🟢 | none (gap-swept) |
| e12ff8aca | David Gumberg | test | 🟢 | none (gap-swept) |
| 70dbc79b0 | David Gumberg | wallet | 🟢 | none (gap-swept) |
| ae66e0116 | David Gumberg | wallet | 🟢 | none (gap-swept) |
| bc6907041 | David Gumberg | refactor | 🟢 | none (gap-swept) |
| a9d64cd49 | David Gumberg | wallet | 🟢 | none (gap-swept) |
| b4a49cc72 | David Gumberg | wallet | 🟢 | none (gap-swept) |
| b15a94a61 | David Gumberg | refactor | 🟢 | none (gap-swept) |
| fab2f3df4 | MarcoFalke | fuzz | 🟢 | none |
| 1911db8c6 | Matthew Zipkin | util | 🟢 | none |
| ee62405cc | Matthew Zipkin | util | 🟢 | none |
| eea38787b | Matthew Zipkin | util | 🟢 | none |
| 4e300df71 | Matthew Zipkin | util | 🟢 | none |
| 0b0d9125c | Matthew Zipkin | net | 🟢 | none |
| 6f7b4323c | Bruno Garcia | test | 🟢 | none |
| bd31a92d6 | Bruno Garcia | script | 🟢 | none (error-code only, validity unchanged) |
| 0ca4dcd78 | Bruno Garcia | script | 🟢 | none (enum shift safe: not exposed via kernel/ABI) |
| 9a9d797ef | yuvicc | kernel | 🟢 | none |
| 1137debb8 | ismaelsadeeq | doc | 🟢 | none |
| c9ce1c7c4 | billymcbip | test | 🟢 | none |
| e1dc4afee | Fabian Jahr | test | 🟢 | none |
| 8b9d30e3f | Lőrinc | test | 🟢 | none |
| 2845f10a2 | node | test | 🟢 | none |
| fab055c90 | MarcoFalke | test | 🟢 | none |
| b851ff6ca | yuvicc | kernel | 🟢 | none |
| fa61fadad | MarcoFalke | doc | 🟢 | none |
| 0aba464ce | rkrux | test | 🟢 | none |
| 3f5211cba | naiyoma | test | 🟢 | none |
| 7cfe79082 | naiyoma | test | 🟢 | none |
| 5b2c3960b | Sebastian Falbesoner | test | 🟢 | none (gap-swept) |
| 4fec726c4 | Fabian Jahr | refactor | 🟢 | none |
| 79e97d45c | Fabian Jahr | doc | 🟢 | none |
| cf4943fdc | Fabian Jahr | refactor | 🟢 | none |
| 385c34a05 | Fabian Jahr | refactor | 🟢 | none |
| fa41fc6a1 | Fabian Jahr | refactor | 🟢 | none |
| 964c44cdc | Hodlinator | test | 🟢 | none |
| 198bbaee4 | Hodlinator | miniscript | 🟢 | none |
| 50cab8570 | Hodlinator | miniscript | 🟢 | none |
| 15fb34de4 | Hodlinator | miniscript | 🟢 | none |
| e55b23c17 | Hodlinator | miniscript | 🟢 | none |
| c6f798b22 | Hodlinator | miniscript | 🟢 | none |
| 22e411531 | Hodlinator | miniscript | 🟢 | none |
| 34bed0ed8 | woltx | test | 🟢 | none |
| b149a28f6 | Hennadii Stepanov | depends | 🟢 | none |
| ccf9172ab | rustaceanrob | util | 🟢 | none |
| fdc9fe2da | Hennadii Stepanov | ci | 🟢 | none |
| 477c5504e | Lőrinc | coins | 🟢 | none |
| faa18dceb | MarcoFalke | refactor | 🟢 | none |
| 81675a781 | naiyoma | test | 🟢 | none |
| 969c840db | Lőrinc | blocks | 🟢 | none |
| babfda332 | Lőrinc | net | 🟢 | none |
| 1658b8f82 | Lőrinc | refactor | 🟢 | none |
| 75b704df9 | Ava Chow | wallet | 🟢 | none |
| 5875a9c50 | Ava Chow | wallet | 🟢 | none |
| 0dafc0d83 | stickies-v | clang-format | 🟢 | none |
| 03f363d37 | Hennadii Stepanov | doc | 🟢 | none |
| d938947b3 | Hennadii Stepanov | doc | 🟢 | none |
| e1a90bcec | Hennadii Stepanov | iwyu | 🟢 | none |
| 19a2edde5 | Hennadii Stepanov | iwyu | 🟢 | none |
| 9482f00df | Padraic Slattery | chore | 🟢 | none |
| 14f99cfe5 | Lőrinc | - | 🟡 | fixed by e67a676df9 (uptime RPC 0 on first call) |
| faf07bd1a | MarcoFalke | doc | 🟢 | none |
| faf66673a | MarcoFalke | move | 🟢 | none |
| fa6947f49 | MarcoFalke | kernel | 🟢 | none |
| a9440b159 | Lőrinc | util | 🟢 | none |
| d45ec3fba | b-l-u-e | test | 🟢 | none |
| a02c4a82d | David Gumberg | refactor | 🟢 | none (gap-swept) |
| 411caf728 | David Gumberg | wallet | 🟢 | none (gap-swept) |
| a48e23f56 | David Gumberg | refactor | 🟢 | none (gap-swept) |
| a5a8c4139 | Hennadii Stepanov | ci | 🟢 | none |
| fa38ffac6 | MarcoFalke | contrib | 🟢 | none |
| fab8bc030 | MarcoFalke | contrib | 🟢 | none |
| faa5a9eba | MarcoFalke | fuzz | 🟢 | none |
| 0972785fd | David Gumberg | wallet | 🟢 | none (gap-swept) |
| f0a046094 | David Gumberg | scripted-diff | 🟢 | none (gap-swept) |
| de509c6df | Hennadii Stepanov | iwyu | 🟢 | none |
| faa59b367 | MarcoFalke | util | 🟢 | none |
| fabb47e4e | MarcoFalke | util | 🟢 | none |
| d94d7b1a4 | fanquake | guix | 🟢 | none |
| fab972143 | MarcoFalke | util | 🟢 | none |
| fac480095 | MarcoFalke | util | 🟢 | none |
| fa6575d6c | MarcoFalke | util | 🟢 | none |
| fabf8d1c5 | MarcoFalke | fuzz | 🟢 | none |
| fac7fed39 | MarcoFalke | refactor | 🟢 | none |
| 6a8dbf9b9 | frankomosh | net | 🟢 | none |
| 3e340672e | w0xlt | test | 🟢 | none |
| fa64d8424 | MarcoFalke | refactor | 🟢 | none |
| faf0c2d94 | MarcoFalke | refactor | 🟢 | none |
| fac70ea8b | MarcoFalke | fuzz | 🟢 | none |
| fad7bd9ba | MarcoFalke | noui | 🟢 | none |
| fa8ebeb33 | MarcoFalke | gui | 🟢 | none |
| fafe71b74 | MarcoFalke | refactor | 🟢 | none |
| fa8d0088e | MarcoFalke | refactor | 🟢 | none |
| fa9078647 | MarcoFalke | iwyu | 🟢 | none |
| f62568c97 | Hennadii Stepanov | ci | 🟢 | none |
| 1fea3bae5 | Ryan Ofsky | test | 🟢 | none — adds IPC disconnect/busy tests |
| fa3792853 | MarcoFalke | build | 🟢 | none |
| 979d41bfa | Hennadii Stepanov | qa | 🟢 | none |
| b39291f4c | Lőrinc | doc | 🟢 | none |
| c7028d336 | Lőrinc | init | 🟢 | none |
| fa2959e16 | MarcoFalke | test | 🟢 | none |
| 31b771a94 | Lőrinc | net | 🟢 | none |
| fa16b275f | MarcoFalke | test | 🟢 | none — assert EXIT_SUCCESS on interrupt |
| fab7c7f56 | MarcoFalke | test | 🟢 | none — split init_stress_test |
| fa5032f0e | MarcoFalke | ci | 🟢 | none |
| d30ad4a91 | Ava Chow | wallet | 🟢 | none |
| 7fc465ece | ismaelsadeeq | doc | 🟢 | none |
| fa0195499 | MarcoFalke | gui | 🟢 | none |
| eeee1e341 | MarcoFalke | refactor | 🟢 | none |
| 7562e2aee | Ryan Ofsky | subtree | 🟢 | none |
| 2f5b1c5f8 | Lőrinc | psbt | 🟢 | none (fixes OOB-by-one bounds assert) |
| fa3df5271 | MarcoFalke | bench | 🟢 | none |
| fa8938f08 | MarcoFalke | bench | 🟢 | none |
| fa51a28a9 | MarcoFalke | bench | 🟢 | none |
| fa790c3ee | MarcoFalke | bench | 🟢 | none |
| dd904298c | Ava Chow | gui | 🟢 | none |
| 61726483e | Greg Sanders | doc | 🟢 | none |
| da56ef239 | Pieter Wuille | txgraph | 🟢 | none |
| d09a19fd4 | Greg Sanders | test | 🟢 | none |
| a0ca851d2 | Matthew Zipkin | net | 🟢 | none |
| 557b41a38 | Lőrinc | validation | 🟢 | none |
| b9c0ab3b7 | Lőrinc | validation | 🟢 | none |
| 8d531c621 | Lőrinc | validation | 🟢 | none |
| 8be54e3b1 | Lőrinc | test | 🟢 | none |
| f51665bee | tboy1337 | psbt | 🟢 | none — fix: rejects invalid MuSig2 pubkeys at PSBT deserialize (see note) |
| facaf5621 | MarcoFalke | contrib | 🟢 | none |
| 2ee7f9b25 | Andrew Toth | coins | 🟢 | none |
| 44b12cdb1 | Sebastian Falbesoner | doc | 🟢 | none |
| f664860e5 | fanquake | doc | 🟢 | none |
| ce63d37eb | woltx | test | 🟢 | none |
| 8ac134be5 | Ava Chow | contrib | 🟢 | none |
| ab41492c6 | Fabian Jahr | test | 🟢 | none |
| 1412b779a | ismaelsadeeq | refactor | 🟢 | none (behavior-identical hoist of early return) |
| 4c7cfd37a | Greg Sanders | wallet | 🟢 | none (removes erroneous-on-reorg Assume) |
| 8fb5e5f41 | brunoerg | test | 🟢 | none |
| eeaf28dbe | David Gumberg | test | 🟢 | none |
| 997e7b4d7 | sedited | init | 🟢 | none |
| d1b227f3a | sedited | keys | 🟢 | none |
| 2cade5d5d | glozow | miniminer | 🟢 | none (removes bad fee Assumes; negative fees possible) |
| fa8d56f9f | MarcoFalke | fuzz | 🟢 | none |
| fabac1b39 | MarcoFalke | fuzz | 🟢 | none |
| 194114daf | Hennadii Stepanov | guix | 🟢 | none |
| 333333356 | MarcoFalke | fuzz | 🟢 | none |
| cbf0bd35b | furszy | test | 🟢 | none |
| 9c7e4771b | Novo | test | 🟢 | none |
| ed945a685 | Novo | - | 🟡 | fixed by f9273f01db (listdescriptors CHECK_NONFATAL on keyless desc) |
| 9e5e9824f | Novo | descriptor | 🟢 | none |
| 5c4db25b6 | Novo | descriptor | 🟢 | none |
| 2dc74e3f4 | Novo | wallet | 🟢 | none (equivalent: old desc_str was dead store) |
| e842eb90b | Novo | descriptor | 🟢 | none |
| f78f6f1dc | Ava Chow | wallet | 🟢 | none (removes dangerous fs::remove_all) |
| a3c71c720 | w0xlt | test | 🟢 | none |
| b7c34d08d | furszy | test | 🟢 | none |
| 82caa8193 | furszy | wallet | 🟢 | none |
| d70b159c4 | furszy | wallet | 🟢 | none |
| f011e0f06 | furszy | test | 🟢 | none |
| 36093bde6 | furszy | test | 🟢 | none |
| f4c7e28e8 | furszy | wallet | 🟢 | none (fixes migration erasing all wallets) |
| 4ed0693a3 | furszy | wallet | 🟢 | none |
| eec551aaf | Andrew Toth | fuzz | 🟢 | none |
| 3e4155fce | Andrew Toth | test | 🟢 | none |
| ee1e40f58 | Lőrinc | txdb | 🟢 | none |
| 2a746500f | fanquake | ci | 🟢 | none |
| fb0e6edfe | Mara van der Laan | guix | 🟢 | none |
| 34909799f | fanquake | guix | 🟢 | none |
| 47be9122a | fanquake | guix | 🟢 | none |
| ea29329eb | fanquake | guix | 🟢 | none |
| 6f54e267d | fanquake | guix | 🟢 | none |
| 7735901ed | fanquake | guix | 🟢 | none |
| 301d9eea6 | WakeTrainDev | qt | 🟢 | none |
| 5b7bf47f9 | Sebastian Falbesoner | doc | 🟢 | none |
| 1808b5aaf | Pieter Wuille | txgraph | 🟢 | none |
| 34a77138b | Pieter Wuille | txgraph | 🟢 | none |
| 3380e0cbb | Pieter Wuille | txgraph | 🟢 | none |
| 62dd88624 | Pieter Wuille | txgraph | 🟢 | none |
| 01ffcf464 | Pieter Wuille | txgraph | 🟢 | none |
| fa578d943 | MarcoFalke | move | 🟢 | none |
| fa392c31e | MarcoFalke | move | 🟢 | none |
| fab0cfa98 | MarcoFalke | move | 🟢 | none |
| fa3e48e3f | MarcoFalke | move | 🟢 | none |
| fad09e77d | MarcoFalke | move | 🟢 | none |
| faf40c2f8 | MarcoFalke | move | 🟢 | none |
| 5805a8b54 | rkrux | psbt | 🟢 | none (hardening: reject invalid MuSig2 pubkeys) |
| 792e2edf5 | 0xb10c | net | 🟢 | none |
| 31852057e | 0xb10c | test | 🟢 | none |
| 48f57bb35 | Sjors Provoost | mining | 🟢 | none |
| 4ce3f4a26 | Sebastian Falbesoner | rpc | 🟢 | none |
| c6ca2b85a | Pieter Wuille | validation | 🟢 | none (FORCE_SYNC keeps cache coherent) |
| 7099e93d0 | Lőrinc | refactor | 🟢 | none |
| 9b57c8d2b | brunoerg | test | 🟢 | none |
| bd730cb0a | fanquake | doc | 🟢 | none |
| c5825d4b7 | Hennadii Stepanov | qa | 🟢 | none |
| fa65bc0e7 | MarcoFalke | test | 🟢 | none |
| fa9fdbce7 | MarcoFalke | test | 🟢 | none |
| 08ed802ba | bensig | doc | 🟢 | none |
| 84d8c5266 | Hennadii Stepanov | doc | 🟢 | none |
| 77c9b3c08 | Robin David | test | 🟢 | none |
| 658d38106 | Lőrinc | policy | 🟢 | none — dead `require_sorted` param removed; both callers passed true |
| b23b90136 | fanquake | doc | 🟢 | none |
| 95ef0fc5e | Greg Sanders | test | 🟢 | none |
| 25e84d377 | Greg Sanders | test | 🟢 | none |
| 337b4a236 | flack | doc | 🟢 | none |
| fab300b37 | MarcoFalke | test | 🟢 | none |
| 11ce5cf79 | David Gumberg | wallet | 🟢 | none — scripted-diff, IsCrypted ≡ HasEncryptionKeys |
| b76253875 | billymcbip | test | 🟢 | none |
| 44e006d43 | Josh Doman | kernel | 🟢 | none — C-API change; asserts on consumer misuse only (pre-existing pattern); taproot-missing-spent-outputs still guarded |
| fa727e3ec | MarcoFalke | test | 🟢 | none |
| 3dd815f04 | Lőrinc | consensus | 🟢 | none (merkle: reserve+push_back ≡ resize+assign) |
| 7fd47e0e5 | Lőrinc | bench | 🟢 | none |
| fac5a1b10 | MarcoFalke | test | 🟢 | none |
| fafbc70d4 | MarcoFalke | rpc | 🟢 | none |
| b261100e7 | WakeTrainDev | qt | 🟢 | none |
| d3a479cb0 | TheCharlatan | kernel | 🟢 | none |
| d69a582e7 | TheCharlatan | kernel | 🟢 | none |
| e44dec027 | Greg Sanders | doc | 🟢 | none |
| 552bc82b1 | Anurag chavan | doc | 🟢 | none |
| ba6315d2f | fanquake | contrib | 🟢 | none |
| 3e4765ee1 | fanquake | doc | 🟢 | none |
| fa66e2d07 | MarcoFalke | rpc | 🟢 | none — cast removals are type-safe, output values unchanged |
| fab1f4b80 | MarcoFalke | rpc | 🟢 | none — *fixes* `(int)` truncation of large chunk fees |
| d59b4cdb5 | Sjors Provoost | mining | 🟢 | none |
| fa1d17d56 | MarcoFalke | refactor | 🟢 | none |
| 1e94e562f | Lőrinc | refactor | 🟢 | none |
| fd9f1accb | Lőrinc | build | 🟢 | none — reverts boost::multi_index::contains for old Boost |
| 75bdb925f | Pieter Wuille | clusterlin | 🟢 | none — SFL simplification, fuzz-covered |
| 91399a791 | Pieter Wuille | clusterlin | 🟢 | none |
| 5ce280074 | Pieter Wuille | clusterlin | 🟢 | none — privacy randomization; PostLinearize restores determinism of chunking |
| 13aad26b7 | Pieter Wuille | clusterlin | 🟢 | none — SFL RNG; Improve() per-call work-bounding change is intentional |
| ddbfa4dfa | Pieter Wuille | clusterlin | 🟢 | none |
| 3efc94d65 | Pieter Wuille | clusterlin | 🟢 | none — LIMO→SFL swap with differential fuzz tests |
| 6a8fa821b | Pieter Wuille | clusterlin | 🟢 | none — LoadLinearization fuzz-tested incl. non-topological input |
| da48ed9f3 | Pieter Wuille | clusterlin | 🟢 | none — test helper |
| c461259fb | Pieter Wuille | clusterlin | 🟢 | none — SFL state + dedicated fuzz target |
| f480c1e71 | Hennadii Stepanov | build | 🟢 | none |
| 95bfe7d57 | Pieter Wuille | bench | 🟢 | none |
| 86dd550a9 | Pieter Wuille | test | 🟢 | none |
| 7b5d256af | stringintech | test | 🟢 | none |
| 2bc326564 | stringintech | validation | 🟢 | none (fixes AddChainstate null-mempool assert) |
| 5f3d6bdb6 | stringintech | chainstate | 🟢 | none |
| aeb7ccb93 | fanquake | doc | 🟢 | none |
| 68a7cb8f8 | fanquake | contrib | 🟢 | none |
| 85314dc0b | Lőrinc | kernel | 🟢 | none |
| fa4cb13b5 | MarcoFalke | test | 🟢 | none |
| 719158db5 | fanquake | depends | 🟢 | none |
| 1841bf9cb | 0xb10c | test | 🟢 | none |
| 1ed8e7616 | rkrux | rpc | 🟢 | none — doc-only |
| 6bb66fccc | billymcbip | test | 🟢 | none |
| 0067abe15 | stringintech | net | 🟢 | none |
| cd98caea4 | Woolfgm | ci | 🟢 | none — checkout v5→v6 |
| 56750c4f8 | Hennadii Stepanov | iwyu | 🟢 | none |
| 2c78814e0 | Hennadii Stepanov | ci | 🟢 | none |
| 94e4f04d7 | Hennadii Stepanov | cmake | 🟢 | none |
| 0f81e0051 | Hennadii Stepanov | cmake | 🟢 | none |
| 73f7844cd | Hennadii Stepanov | iwyu | 🟢 | none |
| 7a65437e2 | Hennadii Stepanov | iwyu | 🟢 | none |
| facd3d56c | MarcoFalke | log | 🟢 | none — SourceLocation stores string_view of `__func__` static literal; lifetime safe |
| f46e3ec0f | Hennadii Stepanov | net | 🟢 | none |
| caf4843a5 | brunoerg | fuzz | 🟢 | none |
| fa5ed16aa | MarcoFalke | refactor | 🟢 | none (move-only MAX_BLOCK_TIME_GAP to qt) |
| 356883f0e | Hodlinator | qa | 🟢 | none |
| 7427a03b5 | Hodlinator | qa | 🟢 | none |
| d7f703c1f | Hodlinator | qa | 🟢 | none |
| 69bcfcad8 | Hodlinator | qa | 🟢 | none |
| fb43b2f8c | Hodlinator | qa | 🟢 | none |
| 59b93f11e | Roman Zeyde | rest | 🟢 | none (error message text) |
| 7fe94a049 | Roman Zeyde | test | 🟢 | none |
| fa5f29774 | MarcoFalke | doc | 🟢 | none — scripted-diff headers |
| faa8ee62f | MarcoFalke | ci | 🟢 | none |
| 893722130 | Vasil Dimov | doc | 🟢 | none |
| 582016fa5 | Vasil Dimov | test | 🟢 | none |
| e74d54e04 | Vasil Dimov | test | 🟢 | none |
| 818b780a0 | Vasil Dimov | rpc | 🟢 | none |
| eab595f9c | Vasil Dimov | net | 🟢 | none (lock order safe: m_mutex leaf, cs_main not held) |
| 37b79f9c3 | Vasil Dimov | net | 🟢 | none |
| 2de53eee7 | Vasil Dimov | net | 🟢 | none |
| 30a9853ad | Vasil Dimov | net | 🟢 | none |
| d1092e5d4 | Vasil Dimov | net | 🟢 | none |
| 9937a12a2 | Vasil Dimov | net | 🟢 | none |
| a098f37b9 | Vasil Dimov | net | 🟢 | none |
| 679ce3a0b | Vasil Dimov | net | 🟢 | none |
| a3faa6f94 | Vasil Dimov | node | 🟢 | none (no hang: wait_callback enqueues regardless) |
| 95c051e21 | Vasil Dimov | net | 🟢 | none |
| bb49d2603 | Vasil Dimov | net | 🟢 | none |
| 01dad4efe | Vasil Dimov | net | 🟢 | none |
| 94aaa5d31 | Vasil Dimov | init | 🟢 | none |
| d6ee490e0 | Vasil Dimov | log | 🟢 | none |
| db2d39f64 | Eugene Siegel | fuzz | 🟢 | none |
| 45f5b2dac | Martin Zumsande | fuzz | 🟢 | none |
| c011e3aa5 | Martin Zumsande | test | 🟢 | none |
| 55d0d19b5 | Roman Zeyde | test | 🟢 | none |
| 89eb53102 | Roman Zeyde | doc | 🟢 | none |
| 6da6f503a | TheCharlatan | coins | 🟢 | none — BatchWrite/Flush/Sync always returned true; dropped FatalError was dead code |
| 41118e17f | Roman Zeyde | blockstorage | 🟢 | none (SaturatingAdd bounds check verified equivalent & overflow-safe) |
| 599effdea | Roman Zeyde | rest | 🟢 | none (formatting) |
| 5ac357952 | rustaceanrob | refactor | 🟢 | none |
| fa8a5d215 | MarcoFalke | log | 🟢 | none |
| fac24bbec | MarcoFalke | test | 🟢 | none |
| f27316766 | stickies-v | ipc | 🟢 | none (logging) |
| 94c51ae54 | stickies-v | libevent | 🟢 | none (logging) |
| b3046cca7 | stratospher | doc | 🟢 | none (gap-swept) |
| a70a14a3f | marcofleon | clusterlin | 🟢 | none |
| ce29d7d62 | marcofleon | fuzz | 🟢 | none (itself a fix to fuzz check) |
| 876e2849b | marcofleon | fuzz | 🟢 | none (itself a fix to fuzz loop bounds) |
| 09dfa4d3f | stratospher | test | 🟢 | none |
| 82be652e4 | Ryan Ofsky | doc | 🟢 | none |
| af455dcb3 | TheCharlatan | validation | 🟢 | none (GetPruneRange move is verbatim) |
| ae85c495f | Ryan Ofsky | validation | 🟢 | none (refactor) |
| 6a572dbda | Ryan Ofsky | validation | 🟢 | none (refactor) |
| 491d827d5 | Ryan Ofsky | validation | 🟢 | none (refactor) |
| e514fe611 | Ryan Ofsky | validation | 🟢 | none (refactor) |
| ee3525068 | Ryan Ofsky | validation | 🟢 | none (refactor) |
| d9e82299f | Ryan Ofsky | validation | 🟢 | none (refactor) |
| 4dfe38391 | Ryan Ofsky | validation | 🟢 | none (refactor) |
| 352ad27fc | Ryan Ofsky | validation | 🟢 | none (refactor) |
| a229cb947 | Ryan Ofsky | validation | 🟢 | none (refactor) |
| a9b7f5614 | Ryan Ofsky | validation | 🟢 | none (refactor) |
| 840bd2ef2 | Ryan Ofsky | validation | 🟢 | none (refactor) |
| 1598a15ae | Ryan Ofsky | validation | 🟢 | none (ActivateSnapshot dedup verified equivalent) |
| 9fe927b6d | Ryan Ofsky | validation | 🟢 | none (refactor) |
| 6082c8471 | Ryan Ofsky | validation | 🟢 | none (refactor) |
| de00e8754 | Ryan Ofsky | test | 🟢 | none |
| e76e88658 | David Gumberg | guix | 🟢 | none |
| ea1be3867 | David Gumberg | guix | 🟢 | none |
| fa904fc68 | MarcoFalke | lint | 🟢 | none |
| 14371fd1f | Ava Chow | gui | 🟢 | none |
| f11a7d248 | Ava Chow | gui | 🟢 | none |
| 16ab6dfc1 | Ava Chow | gui | 🟢 | none (move-only) |
| 4ec2d18a0 | Ava Chow | wallet | 🟢 | none (parameter plumbing) |
| 07135290c | Roman Zeyde | rest | 🟢 | none (/blockpart endpoint; localhost-bound REST, query parsing & error paths verified) |
| 4e2af1c06 | Roman Zeyde | blockstorage | 🟢 | none (partial read bounds: `size==0 \ |
| f2fd1aa21 | Roman Zeyde | blockstorage | 🟢 | none (ReadRawBlock error-code refactor; all callers updated) |
| f0a218310 | Lőrinc | test | 🟢 | none |
| 7e9de20c0 | Lőrinc | fuzz | 🟢 | none |
| fa1de1103 | MarcoFalke | util | 🟢 | none |
| faa109f8b | MarcoFalke | test | 🟢 | none |
| fad4a9fe2 | MarcoFalke | lint | 🟢 | none |
| cdaf25f9c | Ava Chow | test | 🟢 | none |
| ca4a844ee | fanquake | depends | 🟢 | none |
| fa75480c8 | MarcoFalke | test | 🟢 | none |
| e71c4df16 | ANtutov | refactor | 🟢 | none |
| 5f5c1ea01 | Anthony Towns | net | 🟢 | none (cached -capturemessages; test setter provided) |
| 6eb5ba569 | Lőrinc | crypto | 🟢 | none (SipHashState extraction; consistency-tested) |
| 118d22ddb | Lőrinc | blockencodings | 🟢 | none (cached presalted hasher; new Assert matches pre-existing FillShortTxIDSelector-first invariant, not network-reachable) |
| 9ca52a4cb | Lőrinc | crypto | 🟢 | none (SipHashUint256→PresaltedSipHasher; hash-consistency unit+fuzz tested) |
| ec11b9fed | Lőrinc | crypto | 🟢 | none (PresaltedSipHasher intro; state math verified vs CSipHasher) |
| 20330548c | Lőrinc | crypto | 🟢 | none (constants move) |
| 9f9eb7fbc | Lőrinc | test | 🟢 | none |
| d4d184eda | 0xb10c | log | 🟢 | none |
| e7ac5a133 | fanquake | doc | 🟢 | none |
| c4c70a256 | Carl Dong | netbase | 🟢 | none ("tor" alias removal; deprecated long ago, fails loudly at init) |
| fa89f60e3 | MarcoFalke | log | 🟢 | none (scripted-diff) |
| fa6c7a195 | MarcoFalke | log | 🟢 | none (scripted-diff) |
| d8fe5f032 | Ryan Ofsky | test | 🟢 | none |
| a5e61b191 | Ryan Ofsky | test | 🟢 | none |
| cea443e24 | Anthony Towns | net | 🟢 | none (time units consistent before/after) |
| faa23738f | MarcoFalke | refactor | 🟢 | none |
| fa114be27 | MarcoFalke | util | 🟢 | none (vendored std::expected) |
| 57b888ce0 | Chandra Pratap | fuzz | 🟢 | none |
| fa4395dff | MarcoFalke | log | 🟢 | none |
| fa05181d9 | MarcoFalke | log | 🟢 | none (scripted-diff LogPrintf→LogInfo) |
| 5646e6c0d | Martin Zumsande | index | 🟢 | none |
| 032f3503e | Martin Zumsande | index | 🟢 | none — dedup preserves exact logic |
| a67d3eb91 | Martin Zumsande | index | 🟢 | none — same on-disk key format ('s'/'t' prefixes preserved) |
| ff06e2468 | brunoerg | init | 🟢 | none (help text) |
| ded11fb04 | Ryan Ofsky | test | 🟢 | none |
| 0a8d303d6 | Greg Sanders | test | 🟢 | none — fixes broken mempool_cluster test itself |
| d9319b06c | Lőrinc | refactor | 🟢 | none — count→contains semantics verified per-container |
| 039307554 | Lőrinc | refactor | 🟢 | none |
| 8bb9219b6 | Lőrinc | refactor | 🟢 | none |
| e9536faae | fanquake | contrib | 🟢 | none |
| 4b4711369 | Martin Zumsande | validation | 🟢 | none (fork-warning reword + startup call; warning-only) |
| 2f51951d0 | Martin Zumsande | p2p | 🟢 | none (warning log only) |
| 710031ebe | H. Stepanov | guix | 🟢 | none (revert paired with 286f3e49c) |
| 4cf5ea6c3 | H. Stepanov | depends | 🟢 | none |
| 938312d7a | crStiv | doc | 🟢 | none |
| cb7d5bfe4 | A. R. Zubeldia | test | 🟢 | none |
| 7a365244f | A. R. Zubeldia | test | 🟢 | none |
| 76c092ff8 | Sjors Provoost | wallet | 🟢 | none — adds older()>65535 import warning; traversal iterative (no recursion) |
| b8d279a81 | Suhas Daftuar | doc | 🟢 | none |
| aba7500a3 | Suhas Daftuar | rpc | 🟢 | none (param name fix) |
| 6c1325a09 | Suhas Daftuar | rpc | 🟢 | none (output rename + docs) |
| bc2eb931d | Suhas Daftuar | policy | 🟢 | none (lock annotations) |
| 957ae2324 | Suhas Daftuar | doc | 🟢 | none |
| d97d6199c | Suhas Daftuar | doc | 🟢 | none |
| a1b341ef9 | Suhas Daftuar | mempool | 🟢 | none (check()-only sanity assert) |
| 23d6f457c | Suhas Daftuar | rpc | 🟢 | none |
| d2dcd37aa | Suhas Daftuar | mempool | 🟢 | none (mutable fee + txgraph SetTransactionFee kept in sync at both sites) |
| d84ffc24d | Suhas Daftuar | doc | 🟢 | none |
| b0417ba94 | Suhas Daftuar | doc | 🟢 | none |
| 804329400 | Greg Sanders | fuzz | 🟢 | none |
| 2d88966e4 | Suhas Daftuar | miner | 🟢 | none (rename) |
| 6f3e8eb30 | Suhas Daftuar | miner | 🟢 | none |
| b5f245f6f | Suhas Daftuar | mempool | 🟢 | none (dead constant removal) |
| 1dac54d50 | Suhas Daftuar | test | 🟢 | none |
| 04f65488c | Suhas Daftuar | policy | 🟢 | none |
| 634291a7d | Suhas Daftuar | policy | 🟢 | none |
| fc18ef1f3 | Suhas Daftuar | mempool | 🟢 | none (limit removal, epic-coordinated) |
| ed8e81912 | Suhas Daftuar | init | 🟢 | none (deprecation warnings) |
| 80d8df2d4 | Suhas Daftuar | mempool | 🟢 | none |
| 9292570f4 | Suhas Daftuar | mempool | 🟢 | none |
| 3e39ea8c3 | Suhas Daftuar | mempool | 🟢 | none (removeForReorg via GetDescendantsUnion; self-inclusion of GetDescendants verified on master) |
| a3c31dfd7 | Suhas Daftuar | mempool | 🟢 | none (scripted rename) |
| a5a7905d8 | Suhas Daftuar | mempool | 🟢 | none (removeRecursive simplification verified vs txgraph API) |
| 01d852003 | Suhas Daftuar | mempool | 🟢 | none (dead arg) |
| fe1815d48 | H. Stepanov | cmake | 🟢 | none |
| 49c672853 | H. Stepanov | cmake | 🟢 | none |
| ec8eb013a | H. Stepanov | doc | 🟢 | none |
| 48496caa1 | H. Stepanov | ci | 🟢 | none |
| b5a7a685b | Hodlinator | ci | 🟢 | none |
| 9d5021a05 | billymcbip | script | 🟢 | none (new error enum for an already-rejected consensus case; no consensus change — same rejection path, new label; tests added) |
| 7b90b4f5b | fanquake | guix | 🟢 | none |
| 41e657aac | fanquake | guix | 🟢 | none |
| ef4ce19a1 | fanquake | depends | 🟢 | none |
| fa45a1503 | MarcoFalke | log | 🟢 | none |
| fa0018d01 | MarcoFalke | log | 🟢 | none |
| e7e51952d | Hodlinator | contrib | 🟢 | none |
| 592157b75 | Sjors Provoost | test | 🟢 | none |
| 22229de72 | MarcoFalke | doc | 🟢 | none |
| 81e763f1e | Anthony Towns | wallet | 🟢 | none |
| 167df7a98 | Eugene Siegel | net | 🟢 | none (itself a fix: UAF on m_reconnections grant at shutdown; fix live on master at src/net.cpp:3741) |
| 52230a7f6 | S. Falbesoner | test | 🟢 | none |
| fd4ce5512 | Fabian Jahr | contrib | 🟢 | none |
| 1488315d7 | Greg Sanders | policy | 🟢 | none — intentional minrelay relaxation for packages, release-noted (e44dec027) |
| e07e57368 | will | ci | 🟢 | none |
| 3e01b5d0e | fanquake | contrib | 🟢 | none |
| c1213a35a | fanquake | macdeploy | 🟢 | none |
| a33d03454 | fanquake | contrib | 🟢 | none |
| fad611858 | MarcoFalke | test | 🟢 | none |
| fab085c15 | MarcoFalke | contrib | 🟢 | none |
| fa71c15f8 | MarcoFalke | doc | 🟢 | none |
| fae612424 | MarcoFalke | contrib | 🟢 | none |
| fa7d72bd1 | MarcoFalke | lint | 🟢 | none |
| faf39d853 | MarcoFalke | test | 🟢 | none |
| fa83e3a81 | MarcoFalke | lint | 🟢 | none |
| 70d9e8f0a | yuvicc | test | 🟢 | none |
| 540ed333f | yuvicc | test | 🟢 | none |
| 217dbbbb5 | Fabian Jahr | test | 🟢 | none |
| 2909655fb | Fibonacci747 | interfaces | 🟢 | none (exists() takes the lock internally) |
| fa336053a | MarcoFalke | ci | 🟢 | none |
| fa83555d1 | MarcoFalke | ci | 🟢 | none |
| eeee02ea5 | MarcoFalke | ci | 🟢 | none |
| fa21fd1dc | MarcoFalke | ci | 🟢 | none |
| fa37559ac | MarcoFalke | ci | 🟢 | none |
| 666675e95 | MarcoFalke | ci | 🟢 | none |
| 3e4355314 | fanquake | depends | 🟢 | none |
| 04eb84fe3 | fanquake | depends | 🟢 | none |
| bc64013e6 | Suhas Daftuar | mempool | 🟢 | none (dead variable) |
| ade0397f5 | Anthony Towns | txgraph | 🟢 | none |
| c9519c260 | Fabian Jahr | musig | 🟢 | none — Assert on secnonce session-id reuse is defensive; session ids are locally generated, not attacker-controlled |
| e755614be | Fabian Jahr | sign | 🟢 | none — removed assert was duplicate of caller-side check |
| 0f7f0692c | Fabian Jahr | musig | 🟢 | none |
| 2e27bd9c3 | H. Stepanov | ci | 🟢 | none |
| 8343a9ffc | S. Falbesoner | test | 🟢 | none |
| c34bc01b2 | dergoegge | doc | 🟢 | none |
| 675be9302 | stratospher | cli | 🟢 | none (gap-swept) |
| fa9537cde | MarcoFalke | ci | 🟢 | none |
| 17cf9ff7e | Suhas Daftuar | mempool | 🟢 | none (-maxmempool bound now vs cluster limit; =0 allowed, FlushStateToDisk sharing semantics preserved) |
| 315e43e5d | Suhas Daftuar | mempool | 🟢 | none (check()-only) |
| de2e9a24c | Suhas Daftuar | test | 🟢 | none |
| 4ef4ddb50 | Suhas Daftuar | doc | 🟢 | none |
| 79f73ad71 | Suhas Daftuar | mempool | 🟢 | none (check()-only) |
| a86ac1176 | Suhas Daftuar | doc | 🟢 | none |
| 9567eaa66 | Suhas Daftuar | mempool | 🟢 | none (DoWork invoked after Apply/removeForReorg/removeForBlock; GetWorstMainChunk does MakeAllAcceptable synchronously — no TrimToSize livelock) |
| bd130db99 | H. Stepanov | ci | 🟢 | none |
| 0672e727b | H. Stepanov | gui | 🟢 | none (revert) |