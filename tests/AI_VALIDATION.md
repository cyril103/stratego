# Validation Expert +

Comparaison avec le moteur Expert de la version précédente, figé dans `ai_previous.c`.
Les résultats comparatifs suivants concernent le moteur antérieur aux corrections tirées de la partie humaine de 175 demi-coups. Ils n'ont pas été remesurés après ces corrections.
Ils ne constituent ni une preuve d'invincibilité, ni une mesure du niveau contre un humain.

## Banc de développement : mêmes formations aléatoires

Commande : `ai_benchmark 3 700 2`. Graines 2, 3 et 4, camps alternés.
La limite est de 700 demi-coups, et une partie non terminée est comptée séparément.

| Graine | Camp nouvelle IA | Demi-coups | Résultat |
|---|---:|---:|---|
| 2 | 0 | 248 | Défaite |
| 2 | 1 | 338 | Victoire |
| 3 | 0 | 300 | Défaite |
| 3 | 1 | 400 | Victoire |
| 4 | 0 | 375 | Victoire |
| 4 | 1 | 242 | Victoire |

**4 victoires, 2 défaites, aucune partie inachevée.** Ces graines ont servi au développement : ne pas interpréter ce score comme une estimation indépendante du taux de victoire.
Temps observé sur cette machine : moyenne 131 ms, maximum 381 ms, sur 952 décisions.

## Placement distinct : nouvelle formation structurée

Commande : `ai_benchmark 1 700 11 -1 1`. Graine 11, non utilisée pour ajuster les variantes précédentes.
La nouvelle IA utilise sa formation structurée ; l'ancienne conserve le placement aléatoire historique.
Le dernier argument active cette différence de placement.

| Graine | Camp nouvelle IA | Demi-coups | Résultat |
|---|---:|---:|---|
| 11 | 0 | 386 | Défaite |
| 11 | 1 | 242 | Victoire |

**1 victoire, 1 défaite.** Cet échantillon est trop petit pour estimer un niveau de jeu.
Temps observé : moyenne 122 ms, maximum 509 ms, sur 314 décisions.

## Tests de comportement

### Régression sur une défaite humaine enregistrée

Fixture : `fixtures/human_win_175.jsonl`, victoire humaine au demi-coup 175.
Le test `recorded_human_defeat` rejoue et valide tous les coups avec le moteur de règles, puis vérifie :

- Avant le demi-coup 120, la recherche (graine 519) choisit lieutenant 34 → 24 pour commencer à libérer le maréchal. Le plan de dégagement propose ensuite démineur 44 → 34 puis maréchal 54 → 44. Ces deux étapes sont testées comme un plan, sans simuler les réponses humaines.
- Avant le demi-coup 162, aucune des huit graines testées ne choisit une attaque perdante contre un rang déjà connu lorsqu'une alternative existe.
- Une permutation de rangs humains cachés dans cette position ne change ni la décision ni l'état aléatoire final.

La mobilisation utilise uniquement les rangs ennemis révélés. L'évaluation défensive ne compte plus comme garde efficace une pièce enfermée sans liberté immédiate. Ces tests établissent la correction de comportements précis ; ils ne prouvent pas que la nouvelle IA aurait remporté la partie entière.

### Interception après la seconde défaite humaine

Fixture : `fixtures/human_win_349.jsonl`, victoire humaine au demi-coup 349 malgré la disparition de son maréchal et de son général.
Le calcul de risque compare les délais d'arrivée des intrus et des défenseurs sur les plus courts chemins terrestres vers le drapeau. Il tient compte des obstacles, de la protection des cases par les ennemis révélés et du camp qui joue. Un intrus mobile non identifié est considéré comme un démineur potentiel, sans lire son rang caché. Le risque est réévalué après chaque candidat de la recherche, y compris si un échange fait disparaître un défenseur.

À partir du demi-coup 340, le test rejoue l'infiltration humaine 39 → 29 → 19 → 9 → 8 → 7, avec quatre graines de recherche. Dans les quatre essais, le capitaine reste disponible, joue 28 → 18 puis capture l'intrus en 19. La capture du drapeau est empêchée. Une permutation des rangs cachés des deux intrus ne change ni ce calcul de risque ni la décision de l'IA.

Ce scénario teste une continuation précise, pas toutes les réponses possibles du joueur. Les calculs d'interception restent approximatifs : chemins les plus courts, gardes évalués individuellement et déplacements adjacents pour les délais, sans solveur de poursuite complet ni prise en compte de la vitesse longue portée de l'éclaireur dans ce calcul additionnel.

### Autres tests

### Initiative après la partie de 511 demi-coups

