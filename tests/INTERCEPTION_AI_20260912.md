# Improved : reserves et trajets d'interception

## Changement evalue

La version v5 ajoute une affectation implicite du defenseur le plus proche a
chaque menace publique contre le drapeau. Le calcul est dans
`src/ai_intercept.h`, appele par `ai_choose` pour Improved uniquement.

- Les trajets tiennent compte des lacs, des occupants, des bombes et des rangs
  connus qui ne peuvent pas etre traverses. Les rayons d'eclaireurs comptent
  comme un deplacement et s'arretent sur les occupants de rang masque.
- Degager une piece amie mobile ajoute un tempo estime. Ces trajets sont des
  estimations sur la position actuelle, pas des lignes forcees de minimax.
- Une menace connue est evaluee avec son rang public. Une piece inconnue qui
  a bouge est envisagee comme demineur avec sa probabilite publique ; sa vraie
  identite n'est pas lue. Les menaces estimees a plus de huit deplacements
  du drapeau ne declenchent pas ce terme.
- Parmi les defenseurs avec au moins 80 % de chances estimees d'arreter la
  piece, le meilleur temps d'approche est compare avant et apres le coup.
  Rapprocher une autre piece alors qu'un meilleur defenseur est deja place
  ne recoit pas de bonus. Eloigner le seul defenseur rapide est penalise.
- Le poids augmente si le defenseur est plus loin de l'assaillant que celui-ci
  du drapeau. Le bonus est borne a 60 et utilise avant la selection des coups
  puis dans leur score final. Il ne remplace pas les controles de survie,
  les tactiques connues ni la recherche existante.
- Les combats inconnus ou non gagnants ne sont pas traites comme un
  repositionnement reussi. Un deplacement exposant le defenseur a une capture
  adverse connue ne recoit pas ce bonus.

Classique, les regles, les formations, les poids ML et les budgets de recherche
restent identiques. Il ne s'agit pas d'un entrainement par autojeu.

## Protocole

La version v4 figee sert de reference. La serie de developpement comporte les
cinq placements 912 a 916, avec permutation des camps, sans horloge. Une serie
distincte utilise les placements 1901 et 1902, egalement dans les deux camps.
La v5 est figee avant les resultats de ses quatre matchs de validation. Pas de changement
de parametres entre les matchs et pas de selection des seules parties gagnees.
Le plafond technique reste 5000 demi-coups ; une partie au plafond serait
inachevee, jamais transformee en nulle.

Dossiers :
- `reports/tournaments/improved_safety_miners_v4_20260912` : ancienne serie v4.
- `reports/tournaments/v4_holdout_1901` : v4 sur les nouveaux placements.
- `reports/tournaments/intercept_v5_dev` : v5 sur la serie de developpement.
- `reports/tournaments/intercept_v5_holdout` : v5 sur les nouveaux placements.

Chaque tournoi fige l'executable et la politique, enregistre leurs empreintes,
et conserve chaque replay. `tools/compare_tournaments.py` verifie l'identite
des plateaux initiaux et rapporte le premier coup divergent.

## Verification technique

Les 11 tests CTest passent sur v5, y compris les anciennes positions tactiques,
les permutations de rangs caches et de nouveaux cas de trajets/interception.
Les trois tests Python du classement passent. Le test graphique
`--smoke --battle` retourne 0 sur `build-tournament/stratego.exe`.
Le journal CTest est conserve dans le dossier de developpement v5.

## Resultats

Les 14 parties sont terminees, sans interruption ni fin au chronometre.
Les 14 replays de v5 ont ete relus et valides par le moteur.

| Serie | Improved v4 | Improved v5 | Classique contre v5 |
|---|---|---|---|
| 10 parties, placements 912 a 916 | 4,5 / 10 (4 V, 1 N, 5 D) | 5,5 / 10 (5 V, 1 N, 4 D) | 4,5 / 10 |
| 4 parties, placements 1901 et 1902 | 1 / 4 (1 V, 0 N, 3 D) | 2,5 / 4 (2 V, 1 N, 1 D) | 1,5 / 4 |
| Total descriptif | 5,5 / 14 | 8 / 14 (7 V, 2 N, 5 D) | 6 / 14 |

Improved remporte les deux series. Trois issues progressent : 912 camp 0 et
1901 camp 0 passent de defaite a victoire ; 1902 camp 1 passe de defaite a
nulle. Les onze autres issues restent identiques. Aucune ancienne victoire
n'est perdue sur ces placements, mais la victoire 1901 camp 1 devient bien
plus longue. Les fichiers `comparison.json` donnent chaque premier coup
divergent ; ils ne constituent pas une preuve de victoire forcee.

Ce sont sept placements aller-retour, dont deux distincts pour la validation.
Le gain observe est encourageant, mais ne suffit pas a etablir une superiorite
statistique generale, un Elo ou un niveau contre un humain. La longue finale
de 1901 camp 1 rappelle que le score masque parfois une conversion inefficace.
Les durees de calcul ont subi la concurrence des processus de test : elles
ne doivent pas servir a une comparaison precise de vitesse entre versions.

Le jeu habituel `build/stratego.exe`, lance par `Jouer.cmd`, contient v5 dans
le choix **Expert+ Improved**. Son empreinte est identique a celle du jeu teste.
Une sauvegarde de v4 est conservee dans
`reports/tournaments/intercept_v5_dev/stratego_v4.exe`. Les empreintes, les
11 tests CTest, les 3 tests Python, le test graphique et la verification des
replays sont consignes dans `intercept_v5_dev/test_validation.json`.

## Observations sur les parties

- 912, camp 0 : ancienne defaite au demi-coup 518, nouvelle victoire au 443.
  Le premier changement intervient au 119 : marechal 50 vers 51 au lieu de
  50 vers 60. Le diagnostic public donne respectivement +2,45 et -2,45 pour
  le terme d'interception. Cela change la trajectoire de la partie ; ce n'est
  pas une preuve de victoire forcee a partir de cette position.
- 912, camp 1 : les 3018 demi-coups restent exactement identiques a v4,
  y compris la victoire finale.
- 913, camp 0 : la defaite subsiste, par immobilisation au 438 au lieu d'une
  prise du drapeau au 404. Toutes les faiblesses ne sont pas corrigees.
- 1901, camp 0 : nouvelle victoire par immobilisation au 519, contre une
  ancienne defaite par prise du drapeau au 700. Le commandant finit par prendre
  le dernier capitaine adverse ; l'autre capitaine adverse venait de mourir
  contre une bombe.
- 1901, camp 1 : victoire conservee, mais beaucoup plus lente (4873 demi-coups
  contre 476). Elle se termine par le marechal de Classique attaquant une
  bombe. Ce resultat ne demontre pas qu'Improved sait forcer efficacement
  l'echange gagnant en finale ; la conversion des avantages reste perfectible.
