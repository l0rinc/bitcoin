# Compiler optimization investigation

This is a local performance investigation report. It records whether aggressive
compiler optimization exposed simple Bitcoin Core source changes that could
improve normal GCC Release builds without changing release compiler flags.

## Conclusion

Twenty source optimizations were retained after follow-up investigation:

- byte `prevector` deserialization now handles the common case whose serialized
  size fits within the existing 5 MiB per-read allocation cap without entering
  the chunk loop;
- byte `prevector` deserialization reuses the decoded size for the read span
  immediately after `resize_uninitialized()`;
- byte `std::vector` deserialization mirrors that same single-read path when
  the encoded size fits within the allocation cap;
- byte `std::vector` deserialization reuses the decoded size for the read span
  immediately after `resize()`;
- multi-byte CompactSize encodings are staged into a small stack buffer and
  written with one stream write instead of separate prefix and payload writes.
- multi-byte VarInt encodings are now generated in output order and written
  once, while keeping the one-byte path direct.
- `DataStream::read` and `DataStream::ignore` now check requested bytes against
  the available byte count directly instead of using `CheckedAdd`.
- `ReadCompactSize` now returns the single-byte encoding directly before the
  multibyte canonical and range checks.
- deserialized `std::map` and `std::set` temporaries are moved into hinted
  inserts instead of copied.
- the dominant one-byte CompactSize and VarInt paths are annotated as likely.
- the dominant one-byte VarInt read path is favored by marking the continuation
  branch unlikely.
- `ReadVarInt` is forced inline so formatter reads keep the hot decode path in
  the caller.
- `ReadCompactSize` is forced inline so common one-byte size decodes stay in
  the caller.
- `ser_readdata8` is forced inline so the byte reads under CompactSize and
  VarInt decoders stay in those hot callers.
- `WriteCompactSize` is forced inline so common size encodes stay in the
  caller.
- the retained byte `prevector` and `std::vector` single-read paths are
  annotated as likely, while empty byte containers are annotated as unlikely.
- non-empty byte `prevector` and `std::vector` serialization paths are
  annotated as likely.
- `WriteCompactSize` now shortcuts to `SizeComputer::seek()` through
  `ParamsStream` wrappers when only serialized size is being computed.
- `WriteVarInt` now shortcuts to `SizeComputer::seek()` through `ParamsStream`
  wrappers when only serialized size is being computed.
- `GetSizeOfVarInt` uses the continuation condition directly instead of a
  `while(true)` loop with an internal break.

The best isolated signal came from temporary script-level benchmarks on block
413567: inline scriptPubKey deserialization improved by 18.38%, heap scriptSig
deserialization in the 76..252 byte range improved by 17.85%, and heap
scriptSig deserialization in the 253..254 byte range improved by 12.36%. In the
full production benchmark this translated into a 1.84% `DeserializeBlockTest`
median improvement against the alternating clean baseline.

`ReadBlockBench` was treated as layout and I/O sensitive during the `prevector`
work. The final combined patch set did not reproduce the earlier slowdown, but
the workload is noisy enough that the strongest evidence is still the stable
deserialization and block-write improvements rather than a single read-block
timing row.

GCC `-O3` still found a broader oracle win in block read/deserialization
benchmarks, but most of that came from broad inlining and code layout changes in
templated serialization/deserialization paths. The retained patches capture the
narrow parts that stayed reviewable under normal GCC `-O2`.

## Environment

- Repository: `10ca73c02cbff59f2134c0c7da3b8d0a7e727475`
- OS: Linux `6.14.0-1013-raspi`, `aarch64`
- CPU: Cortex-A76, 4 CPUs, fixed 2400 MHz, performance governor
- GCC: Ubuntu GCC 14.2.0
- Clang: Ubuntu Clang 22.0.0
- Linker: GNU ld 2.44
- `ld.lld`: unavailable
- CMake: 3.31.6
- Ninja: 1.12.1
- hyperfine: 1.19.0
- perf: 6.14.11

`python3 -m pyperf system show` reported a performance governor and fixed CPU
frequency. Benchmark commands were pinned to CPU 3 with `taskset -c 3`.

## Builds

Baseline GCC Release build:

```bash
cmake -S . -B build-gcc-base -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_BENCH=ON \
  -DBUILD_TESTS=ON \
  -DBUILD_GUI=OFF \
  -DENABLE_WALLET=OFF \
  -DBUILD_DAEMON=OFF \
  -DBUILD_CLI=OFF \
  -DBUILD_TX=OFF \
  -DBUILD_UTIL=OFF \
  -DCMAKE_C_COMPILER=gcc \
  -DCMAKE_CXX_COMPILER=g++
cmake --build build-gcc-base --target bench_bitcoin test_bitcoin -j4
```

GCC `-O3` oracle build:

```bash
cmake -S . -B build-gcc-o3 -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_BENCH=ON \
  -DBUILD_TESTS=OFF \
  -DBUILD_GUI=OFF \
  -DENABLE_WALLET=OFF \
  -DBUILD_DAEMON=OFF \
  -DBUILD_CLI=OFF \
  -DBUILD_TX=OFF \
  -DBUILD_UTIL=OFF \
  -DBUILD_BITCOIN_BIN=OFF \
  -DCMAKE_C_COMPILER=gcc \
  -DCMAKE_CXX_COMPILER=g++ \
  -DAPPEND_CFLAGS=-O3 \
  -DAPPEND_CXXFLAGS=-O3 \
  -DSECP256K1_APPEND_CFLAGS=-O3
cmake --build build-gcc-o3 --target bench_bitcoin -j4
```

Clang `-O3` oracle build:

