# Exploiter la victoire par élimination des pièces mobiles

Source : `fixtures/ai_win_301.jsonl`, victoire d'Expert+ Improved au demi-coup 301, raison `END_IMMOBILE`.

## Constats

Les échanges des grands grades ont laissé deux commandants dominants à l'IA. La poursuite des pièces mobiles a finalement permis de capturer l'espion en D3 au demi-coup 300. Le dernier démineur humain, F1, n'avait alors qu'un coup légal : attaquer le lieutenant G1, ce qui entraînait sa perte. Ce dernier coup était forcé.

À l'inverse, le capitaine IA avait attaqué une bombe inconnue en G3 au demi-coup 118. Le moteur reproduisait ce choix avec la graine 519. Ce n'était pas une perte obligatoire pour gagner.

## Corrections

- La prudence contre les bombes probables s'étend désormais au capitaine : si une cible dépasse le seuil de probabilité de bombe de 16 %, qu'une pièce de rang inférieur autre que l'espion peut jouer et qu'une autre action non exclue existe, l'attaque est écartée. Cela conserve les exceptions existantes pour les coups forcés, les démineurs et la capture d'un drapeau connu.
- L'activation d'une pièce dominante ne se limite plus aux grands officiers. Pour les sergents, lieutenants, capitaines et commandants, lorsque le bilan public des pertes exclut tout adversaire de grade égal ou supérieur, une prime modérée favorise le rapprochement des ennemis ayant bougé ou déjà identifiés comme mobiles. Les lacs et les pièces bloquant le parcours sont pris en compte. Les pièces immobiles inconnues ne deviennent pas des cibles de cette poursuite.

Ces changements portent uniquement sur Expert+ Improved. Ils ne modifient ni le moteur Classique ni le modèle appris.

Lorsqu'il ne reste qu'une ou deux pièces mobiles adverses, une capture certaine par un de ces grades dominants devient prioritaire si aucune urgence de drapeau n'est détectée avant ou après le coup. La cible doit avoir bougé ou être connue comme mobile : aucune supposition de bombe ou de drapeau caché ne justifie cette priorité. Une capture de drapeau révélé reste prioritaire sur cette règle. Cela évite que des continuations hypothétiques retardent l'élimination d'une pièce réellement accessible.

## Régressions

La position avant 118 est testée avec huit graines pour éviter le sacrifice du capitaine. Une permutation des rangs ennemis cachés doit laisser le choix et l'état aléatoire identiques. La finale avant 300 est aussi testée avec huit graines : capture de l'espion, unique réponse humaine, puis victoire par immobilisation. Des tests synthétiques vérifient la poursuite par un commandant dominant, son arrêt lorsqu'un général adverse subsiste et l'absence de poursuite aveugle des pièces immobiles.

Ces tests ne démontrent pas un gain d'Elo. Aucun entraînement automatique n'est lancé.
