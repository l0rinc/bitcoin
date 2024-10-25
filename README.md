## Bitcoin Core contributions

### Mission

Having contributed to other open source projects before (e.g. Gradle, JavaSlang, Scala and the ChatGPT tokenizer lately),
I consider Bitcoin to be the most important project I can work on, and I'm committed to making both its usage and development more accessible.

I love diving deep into performance optimizations and improving code quality, especially when it comes to speeding up Initial Block Download times for users.
Having seen countless frustrated users express their concerns on Twitter and Telegram groups, I understand how critical this first experience is for Bitcoin adoption.

When I work with Bitcoin Core's codebase, I explore hundreds of possible approaches until I find solutions that really fit the project's distinct needs.

Benchmarking is one way I weigh different trade-offs and find that sweet spot between competing priorities.
* [`[IBD] Tracking PR for speeding up Initial Block Download`](https://github.com/bitcoin/bitcoin/pull/32043#issue-2914501684)

I'm constantly re-examining the balance points in Bitcoin Core's code, taking into account how improvements stack up over time and how requirements evolve.

When the data shows we need to shift direction, I make sure to advocate for changes that align with both our new insights and Bitcoin Core's high standards and established trade-offs.
* [`refactor: Allow CScript's operator<< to accept spans, not just vectors`](https://github.com/bitcoin/bitcoin/pull/30765/files#diff-e20339c384d6f19b519ea2de7f4ba4fed92a36d66a80f0339b09927c3fa38d6dR73-R77)

I believe strongly in writing code that's not just fast, but also maintainable.
Having experienced how inconsistent code patterns and practices can distract from actual development, I understand how this can discourage new contributors.
I always try to "leave the campground a bit cleaner than I found it," focusing on readability and long-term stability to make the codebase more welcoming for future developers.
* [`refactor: migrate bool GetCoin to return optional<Coin>`](https://github.com/bitcoin/bitcoin/pull/30849/files#diff-a8f78513bc27f9bf679eead54819a8e5be720401c6ae40858da226a66ca002e2R149-R154)

I thoroughly analyze and dissect other PRs to truly understand how they work.
Code reviews provide focused opportunities to explore solutions that align with the project's goals.
During reviews, I propose alternative approaches and back them up with data to ensure our decisions are driven by performance metrics rather than just familiarity.
* [`Don't empty dbcache on prune flushes: >30% faster IBD`](https://github.com/bitcoin/bitcoin/pull/28280#discussion_r1659971494)

I do my best work in small, focused teams where everyone brings deep expertise to the table.
I value working alongside knowledgeable peers who help me maintain perspective on broader implications while I explore intricate implementation details.
* [`build: optimize .h generation in GenerateHeaderFrom{Raw,Json}.cmake`](https://github.com/bitcoin/bitcoin/pull/30888#pullrequestreview-2302909740)

### Key Bitcoin Core Pull Requests

Between February 2024 and April 2025, I've had [`13` open](https://github.com/bitcoin/bitcoin/pulls?q=author%3Al0rinc+is%3Aopen+sort%3Acomments-desc) and [`37` merged Pull Requests](https://github.com/bitcoin/bitcoin/pulls?q=author%3Al0rinc+is%3Amerged+sort%3Acomments-desc), including:
* Optimizations (achieving an ~18% cumulative IBD speedup)
  * [`[IBD] flush UTXOs in bigger batches`](https://github.com/bitcoin/bitcoin/pull/31645)
  * [`[IBD] batch block reads/writes during AutoFile serialization`](https://github.com/bitcoin/bitcoin/pull/31551)
  * [`[IBD] multi-byte block obfuscation`](https://github.com/bitcoin/bitcoin/pull/31144#issue-2610689777)
  * [`[IBD] precalculate SipHash constant salt calculations`](https://github.com/bitcoin/bitcoin/pull/30442)
  * [`[IBD] specialize block serialization`](https://github.com/bitcoin/bitcoin/pull/31868)
  * [`[IBD] specialize CheckBlock's input & coinbase checks`](https://github.com/bitcoin/bitcoin/pull/31682)
  * [`optimization: Moved repeated -printpriority fetching out of AddToBlock`](https://github.com/bitcoin/bitcoin/pull/30324#pullrequestreview-2150128035)
  * [`optimization: Reduce cache lookups in CCoinsViewCache::FetchCoin`](https://github.com/bitcoin/bitcoin/pull/30326#pullrequestreview-2161585202)
  * [`optimization: reserve memory allocation for transaction inputs/outputs`](https://github.com/bitcoin/bitcoin/pull/30093#discussion_r1599564471)
* Refactoring and code quality improvements
  * [`refactor: Allow CScript's operator<< to accept spans, not just vectors`](https://github.com/bitcoin/bitcoin/pull/30765/files#diff-e20339c384d6f19b519ea2de7f4ba4fed92a36d66a80f0339b09927c3fa38d6dR73-R77)
  * [`refactor: Preallocate result in TryParseHex to avoid resizing`](https://github.com/bitcoin/bitcoin/pull/29458#pullrequestreview-1898513246)
  * [`refactor: Reduce memory copying operations in bech32 encoding`](https://github.com/bitcoin/bitcoin/pull/29607#issuecomment-2105655507)
  * [`refactor: prohibit direct flags access in CCoinsCacheEntry and remove invalid tests`](https://github.com/bitcoin/bitcoin/pull/30906)
  * [`refactor: use original log string when no suspicious chars found`](https://github.com/bitcoin/bitcoin/pull/31923)
  * [`refactor: modernize outdated trait patterns using helper aliases (C++14/C++17)`](https://github.com/bitcoin/bitcoin/pull/31904)
  * [`refactor: inline UndoWriteToDisk and WriteBlockToDisk to reduce serialization calls`](https://github.com/bitcoin/bitcoin/pull/31490)
  * [`refactor: migrate bool GetCoin to return optional<Coin>`](https://github.com/bitcoin/bitcoin/pull/30849/files#diff-a8f78513bc27f9bf679eead54819a8e5be720401c6ae40858da226a66ca002e2R149-R154)
  * [`coins: warn on shutdown for big UTXO set flushes`](https://github.com/bitcoin/bitcoin/pull/31534)
* Test and infrastructure changes
  * [`build: optimize .h generation in GenerateHeaderFrom{Raw,Json}.cmake`](https://github.com/bitcoin/bitcoin/pull/30888#pullrequestreview-2302909740)
  * [`test: support std::optional in BOOST_CHECK_* and increase FromUserHex fuzz feature coverage`](https://github.com/bitcoin/bitcoin/pull/30618#pullrequestreview-2277400199)
  * [`test: Validate oversized transactions or without inputs`](https://github.com/bitcoin/bitcoin/pull/29862#discussion_r1643298389)
  * [`test: generalize HasReason and use it in FailFmtWithError`](https://github.com/bitcoin/bitcoin/pull/30921)
  * [`fuzz: provide more realistic values to the base58(check) decoders`](https://github.com/bitcoin/bitcoin/pull/31917)
  * [`build: Use character literals for generated json headers to avoid narrowing`](https://github.com/bitcoin/bitcoin/pull/31547)
* Documentation changes
  * [`doc: update documentation and scripts related to build directories`](https://github.com/bitcoin/bitcoin/pull/30741#pullrequestreview-2268432242)
  * [`doc: fix a few likely documentation typos related to CMake migration`](https://github.com/bitcoin/bitcoin/pull/30734)
  * [`doc: replace -? with -h and -help`](https://github.com/bitcoin/bitcoin/pull/31118#pullrequestreview-2381587440)
  * [`doc: Fix invalid txid in gettransaction RPC example`](https://github.com/bitcoin/bitcoin/pull/31610)
  * [`doc: shallow clone qa-assets`](https://github.com/bitcoin/bitcoin/pull/32083)
  * [`doc: update translation generation cmake example`](https://github.com/bitcoin/bitcoin/pull/31731) 

### Reviews

Some of my work was inspired while reviewing others' contributions:
* [`Andrew Toth`](https://github.com/andrewtoth)'s IBD and coins optimizations:
  * [`Don't empty dbcache on prune flushes: >30% faster IBD`](https://github.com/bitcoin/bitcoin/pull/28280#discussion_r1659971494)
  * [`coins: remove logic for spent-and-FRESH cache entries and writing non-DIRTY entries`](https://github.com/bitcoin/bitcoin/pull/30673#discussion_r1721727681)
  * [`refactor: prohibit direct flags access in CCoinsCacheEntry and remove invalid tests`](https://github.com/bitcoin/bitcoin/pull/30906#issue-2526919287)
  * [`validation: write chainstate to disk every hour`](https://github.com/bitcoin/bitcoin/pull/30611#discussion_r1740812194)
* [`Sipa`](https://github.com/sipa)'s FeeFrac:
  * [`feefrac: add support for evaluating at given size`](https://github.com/bitcoin/bitcoin/pull/30535#discussion_r1698947273)
  * [`Use number of dirty cache entries in flush warnings/logs`](https://github.com/bitcoin/bitcoin/pull/31703#pullrequestreview-2620965099)
* [`Josie`](https://github.com/josibake)'s Silent payments:
  * [`refactor: Model the bech32 charlimit as an Enum`](https://github.com/bitcoin/bitcoin/pull/30047#discussion_r1597108664)
  * [`fuzz: replace hardcoded numbers for bech32 limits`](https://github.com/bitcoin/bitcoin/pull/30596/files#r1705375642)
  * [`crypto, refactor: add new KeyPair class`](https://github.com/bitcoin/bitcoin/pull/30051#discussion_r1702776795)
* [`Marco`](https://github.com/maflcko)'s Mempool obfuscation, cleanups, heavy refactors:
  * [`[IBD] multi-byte block obfuscation`](https://github.com/bitcoin/bitcoin/pull/31144#issue-2610689777)
  * [`fuzz: Speed up PickValue in txorphan`](https://github.com/bitcoin/bitcoin/pull/30474#discussion_r1682771932)
  * [`util: Use consteval checked format string in FatalErrorf, LogConnectFailure`](https://github.com/bitcoin/bitcoin/pull/30546#pullrequestreview-2272243082)
  * [`log: Enforce trailing newline`](https://github.com/bitcoin/bitcoin/pull/30929#discussion_r1778728317)
  * [`refactor: Use std::span over Span`](https://github.com/bitcoin/bitcoin/pull/31519#pullrequestreview-2511711814)
* [`Hodlinator`](https://github.com/hodlinator)'s ParseHex cleanups and endianness documentations:
  * [`refactor: Replace ParseHex with consteval ""_hex literals`](https://github.com/bitcoin/bitcoin/pull/30377#discussion_r1723595926)
  * [`fix: Make TxidFromString() respect string_view length`](https://github.com/bitcoin/bitcoin/pull/30436#discussion_r1686332889)
  * [`doc: Correct uint256 hex string endianness`](https://github.com/bitcoin/bitcoin/pull/30526#issuecomment-2254461008)
* [`Stickies`](https://github.com/stickies-v)' Hex changes:
  * [`node: reduce unsafe uint256S usage`](https://github.com/bitcoin/bitcoin/pull/30569#discussion_r1702863593)
  * [`refactor: remove deprecated TxidFromString() in favour of transaction_identifier::FromHex()`](https://github.com/bitcoin/bitcoin/pull/30532#discussion_r1699879316)
  * [`Remove unsafe uint256S() and test-only uint160S()`](https://github.com/bitcoin/bitcoin/pull/30773#discussion_r1741081872)
* [`Hebasto`](https://github.com/hebasto)'s CMake migration:
  * [`build: Introduce CMake-based build system`](https://github.com/bitcoin/bitcoin/pull/30454#issuecomment-2267493946)
* [`LarryRuane`](https://github.com/LarryRuane)'s memory accounting fixes
  * [`improve MallocUsage() accuracy`](https://github.com/bitcoin/bitcoin/pull/28531#pullrequestreview-2697495377) 

* Other benchmarking and reviewing work:
  * [`validation: write chainstate to disk every hour`](https://github.com/bitcoin/bitcoin/pull/30611#issuecomment-2408917989)
  * [`refactor: Add consteval uint256 constructor`](https://github.com/bitcoin/bitcoin/pull/30560#discussion_r1702493873)
  * [`Don't zero-after-free DataStream: Faster IBD on some configurations`](https://github.com/bitcoin/bitcoin/pull/30987#issuecomment-2407036182)
  * [`validation: fetch block inputs on parallel threads 10% faster IBD`](https://github.com/bitcoin/bitcoin/pull/31132)
  * [`coins: remove logic for spent-and-FRESH cache entries and writing non-DIRTY entries`](https://github.com/bitcoin/bitcoin/pull/30673)
  * [`CCoinMap Experiments`](https://github.com/bitcoin/bitcoin/pull/32128)

### Additional Projects & Tools Affecting Bitcoin Core Development

- [**bitcoin-dev-tools/benchcoin**](https://github.com/bitcoin-dev-tools/benchcoin/pulls?q=author%3Al0rinc)
  – 70+ PRs for measuring potential speed gains - some of them were performance experiments, others improved the benchmarking infrastructure 
- [**llvm/llvm-project #127811**](https://github.com/llvm/llvm-project/pull/127811)
  – Updated outdated trait patterns in compiler-rt
- [**bitcoin-data/stale-blocks**](https://github.com/bitcoin-data/stale-blocks/pulls?q=author%3Al0rinc)
  – Collected more stale block data
- [**maflcko/DrahtBot**](https://github.com/maflcko/DrahtBot/pulls?q=author%3Al0rinc)
  – Summaries now include performance benchmarks
- [**google/leveldb #1259**](https://github.com/google/leveldb/pull/1259) 
  – Introduced `WriteBatch::Reserve` for fixed-size batch usage

### Other Contributions & Activities

- **[Spark Sessions (Feb 2025 Jamaica Core Dev)](https://gist.github.com/l0rinc/dc8c5e5289de2d801df16f4468b5a58c)**
  - Presented on macro benchmarking methodologies and IBD speedup efforts.

- **[Hajdúszoboszló Bitcoin Conference](https://hotelatlantis.hu/en/packs/bitcoinwhale)**
  - Covered Bitcoin Core development processes, privacy considerations, and code contributions.

- **[BitCan (DIY Seed Storage)](https://bitcan.world/#durable_setup)**
  - Extended the guide to include a home-made (fireproof) dogtag engraving option.

### Appendix

<details>
<summary>80 Merged Commits</summary>

> git --no-pager log --author="lorinc" --no-merges --pretty=format:"%h %ad %s" --date=format:"%Y-%m-%d"

* [`2025-03-17 doc: shallow clone qa-assets`](https://github.com/bitcoin/bitcoin/commit/6f9f415a4f)
* [`2025-03-06 makeseeds: regex improvements`](https://github.com/bitcoin/bitcoin/commit/236687083f)
* [`2025-02-20 fuzz: make sure DecodeBase58(Check) is called with valid values more often`](https://github.com/bitcoin/bitcoin/commit/d5537c18a9)
* [`2025-02-20 fuzz: Always restrict base conversion input lengths`](https://github.com/bitcoin/bitcoin/commit/bad1433ef2)
* [`2025-02-19 scripted-diff: modernize outdated trait patterns - values`](https://github.com/bitcoin/bitcoin/commit/ab2b67fce2)
* [`2025-02-19 scripted-diff: modernize outdated trait patterns - types`](https://github.com/bitcoin/bitcoin/commit/8327889f35)
* [`2025-02-19 refactor: modernize remaining outdated trait patterns`](https://github.com/bitcoin/bitcoin/commit/4cd95a2921)
* [`2025-01-24 doc: update translation generation cmake example`](https://github.com/bitcoin/bitcoin/commit/758a93d621)
* [`2025-01-09 scripted-diff: rename block and undo functions for consistency`](https://github.com/bitcoin/bitcoin/commit/223081ece6)
* [`2025-01-09 refactor,bench: rename bench/readblock.cpp to bench/readwriteblock.cpp`](https://github.com/bitcoin/bitcoin/commit/34f9a0157a)
* [`2025-01-09 bench: add SaveBlockBench`](https://github.com/bitcoin/bitcoin/commit/86b85bb11f)
* [`2025-01-07 init,log: Unify block index and chainstate loading log line`](https://github.com/bitcoin/bitcoin/commit/e04be3731f)
* [`2024-12-27 fuzz: Add fuzzing for max_ret_len in DecodeBase58/DecodeBase58Check`](https://github.com/bitcoin/bitcoin/commit/f919d919eb)
* [`2024-12-20 build: Use character literals for generated headers to avoid narrowing`](https://github.com/bitcoin/bitcoin/commit/63b6b638aa)
* [`2024-12-18 refactor,blocks: remove costly asserts and modernize affected logs`](https://github.com/bitcoin/bitcoin/commit/baaa3b2846)
* [`2024-12-18 refactor,blocks: inline WriteBlockToDisk`](https://github.com/bitcoin/bitcoin/commit/dfb2f9d004)
* [`2024-12-18 refactor,blocks: inline UndoWriteToDisk`](https://github.com/bitcoin/bitcoin/commit/42bc491465)
* [`2024-12-18 refactor,blocks: deduplicate block's serialized size calculations`](https://github.com/bitcoin/bitcoin/commit/fa39f27a0f)
* [`2024-12-18 coins: warn on shutdown for big UTXO set flushes`](https://github.com/bitcoin/bitcoin/commit/5709718b83)
* [`2024-11-24 refactor: Fix remaining clang-tidy performance-unnecessary-copy-initialization errors`](https://github.com/bitcoin/bitcoin/commit/3305972f7b)
* [`2024-11-18 refactor: Reserve vectors in fuzz tests`](https://github.com/bitcoin/bitcoin/commit/11f3bc229c)
* [`2024-11-18 refactor: Preallocate PrevectorFillVector(In)Direct without vector resize`](https://github.com/bitcoin/bitcoin/commit/152fefe7a2)
* [`2024-11-17 refactor: Fix remaining clang-tidy performance-inefficient-vector errors`](https://github.com/bitcoin/bitcoin/commit/a774c7a339)
* [`2024-10-31 coins, refactor: Make AddFlags, SetDirty, SetFresh static`](https://github.com/bitcoin/bitcoin/commit/fc8c282022)
* [`2024-10-31 coins, refactor: Assume state after SetClean in AddFlags to prevent dangling pointers`](https://github.com/bitcoin/bitcoin/commit/6b733699cf)
* [`2024-10-21 Change default help arg to `-help` and mention `-h` and `-?` as alternatives`](https://github.com/bitcoin/bitcoin/commit/33a28e252a)
* [`2024-10-19 doc: replace `-?` with `-h` for bench_bitcoin help`](https://github.com/bitcoin/bitcoin/commit/f0130ab1a1)
* [`2024-09-18 test: Validate error messages on fail`](https://github.com/bitcoin/bitcoin/commit/c0b4b2c1ee)
* [`2024-09-18 test: generalize HasReason and use it in FailFmtWithError`](https://github.com/bitcoin/bitcoin/commit/6c3c619b35)
* [`2024-09-18 test, refactor: Compact ccoins_access and ccoins_spend`](https://github.com/bitcoin/bitcoin/commit/50cce20013)
* [`2024-09-17 test, refactor: Remove remaining unbounded flags from coins_tests`](https://github.com/bitcoin/bitcoin/commit/0a159f0914)
* [`2024-09-16 test, refactor: Migrate GetCoinsMapEntry to return MaybeCoin`](https://github.com/bitcoin/bitcoin/commit/ca74aa7490)
* [`2024-09-15 test: Group values and states in tests into CoinEntry wrappers`](https://github.com/bitcoin/bitcoin/commit/d5f8d607ab)
* [`2024-09-13 coins, refactor: Split up AddFlags to remove invalid states`](https://github.com/bitcoin/bitcoin/commit/cd0498eabc)
* [`2024-09-13 coins, refactor: Remove direct GetFlags access`](https://github.com/bitcoin/bitcoin/commit/15aaa81c38)
* [`2024-09-12 build: Minimize I/O operations in GenerateHeaderFromRaw.cmake`](https://github.com/bitcoin/bitcoin/commit/aa003d1568)
* [`2024-09-12 build: Minimize I/O operations in GenerateHeaderFromJson.cmake`](https://github.com/bitcoin/bitcoin/commit/2a581144f2)
* [`2024-09-10 prevector: avoid GCC bogus warnings in insert method`](https://github.com/bitcoin/bitcoin/commit/c78d8ff4cb)
* [`2024-09-09 Replace CScript _hex_v_u8 appends with _hex`](https://github.com/bitcoin/bitcoin/commit/5e190cd11f)
* [`2024-09-09 Allow CScript's operator<< to accept spans, not just vectors`](https://github.com/bitcoin/bitcoin/commit/cac846c2fb)
* [`2024-09-08 refactor: Return optional of Coin in GetCoin`](https://github.com/bitcoin/bitcoin/commit/46dfbf169b)
* [`2024-09-08 refactor: Remove unrealistic simulation state`](https://github.com/bitcoin/bitcoin/commit/e31bfb26c2)
* [`2024-09-07 refactor: Rely on returned value of GetCoin instead of parameter`](https://github.com/bitcoin/bitcoin/commit/4feaa28728)
* [`2024-09-01 Use BOOST_CHECK_EQUAL for optional, arith_uint256, uint256, uint160`](https://github.com/bitcoin/bitcoin/commit/19947863e1)
* [`2024-09-01 Add std::optional support to Boost's equality check`](https://github.com/bitcoin/bitcoin/commit/743ac30e34)
* [`2024-08-29 test: Fuzz Base32/Base58/Base64 roundtrip conversions`](https://github.com/bitcoin/bitcoin/commit/635bc58f46)
* [`2024-08-29 test: Extend base58_encode_decode.json with edge cases`](https://github.com/bitcoin/bitcoin/commit/5dd3a0d8a8)
* [`2024-08-28 util: Add consteval ""_hex[_v][_u8] literals`](https://github.com/bitcoin/bitcoin/commit/5b74a849cf)
* [`2024-08-28 Update spelling.ignore-words`](https://github.com/bitcoin/bitcoin/commit/837fbca036)
* [`2024-08-28 doc: Prepend 'build/' to binary paths under 'src/' in docs`](https://github.com/bitcoin/bitcoin/commit/6a68343ffb)
* [`2024-08-28 Compare FromUserHex result against other hex validators and parsers`](https://github.com/bitcoin/bitcoin/commit/1eac96a503)
* [`2024-08-22 doc: Update documentation generation example in developer-notes.md`](https://github.com/bitcoin/bitcoin/commit/91b3bc2b9c)
* [`2024-08-22 doc: fix a few simple codespell warnings`](https://github.com/bitcoin/bitcoin/commit/f9a08f35a5)
* [`2024-08-18 Remove unused src_dir param from run_tests`](https://github.com/bitcoin/bitcoin/commit/2ad560139b)
* [`2024-08-18 Fix a few likely documentation typos`](https://github.com/bitcoin/bitcoin/commit/7ee5c3c5b2)
* [`2024-08-18 Extend possible debugging fixes with file-name-only`](https://github.com/bitcoin/bitcoin/commit/1b0b9b4c78)
* [`2024-08-18 Add gdb and lldb links to debugging troubleshooting`](https://github.com/bitcoin/bitcoin/commit/cb7c5ca824)
* [`2024-08-10 Split out bech32 separator char to header`](https://github.com/bitcoin/bitcoin/commit/c1a5d5c100)
* [`2024-08-10 Fuzz HRP of bech32 as well`](https://github.com/bitcoin/bitcoin/commit/9b7023d31a)
* [`2024-07-22 Fix lint-spelling warnings`](https://github.com/bitcoin/bitcoin/commit/bccfca0382)
* [`2024-06-30 Moved the repeated -printpriority fetching out of AddToBlock`](https://github.com/bitcoin/bitcoin/commit/323ce30308)
* [`2024-06-27 Refactor SipHash_32b benchmark to improve accuracy and avoid optimization issues`](https://github.com/bitcoin/bitcoin/commit/42066f45ff)
* [`2024-06-23 Reduce cache lookups in CCoinsViewCache::FetchCoin`](https://github.com/bitcoin/bitcoin/commit/204ca67bba)
* [`2024-06-18 Replace hard-coded constant in test`](https://github.com/bitcoin/bitcoin/commit/969e047cfb)
* [`2024-05-13 Reserve space for transaction outputs in CreateTransactionInternal`](https://github.com/bitcoin/bitcoin/commit/c76aaaf900)
* [`2024-05-13 Reserve space for transaction inputs in CreateTransactionInternal`](https://github.com/bitcoin/bitcoin/commit/ec585f11c3)
* [`2024-04-24 Validate oversized transaction`](https://github.com/bitcoin/bitcoin/commit/327a31d1a4)
* [`2024-04-12 Validate transaction without inputs`](https://github.com/bitcoin/bitcoin/commit/1984187840)
* [`2024-04-12 Use SCRIPT_VERIFY_NONE instead of hard-coded 0 in transaction_tests`](https://github.com/bitcoin/bitcoin/commit/c3a8843189)
* [`2024-04-10 test: Add padding tests for Base32/Base64`](https://github.com/bitcoin/bitcoin/commit/ae40cf1a8e)
* [`2024-04-09 Change MAC_OSX macro to __APPLE__ in crypto package`](https://github.com/bitcoin/bitcoin/commit/a71eadf66b)
* [`2024-03-09 Reserve hrp memory in Decode and LocateErrors`](https://github.com/bitcoin/bitcoin/commit/d5ece3c4b5)
* [`2024-03-09 refactor: replace hardcoded numbers`](https://github.com/bitcoin/bitcoin/commit/7f3f6c6dc8)
* [`2024-03-09 Reduce memory copying operations in bech32 encode`](https://github.com/bitcoin/bitcoin/commit/07f64177a4)
* [`2024-03-09 Preallocate addresses in GetAddr based on nNodes`](https://github.com/bitcoin/bitcoin/commit/66082ca348)
* [`2024-03-08 Reserve memory for ToLower/ToUpper conversions`](https://github.com/bitcoin/bitcoin/commit/6f2f4a4d09)
* [`2024-02-25 Preallocate result in `TryParseHex` to avoid resizing`](https://github.com/bitcoin/bitcoin/commit/a19235c14b)
* [`2024-02-23 Fix CI-detected codespell warnings`](https://github.com/bitcoin/bitcoin/commit/b03b20685a)
* [`2024-02-20 Add benchmark for TryParseHex`](https://github.com/bitcoin/bitcoin/commit/b7489ecb52)
* [`2024-02-19 build: Replace MAC_OSX macro with existing __APPLE__`](https://github.com/bitcoin/bitcoin/commit/6c6b2442ed)

</details>

<details>
<summary>14 Merged Co-authored Commits</summary>

> git --no-pager log --grep="lorinc" --no-merges --pretty=format:"%h %ad %s" --date=format:"%Y-%m-%d"

* [`2024-12-06 test: Prove+document ConstevalFormatString/tinyformat parity`](https://github.com/bitcoin/bitcoin/commit/533013cba2)
* [`2024-12-05 test: Add missing %c character test`](https://github.com/bitcoin/bitcoin/commit/c93bf0e6e2)
* [`2024-10-28 util: Support dynamic width & precision in ConstevalFormatString`](https://github.com/bitcoin/bitcoin/commit/184f34f2d0)
* [`2024-08-21 test refactor: util_tests - parse_hex clean up`](https://github.com/bitcoin/bitcoin/commit/dc5f6f6812)
* [`2024-08-03 bench: add benchmark for signing with a taptweak`](https://github.com/bitcoin/bitcoin/commit/f14900b6e4)
* [`2024-07-24 doc: Update for CMake-based build system`](https://github.com/bitcoin/bitcoin/commit/6ce50fd9d0)
* [`2024-07-21 tests: add key tweak smoke test`](https://github.com/bitcoin/bitcoin/commit/5d507a0091)
* [`2024-07-17 fuzz: Speed up PickValue in txorphan`](https://github.com/bitcoin/bitcoin/commit/fa33a63bd9)
* [`2024-07-17 coins: pass linked list of flagged entries to BatchWrite`](https://github.com/bitcoin/bitcoin/commit/7825b8b9ae)
* [`2024-06-28 test: add cache entry linked list tests`](https://github.com/bitcoin/bitcoin/commit/a14edada8a)
* [`2024-06-28 refactor: encapsulate flags access for dirty and fresh checks`](https://github.com/bitcoin/bitcoin/commit/df34a94e57)
* [`2024-06-28 coins: track flagged cache entries in linked list`](https://github.com/bitcoin/bitcoin/commit/24ce37cb86)
* [`2024-04-20 test: Add unit tests for urlDecode`](https://github.com/bitcoin/bitcoin/commit/46bc6c2aaa)
* [`2024-03-09 refactor: replace hardcoded numbers`](https://github.com/bitcoin/bitcoin/commit/7f3f6c6dc8)

</details>