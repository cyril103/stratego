# Défense adaptée aux forces restantes

Régression : `fixtures/human_win_479.jsonl`, partie du 22 septembre à 11:47.

Les missions d'interception incluent désormais les sergents, lieutenants et
capitaines identifiés. Le choix d'un défenseur favorise un grade capable de
gagner le combat plutôt qu'un échange, et évite autant que possible de charger
la même pièce de plusieurs missions. Un allié peut recevoir une prime pour
libérer le premier pas du trajet du défenseur, avec contrôle des menaces
connues et du risque de contact du maréchal avec un espion inconnu.

En finale, le rappel du meilleur défenseur fonctionne dès le sergent ; les
menaces identifiées ne sont plus limitées aux grands officiers. La preuve
tactique de capture du drapeau couvre aussi les attaques d'autres grades
près d'une enceinte ouverte, avec au plus quatre pièces mobiles alliées.

Le coût de perte du dernier officier couvre les échanges directs à égalité
et les reprises après une capture. Il utilise les grades restants déduits des
pertes publiques : en infériorité numérique, céder son dernier contre est
pénalisé si l'ennemi garde des pièces au moins aussi fortes que la meilleure
réserve restante. Un officier de remplacement annule cette pénalité. Éliminer
la dernière menace ou échanger les dernières pièces mobiles reste autorisé.
Cette pénalité est une heuristique, pas la preuve qu'un échange perd la partie.

La recherche de pièges couvre aussi l'espion tant que le maréchal adverse
survit. Elle considère les mouvements des alliés, afin de favoriser un
dégagement avant l'encerclement. Une recherche qui épuise son budget ne conclut
pas à une perte forcée ; toutes les cages ne sont donc pas détectées.

Un essai complet a également révélé une poursuite circulaire du colonel adverse
par le maréchal : dernier combat au 138, puis partie inachevée au 1 200.
Le moteur détecte maintenant deux pièces qui ont chacune joué au moins six
fois dans leurs huit derniers tours sur un circuit d'au plus quatre cases.
Répéter une case avec le poursuivant devient coûteux, pour laisser jouer les
autres pièces. Une capture, une nouvelle case, une menace connue sur le
poursuivant ou une urgence du drapeau désactive cette pénalité.

## Vérifications

`adaptive_endgame_defense` rejoue six positions de la partie avec les graines
1 et 519 et une permutation des grades adverses inconnus. Les décisions et
l'état du générateur aléatoire doivent rester identiques après permutation.

- Au 164, le commandant C8–C7 dégage la zone de l'espion ; le départ de
  l'espion B8–A8 est identifié comme un piège dans cette position.
- Au 172, le déplacement du commandant E7–D7 reçoit une prime de dégagement
  pour le maréchal E6. Ce contrôle de coordination ne prétend pas sauver un
  espion déjà enfermé dans le déroulement enregistré.
- Au 406, le maréchal va en J8 au lieu de s'échanger en I7.
- Au 434, l'IA déplace le lieutenant E9–D9 au lieu d'échanger les généraux.
- Au 438, le capitaine D7–D8 revient vers la défense.
- Au 460, le capitaine E8–D8 répond à une mission contre les capitaines adverses.

Le rejeu de l'encerclement depuis le 164 conserve l'espion et s'arrête au 174,
car le prochain coup humain enregistré n'est plus légal. Ce résultat indique
une divergence utile, pas une preuve de survie contre toute continuation.
Le nouveau fixture `unfinished_pursuit_1200.jsonl` vérifie également le
demi-coup 1001 : les graines 1 et 519 déplacent le démineur F3–F4 au lieu de
continuer la ronde du maréchal. Ce test vérifie aussi l'invariance aux grades
adverses cachés.

Des finales construites vérifient également qu'un échange utile n'est pas
pénalisé, qu'une réserve de même grade permet l'échange, et que déplacer un
capitaine de couverture peut laisser perdre le drapeau contre un capitaine
adverse, même sans démineur attaquant.

Les sorties détaillées de cette validation sont dans `reports/defense479-*`.
Les parties complètes utilisent le placement 204, dans les deux camps contre
Expert+ Classique, également utilisé avant cette correction. La limite de
1 200 demi-coups produit un résultat « inachevé », jamais une victoire fictive.
Un échantillon aussi réduit ne mesure pas le niveau contre un joueur humain.

Validation du 22 septembre : les 19 tests CTest passent après la dernière
modification (300,96 secondes, tournoi exécuté en parallèle). Le binaire
installé dans `build/stratego.exe` et celui de `build-napoleonic/stratego.exe`
ont le même SHA256 :
`F0E8BC13858694CFEB6982AB2F4D72B3E0AF1907AA95A6285FFFD7C05DBD566F`.
La sauvegarde précédente est `reports/stratego-before-defense479.exe`.

Le tournoi de développement, avant interruption des poursuites circulaires,
termine avec deux parties inachevées à 1 200 demi-coups, sans victoire ni
défaite. La première n'avait plus de combat depuis le 138 ; la seconde se
termine avec un capitaine contre un commandant et un sergent. Ces résultats
ne doivent pas être présentés comme deux victoires ou deux nulles officielles.
Les journaux du contrôle final sont dans `reports/defense479-final-tournament`.

Le contrôle final termine avec **une victoire, zéro défaite et une partie
inachevée**. Avec l'IA dans le premier camp, victoire par immobilisation au
603, contre une partie inachevée pour la version précédente. Avec les camps
inversés, la limite de 1 200 est atteinte ; le capitaine IA fait face à un
commandant, sans démineur dans les deux camps. Le dernier combat est au 829.
Avant ces corrections, le même comparatif donnait une défaite au 551 et une
partie inachevée. Il s'agit d'un indice favorable sur ce placement, pas d'une
mesure fiable du niveau général ou du taux de victoire contre un humain.

Temps moyen de décision : 373,05 ms pour la nouvelle IA contre 229,97 ms
pour Classique ; maximum observé 3 150,60 ms. Les compilations et tests ont
chevauché une partie du tournoi : ces temps ne constituent pas un benchmark
isolé. Le test graphique du binaire installé termine 150 images et trois
coups, et valide le pointage des 100 cases (`reports/defense479-smoke-final.log`).
