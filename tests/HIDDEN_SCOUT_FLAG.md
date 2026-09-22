# Hidden scout flag defense

The human victory in `fixtures/human_win_181.jsonl` ends with 64->4:
an unrevealed scout takes the flag over six squares. At ply 179 the human
colonel vacates square 24, opening the ray. At ply 180 Improved previously
chose 45->55 rather than blocking with 25->24 or 45->44.

The existing immediate-defeat filter considers only revealed attackers.
General invasion distances treat an unidentified mover as a miner, walking
one square per move. Neither provided a reliable immediate scout-ray veto.

## Change

At the root, `scout_flag_risk` evaluates immediate scout captures after each
candidate move, alongside the existing terminal-risk filters. It works at
every army size, before strategic shortcuts and sampled search. A known
winning flag capture still returns immediately.

From the own flag, each ray stops at its first occupant or a lake; edges may
not wrap between rows. An enemy occupant is a threat if its public scout
probability is positive and a hypothetical scout can legally capture the flag,
including move-repetition restrictions. This includes unmoved unidentified
pieces; moving one square is not required evidence of scout possibility.
Masked enemy pieces still block rays to pieces behind them.

The largest ray probability is a lower bound, not an independent-event model
of the whole army. Root combats are averaged over public target probabilities:
a failed attack on a scout cannot count as a successful screen, while taking
the enemy flag ends the game. This term is combined with the prior terminal
risks using their maximum, avoiding an extra penalty for the same threat.
The least-risk legal candidates remain eligible for normal search, so an
unavoidable open ray does not leave the AI with an empty move list.

No enemy hidden rank is read. The replay gives square 64 a 30% scout prior.
Across eight seeds the corrected AI blocks the attack with 25->24 (six seeds)
or 45->44 (two). Swapping that hidden scout with the moved, hidden human
marshal preserves both the chosen move and the resulting RNG state.

## Tests

`hidden_scout_flag_defense` covers the recorded position, hidden identity
permutation, rotated board with sides exchanged, identified scout, friendly
and masked enemy blockers, lakes, board edges, zero remaining scout odds,
unmoved possible scout, uncertain root combats and immediate winning flag
capture. Each replay choice is applied through the rules engine and verified
to prevent the recorded flag capture. This proves that immediate defense,
not a win from the resulting position or a general win-rate improvement.

Release validation: compilation without warnings; all 28 CTest regressions
passed in 258.58 seconds, including the officer-secrecy and spy-comeback tests.
Local logs: `reports/scout-flag-build.log`, `reports/scout-flag-tests.log`.
