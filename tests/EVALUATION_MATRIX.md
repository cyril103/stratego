# Reproducible broader evaluation

Build `ai_benchmark` before running:

```powershell
python tools/tournament.py --binary build-napoleonic/ai_benchmark.exe --output reports/armycare-matrix-final --models 1 2 3 --focus 1 --pairs 2 --seed 204 --formations auto random --seconds 0 --plies 600 --jobs 2
python tools/audit_tournament.py reports/armycare-matrix-final
python tests/test_tournament.py
ctest --test-dir build-napoleonic --output-on-failure
```

This schedules 16 games: two seeds, random versus strategic placements, two
opponents (learned and classic), and both camps. Every seed/placement board
must match across paired games. The runner snapshots binaries, policies and
sources with hashes, and retains replays and decision times. Native model
budgets are retained; parallel timings are not isolated latency measurements.

The 600-ply cap is a bounded screening protocol, not a draw rule. Unfinished
games stay separate from draws. This sample does not establish an Elo or a
before/after win-rate improvement. Longer paired runs and additional seeds
remain available through the same CLI. Comparisons use opponent, formation,
seed and camp together so different experiments cannot overwrite each other.

## Validation on 2026-09-22

Release build completed without compiler warnings. All 24 CTest regressions
passed in 508.59 seconds, alongside five Python tournament tests. The full
suite caught a double-counted rescue premium; removing the already-scored
largest threat restored the recorded general response, flag race and defensive
tempo without relaxing those regression assertions.

The final graphical smoke test rendered 150 frames and checked picking on all
100 board squares. The executable installed for `Jouer.cmd` has SHA256
`036ce18446d7117e1876984ae2e96bffb1f38ed331e1c22bb6f450719213141f`.
The tournament executable has SHA256
`6ad4f405f443f8dae4a44056fe86d5992608def9b3f6aadf12967aa67e0d09d6`.

Raw evidence is retained locally in `reports/step4-final-tests.log`,
`reports/step4-final-smoke.log` and `reports/armycare-matrix-final/`.
The earlier interrupted matrix is not part of the final results.

### Completed matrix

| Opponent | Placement | Wins | Losses | Draws | Unfinished |
|---|---|---:|---:|---:|---:|
| Learned | Strategic | 4 | 0 | 0 | 0 |
| Learned | Random | 4 | 0 | 0 | 0 |
| Classic | Strategic | 4 | 0 | 0 | 0 |
| Classic | Random | 2 | 1 | 0 | 1 |
| Total | Both | 14 | 1 | 0 | 1 |

All 16 replays passed the combat-log audit. The loss is seed 205, random
deployment, Improved playing side 0: its flag fell at ply 562, with a nominal
material deficit of 33. That replay is preserved as
`tests/fixtures/evaluation_loss_562.jsonl`. Both adverse outcomes occurred
against Classic on random placements. These results support targeted follow-up
on those positions, not a claim that the new engine is unbeatable or that its
win rate improved by a measured amount over the previous build.

### Adverse unfinished position

Seed 204, random deployment, Improved playing side 1 against Classic reached
600 plies without a result. Improved had only its general left as a mobile
piece against 14 opposing mobiles, with a nominal material deficit of 137.
Its last miner died at ply 397. The longest quiet sequence was 155 plies.
This is a materially adverse unfinished position, not an implied draw or an
unconverted winning advantage. The replay is preserved as
`tests/fixtures/evaluation_stalled_600.jsonl` for subsequent investigation.
The added preferences do not guarantee survival of all miners or solve every
invasion. This benchmark is intended to expose such failures as well as wins.
