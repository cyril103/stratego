# Surviving contact captures

At ply 88 of `fixtures/ai_win_resignation_220.jsonl`, the AI marshal on 35
could attack the moved, isolated enemy on 45. The target was the human spy,
but its public identity remained unknown. The old AI instead retreated to 34.
The game later ended in human resignation at ply 220.

The contact-risk heuristic gave the retreat 200 points of safety credit.
The projected capture removed the same threat, but its entire positive
credit was discarded because the defender was unidentified. Public odds
gave a 23/24 chance of winning and surviving, and 1/24 chance of exchanging
marshals; a bomb was impossible because the target had moved.

`contact_safety_bonus` now multiplies positive relief from a capture by the
public probability that the attacker wins and survives. A losing attack or
equal exchange receives no rescue credit. Known winning captures and quiet
retreats retain their prior credit. Negative relief, including newly exposed
contact with a possible supporting spy, remains fully penalized rather than
being discounted by success probability. Existing combat, counterattack,
marshal-suspect and terminal flag filters remain in place.

The new credit for an uncertain capture applies only to an already revealed
attacker. Hidden attackers retain the old cautious probe policy: the initial
unrestricted version also failed the prior officer-secrecy opening regression.
The new positive credit for an uncertain capture is also withheld when any
publicly possible immediate recapture could kill or exchange the attacker.
An initial version without this condition failed the old ply-322 regression
in `human_win_429.jsonl`: taking a captain stepped beside a hidden supporting
spy. That relaxation was rejected; the existing escape regression is retained.

The projected successful capture reveals the attacking officer before its
new exposure is evaluated. Pursuit history stays anchored to the original
board. The helper never reads a hidden enemy rank.

## Targeted validation

The recorded capture now receives 191.67 safety points. Across eight seeds,
the AI chooses 35->45, and applying this move through the rules engine captures
the spy with no immediate enemy attack on the marshal's new square. Swapping
the hidden spy with the moved hidden human marshal leaves both the decision
and final RNG state unchanged: the AI accepts a possible equal exchange and
does not know which of the two identities it is attacking.

`officer_contact_capture` also checks zero credit for losing attacks and
equal trades, undiscounted new support danger, and the AI spy's recorded
winning recapture at ply 216. These tactical checks do not establish a new
overall win rate or guarantee the outcome of a continued game.

Final Release validation: no compiler warnings; all 29 CTest regressions
passed in 245.97 seconds, including officer secrecy, hidden scout flag defense
and the older supported-spy traps. Logs: `reports/contact-build.log` and
`reports/contact-tests.log`; the rejected first run is preserved locally in
`reports/contact-rejected-tests.log`.
