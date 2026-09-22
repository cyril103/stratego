# Collective retreat

The root scores the whole public formation after quiet moves and certain
winning captures. It measures unanswered captures and approaches by revealed
invaders, discounts secondary victims, and rewards reducing that exposure.
Unknown ranks cannot create a known invading officer. Recapture support is
checked through legal moves. This remains a soft preference after terminal
defense filters, not a forced retreat order.

`collective_army_care` checks multiple victims, retreat, hidden identities and
support. The approach estimate is local, not a proof that every victim can be
saved or that a two-move attack is forced.
The largest threat is subtracted from this additional term because the main
evaluation already scores it. Otherwise a lone exposed unit could improperly
interrupt a winning flag race; the full recorded-game suite guards this case.

## Scarce miners

With one to three miners and enemy bombs remaining, root evaluation adds a
scarcity-weighted capability cost to probable combat losses and legal enemy
counterattacks. This applies to immobile unknown targets and quiet moves too.
Safe defusing has no cost; a certain flag capture has no counterattack cost.
Tests cover scarcity, bombs, quiet exposure, flag capture and exhausted bombs.
The preference is soft so a necessary defensive sacrifice remains available.
An immobile target in the enemy camp with a majority bomb-or-flag probability
keeps the existing assault evaluation: preserving a capability must not prevent
using it against a probable bomb screen. The recorded continuity test checks
that the miner still converts its flag route on four seeds.

## Coordinated pursuit

In small armies, a second stronger piece earns a bounded bonus for reducing
an identified target's legal safe exits while its partner retains an immediate
capture threat. Exit evaluation includes scouts' rays, obstacles, combat and
legal recaptures. Moves exposing the helper or increasing public flag risk get
no bonus. Tests distinguish closing exits from merely moving nearby, require
a partner, exclude hidden prey and reject an exposed helper. This is a local
coordination preference, not a claim of a forced multi-turn capture.