```bash
cmake -S . -B build-clang-o3 -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_BENCH=ON \
  -DBUILD_TESTS=OFF \
  -DBUILD_GUI=OFF \
  -DENABLE_WALLET=OFF \
  -DBUILD_DAEMON=OFF \
  -DBUILD_CLI=OFF \
  -DBUILD_TX=OFF \
  -DBUILD_UTIL=OFF \
  -DBUILD_BITCOIN_BIN=OFF \
  -DCMAKE_C_COMPILER=clang \
  -DCMAKE_CXX_COMPILER=clang++ \
  -DAPPEND_CFLAGS=-O3 \
  -DAPPEND_CXXFLAGS=-O3 \
  -DSECP256K1_APPEND_CFLAGS=-O3
cmake --build build-clang-o3 --target bench_bitcoin -j4
```

## Workloads

The investigation used at most three workloads:

- `VerifyScriptP2WPKH`
- `ReadBlockBench`
- `DeserializeBlockTest`

`ConnectBlock*` benchmarks were screened first, but were too expensive for the
full oracle comparison loop. Important paths not deeply investigated include
full IBD/reindex, full validation, UTXO flush, mempool admission, compact block
relay, block download/send, net processing, assumeutxo, libbitcoinkernel, PGO,
AutoFDO, BOLT, and ThinLTO.

Each final benchmark row below used 10 pinned invocations and nanobench's
internal `median(elapsed)` value. Process-level runtime was not used for speedup
claims because `bench_bitcoin -min-time` intentionally runs to a target duration.

Example loop:

```bash
for build in gcc-base gcc-o3 clang-o3; do
  for i in 1 2 3 4 5 6 7 8 9 10; do
    taskset -c 3 build-$build/bin/bench_bitcoin \
      -filter=^ReadBlockBench$ \
      -min-time=500 \
      -output-json=/tmp/readblock_${build}_run${i}.json \
      >/tmp/readblock_${build}_run${i}.txt
  done
done
```

## Benchmark results

| Workload | GCC O2 baseline | GCC O3 oracle | Clang O3 oracle |
| --- | ---: | ---: | ---: |
| `VerifyScriptP2WPKH` | 155088.42 ns/script, MAD 36.02 | 154550.00 ns/script, MAD 17.65, -0.35% | 153386.90 ns/script, MAD 25.25, -1.10% |
| `ReadBlockBench` | 3915.73 us/op, MAD 13.21 | 3822.25 us/op, MAD 19.36, -2.39% | 3914.94 us/op, MAD 59.00, flat |
| `DeserializeBlockTest` | 2754.00 us/block, MAD 16.06 | 2696.65 us/block, MAD 4.16, -2.08% | 2829.13 us/block, MAD 3.46, +2.73% |

Instruction and branch counts moved in the same direction for the GCC `-O3`
block workloads:

| Workload | Build | Instructions | Branches | Branch misses |
| --- | --- | ---: | ---: | ---: |
| `ReadBlockBench` | GCC O2 | 15637725.92 | 2681455.08 | 13867.88 |
| `ReadBlockBench` | GCC O3 | 14895855.36 | 2545638.64 | 13586.94 |
| `DeserializeBlockTest` | GCC O2 | 14587023.11 | 2381959.11 | 13374.73 |
| `DeserializeBlockTest` | GCC O3 | 14331547.06 | 2346997.11 | 13308.37 |

## Retained prevector optimization

Block 413567 contains 8468 serialized `CScript` prevectors:

| Script field | Count | Median bytes | Mean bytes | Max bytes | Inline count |
| --- | ---: | ---: | ---: | ---: | ---: |
| `scriptSig` | 4887 | 107 | 135.53 | 254 | 0 |
| `scriptPubKey` | 3581 | 25 | 24.59 | 30 | 3581 |

The `scriptSig` distribution is split between 4261 scripts in the 76..252 byte
range and 626 scripts in the 253..254 byte range. Every script size in this
block is below the existing 5 MiB per-read cap in `prevector` deserialization,
so the chunk loop only executed one iteration for these scripts.

Temporary script-level benchmarks were added locally and removed before commit.
They measured the prevector deserialization work directly:

| Temporary workload | Clean GCC O2 | Patched GCC O2 | Result |
| --- | ---: | ---: | ---: |
| Inline scriptPubKey prevectors | 15.630 ns/script | 12.756 ns/script | -18.38% |
| Heap scriptSig prevectors, 76..252 bytes | 39.861 ns/script | 32.746 ns/script | -17.85% |
| Heap scriptSig prevectors, 253..254 bytes | 42.139 ns/script | 36.930 ns/script | -12.36% |
| Mutable block deserialization | 832.899 us/block | 814.122 us/block | -2.25% |

The production benchmark used the saved clean GCC O2 binary from the original
investigation and the final patched GCC O2 binary:

| Workload | Clean baseline | Patched | Result |
| --- | ---: | ---: | ---: |
| `DeserializeBlockTest` | 2757.548 us/block, MAD 4.541 | 2706.907 us/block, MAD 18.851 | -1.84% |
| `ReadBlockBench` | 4050.154 us/op, MAD 17.753 | 4007.321 us/op, MAD 19.050 | -1.06% |

The cleaner alternating run before the final rebuild had `ReadBlockBench` at
+0.08%, effectively flat. That is why the retained conclusion relies on the
stable deserialization improvement and treats `ReadBlockBench` as non-regressed
within noise, not as a primary win.

## Retained byte vector optimization

After the `prevector` cleanup, byte `std::vector` deserialization still entered
the chunk loop even when the encoded size fit in one permitted read. The
retained change mirrors the `prevector` fast path: empty vectors return
immediately, single-chunk vectors resize once and read once, and the existing
chunk loop remains for oversized serialized lengths.

Alternating pinned benchmark results against the retained
`prevector + CompactSize + VarInt + DataStream + CompactSize read` baseline:

| Workload | Baseline | Patched | Result |
| --- | ---: | ---: | ---: |
| `DeserializeBlockTest` | 2681.178 us/block, MAD 0.938 | 2668.535 us/block, MAD 6.069 | -0.47% |
| `ReadBlockBench` | 3873.898 us/op, MAD 12.961 | 3880.407 us/op, MAD 11.171 | +0.17% |

