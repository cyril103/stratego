# Army survival after the 375-ply human victory

Source: `fixtures/human_win_375.jsonl`. The AI loses by immobility, with its
flag intact. The changes address three reproduced decisions, using only
public observations, casualty counts and the AI's own pieces.

## Bound speculative rescue credit

At ply 174, the revealed colonel attacked a moved, unidentified general.
Public survival odds were 88.89%, but the recent contact-rescue term added
85.33 points. If any publicly possible target rank can strictly defeat the
attacker, this positive credit is now capped at half the attacker's nominal
value (12 for the colonel). This caps a heuristic incentive, not the attack
itself. Known captures and the marshal's isolated moving target retain their
previous treatment: no mobile rank can strictly defeat an attacking marshal.
An unmoved possible bomb also triggers the cap.

## Look ahead before advancing the spy

At ply 334, the spy's 24->34 advance allows 45->44 to close its safe exits.
The alternative 24->23 retains escape squares 22 and 13 after that response.
For quiet spy moves while the enemy marshal lives, a local check considers
one legal quiet enemy move into contact, then legal safe spy exits. Revealed
ranks and moved unknown pieces are considered; an unknown mover needs no
guessed identity to threaten a spy. A capture of an identified marshal also
counts as a useful reply. Potential boxing receives a bounded 40-point cost.
Already immediately unsafe moves retain the existing immediate-loss scoring.

This local heuristic checks the spy's own exits, not every possible teammate
rescue or a forced whole-game loss. It does not prohibit the move, replace
search, or suppress the existing comeback rule for spy attacks.

## Preserve the last high officer

At ply 350, the AI leaves its marshal adjacent to the identified enemy
marshal while moving its sergeant. The equal exchange leaves only that
sergeant against thirteen mobile enemies. In a losing small army (at most
three other mobiles), leaving the sole highest officer available for a known
equal exchange, or initiating that exchange, costs twice its nominal value
when superior enemy ranks would still remain above the friendly reserve.
The minimum officer rank is captain. Tied friendly top ranks, favorable
material balance and removal of the last superior enemy do not qualify.
The cost is a preference; immediate flag safety and winning captures retain
priority.

## Targeted validation

Across eight seeds at each recorded checkpoint, the colonel no longer probes
35, the spy chooses 24->23 and survives the recorded follow-up, and the marshal
retreats 15->25. Swapping hidden enemy identities preserves the choices and
RNG states. Additional checks retain known flag captures, allow an equal
exchange without a superior enemy reserve, and disable spy conservation
when the opposing marshal has been removed.

These position regressions do not prove victory from the initial deployment
and do not establish a new overall win rate.

Release validation: build without compiler warnings; all 30 CTest regressions
passed in 271.85 seconds, including the prior isolated-spy capture, supported
spy traps, officer secrecy, comeback attack and hidden-scout flag defense.
Logs: `reports/endgame-care-build.log` and `reports/endgame-care-tests.log`.
