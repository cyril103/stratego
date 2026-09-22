# Persistent tactical evidence and spy cooperation

The 413-ply human victory is recorded in `fixtures/human_win_413.jsonl`.
`persistent_spy_tactics` checks the public positions, hidden-rank permutations
and four deterministic seeds. Existing tactical regressions remain separate.

## Memory

`Game.marshal_suspects` stores public withdrawals by observer and piece ID in
32 bytes. It does not expire when the eight-move repetition history rolls over.
The original rule history stays unchanged. Replays rebuild the evidence through
`game_apply`, and worker/search copies carry it with the position. Only a
publicly revealed marshal creates evidence, and no enemy hidden rank is read.
Combat, scout-ray revelation and a new game invalidate the applicable memory.
Rank probabilities still determine whether a suspect can be a spy; memory is
not an assertion that its rank is known. Terminal-defense filters and flag
capture opportunities retain priority.

The recorded retreat at ply 296 remains meaningful at ply 402. Tests erase the
short history, vary hidden identities, cover both camps/high piece IDs, reveal
a scout and reset a game. Known identities stop being unresolved suspects.

## Spy safety and coordination

Every mobile attacker eliminates a defending spy, including a mutual spy
exchange. A moved unknown adjacent to the spy therefore provides sufficient
public evidence of the loss. The planner now recognizes it both in whole-army
safety and in spy approach routes. Before ply 282, the unsafe 25->35 approach
receives no chase bonus and is rejected by the tested decisions.

A clearance preference detects a boxed spy and an enemy that can legally
approach next turn. An ally may vacate a safe neighboring escape square before
that happens, provided the clearance does not expose the ally to a known loss
or increase flag risk. In the recorded ply-310 position the ally clears 24;
after the original 27->26 reply, the spy can escape 25->24.
The clearance must not leave another miner or officer under a known immediate
capture. This retains the earlier miner-rescue regression while permitting
an expendable scout screen to buy the spy's escape tempo.

A bounded ambush preference values an allied sergeant/lieutenant/captain which
the enemy marshal cannot capture without allowing a legal spy recapture. It
does not assume the opponent accepts the bait and does not reward exposed
spies. Tests verify both square control and a threatening scout exception.
The capability cost is also charged on risky spy attacks; otherwise the AI
could avoid a large defensive loss score by suiciding into the unknown attacker.
A certain marshal capture remains exempt. The older 16-seed spy/marshal rescue
test guards against precisely that regression.

## Active defense

Marshal interception routes no longer promise progress through contact with
persistent unresolved spy suspects. This lets the route go around the threat
instead of penalizing every alternative while rejecting the apparent shortcut.
At ply 342 the tested marshal takes 44->45 rather than remaining in 44; at
402 it refuses the old 44->34 trap. No inactivity timer forces random movement.

After an identified officer has just captured, the immediate next victim is
valued fully in the existing whole-army loss calculation, rather than receiving
the quarter-weight assigned to ordinary remote infantry. This addresses the
recorded sequence of captures across a row without claiming every loss can be
avoided. The preference still accounts for legal recaptures and urgent defense.
Following the attacker into its previous square gets no positive strategic
progress credit while another unprotected victim has a legal retreat without a
known immediate loss. Public flag emergencies remain exempt. At ply 114 the
lieutenant now moves 36->26 on all four seeds; the recorded 37->36 response
therefore captures nothing instead of the lieutenant.
The older defensive-tempo test also accepts a verified captain rescue at
ply 112 of the 383-ply replay: a known colonel can take it in 20, whereas the
new 20->21 retreat removes that immediate threat without increasing flag risk.
Its old assertion accepted only defender approach, excluding this useful rescue.

## Validation

Final Release build: no compiler warnings; 25/25 CTest regressions passed in
491.68 seconds. Smoke test: 150 frames and all 100 projected squares checked.
Installed executable SHA256:
`1acb2985e3423a8c8c280b88b88c2ee7a99c26b86cc8a98dfd5351b4a10e3943`.
Benchmark executable SHA256:
`47151f6704ed7b5390d2333fef08653c9ed7c21a7f3199005617f115ee142cbf`.
Logs: `reports/persistent-final-build.log`, `persistent-final-tests.log`,
`persistent-final-smoke.log`. The first interrupted tournament is not evidence
for the final version.

Paired comparison command (new output directory required):

```powershell
python tools/tournament.py --binary build-napoleonic/ai_benchmark.exe --output reports/persistent-random-final --models 1 3 --pairs 2 --seed 204 --formations random --seconds 0 --plies 600 --jobs 2
python tools/compare_tournaments.py reports/armycare-matrix-final reports/persistent-random-final
```

These are the same four random deployments/camps as the earlier matrix, against
Classic with native budgets and no clock. The 600-ply ceiling is not a draw
rule. Timings share the CPU with regression tests and are not isolated latency
measurements. Four games do not establish an Elo or a general win rate.

| Seed | Improved side | Previous result / plies | New result / plies |
|---|---:|---|---|
| 204 | 0 | Win / 591 | Win / 406 |
| 204 | 1 | Unfinished / 600 | Unfinished / 600 |
| 205 | 0 | Loss / 562 | Win / 541 |
| 205 | 1 | Win / 409 | Win / 304 |

All four combat logs passed the audit. The unfinished position improved from
a nominal material deficit of 137 (one general against 14 mobiles) to an
advantage of 56 (six mobiles against one), but the AI still did not finish
within 600 plies. This remains a conversion limitation, not a counted win.
Raw snapshots, hashes, replays and results are in
`reports/persistent-random-final/`; the pairing check is in
`reports/persistent-comparison.json`.