Median instructions dropped from 14,199,666 to 14,191,030 for
`DeserializeBlockTest`, and from 15,663,583 to 15,659,597 for `ReadBlockBench`.
The read-block timing movement was treated as noise because the instruction
count still moved slightly in the expected direction and the source change is a
small cleanup that matches the retained `prevector` path.

## Retained prevector read-size reuse

The byte `prevector` single-read path already knows the decoded element count
as `nSize`. Immediately after `resize_uninitialized(nSize)`, the retained
cleanup uses that value for the read span instead of calling `v.size()` again.

Temporary per-size benchmark using `prevector<36, uint8_t>` and a fresh
`SpanReader` each iteration:

| Encoded size | Baseline | Patched | Result |
| --- | ---: | ---: | ---: |
| 1 | 10.5654 ns/op | 10.0222 ns/op | -5.14% |
| 25 | 10.5345 ns/op | 10.0606 ns/op | -4.50% |
| 35 | 11.0295 ns/op | 10.1861 ns/op | -7.65% |
| 36 | 10.0847 ns/op | 9.64085 ns/op | -4.40% |
| 37 | 27.5819 ns/op | 27.1378 ns/op | -1.61% |
| 107 | 28.6133 ns/op | 27.7249 ns/op | -3.11% |
| 254 | 32.7568 ns/op | 31.6371 ns/op | -3.42% |

The zero-size row was measured too, but the retained line is not reached for
empty prevectors, so that row was treated as harness noise.

## Retained byte vector read-size reuse

The byte `std::vector` single-read path has the same shape as the `prevector`
path: after `resize(nSize)`, the decoded size can be reused directly for the
read span. The retained change is a one-line cleanup that avoids asking the
container for a value the caller just set.

Temporary per-size benchmark using `std::vector<uint8_t>` and a fresh
`SpanReader` each iteration:

| Encoded size | Baseline | Patched | Result |
| --- | ---: | ---: | ---: |
| 1 | 29.3326 ns/op | 29.4784 ns/op | +0.50% |
| 25 | 36.7228 ns/op | 36.6296 ns/op | -0.25% |
| 35 | 36.7392 ns/op | 36.4750 ns/op | -0.72% |
| 36 | 36.5214 ns/op | 36.6935 ns/op | +0.47% |
| 37 | 36.3994 ns/op | 36.9012 ns/op | +1.38% |
| 107 | 38.6092 ns/op | 37.8101 ns/op | -2.07% |
| 254 | 40.9207 ns/op | 40.5055 ns/op | -1.01% |

The timing signal was mixed at very small sizes, so this was retained as a
small readability cleanup with measurable wins in the larger temporary rows.

## Retained CompactSize optimization

The transaction hash serialization profile still showed visible time under
`WriteCompactSize` after the `prevector` patch. For CompactSize values that need
a marker byte, the old code emitted the marker and little-endian payload with
separate stream writes. Block 413567 has 626 `scriptSig` lengths in the
253..254 byte range, so transaction hashing and block serialization repeatedly
paid that extra tiny write.

The retained change stages the marker plus payload in a small stack buffer and
writes it once. The one-byte CompactSize path is unchanged.

Alternating pinned benchmark results against the retained `prevector` baseline:

| Workload | Prevector baseline | CompactSize patch | Result |
| --- | ---: | ---: | ---: |
| `DeserializeBlockTest` | 2694.041 us/block, MAD 5.140 | 2675.437 us/block, MAD 8.518 | -0.69% |
| `ReadBlockBench` | 3993.744 us/op, MAD 23.497 | 3954.876 us/op, MAD 11.704 | -0.97% |
| `WriteBlockBench` | 1370.763 us/op, MAD 11.135 | 1331.327 us/op, MAD 20.100 | -2.88% |
| `ReadRawBlockBench` | 287.999 us/op, MAD 2.262 | 286.976 us/op, MAD 1.991 | -0.36% |

The patch reduced median instructions in `DeserializeBlockTest` from
14,280,558 to 14,198,603 and in `WriteBlockBench` from 3,941,006 to 3,867,821.
The cost was about 22 KiB of additional `.text` in `bench_bitcoin` because
`WriteCompactSize` is a widely instantiated template.

## Retained VarInt write optimization

A follow-up review of a serialization-specialization patch stack identified a
small `WriteVarInt` cleanup worth keeping. The original code built the encoded
bytes in reverse order and then wrote them one byte at a time. The retained
change keeps the one-byte VarInt path direct, builds multi-byte encodings in
output order inside the same stack buffer, and writes the resulting span once.

The fixed-threshold VarInt variants from the reviewed patch stack were not
retained because they add more branch thresholds and magic constants than this
general batching change.

Temporary focused benchmark:

| Workload | Baseline | Patched | Result |
| --- | ---: | ---: | ---: |
| Mixed VarInt writes | 6918.08 ns/op | 6875.66 ns/op | -0.61% |

Alternating pinned `bench_bitcoin` results against the retained
`prevector + CompactSize` baseline:

| Workload | Baseline | VarInt patch | Result |
| --- | ---: | ---: | ---: |
| `DeserializeBlockTest` | 2679.383 us/block, MAD 8.065 | 2683.728 us/block, MAD 10.671 | +0.16% |
| `WriteBlockBench` | 1396.129 us/op, MAD 9.861 | 1338.126 us/op, MAD 25.915 | -4.16% |
| `LinearizeOptimallyTotal` | 191.956 us/op, MAD 0.108 | 191.979 us/op, MAD 0.119 | +0.01% |
| `LinearizeOptimallyPerCost` | 18475.150 us/op, MAD 2.280 | 18474.174 us/op, MAD 1.672 | flat |

