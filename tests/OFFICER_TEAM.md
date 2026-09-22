# Retraites, interception et couverture par l'espion

La défaite `fixtures/human_win_379.jsonl` du 22 septembre 2026 se termine par
l'élimination des 33 pièces mobiles de l'IA. Le correctif ajoute quatre
comportements à Expert+ Improved, à partir des seules informations publiques.

## Retraites et dégagements

Un contrôle tactique suit chaque officier à partir du colonel lorsqu'un
adversaire révélé plus fort se trouve à trois cases au plus. Il explore les
poursuites de cet adversaire et tous les coups légaux du défenseur, jusqu'à
sept demi-coups après le candidat, avec 1 600 nœuds par officier. Les restrictions
d'aller-retour sont conservées. Un budget épuisé ne produit pas de pénalité.
Une reprise possible du poursuivant, notamment par un espion contre un
maréchal, empêche de qualifier la capture de perte sans compensation.

Il s'agit d'une vérification ciblée de perte d'officier, pas d'une preuve de
défaite de la partie. Les combats inconnus du défenseur sont optimistes ;
une hypothétique prise de drapeau inconnu dans une branche future ne fait pas
disparaître l'enfermement. Une capture inconnue au coup candidat conserve
son évaluation générale et n'est pas pénalisée par ce contrôle.

Au demi-coup 108, le général choisit I7–J7 au lieu d'I7–I8, où ses alliés
bloquent toutes ses sorties. Au 226, le démineur libère I9 par I9–H9 avant que
le colonel se réfugie dans le coin. Deux rejeux depuis le 220, avec les coups
humains enregistrés jusqu'au 233, conservent le colonel vivant. Cela valide
cette poursuite, sans supposer que l'adversaire doit toujours la reproduire.

## Interception

Le maréchal peut désormais recevoir une mission contre un général identifié
même si l'espion adverse est encore vivant. La distance d'affectation est
limitée à huit unités de trajet pour ce cas ; les autres missions restent à
six. Le bonus est refusé si le mouvement place le maréchal au contact d'une
pièce inconnue alors que l'espion reste possible. Les protections contre les
attaques connues et l'abandon d'une défense urgente restent applicables.

## Général et espion

Quand un maréchal révélé se rapproche du général, l'IA recherche une case
adjacente au général où l'espion peut préparer une reprise. Le trajet évite
les pièces, les lacs, les attaques connues (y compris les longs déplacements
d'éclaireur) et le contact avec les pièces inconnues. Un déplacement du
général peut aussi maintenir cette formation. L'espion doit pouvoir attaquer
la case du général après sa capture ; la simple proximité ne suffit pas.

Le test de décision choisit E8–E7 pour placer l'espion derrière le général E6.
Le test tactique vérifie ensuite que l'espion élimine le maréchal qui prend
le général. Une variante avec un éclaireur menaçant la case de couverture
supprime le bonus de placement dangereux.

## Conservation du dernier haut grade

Une capture suivie d'un échange connu à égalité reçoit une pénalité
supplémentaire si elle retire le dernier officier capable de contenir les
grades adverses encore possibles, avec infériorité numérique en pièces
mobiles. Les inventaires proviennent des pertes publiques. Ce n'est pas une
interdiction générale des échanges : les captures décisives restent prioritaires.

Au 312, le maréchal choisit E8–E9 au lieu de prendre le général F8 couvert par
le maréchal F7. Il conserve ainsi le dernier haut grade de son armée.

## Validation

`coordinated_officers` couvre les décisions aux demi-coups 108, 226 et 312 sur
les graines 1, 2, 3 et 519. La permutation des grades cachés doit conserver
le coup et l'état du générateur aléatoire. Il couvre aussi les deux poursuites
complètes, l'affectation du maréchal et la tactique général–espion.

Commande : `ctest --test-dir build-napoleonic --output-on-failure`.
Validation du 22 septembre : les 17 tests passent, exécutés en deux groupes
(9 et 8 tests), sans modification des anciens tests. Le test graphique
`--smoke --battle` termine 150 images et deux coups sans erreur ; le pointage
des 100 cases est vérifié. Le binaire est installé dans `build/stratego.exe`.
Ces tests ne constituent pas une mesure du taux de victoire global.
