# SipHash benchmarks

## Baseline (5 runs, min-time 2000 ms)

System: rpi5-8 | aarch64 | Cortex-A76 | 4 cores | 7.7Gi RAM | ext4 | SSD | Ubuntu 25.04

Commands:
- `build-bench-gcc/bin/bench_bitcoin -filter='SipHash.*' -min-time=2000`
- `build-bench-clang/bin/bench_bitcoin -filter='SipHash.*' -min-time=2000`

GCC 15.0.1 results (median [min-max] ns/op):
- SipHash13Jumbo_32b: 18.34 [18.33-18.42]
- SipHash13Jumbo_36b: 18.40 [18.26-18.46]
- SipHash13_32b: 22.79 [22.72-22.79]
- SipHash13_36b: 23.35 [23.22-23.49]
- SipHash24_32b: 34.49 [34.47-34.58]
- SipHash24_36b: 35.05 [35.04-35.07]

Clang 22.0.0 results (median [min-max] ns/op):
- SipHash13Jumbo_32b: 18.49 [18.38-21.00]
- SipHash13Jumbo_36b: 17.82 [17.79-17.83]
- SipHash13_32b: 22.86 [22.46-25.13]
- SipHash13_36b: 22.72 [22.61-22.76]
- SipHash24_32b: 33.81 [33.74-36.29]
- SipHash24_36b: 33.84 [33.80-34.02]
