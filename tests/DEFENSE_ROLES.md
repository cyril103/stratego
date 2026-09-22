# Défense et rôles : partie humaine en 625 demi-coups

Référence : `fixtures/human_win_625.jsonl`, partie du 22 septembre 2026 à
10 h 46, gagnée par le joueur par capture du drapeau. Ce correctif traite
les calculs généraux, sans reconnaître le nom du fichier ni les cases du rejeu.

## Connaissance des bombes et trajets d'interception

L'IA connaît les grades de ses propres pièces même lorsqu'ils ne sont pas
révélés à son adversaire. `intercept_distances` utilise désormais cette
connaissance pour estimer une arrivée adverse : un démineur hypothétique peut
traverser une bombe de l'IA, tandis qu'un commandant ne le peut pas. Les grades
adverses cachés restent masqués. Les trajets des réserves évitent les cases
intermédiaires contrôlées par une pièce connue plus forte.

Les cartes d'arrivée au drapeau sont calculées une fois par grade, puis
réutilisées pour tous les intrus, au lieu d'être recalculées pour chaque pièce.
Le test au demi-coup 610 retrouve une menace ; révéler artificiellement les
seules pièces alliées ne change plus cette estimation.

## Pièces menacées : valeur matérielle et rôle

La perte connue sans reprise est évaluée pour toutes les pièces mobiles.
Les pièces ordinaires conservent une pénalité matérielle réduite ; les hauts
grades, derniers démineurs utiles et derniers gardiens proches du drapeau
conservent une priorité forte. L'espion a une valeur stratégique renforcée
tant que le maréchal adverse subsiste. Il ne suffit donc plus de déplacer
une autre pièce pour ignorer sa capture imminente.

Au 350, l'espion choisit F10–E10 et échappe au commandant F9, au lieu d'être
abandonné. Un gardien actuellement attaquable ne compte plus comme réserve
disponible dans l'estimation du drapeau lorsque l'adversaire joue ensuite.

## Captures et reprises

La protection du dernier haut grade examine maintenant les grades possibles
d'une cible inconnue, pondérés par les probabilités publiques. Elle prend
aussi en compte un adversaire de grade égal au meilleur allié restant,
lorsque l'IA est en infériorité numérique après l'échange.

Au 526, le maréchal choisit D8–D7 contre le commandant connu plutôt que
D8–E8 contre une pièce inconnue couverte par le maréchal. Si le joueur reprend,
l'échange retire son dernier commandant en plus des deux maréchaux.
Il s'agit d'une meilleure cible dans cette position, pas d'une victoire forcée.

## Affectation défensive et rappel de réserve

Les missions contre un intrus connu commencent au commandant. Un intrus
proche du drapeau reste une menace même si aucune pièce faible ne lui est
immédiatement adjacente. Le maréchal peut contenir son homologue lorsque
l'espion allié a été perdu ; le contrôle des contacts inconnus reste actif.
Au 400, un commandant est mobilisé pour répondre à l'intrusion.

Avec au plus six pièces mobiles, le plus haut grade restant reçoit aussi
une priorité de retour vers le drapeau lorsqu'un officier connu ou un démineur
possible approche. L'objectif est la zone défensive, pas uniquement la case
courante d'une cible : un déplacement d'attente adverse n'efface pas la mission.
Les distances prennent en compte les lacs. Ce rappel ne récompense pas une
case exposée à une capture connue sans reprise.

Au 600, le commandant choisit I6–I7 au lieu de la capture éloignée en J6.
Les deux rejeux depuis cette position interrompent l'attaque enregistrée
au demi-coup 616 et conservent le drapeau. Le joueur pourrait choisir une
autre continuation : ces contrôles ne prouvent pas une victoire contre toute
réponse.

## Vérifications

`strategic_defense_roles` couvre les positions 350, 400, 526 et 600 sur quatre
graines (1, 2, 3, 519), avec permutation des grades adverses cachés, ainsi que
les trajets à travers les bombes et deux suites de défense depuis le 600.

Deux attentes de diagnostic dans `test_search.c` évoluent avec le comportement
visé : un gardien condamné ne doit plus créer l'ancien faux soulagement du
risque, et un démineur devenu ordinaire après disparition des bombes garde
sa valeur matérielle, sans prime stratégique. Les contrôles de décision et
les interdictions de sacrifices des anciens rejeux sont conservés.

Les résultats des parties complètes sont conservés dans
`reports/defense625-tournament` (développement, graine 203) et
`reports/defense625-final-tournament` (contrôle final, graine 204).
Chaque placement est joué dans les deux camps contre Expert+ Classique,
avec une limite de 1 200 demi-coups. Une partie atteignant cette limite sans
résultat est comptée comme inachevée, jamais comme une victoire ou une nulle.
Ce petit échantillon n'établit pas un taux de victoire contre un humain.

Validation du 22 septembre : les 18 tests passent (17 tests CTest et le test
de recherche exécuté séparément après sa mise à jour). Le test graphique
termine 150 images et trois coups, avec vérification du pointage des 100 cases.
Le binaire installé dans `build/stratego.exe` correspond à la compilation :
SHA256 `6FFF7F5760A677F607050F92967B025E64766F84CF718C06E8849734FBA119F3`.
Le comparatif de développement termine ses deux parties : une victoire en
541 demi-coups et une défaite en 877, sans partie inachevée.

Le contrôle final (graine 204, même placement dans les deux camps) donne
zéro victoire, une défaite par capture du drapeau en 551 demi-coups et une
partie inachevée à 1 200 demi-coups. Cette dernière conserve un maréchal pour
la nouvelle IA contre un colonel, sans combat depuis le 818. La partie perdue
se termine sous deux menaces proches du drapeau ; l'échange des maréchaux
au 544 précède sa capture au 551. Ces essais ne démontrent pas de progression
globale de niveau : la défense de plusieurs menaces simultanées et la
conversion des finales restent des limites à étudier.

Les temps moyens de décision de la version finale sont de 319,77 ms contre
186,17 ms pour Classique, avec un maximum de 1 867,72 ms pour la nouvelle IA.
Une partie de ces mesures chevauche les autres tests sur la même machine :
ce n'est pas un benchmark de performances isolé.
