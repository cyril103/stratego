# Audit Improved contre Expert+ Classique — 17 septembre 2026

Cet audit décrit la version antérieure. Le cycle suivant et la version actuellement
livrée sont documentés dans [l'audit stratégique](STRENGTH_AUDIT_20260917.md).

## Résultats mesurés

| Version d'Improved | Victoires | Défaites | Inachevées |
|---|---:|---:|---:|
| Référence avant audit | 3 | 5 | 2 |
| Premier correctif complet | 4 | 6 | 0 |
| Version finale livrée | 4 | 6 | 0 |

Aucune partie nulle. Le progrès est ciblé, pas une supériorité démontrée :
les deux parties inachevées deviennent des victoires, mais une ancienne
victoire devient une défaite. Expert+ Classique gagne encore le match 6–4.
Les cinq défaites initiales restent des défaites. Ces placements ont servi
aux corrections ; il faudra d'autres placements pour mesurer la généralisation.

| Graine / camp Improved | Avant | Premier correctif | Version finale |
|---|---|---|---|
| 2917 / 0 | Défaite 632 | Défaite 632 | Défaite 632 |
| 2917 / 1 | Défaite 575 | Défaite 575 | Défaite 575 |
| 2918 / 0 | Inachevée 5000 | Victoire 879 | Victoire 879 |
| 2918 / 1 | Défaite 661 | Défaite 621 | Défaite 621 |
| 2919 / 0 | Victoire 559 | Victoire 415 | Victoire 378 |
| 2919 / 1 | Inachevée 5000 | Victoire 458 | Victoire 485 |
| 2920 / 0 | Défaite 806 | Défaite 659 | Défaite 659 |
| 2920 / 1 | Défaite 589 | Défaite 539 | Défaite 539 |
| 2921 / 0 | Victoire 361 | Défaite 414 | Victoire 283 |
| 2921 / 1 | Victoire 363 | Victoire 380 | Défaite 465 |

Les nombres sont des demi-coups. Les victoires finales sont obtenues par
immobilisation adverse, sauf 2921/camp 0 : l'éclaireur capture le drapeau
par un déplacement long de 65 à 5 au demi-coup 283.

### Régression restante et limites

Le correctif qui désactive une mission sans premier pas constructif rétablit
la victoire de 2921/camp 0, mais modifie aussi 2921/camp 1 dès le demi-coup
108 : l'éclaireur 37–27 remplace le commandant 55–54. Ce premier écart ne
prouve pas, à lui seul, que le déplacement de l'éclaireur est une faute.
La suite perd néanmoins un avantage matériel mobile qui a culminé à +54.
Les démineurs sont capturés aux demi-coups 223 et 335 ; les deux derniers
disparaissent dans des échanges aux 394 et 399. Le dernier sergent est
échangé au 465 et Improved n'a plus de pièce mobile. L'ancienne victoire
au 363 dépendait d'une attaque du dernier espion adverse sur une bombe.
La nouvelle défaite reste une régression observée, même si l'ancien gain
n'était pas une preuve de gain forcé.

Autres faiblesses persistantes : en 2920/camp 0 et camp 1, un espion adverse
capture le maréchal connu aux demi-coups 578 et 447. Les améliorations
d'assaut ne corrigent donc pas toutes les erreurs tactiques ou la conversion
d'un avantage. La recherche locale de drapeau ne remplace pas une recherche
complète de finale. La version est livrée pour ses corrections ciblées et
la résolution des blocages, sans la présenter comme globalement supérieure.

## Protocole

Les modèles sont ceux du menu : Improved (1) et Expert+ Classique (3).
Cinq placements prédéfinis (graines 2917 à 2921), chacun joué dans les deux
camps, constituent la série de dix parties. Les budgets de recherche natifs
restent identiques à ceux du jeu ; aucun chronomètre et aucune offre de nulle.
Le plafond technique est de 5 000 demi-coups : une partie au plafond reste
inachevée, sans victoire ni nulle attribuée.

Chaque série conserve son exécutable figé, les sources, les paramètres et
empreintes, les dix journaux détaillés et les résultats individuels.
Les mêmes placements servent à la comparaison avant/après. Ils ont aussi
servi au diagnostic : ce n'est donc pas une évaluation indépendante ni une
mesure d'Elo. Les processus peuvent tourner simultanément ; leurs durées ne
constituent pas une comparaison fiable de vitesse.
Les modifications portent sur le moteur de décision d'Improved. Classique,
les règles, les formations et les poids du modèle d'apprentissage restent
inchangés ; il ne s'agit pas d'un entraînement neuronal sur ces dix parties.

Référence : `reports/tournaments/assault_audit_20260917_before/`.
Premier correctif complet (4 victoires, 6 défaites) :
`reports/tournaments/assault_audit_20260917_after/`.
Version finale : `reports/tournaments/assault_audit_20260917_final/`.
Un essai intermédiaire `assault_audit_20260917_candidate1` a été arrêté pendant
sa première partie après découverte du blocage du démineur par ses alliés.
Cette partie partielle n'entre dans aucun score avant/après.
La série finale utilise deux processus de match simultanés, sans chronomètre.
Le lanceur interdit ce parallélisme avec une limite de temps. Un test court
distinct vérifie la collecte des résultats ; il n'entre pas dans le score.

## Défauts établis et corrections

### Officiers encore cachés

Dans 2918/camp 0, le colonel caché joue 55–45 au demi-coup 29, à côté d'une
pièce mobile inconnue en 44, puis est capturé par le maréchal. L'ancienne
protection des contacts inconnus ne concernait que les officiers révélés.
Elle s'applique maintenant aussi aux officiers cachés, avec un poids moindre.
Les identités cachées adverses sont toujours effacées avant l'évaluation.
Le test rejoue cette position sur quatre graines et vérifie que l'entrée
dans ce contact est évitée.

### Assaut bloqué malgré l'avantage

Dans 2918/camp 0, une longue finale oppose notamment deux généraux. Le plan
d'assaut exigeait la disparition de tout grade égal : l'armée numériquement
supérieure pouvait donc rester inactive malgré son démineur survivant.
Un grade égal est désormais admis si l'avance en matériel mobile vaut au
moins l'officier engagé et si l'IA conserve au moins deux pièces mobiles
supplémentaires. Aucun grade strictement supérieur adverse n'est admis.

La même position révèle un autre blocage : les capitaines et le général
entourent le démineur. Une pièce amie reçoit maintenant une prime pour ouvrir
sa première avancée sûre vers l'objectif, à portée de l'escorte. Elle ne la
reçoit pas si le démineur peut déjà avancer, si elle expose sa propre pièce,
si le passage exige un combat incertain ou si la défense du drapeau est
compromise. Le test exige un dégagement utile choisi par l'IA dans la position
du demi-coup 561, et pas seulement l'existence d'une mission théorique.

Le premier correctif complet a révélé une régression sur 2921/camp 0 : la
victoire en 361 devenait une défaite en 414. La première divergence est au
159 : la mission pénalisait l'avance du démineur 77–67, alors qu'aucun coup
de la paire ou de son soutien ne recevait une prime positive. La sélection
du plan exige désormais un premier pas légal et constructif. Sinon elle
laisse jouer l'évaluation normale et réexamine la mission au tour suivant.
La fixture `audit_regression_2921.jsonl` vérifie ce cas sans imposer de lire
le rang réel du drapeau adverse.

Une continuation appariée repart de cette position exacte avec les graines
20260917 et 20260918, identiques pour les deux versions, contre le même Classique.
La référence reste inachevée après 400 demi-coups supplémentaires (demi-coup
960). La correction gagne en 40 (demi-coup 600), par immobilisation : le démineur
ouvre une bombe au 595, le général adverse le capture au 598, puis échange
contre le général d'Improved au 600 et ne laisse plus aucune pièce adverse
mobile. Ce résultat dépend des décisions de Classique ; ce n'est pas une
preuve de victoire forcée. Ces continuations ne sont pas ajoutées au score
des dix parties. Sources et journaux :
`tools/continue_match.c` et `reports/tournaments/assault_audit_20260917_probes/`.

### Courses locales vers le drapeau

La finale de 2917/camp 0 illustre une poursuite qui laisse passer un intrus,
avec une restriction d'aller-retour sur le dernier défenseur. Au demi-coup
623, la position réelle est déjà perdue dans le calcul local : le test ne
prétend pas qu'un changement de case suffit à la sauver.

Le nouveau calcul examine jusqu'à deux coups adverses, trois lorsque l'IA
possède au plus six pièces mobiles, et les réponses légales du défenseur.
Il respecte les bombes, les rayons d'éclaireur et les restrictions de
répétition. Les rangs inconnus sont pondérés par leurs probabilités publiques.
Le budget est borné ; une recherche incomplète ne déclare pas une course
forcée. Ce modèle local n'inclut pas toutes les combinaisons entre assaillants.
Des positions contrôlées vérifient le choix de bloquer l'accès plutôt que de
poursuivre, et distinguent un espion d'un démineur devant une bombe.

## Lecture des dix parties de référence

- **2917, camps 0 et 1 : défaites en 632 et 575 demi-coups.** Les hauts grades
  subissent des captures ou des échanges précoces ; les défenses cèdent ensuite.
  Le colonel du camp 0 finit enfermé par les allers-retours au 54. Le colonel
  du camp 1 se replie dans un couloir bloqué, puis est pris au 31. La fin du
  camp 0 ne doit pas être décrite comme une position encore sauvable au 623.
- **2918, camp 0 : inachevée au 5000.** Il reste à Improved un général, deux
  capitaines, un sergent et un démineur, contre le seul général mobile adverse.
  Les 4 441 derniers demi-coups sont sans combat. C'est le cas principal du
  déblocage d'assaut. **Camp 1 : défaite au 661**, après la perte du général
  au 505 puis du colonel au 521 face au maréchal adverse.
- **2919, camp 0 : victoire par immobilisation au 559. Camp 1 : inachevée au
  5000.** Dans cette seconde finale, aucun camp ne possède de démineur ; le
  colonel de Classique fait face à un capitaine, un lieutenant et un sergent.
  Elle diffère donc de la panne d'assaut du placement 2918 : un gain forcé
  d'Improved n'est pas établi par le seul bilan matériel.
- **2920, camp 0 : défaite par drapeau au 806 malgré un avantage matériel.**
  Le démineur adverse ouvre la bombe 90 au 800, puis prend le drapeau 91 au
  806 ; les officiers reviennent trop tard. **Camp 1 : défaite par
  immobilisation au 589**, après des pertes importantes de hauts grades et
  de démineurs. Ces cas rappellent que protéger le matériel ne suffit pas.
- **2921 : victoires en 361 et 363 demi-coups.** Au camp 0, le démineur prend
  le drapeau. Au camp 1, le dernier espion de Classique attaque une bombe :
  cette erreur adverse ne constitue pas une preuve de victoire forcée.

## Vérifications techniques

Les fixtures de l'audit sont conservées dans `tests/fixtures/audit_*.jsonl`.
`tools/audit_tournament.py` relève les combats, pertes et informations connues
avant chaque combat, les séquences sans combat, les diagrammes répétés et les
révélations d'éclaireurs. L'avantage matériel y compte seulement les pièces
mobiles. Un diagramme répété n'est pas présenté comme une infraction aux règles.

Validation finale : 15 tests CTest sur 15 réussis, trois tests Python du
classement réussis, et démarrage graphique `--smoke --battle` terminé avec
code 0. Les dix replays finaux ont été relus et validés par le moteur.
Le match final 2921/camp 0 produit exactement les mêmes événements de jeu
en exécution isolée et dans la série parallèle. Les continuations ciblées
restent distinctes du score des dix parties.

Les résultats, comparaisons, audits détaillés et preuves de validation sont
dans `reports/tournaments/assault_audit_20260917_final/`. L'exécutable de jeu
livré est copié depuis la compilation testée ; son empreinte et celle de la
copie installée sont consignées dans `validation.json`. L'ancien exécutable
est conservé dans ce même dossier sous `stratego_before_audit.exe`.
