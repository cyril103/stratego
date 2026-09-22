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