The existing repository benchmarks were mostly neutral, but the change reduced
`bench_bitcoin` `.text` by about 49 KiB and simplified the multibyte write path.
It was retained under the lower bar for tiny, clean serialization changes.

## Retained DataStream bounds-check cleanup

`DataStream` maintains `m_read_pos <= vch.size()`, and its public `size()`
already reports the available byte count after the read position. The old
`read` and `ignore` paths used `CheckedAdd(m_read_pos, n)` and then compared
the result against `vch.size()`. The retained cleanup compares the request
directly against `size()` and then increments `m_read_pos` after the bounds
check.

Temporary focused benchmark:

| Workload | Baseline | Patched | Result |
| --- | ---: | ---: | ---: |
| `DataStream::read`, 8x32 bytes | 94.152 ns/op | 84.161 ns/op | -10.61% |
| `DataStream::ignore`, 8x32 bytes | 89.316 ns/op | 84.135 ns/op | -5.80% |

The change removes the `util/overflow.h` include from `streams.h`. It increased
the final `bench_bitcoin` `.text` size relative to the VarInt-only build, but
the source is simpler and the focused stream benchmark improved.

## Retained CompactSize read cleanup

CompactSize values below 253 are encoded in one byte and cannot violate the
`MAX_SIZE` range check. The retained cleanup returns those values directly after
reading the first byte, leaving the canonical and range checks on the multibyte
paths only.

Alternating pinned benchmark results against the retained
`prevector + CompactSize + VarInt + DataStream` baseline:

| Workload | Baseline | Patched | Result |
| --- | ---: | ---: | ---: |
| `DeserializeBlockTest` | 2672.046 us/block, MAD 4.647 | 2669.416 us/block, MAD 10.078 | -0.10% |
| `ReadBlockBench` | 3912.891 us/op, MAD 12.776 | 3929.036 us/op, MAD 22.274 | +0.41% |

Instruction counts were effectively unchanged. The change was retained because
the common path is clearer and skips an unnecessary trailing range-check branch;
the `ReadBlockBench` movement was treated as noise.

## Retained associative-container move cleanup

`std::map` and `std::set` deserialization build a temporary entry/key for each
element and insert it once. The retained cleanup moves those temporaries into
the hinted insert calls instead of copying them. This is a two-line source
cleanup and is most visible when the deserialized key or value owns memory.

Temporary focused benchmark with 1024 entries carrying 256-byte vector payloads:

| Workload | Baseline | Patched | Result |
| --- | ---: | ---: | ---: |
| `std::map<uint64_t, std::vector<uint8_t>>` deserialize | 211246 ns/op | 166255 ns/op | -21.30% |
| `std::set<std::vector<uint8_t>>` deserialize | 213492 ns/op | 173678 ns/op | -18.65% |

This benchmark is intentionally focused on movable payloads rather than block
deserialization. The normal serialization/prevector/streams tests passed, and
block read/deserialization sanity checks still completed.

## Retained compact/varint branch hints

CompactSize values below 253 and VarInt values below 128 are the dominant
encodings in the block serialization/deserialization workloads used here. The
retained change annotates those one-byte paths as `[[likely]]`.

Alternating pinned benchmark results against the retained map/set-move
baseline:

| Workload | Baseline | Patched | Result |
| --- | ---: | ---: | ---: |
| `DeserializeBlockTest` | 2678.273 us/block, MAD 5.732 | 2662.998 us/block, MAD 9.462 | -0.57% |
| `ReadBlockBench` | 3910.368 us/op, MAD 28.620 | 3888.997 us/op, MAD 7.497 | -0.55% |
| `WriteBlockBench` | 1384.836 us/op, MAD 14.160 | 1371.392 us/op, MAD 8.714 | -0.97% |

`WriteBlockBench` also dropped from 3,868,269 to 3,852,752 median
instructions. The read-side instruction counts were effectively flat, so this
is kept as a small code-layout hint rather than a broad algorithmic win.

## Retained VarInt read branch hint

`ReadVarInt` exits after one byte for the common encoding. The retained change
marks the continuation branch as `[[unlikely]]`, mirroring the write-side
one-byte hint without changing the decode loop.

Temporary focused benchmark with one multibyte VarInt every eight entries:

| Workload | Baseline | Patched | Result |
| --- | ---: | ---: | ---: |
| `ReadVarIntMixed` | 20107.3 ns/op | 18676.1 ns/op | -7.12% |

## Retained VarInt read inlining

The VarInt read branch hint helped, but the formatter path still benefited from
forcing `ReadVarInt` into the caller. The retained change adds `ALWAYS_INLINE`
to `ReadVarInt`.

Temporary focused benchmark with one multibyte VarInt every eight entries:

| Workload | Baseline | Patched | Result |
| --- | ---: | ---: | ---: |
| `ReadVarIntMixed` | 19397.5 ns/op | 8572.54 ns/op | -55.81% |

## Retained CompactSize read inlining

`ReadCompactSize` is another tiny formatter-heavy decode helper. The retained
change adds `ALWAYS_INLINE`, keeping the one-byte path in the caller at the
cost of additional generated code.

Temporary focused benchmark with one 254-byte CompactSize value every eight
entries:

| Workload | Baseline | Patched | Result |
| --- | ---: | ---: | ---: |
| `ReadCompactSizeMixed` | 17752.8 ns/op | 8184.2 ns/op | -53.90% |

This increased `bench_bitcoin` `.text` by roughly 14 KiB in the retained build,
which was considered acceptable for the focused decode win.

## Retained CompactSize write inlining

The write side did not show as large a win as the read side, but forcing
`WriteCompactSize` inline still improved the focused mixed CompactSize write
benchmark.

Temporary focused benchmark with one 254-byte CompactSize value every eight
entries:

| Workload | Baseline | Patched | Result |
| --- | ---: | ---: | ---: |
| `WriteCompactSizeMixed` | 9504.43 ns/op | 9421.75 ns/op | -0.87% |