Fixture : `fixtures/human_win_511.jsonl`. La recherche valorise désormais l'information nouvelle d'un combat selon l'incertitude des rangs possibles. Ce bonus est particulièrement important pour les éclaireurs, réduit pour les démineurs et espions, nul contre une pièce déjà identifiée. Il diminue lorsqu'une capture favorable connue est disponible. Les petites pièces peuvent prendre un risque contre une pièce inconnue ; les interdictions contre les bombes identifiées restent en place. Le bonus de simple déplacement d'un grand officier a été réduit, et une riposte favorable immédiate reçoit un bonus.

Avec la graine de régression 519, au demi-coup 112 l'IA choisit une reconnaissance 59 → 69 avec un éclaireur, au lieu du déplacement de démineur enregistré. Au demi-coup 450 elle approche son général 58 → 59, puis, après la réponse humaine enregistrée 31 → 21, capture le commandant identifié en 69. D'autres graines peuvent sélectionner un autre plan : ce test ne prétend pas imposer systématiquement ces coups. Les tests des deux défaites précédentes sont conservés.

Ce réglage donne un prix heuristique à l'information ; il ne calcule pas exactement sa valeur future et ne constitue pas une mesure indépendante du taux de victoire.

### Règles et recherche

- Composition et identités uniques sur 100 formations structurées (50 graines, deux camps).
- Équivalence de la génération rapide des coups avec le prédicat de légalité exhaustif.
- Règles de combat, lacs, éclaireurs, immobilisation et répétitions.
- Capture immédiate du drapeau, démineur/bombe, espion/maréchal.
- Rejet d'une capture protégée et repli d'un général menacé.
- Attaque gagnante à cinq demi-coups vérifiée par un oracle exhaustif indépendant.
- Protection du maréchal contre une bombe inconnue, pour plusieurs graines et permutations cachées.
- Protection d'une ligne directe d'éclaireur vers le drapeau allié.
- Invariance du coup et du générateur aléatoire après permutation des rangs cachés.
- 2 260 demi-coups simulés pour les règles, dont 160 avec la recherche Expert +, en complément des parties de comparaison.
- Instantané indépendant, double lancement, arrêt, reprise et publication du résultat du calcul en arrière-plan.

Les comparaisons servent aussi à détecter les régressions : augmenter la profondeur ou ajouter une règle heuristique ne suffit pas à améliorer systématiquement une IA à information cachée.

## Limites

### Garde derrière une bombe : partie de 265 demi-coups

La fixture `fixtures/human_win_265.jsonl` reproduit la défaite au demi-coup 265. Au demi-coup 262, le général quittait la case 17 pour capturer le colonel en 18 ; le démineur humain jouait ensuite 8 → 7 (bombe), puis 7 → 6 (drapeau).

Le calcul d'interception admet maintenant une bombe alliée comme case de reprise après son désamorçage, sans autoriser le défenseur à traverser cette bombe intacte. Le test rejoue l'attaque avec quatre graines : le général reste en 17, puis reprend le démineur en 7 après le désamorçage. La permutation du rang caché de l'intrus avec celui d'une autre pièce mobile cachée ne change ni le risque calculé ni la décision. Les régressions de reconnaissance, contre-attaque et défense des parties précédentes restent actives. Cela établit la correction de cette fin de partie, pas un taux de victoire global.

### Limites générales

### Maréchal exposé à un espion possible : partie de 239 demi-coups

La fixture `fixtures/human_win_239.jsonl` reproduit la partie du 8 septembre à 11:21. Au demi-coup 26, le maréchal 55 → 65 se plaçait à côté de la pièce adverse non identifiée en 66, qui le capturait au coup suivant en tant qu'espion. La recherche Expert ajoute désormais un coût d'exposition à un espion possible, calculé uniquement à partir des probabilités publiques, indépendant de la présence de cet espion dans les seize simulations. Ce coût disparaît quand les informations publiques excluent ce rang chez les voisins concernés.

Huit graines vérifient l'abandon du déplacement 55 → 65 et l'invariance du choix après permutation de l'espion caché avec un démineur caché déjà mobile. Les tests des autres parties restent actifs. Cela ne prouve pas que l'Expert remporterait toute cette partie.

Les anciens journaux étiquetaient par erreur tout niveau non nul comme `ExpertPlus`, y compris le mode entraîné. Le mode exact de cette ancienne partie ne peut donc pas être établi à partir de ce champ seul. Les nouveaux journaux incluent le niveau numérique et distinguent `SelfPlayPolicyV1` de `ExpertPlus`. Cette correction de recherche concerne le mode Expert ; elle n'est pas un nouvel entraînement du modèle expérimental.

La recherche reste sélective et bornée. Les formations cachées sont échantillonnées ; les futurs coups d'une simulation reposent sur des rangs supposés. Les plans peuvent donc rester erronés, et l'adversaire peut bluffer. Il n'y a pas de réseau neuronal entraîné par autojeu, de solveur complet du Stratego, ni d'apprentissage automatique à partir des journaux des parties.
