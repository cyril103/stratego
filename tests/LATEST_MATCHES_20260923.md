# Corrections issues des deux parties humaines du 23 septembre 2026

Replays : `ai_win_284_20260923.jsonl` (capitulation humaine) et
`human_win_285_20260923.jsonl` (drapeau de l'IA pris par un démineur).
Les numéros sont les demi-coups du journal ; les positions sont chargées avant
le demi-coup indiqué. A1 correspond à l'indice 0.

## Dernier démineur : préparer une sortie avant l'attaque

Au 168, le dernier démineur de l'IA en D4 est encore indemne, mais les unités
alliées et le lac l'empêchent de fuir après l'approche du commandant connu.
Au 170, la protection contre une capture immédiate arrive trop tard.

L'IA examine maintenant une approche ennemie supplémentaire, uniquement avec
des attaquants identifiés. Elle peut déplacer le bloqueur en D3, puis évacuer
le démineur après B4–C4. Une interception certaine par un allié compte aussi
comme protection : ouvrir une sortie n'est pas obligatoire dans ce cas.
Cette priorité ne s'applique qu'au dernier démineur, si des bombes adverses
restent, et après les filtres de défaite immédiate.

## Espion : tirs possibles d'éclaireurs inconnus

Au 238, l'espion en F3 est exposé à une pièce mobile non identifiée en F10.
Son avance en F4 ne supprime pas le danger. Il est capturé au 239 alors que le
maréchal humain est encore vivant.

La protection considère désormais la ligne de tir d'un éclaireur possible.
Le nombre d'éclaireurs encore cachés est déduit des captures et des pièces
révélées, sans consulter les identités cachées. Un retrait ou l'interposition
d'un allié est accepté. Les dangers certains et les tirs seulement possibles
restent séparés afin de préserver les manœuvres préparant une sortie. Une
retraite qui enferme l'espion au tour suivant ne constitue pas un sauvetage.

## Reprises et défense du drapeau

Au 260, F2–E2 offrait un capitaine au maréchal humain. L'évaluation comptait
sur une reprise par le maréchal de l'IA, alors que la conservation du dernier
officier décourageait cette même reprise après la perte du capitaine.
Le calcul tient maintenant compte des pertes et de cette contrainte : une
reprise coûteuse du dernier officier ne rend plus le sacrifice gratuit.

Au 262, l'échange des maréchaux en E2 libère les routes de défense du
lieutenant restant. Une exception autorise l'échange lorsqu'il réduit
fortement la pression sur le drapeau. La comparaison utilise le même trait
(adversaire au prochain tour) des deux côtés.
Dans une petite défense, si cet échange laisse une pression nettement plus
faible que chacune des autres options admissibles, il devient prioritaire.
Le coût matériel de la perte du dernier officier reste néanmoins comptabilisé.

L'échange seul était insuffisant : le lieutenant repartait ensuite vers le
centre. Lorsque cet unique défenseur peut battre un démineur identifié à six
pas au plus du drapeau, il poursuit maintenant une interception accessible.
Les filtres de survie restent prioritaires, et une tentative de capture du
drapeau avec une probabilité supérieure à 50 % reste admissible.

## Vérification

Résultat : compilation sans avertissement du compilateur ; **33 tests CTest
réussis sur 33** (276,13 secondes pour la validation finale).

`latest_human_matches` vérifie les quatre positions sur huit graines, ainsi
que l'invariance des décisions après permutation de rangs adverses cachés.
Le démineur doit réellement pouvoir s'échapper après l'approche enregistrée ;
l'espion doit survivre à toute capture immédiatement légale dans le replay.
Après l'échange défensif, le test rejoue la progression humaine jusqu'à
l'interception : il exige la capture du démineur et la conservation du drapeau.

Le test conserve également l'attaque réussie du maréchal contre l'espion
inconnu au demi-coup 82 de la partie gagnée par l'IA. Il couvre les exceptions
sans bombes, avec plusieurs démineurs, sans éclaireur encore caché, sans
maréchal adverse et avec un allié capable d'intercepter l'attaquant.

```powershell
cmake -S . -B build-napoleonic
cmake --build build-napoleonic -j 6
ctest --test-dir build-napoleonic --output-on-failure -j 4
```

Ces tests établissent la correction des séquences enregistrées et la
préservation des comportements couverts. Ils ne prouvent pas une victoire
forcée contre toutes les réponses humaines ni un nouveau taux de victoire
en tournoi.
