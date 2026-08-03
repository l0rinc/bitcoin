# Optimization-mode flag and generated-rule parity audit

Seeded after Cycle 319's compiler differential. The current libsecp build showed that a CMake cache value and configure summary can say `-O3` while the nested generated compile rule intentionally contains base `-O2`; the supported explicit append variable then adds a final `-O3`. This is a configuration-contract surface, not a confirmed defect.

Future cycles must compare cache values, configure summaries, generated compile/link commands, target properties, and artifact behavior for Debug, Release, RelWithDebInfo, Coverage, sanitizers, IPO/LTO, PGO, and explicit append flags. Reused build trees must be checked for stale profile or optimization paths. Keep top-level Bitcoin and standalone libsecp evidence separate, and do not repeat the broad compiler matrices unless a source, toolchain, generator, or policy change creates a new cell.

Initial evidence:

- `cmake/module/ProcessConfigurations.cmake` and `src/secp256k1/CMakeLists.txt` deliberately normalize Release `-O3` to `-O2`.
- `/data/my_storage/tmp/cycle247-secp-clang19-o3` required `SECP256K1_APPEND_CFLAGS=-O3` for generated rules to end in actual `-O3`.
- `/data/my_storage/tmp/cycle247-secp-gcc-lto` generated `-O2 -g -flto -flto=auto -fno-fat-lto-objects` as expected.
- Current focused and full libsecp tests passed under GCC O2, GCC LTO, and Clang 19 actual O3; no defect is currently established.

Next experiment: configure small isolated trees with one mode or append variable changed at a time, then prove target-level flag parity and stale-cache behavior from generated rules. Record unsupported host/toolchain cases rather than weakening the check.