This increased `bench_bitcoin` `.text` by roughly 35 KiB in the retained build,
so this is a speed/code-size tradeoff rather than a free cleanup.

## Retained byte read inlining

The inlined CompactSize and VarInt decode helpers still read every encoded byte
through `ser_readdata8`. The retained change forces that byte reader inline.

Temporary focused benchmarks on the retained tree:

| Workload | Baseline | Patched | Result |
| --- | ---: | ---: | ---: |
| `ReadCompactSizeMixed` | 17513.5 ns/op | 16342.5 ns/op | -6.69% |
| `ReadVarIntMixed` | 17159.8 ns/op | 17161.9 ns/op | flat |

The follow-up validation build passed the selected serialize, streams, and
transaction tests plus the benchmark sanity check. The retained
`bench_bitcoin` binary was 12,491,578 bytes of `.text` and 15,143,248 bytes on
disk after this change.

## Retained byte-container branch hints

The retained byte `prevector` and `std::vector` read cleanups made the
single-chunk path explicit. A follow-up hint marks that path `[[likely]]` and
the empty-container return `[[unlikely]]`.

The first alternating run was mixed, with `ReadBlockBench` faster but
`DeserializeBlockTest` slower and noisy. A longer rerun showed effectively flat
timings with lower instruction counts:

| Workload | Baseline | Patched | Result |
| --- | ---: | ---: | ---: |
| `DeserializeBlockTest` | 2674.190 us/block, MAD 10.821 | 2670.351 us/block, MAD 24.735 | -0.14% |
| `ReadBlockBench` | 3886.841 us/op, MAD 34.196 | 3883.063 us/op, MAD 30.107 | -0.10% |

Median instructions dropped from 14,193,155 to 14,129,904 for
`DeserializeBlockTest` and from 15,663,869 to 15,404,443 for `ReadBlockBench`.
This is kept as a code-layout hint with a measurable instruction reduction, not
as a strong timing win.

## Retained byte-container write hints

The byte container read paths already marked non-empty, single-read containers
as common. The retained write-side hint mirrors that for byte `prevector` and
byte `std::vector` serialization.

Temporary focused benchmark serializing 107-byte containers into `DataStream`:

| Workload | Baseline | Patched | Result |
| --- | ---: | ---: | ---: |
| `PrevectorSerialize107` | 139.386 ns/op | 137.280 ns/op | -1.51% |
| `VectorSerialize107` | 137.110 ns/op | 135.889 ns/op | -0.89% |

## Retained CompactSize sizing shortcut

`SizeComputer` already had a direct `WriteCompactSize(SizeComputer&, ...)`
overload, but `TX_WITH_WITNESS(...)` and other parameter wrappers route writes
through `ParamsStream`. The retained change lets wrappers expose an underlying
`SizeComputer`, so generic `WriteCompactSize` can call `seek()` directly instead
of staging bytes that will never be written.

Temporary focused benchmark using `GetSerializeSize(TX_WITH_WITNESS(payload))`
on 1024 byte vectors of 254 bytes:

| Workload | Baseline | Patched | Result |
| --- | ---: | ---: | ---: |
| `SizeComputerCompactPayload` | 2147.71 ns/op | 1791.77 ns/op | -16.57% |

The retained tree passed `serialize_tests`, `transaction_tests`, and
`blockmanager_tests`, plus block read/write/deserialization benchmark sanity
checks.

## Retained VarInt sizing shortcut

The same wrapper issue existed for VarInt size computation. The retained change
lets generic `WriteVarInt` call `SizeComputer::seek()` through `ParamsStream`
wrappers instead of generating temporary VarInt bytes when only the serialized
size is needed.

Temporary focused benchmark using `GetSerializeSize(TX_WITH_WITNESS(payload))`
on 2048 `VARINT(uint64_t)` values:

| Workload | Baseline | Patched | Result |
| --- | ---: | ---: | ---: |
| `SizeComputerVarIntPayload` | 8543.45 ns/op | 6485.23 ns/op | -24.09% |

The retained tree passed `serialize_tests`, `transaction_tests`, and
`blockmanager_tests`, plus block read/write/deserialization benchmark sanity
checks.

## Retained VarInt size-loop cleanup

After the `SizeComputer` VarInt shortcut, `GetSizeOfVarInt` became part of a
hot sizing path. The retained cleanup initializes the byte count to one and
loops while another VarInt byte is needed, removing the `while(true)` plus
internal `break` shape.

Temporary focused benchmark using `GetSerializeSize(TX_WITH_WITNESS(payload))`
on 2048 `VARINT(uint64_t)` values:

| Workload | Baseline | Patched | Result |
| --- | ---: | ---: | ---: |
| `SizeComputerVarIntPayload` | 6486.74 ns/op | 6473.86 ns/op | -0.20% |

## Binary size

