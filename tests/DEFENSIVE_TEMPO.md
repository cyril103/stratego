# Protection des officiers et du dernier défenseur

La partie `fixtures/human_win_383.jsonl` expose trois faiblesses corrigées dans
Expert+ Improved. Les grades adverses sont effacés avant toute évaluation ;
les nouvelles règles utilisent uniquement les déplacements, les rangs révélés
et les possibilités restantes dans l'armée adverse.

- Avant le demi-coup 64, le maréchal révélé E6 doit se replier en F6 devant
  l'approche de l'espion inconnu E5. L'intention de poursuite est évaluée par
  rapport à la position réelle de l'officier au moment du mouvement adverse,
  reconstruite à partir de l'historique, pas à sa case de repli simulée.
  Un voisin de cette future case n'a pas nécessairement poursuivi l'officier.
  Un retour sur une case récemment occupée conserve cependant le danger de
  poursuite : un aller-retour ne doit pas effacer la menace.
- Avant le 112, un officier capable d'arrêter le colonel connu reçoit une
  mission d'interception. Cette mission tient compte des lacs et des écrans
  de pièces ; elle existe même si les bombes ferment l'accès direct au drapeau.
  Elle ne doit pas récompenser l'abandon d'une défense du drapeau plus urgente.
  Initialement, cette mission supplémentaire ne mobilisait pas le maréchal
  tant que l'espion adverse subsistait et se limitait à six unités de trajet.
  Le correctif suivant autorise une interception locale du général par le
  maréchal, avec contrôle du contact inconnu : voir `OFFICER_TEAM.md`.
- Avant le 374, le commandant doit choisir D8–E8 plutôt que D8–D7. Le deuxième
  déplacement permet aux commandants adverses d'enfermer la dernière pièce
  disponible en dehors du gardien B9. Une fois cette pièce éliminée, le colonel
  doit bouger et abandonner la bombe A9 au démineur A8.

La première analyse attribuait la faute à l'échange du 378. La vérification
des réponses montre que les replis à cet instant perdaient également : les
commandants adverses pouvaient forcer l'échange. Il faut éviter le piège plus
tôt. Le test porte donc sur le 374, pas sur un prétendu sauvetage au 378.

`ai_guardtempo.h` vérifie des suites courtes de capture du drapeau lorsqu'il
reste au plus trois pièces mobiles et qu'un démineur identifié est à deux cases
du drapeau. La recherche conserve tous les coups du défenseur, dont les coups
d'attente, et les restrictions de répétition. Elle explore jusqu'à neuf
demi-coups, avec au plus 20 000 nœuds par hypothèse. L'épuisement du budget ne
constitue jamais une preuve de défaite. Les grades inconnus susceptibles de
fermer les sorties sont pondérés par leurs probabilités publiques ; ils ne sont
pas lus dans l'état réel. Ce contrôle limité complète la recherche générale,
sans prétendre résoudre toutes les fins de partie à information cachée.

`defensive_tempo_tests` rejoue les trois positions avec les graines 1, 2, 3 et
519 et vérifie l'identité des décisions après permutation de grades cachés.
Il contrôle aussi une fin déjà perdue, l'arrêt par budget, la restauration
d'un coup d'attente et un général non révélé qui doit réellement battre le
démineur adverse. Les autres tests d'officiers, d'interception, de règles et
de parties enregistrées restent applicables.

Commande : `ctest --test-dir build-napoleonic --output-on-failure`.

Validation du 22 septembre 2026 : les 16 tests passent, exécutés en deux
groupes (4 tests de parties/officiers, puis les 12 autres). Aucun ancien test
n'a été assoupli. Le test graphique `--smoke --battle` termine 150 images et
deux coups sans erreur, avec vérification du pointage des 100 cases. Le nouvel
exécutable est installé dans `build/stratego.exe`, utilisé par `Jouer.cmd`.
Ce contrôle ne mesure pas un nouveau taux de victoire contre un humain.
