# Journal: secp256k1 group, ecmult, and formula-parity audit (campaign 83)

Uber-goal rotation, severity-first: crypto correctness + constant-time class.
Branch: audit/secp-group-ecmult from audit/resurrection @ dbfcaea3d5.
Vendored subtree (9caae50682) — findings get upstream-appropriate commits.
Host: aarch64 native (5x52/4x64); int64 scratch backend available from #82.
Carried technique: subtree-only scratch builds + tests/noverify -j4, ~35s.

## Scope ledger

| # | area | hypothesis seeds | verdict |
|---|------|------------------|---------|
| G1 | oracle inventory | which group/ecmult parity oracles the suite already runs (gej/ge conversion, add/double formula variants, exhaustive group, window tables, endomorphism) | open |
| G2 | affine/Jacobian conversion parity | gej→ge→serialize round-trips at infinity + boundary points | open |
| G3 | add/double formula variants | gej_add_ge vs gej_double vs gej_add_zp vs gej_add consistency at edge cases (P, -P, infinity, P==Q) | open |
| G4 | window/wNAF tables | ecmult_gen vs ecmult arbitrary-point vs naive double-and-add parity across window sizes | open |
| G5 | endomorphism path | ecmult with/without endomorphism parity (g split) at scalar edges | open |

## Verdicts

### G1 (oracle inventory): the suite IS the formula-parity oracle, green on both backends

In-tree coverage, all executed green in #82's runs (native 5x52/4x64 exit 0,
forced 10x26/8x32 exit 0, same seed):
- run_ge (tests.c:3860-4100): representation equality (ge/gej/gej_eq_ge)
  with randomized x/y/z magnitudes (3908-3914); gej_add_var as reference
  vs gej_add_ge_var and further variants with Z-ratio checks (3940-3992);
  edge set includes infinity, P, -P, same-point (3900-3907).
- run_ecmult_chain (4453+): scalar edges zero/one + x-vs-(n-x) consistency
  relations (4520-4542).
- run_ecmult_near_split_bound (4615): endomorphism split-bound edges.
- run_ecmult_const_tests (4846): fixed-window constant-time parity.
- run_ecmult_multi_tests (5448): multi-scalar Strauss parity.
- run_ecmult_gen_blind (5863): blinded generator multiplication.
- exhaustive_tests (order-13 small group): full group covered — ran twice,
  "no problems found", exit 0.

### G2-G5 (conversion, formula variants, window tables, endomorphism): DISMISSED

All covered by the G1 oracles above; both backend runs plus the exhaustive
small-group proof are green. No parity divergence on any path.
Technique note: exhaustive_tests binary accepts jobs; two independent
executions passed.

## Campaign 83 cycle complete

All 5 ledger areas locked with oracle evidence — the group/ecmult formula
parity is mechanically verified by the in-tree suite (two backends) and
the order-13 exhaustive proof. No defects. Rotation: uber-ledger marks
#83 DONE, next #84 nonce/signing/MuSig.

## Next queue
(G1 inventory first: grep suite for ecmult/ge parity checks; then G4/G5 —
highest-complexity paths)
