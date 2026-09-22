# Officer secrecy and spy risk

Source: `fixtures/human_win_555.jsonl`, human victory on 22 September 2026.
The AI first revealed its marshal at ply 28 (35->34, hidden sergeant)
and its general at ply 106 (59->58, known captain). The human marshal
remained hidden until it captured the AI spy at ply 527.

## Step 1: first-combat disclosure

`officer_disclosure_cost` charges a surviving general or marshal for its
first combat, using only its own hidden mobile army and public combat odds.
The maximum scale is 1.5 times nominal officer value, multiplied by:

- the fraction of other ranks among the own hidden mobile pieces;
- the remaining hidden mobile population divided by 24, capped at one;
- the probability of winning a nonterminal combat and remaining on board.

The price is zero for an already exposed officer or when its mobile rank is
already inferable. It diminishes with the hidden army rather than elapsed
turns, and is included in both root ordering and final scoring. Known flag
captures and existing immediate-defeat filters retain priority.

The recorded opening is a tactical regression, not a measured improvement
in overall win rate. The general's captain capture remains a legal tactical
choice: the change values its disclosure without forbidding all first attacks.

`officer_secrecy` checks the recorded opening on four seeds, identical choices
and RNG state after swapping hidden enemy ranks, both officers' disclosure
costs, a known flag capture and an emergency defense requiring the marshal.
On these four seeds the opening now chooses 45->55 instead of 35->34;
at ply 106 it chooses 27->37 instead of 59->58.

Step 1 validation: Release build without warnings; all 26 CTest tests passed.
Local logs: `reports/secrecy-build.log`, `reports/secrecy-tests.log`.

## Step 2: spy comeback opportunities

Before ply 522 the AI has four mobile pieces against thirteen. Its spy at
68 can legally attack the moved, unidentified enemy at 67. Public remaining
ranks give that target 10% marshal probability. Previously the extra spy
capability penalty alone was 180 points, regardless of the army deficit.

For moved, unidentified targets with nonzero marshal probability, the new
rule requires a negative public force balance and a mobile-number deficit
greater than 25%. Risk tolerance grows continuously to full strength at
65%. With no legally safe adjacent retreat it grows 50% faster. Stationary
targets, known losing targets, impossible marshal identities and balanced
armies do not receive this relaxation.

Risk tolerance progressively removes the extra capability penalty and adds
an analytic upside of twice marshal value times marshal probability. At the
recorded position this changes the additional term from a 180-point cost to
a 10-point bonus. Normal combat evaluation, retaliation and terminal flag
filters still apply; the move is not forced by a hard-coded replay rule.

A related accounting correction updates public captured inventory when
evaluating retaliation after a *known* winning capture. Taking an identified
marshal no longer incurs the old 200-point anti-marshal reserve penalty if
the spy can then be recaptured. An uncertain attack never assumes that it
has already removed the marshal.

`spy_comeback_risk` checks the recorded attack on eight seeds and swaps the
hidden target marshal with a moved hidden lieutenant: choices and final RNG
states must match. The original replay now chooses 68->67 on all eight seeds.
Other checks cover safe retreats versus a boxed spy, moderate versus severe
deficits, stationary/known targets, zero marshal odds, balanced armies,
capture of an identified marshal and an immediate flag-defense override.

These are local decision regressions; neither a full-game victory nor a new
overall win rate has been established.

Step 2 validation: Release build without warnings; all 27 CTest tests passed
in 245.86 seconds. Local logs: `reports/spy-build.log`, `reports/spy-tests.log`.