```text
   text      data    bss     dec      hex  binary
12465614   88000   31680  12585294  c0094e  GCC O2 clean baseline
12442438   88000   31680  12562118  bfaec6  GCC O2 retained prevector patch
12464874   88000   31680  12584554  c0066a  GCC O2 retained prevector + CompactSize patches
12415682   88000   31680  12535362  bf4642  GCC O2 retained prevector + CompactSize + VarInt patches
12457586   88000   31680  12577266  bfe9f2  GCC O2 retained prevector + CompactSize + VarInt + DataStream patches
12457570   88000   31680  12577250  bfe9e2  GCC O2 retained prevector + CompactSize + VarInt + DataStream + CompactSize read patches
12460822   88000   31680  12580502  bff696  GCC O2 retained prevector + CompactSize + VarInt + DataStream + CompactSize read + byte vector patches
12415734   88000   31680  12535414  bf4676  GCC O2 retained prevector + CompactSize + VarInt + DataStream + CompactSize read + byte vector + map/set move patches
12478550   88000   31680  12598230  c03bd6  GCC O2 retained prevector + CompactSize + VarInt + DataStream + CompactSize read + byte vector + map/set move + likely hints
12439394   88000   31680  12559074  bfa2e2  GCC O2 retained prevector + CompactSize + VarInt + DataStream + CompactSize read + byte vector + map/set move + likely hints + byte-container hints
12443078   88000   31680  12562758  bfb146  GCC O2 retained prevector + CompactSize + VarInt + DataStream + CompactSize read + byte vector + map/set move + likely hints + byte-container hints + CompactSize sizing shortcut
12414382   88000   31680  12534062  bf412e  GCC O2 retained prevector + CompactSize + VarInt + DataStream + CompactSize read + byte vector + map/set move + likely hints + byte-container hints + CompactSize sizing + VarInt sizing shortcuts
12439170   88000   31680  12558850  bfa202  GCC O2 retained prevector + CompactSize + VarInt + DataStream + CompactSize read + byte vector + map/set move + likely hints + byte-container hints + CompactSize sizing + VarInt sizing + prevector read-size shortcuts
12455586   88000   31680  12575266  bfe222  GCC O2 retained prevector + CompactSize + VarInt + DataStream + CompactSize read + byte vector + map/set move + likely hints + byte-container hints + CompactSize sizing + VarInt sizing + prevector/vector read-size shortcuts
12447362   88000   31680  12567042  bfc202  GCC O2 retained prevector + CompactSize + VarInt + DataStream + CompactSize read + byte vector + map/set move + likely hints + byte-container hints + CompactSize sizing + VarInt sizing + prevector/vector read-size + VarInt read hint shortcuts
12417454   88000   31680  12537134  bf4d2e  GCC O2 retained prevector + CompactSize + VarInt + DataStream + CompactSize read + byte vector + map/set move + likely hints + byte-container read/write hints + CompactSize sizing + VarInt sizing + prevector/vector read-size + VarInt read hint shortcuts
12425614   88000   31680  12545294  bf6d0e  GCC O2 retained prevector + CompactSize + VarInt + DataStream + CompactSize read + byte vector + map/set move + likely hints + byte-container read/write hints + CompactSize sizing + VarInt sizing + VarInt size loop + prevector/vector read-size + VarInt read hint shortcuts
12433738   88000   31680  12553418  bf8cca  GCC O2 retained prevector + CompactSize + VarInt + DataStream + CompactSize read + byte vector + map/set move + likely hints + byte-container read/write hints + CompactSize sizing + VarInt sizing + VarInt size loop + prevector/vector read-size + VarInt read hint/inline shortcuts
12447438   88000   31680  12567118  bfc24e  GCC O2 retained prevector + CompactSize + VarInt + DataStream + CompactSize read + byte vector + map/set move + likely hints + byte-container read/write hints + CompactSize sizing + VarInt sizing + VarInt size loop + prevector/vector read-size + VarInt/CompactSize read hint/inline shortcuts
12482642   88000   31680  12602322  c04bd2  GCC O2 retained prevector + CompactSize + VarInt + DataStream + CompactSize read + byte vector + map/set move + likely hints + byte-container read/write hints + CompactSize sizing + VarInt sizing + VarInt size loop + prevector/vector read-size + VarInt/CompactSize read/write hint/inline shortcuts
13520074   87784   31696  13639554  d01f82  GCC O3 oracle
12360393   79792   29320  12469505  be4501  Clang O3 oracle
```

File sizes:

```text
GCC O2 clean baseline               15077504
GCC O2 retained prevector patch      15076128
GCC O2 retained prevector + CompactSize patches 15077392
GCC O2 retained prevector + CompactSize + VarInt patches 15011136
GCC O2 retained prevector + CompactSize + VarInt + DataStream patches 15077304
GCC O2 retained prevector + CompactSize + VarInt + DataStream + CompactSize read patches 15077304
GCC O2 retained prevector + CompactSize + VarInt + DataStream + CompactSize read + byte vector patches 15077544
GCC O2 retained prevector + CompactSize + VarInt + DataStream + CompactSize read + byte vector + map/set move patches 15011088
GCC O2 retained prevector + CompactSize + VarInt + DataStream + CompactSize read + byte vector + map/set move + likely hints 15077608
GCC O2 retained prevector + CompactSize + VarInt + DataStream + CompactSize read + byte vector + map/set move + likely hints + byte-container hints 15076408
GCC O2 retained prevector + CompactSize + VarInt + DataStream + CompactSize read + byte vector + map/set move + likely hints + byte-container hints + CompactSize sizing shortcut 15076200
GCC O2 retained prevector + CompactSize + VarInt + DataStream + CompactSize read + byte vector + map/set move + likely hints + byte-container hints + CompactSize sizing + VarInt sizing shortcuts 15009832
GCC O2 retained prevector + CompactSize + VarInt + DataStream + CompactSize read + byte vector + map/set move + likely hints + byte-container hints + CompactSize sizing + VarInt sizing + prevector read-size shortcuts 15075792
GCC O2 retained prevector + CompactSize + VarInt + DataStream + CompactSize read + byte vector + map/set move + likely hints + byte-container hints + CompactSize sizing + VarInt sizing + prevector/vector read-size shortcuts 15076424
GCC O2 retained prevector + CompactSize + VarInt + DataStream + CompactSize read + byte vector + map/set move + likely hints + byte-container hints + CompactSize sizing + VarInt sizing + prevector/vector read-size + VarInt read hint shortcuts 15076304
GCC O2 retained prevector + CompactSize + VarInt + DataStream + CompactSize read + byte vector + map/set move + likely hints + byte-container read/write hints + CompactSize sizing + VarInt sizing + prevector/vector read-size + VarInt read hint shortcuts 15009776
GCC O2 retained prevector + CompactSize + VarInt + DataStream + CompactSize read + byte vector + map/set move + likely hints + byte-container read/write hints + CompactSize sizing + VarInt sizing + VarInt size loop + prevector/vector read-size + VarInt read hint shortcuts 15010200
GCC O2 retained prevector + CompactSize + VarInt + DataStream + CompactSize read + byte vector + map/set move + likely hints + byte-container read/write hints + CompactSize sizing + VarInt sizing + VarInt size loop + prevector/vector read-size + VarInt read hint/inline shortcuts 15075320
GCC O2 retained prevector + CompactSize + VarInt + DataStream + CompactSize read + byte vector + map/set move + likely hints + byte-container read/write hints + CompactSize sizing + VarInt sizing + VarInt size loop + prevector/vector read-size + VarInt/CompactSize read hint/inline shortcuts 15139784
GCC O2 retained prevector + CompactSize + VarInt + DataStream + CompactSize read + byte vector + map/set move + likely hints + byte-container read/write hints + CompactSize sizing + VarInt sizing + VarInt size loop + prevector/vector read-size + VarInt/CompactSize read/write hint/inline shortcuts 15142904
GCC O3 oracle                        16096080
Clang O3 oracle                      15537800
```

