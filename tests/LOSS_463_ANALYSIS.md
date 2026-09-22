# Sacrifice avant l'ouverture de la protection du drapeau

Source conservée : `fixtures/human_win_463.jsonl`, victoire humaine par capture du drapeau au demi-coup 463.

Avant 448, le général IA J9 peut capturer la pièce mobile inconnue I9, qui est en réalité un démineur. Le maréchal humain I10 peut reprendre le général. Le moteur préférait J9→J8 et laissait le démineur atteindre les bombes. Une reprise du démineur après leur ouverture ne suffit pas : son escorte peut alors accéder au drapeau.

La correction concerne uniquement **Expert+ Improved**. Une prime de sacrifice défensif s'applique lorsque :

- la cible est un démineur possible selon les informations publiques, et la capture est gagnante contre les rangs possibles (probabilité calculée d'au moins 99,9 %) ;
- le drapeau est encore entouré de bombes alliées sur toutes ses cases adjacentes ;
- la cible est à quatre déplacements géométriques au plus du drapeau, en contournant les lacs ;
- un haut gradé révélé peut effectivement reprendre l'intercepteur.

La prime vaut 480 fois la probabilité publique de démineur et s'ajoute aux autres évaluations. Ce n'est pas un sacrifice obligatoire, ni une preuve de victoire. Le calcul de proximité ne résout pas toutes les interactions et tous les blocages futurs. Aucune prime n'est accordée pour cette règle si la protection est déjà ouverte, si la capture peut perdre contre un rang encore possible, ou si l'escorte n'est pas identifiée.

Le test de replay vérifie J9→I9 avec huit graines, puis la reprise I10→I9 : le démineur disparaît et les trois bombes entourant F10 restent intactes. Une permutation des rangs cachés doit conserver le choix et l'état aléatoire. Les positions synthétiques couvrent les conditions d'exclusion. La partie est conservée avec les autres régressions.

Il s'agit d'une correction heuristique ciblée, pas d'un apprentissage automatique ni d'un gain d'Elo mesuré.
