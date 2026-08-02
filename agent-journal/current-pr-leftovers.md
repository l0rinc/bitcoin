# Journal: current branch and PR leftover sweep (campaign 3)

Uber-goal rotation. Branch: audit/current-pr-leftovers from
audit/resurrection @ 567f80cef2. Method: per own change, state the
intended rule, then search the tree for analogous sites that should have
changed but didn't.

## Cycle 1 verdicts

### LEFTOVER FOUND + FIXED: WriteDescriptor in TopUpWithDB (2a4e8edcfc)

The W4 series (0e7a8fabb5) checked WriteDescriptor results in
GetNewDestination/ReturnDestination but missed TopUpWithDB, where the
WriteDescriptorCacheItems call IS checked (scriptpubkeyman.cpp:1107,
throws) while the trailing WriteDescriptor (was:1114) was not. On DB
write failure, range_end already advanced in memory → silent
memory/disk divergence (W4 consequence class: mis-tracked keypool range,
keys safe). Fixed: check + throw, matching both siblings. Build +
scriptpubkeyman_tests + wallet_tests green.

### FALSE POSITIVE (retracted): asserts at init.cpp:1338/1357/1369

I claimed the ApplyArgsManOptions asserts could fire for paths not
pre-validated (32-bit -maxmempool). WRONG: AppInitParameterInteraction
calls the SAME ApplyArgsManOptions functions at init.cpp:1150/1163/1168
and surfaces every error via InitError — the asserts are unreachable by
construction. My fix (36156ad934) was reverted on
audit/public-interface-contracts (5a16d316af) with a retraction note in
that journal. Lesson: absence of a foo-specific check != absence of
validation; the generic sweep covers it.

### NOTED, no commit (proportionate-response unclear): WriteBestBlock unchecked

wallet.cpp:1994 (scan progress) and 4622 (periodic best block) ignore
WriteBestBlock results. Consequence: scan progress not persisted →
longer rescan next load. Wasted work only; fail-soft is arguably
correct (a throw mid-sync would be worse). Left as journal note.

### Other rotation fixes — no leftovers
GetNetClass dedup (769822b5a6): the only inner-redundancy site; other
classification callers evaluate IsRoutable once per call. FormatHDKeypath
suppression: campaign 98 swept the whole file. Bump fuzz target: complete.

## Next queue
(rotate per uber-ledger — next re-rank; PR-stack sweep continues as PRs evolve)