## Profiles

`VerifyScriptP2WPKH` was dominated by the ECDSA verification path:

- `VerifyScript`
- `EvalScript`
- `GenericTransactionSignatureChecker<CMutableTransaction>::CheckECDSASignature`
- `CPubKey::Verify`
- `secp256k1_ecdsa_verify`
- `secp256k1_ecmult_strauss_wnaf`
- `secp256k1_gej_double`
- `secp256k1_gej_add_ge_var`
- `secp256k1_fe_mul_inner`

GCC `-O3` was only about 0.35% faster. Clang `-O3` was about 1.10% faster and
had fewer branch instructions and branch misses, but this was below the
preferred threshold and was not a GCC-path source finding.

`ReadBlockBench` was dominated by block file reading, transaction hash
computation, serialization, SHA256, vector unserialization, and allocation/copy
costs:

- `node::BlockManager::ReadBlock`
- `node::BlockManager::ReadRawBlock`
- `CTransaction::CTransaction(CMutableTransaction&&)`
- `CTransaction::ComputeHash`
- `SerializeTransaction<ParamsStream<HashWriter&, TransactionSerParams>, CTransaction>`
- `VectorFormatter<DefaultFormatter>::Unser`
- `CSHA256::Write`
- `sha256_arm_shani::Transform`
- `AutoFile::read`
- `_IO_fread`

Perf collection commands:

```bash
perf record -o /tmp/perf_verify_p2wpkh_gcc_base.data -F 997 --call-graph fp -- \
  taskset -c 3 build-gcc-base/bin/bench_bitcoin \
  -filter=^VerifyScriptP2WPKH$ -min-time=5000

perf record -o /tmp/perf_readblock_gcc_base.data -F 997 --call-graph fp -- \
  taskset -c 3 build-gcc-base/bin/bench_bitcoin \
  -filter=^ReadBlockBench$ -min-time=5000

perf record -o /tmp/perf_readblock_gcc_o3.data -F 997 --call-graph fp -- \
  taskset -c 3 build-gcc-o3/bin/bench_bitcoin \
  -filter=^ReadBlockBench$ -min-time=5000
```

## Assembly observations

The relevant symbol-size changes were code-size heavy:

- `SerializeTransaction<ParamsStream<HashWriter&, TransactionSerParams>, CTransaction>`
  grew from `0x364` to `0x44c`.
- `BlockManager::ReadBlock` grew from `0x1274` to `0x24cc`.
- `VectorFormatter<DefaultFormatter>::Unser` transaction input/output clones
  also grew substantially.
- `ReadCompactSize` and `WriteCompactSize` symbol sizes were unchanged.

In `SerializeTransaction`, GCC `-O3` rearranged branches and inlined some
compact-size handling in witness serialization. In
`VectorFormatter<DefaultFormatter>::Unser`, GCC `-O3` inlined more prevector and
script deserialization handling for small scripts. This reduced instructions
and branches in the block benchmarks, but did so by broadly expanding template
instantiations.

For the retained patch, disassembly showed that GCC inlined the new byte
`prevector` fast path into the transaction input/output vector unserializer
clones. The clean build had an out-of-line byte `prevector` unserializer for
`ParamsStream<SpanReader&, TransactionSerParams>`; the patched build did not.
The resulting hot path resizes once and calls `SpanReader::read` once when the
serialized size is below the cap, then returns to the surrounding transaction
input/output parsing code.

## Rejected source experiments

A small source experiment changed serialization helpers to use static-extent
spans and added one-byte overloads for `HashWriter`, `ParamsStream`, and
`SpanReader`. It was inspired by the idea that reducing small write/read helper
overhead could reproduce part of the `-O3` behavior under normal GCC `-O2`.

After fixing one overload ambiguity, it compiled, but benchmark results did not
support keeping it:

| Workload | Baseline | Patched GCC O2 | Result |
| --- | ---: | ---: | ---: |
| `VerifyScriptP2WPKH` | 155088.42 ns/script | 155130.79 ns/script | +0.03% |
| `ReadBlockBench` | 3915.73 us/op | 3949.31 us/op | +0.86% slower |
| `DeserializeBlockTest` | 2754.00 us/block | 2749.97 us/block | -0.15% |

The experiment was reverted. Reasons:

- no instruction or branch reduction;
- `ReadBlockBench` regressed;
- the small `DeserializeBlockTest` movement was below the useful threshold;
- it did not explain or reproduce the GCC `-O3` oracle win.

A second experiment added a no-witness transaction hash serialization helper to
avoid checking witness state while hashing transactions that do not carry
witness data. It reduced some instruction and branch counts, but the timing was
not useful: `ReadBlockBench` regressed by about 0.49% and
`DeserializeBlockTest` was effectively flat. That experiment was also reverted.

Follow-up candidates from the serialization-specialization patch stack were
also screened:

- a generic `VectorFormatter` single-batch path was rejected because it made
  `DeserializeBlockTest` 2.23% slower and triggered a compiler array-bounds
  warning in a nontrivial `prevector` benchmark instantiation;
- a `VectorWriter` append fast path was rejected because the local append
  microbenchmark was substantially slower under GCC `-O2`;
- changing `DataStream::write` to insert from raw span pointers instead of span
  iterators improved a focused single-byte write benchmark from 619.861 ns/op
  to 608.812 ns/op, but was rejected because the normal GCC build emitted an
  `array-bounds` warning through a `BlockFilter` serialization instantiation;
- marking CompactSize/VarInt canonical, range, and overflow error checks as
  `[[unlikely]]` was rejected because it slowed `DeserializeBlockTest` by
  0.25%, `ReadBlockBench` by 0.08%, and `WriteBlockBench` by 0.67%;
- marking `SpanReader` and `DataStream` zero-length and bounds-failure checks
  as `[[unlikely]]` was rejected because it slowed `DeserializeBlockTest` by
  1.70% and `ReadBlockBench` by 1.24%;
- replacing the retained `prevector` fast-path `&v[0]` span source with
  `v.data()` was rejected because it produced identical instruction counts and
  no useful timing signal;
- replacing `SerializeTransaction`'s writer-side flags byte with a direct
  `serialize_witness` boolean was rejected because it slowed
  `DeserializeBlockTest` by 0.26%, `ReadBlockBench` by 0.58%, and was flat to
  slightly worse on `WriteBlockBench`;
- marking the 16-bit CompactSize read/write branches as `[[likely]]` after the
  one-byte branch was rejected because instructions were effectively unchanged
  and `DeserializeBlockTest`, `ReadBlockBench`, and `WriteBlockBench` all moved
  slower;
- adding `SizeComputer` shortcuts to primitive `ser_writedata*` helpers was
  rejected because a temporary `SizeComputerBlock` benchmark only moved from
  47438.29 ns/block to 47250.96 ns/block while instruction counts stayed
  exactly flat at 171121.03 instructions/block, and the diff made the helpers
  more repetitive;
- adding a `SizeComputer` shortcut to `CustomUintFormatter::Ser` was rejected
  because a focused wrapper benchmark was flat/slightly slower, moving from
  4676.35 ns/op to 4677.72 ns/op;
- adding a one-byte `DataStream::write` overload was rejected because a focused
  4096-byte serialization benchmark was flat/slightly slower, moving from
  3526.91 ns/op to 3527.19 ns/op;
- adding a one-byte `VectorWriter::write` overload was rejected because a
  focused 4096-byte append benchmark was flat, moving from 6900.52 ns/op to
  6900.46 ns/op;
- adding a one-byte `DataStream::read` overload was rejected because a focused
  4096-byte read benchmark barely moved, from 10517.6 ns/op to 10514.9 ns/op;
- adding a one-byte `SpanReader::read` overload was rejected because a focused
  4096-byte read benchmark was flat/slightly slower, moving from 9332.01 ns/op
  to 9332.31 ns/op;
- changing byte `prevector`/`std::vector` chunk-loop counters from
  `unsigned int` to `size_t` was rejected locally because an oversized
  5.1 MiB read benchmark moved slower for both `std::vector` (2.179 ms to
  2.184 ms) and `prevector` (878 us to 881 us);
- marking non-empty string serialization/deserialization paths as likely was
  rejected because a focused 107-byte string benchmark slowed serialization
  from 80.16 ns/op to 82.23 ns/op and deserialization from 40.85 ns/op to
  41.28 ns/op;
- reusing the byte container size for write-side empty checks was rejected
  because the focused 107-byte benchmark was effectively flat for `prevector`
  and slightly slower for `std::vector`;
- marking the sub-253 branch in `GetSizeOfCompactSize` as likely was rejected
  because the focused 254-byte `SizeComputerCompactPayload` benchmark slowed
  from 1791 ns/op to 2150 ns/op;
- simplifying bool serialization/deserialization casts was rejected because a
  focused bool benchmark was mixed: serialization slowed from 3527.38 ns/op to
  3528.76 ns/op and deserialization was effectively flat;
- forcing `WriteVarInt` inline was rejected because the mixed VarInt write
  benchmark was flat/slightly slower, moving from 3538.87 ns/op to
  3540.12 ns/op;
- larger stream/span and `SizeComputer` specialization refactors were deferred
  because they are broader than the current simple-source-change scope.

## Validation

The retained work passed these validation commands as it was built up:

```bash
build-gcc-base/bin/bench_bitcoin \
  -filter='^(DeserializeBlockTest|WriteBlockBench|LinearizeOptimallyTotal|LinearizeOptimallyPerCost)$' \
  -sanity-check

build-gcc-base/bin/test_bitcoin \
  --run_test=serialize_tests,streams_tests,prevector_tests \
  --catch_system_errors=no

build-gcc-base/bin/bench_bitcoin \
  -filter='^(DeserializeBlockTest|ReadBlockBench)$' \
  -sanity-check

cmake --build build-gcc-base --target bench_bitcoin test_bitcoin -j4 && \
build-gcc-base/bin/test_bitcoin \
  --run_test=serialize_tests,transaction_tests,blockmanager_tests \
  --catch_system_errors=no && \
build-gcc-base/bin/bench_bitcoin \
  -filter='^(DeserializeBlockTest|ReadBlockBench|WriteBlockBench)$' \
  -sanity-check
```

Full `test_bitcoin` was not run.

## Follow-ups

The remaining GCC `-O3` block deserialization oracle is real enough to justify
future work beyond this retained patch. Useful next steps would be narrower
per-function experiments, PGO/BOLT analysis, or a targeted workload closer to
IBD/reindex. Any future patch should show a normal GCC baseline win without
code-size-heavy template expansion or reduced readability.
