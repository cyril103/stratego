# Tournoi entre les modeles du jeu

Depuis la racine du projet, avec CMake, MinGW et Python disponibles :

```powershell
cmake -S . -B build-tournament -G "MinGW Makefiles" -DSTRATEGO_TESTS_ONLY=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build-tournament --target ai_benchmark -j4
python tests/test_tournament.py
python tools/tournament.py --output reports/tournaments/mon_tournoi --seconds 180
```

Le dossier de sortie doit etre nouveau. Les quatre moteurs passent par le meme
selecteur que le menu du jeu. Le modele entraine doit se charger correctement :
aucun remplacement silencieux par une autre IA n'est accepte.

Par defaut, les six confrontations se jouent en aller-retour : 12 parties,
6 par modele, avec une seule position initiale commune (graine 912). Chaque
modele joue une fois chaque camp contre chaque adversaire. Le programme verifie
que les plateaux initiaux sont identiques. Avec `--pairs 3`, chaque confrontation
utilise trois graines consecutives, pour 36 parties et davantage de diversite.

Les parties se deroulent successivement par defaut. Chaque moteur garde son budget de
recherche du jeu : c'est un comparatif pratique, pas un comparatif a puissance
de calcul egale. Le temps total de partie est limite a `--seconds` ; a expiration,
le moteur applique sa fin de periode de jeu (nulle). Le plafond technique
`--plies` vaut 100000 demi-coups : l'atteindre produit une partie inachevee,
jamais une nulle artificielle. Aucune proposition de nulle n'est simulee.

Une victoire rapporte 1 point, une nulle 0,5, une defaite 0. Les inachevees sont
signalees separement ; un classement avec des inachevees reste provisoire.
Les egalites de points ne sont pas departagees arbitrairement.

`classement.md` et `results.json` sont mis a jour apres chaque partie. Les
replays, sorties du moteur, binaire et politique sont conserves. Le manifeste
contient leurs empreintes SHA-256 et les parametres. Les choix utilisent des
graines fixes ; une fin sur horloge peut varier avec la charge de la machine.
Douze parties ne suffisent pas a etablir un Elo fiable.

Pour dix parties entre Improved et Classique, sans horloge :

```powershell
python tools/tournament.py --output reports/tournaments/duel_sans_horloge --models 1 3 --pairs 5 --seconds 0 --plies 5000
```

`--seconds 0` desactive a la fois l'horloge du moteur et le delai maximal du
sous-processus. Cela ne modifie pas les budgets de noeuds propres aux moteurs.
Le plafond de demi-coups reste un arret technique signale comme inacheve,
sans adjudication de victoire ni de nulle.

Avec `--seconds 0`, `--jobs 2` permet deux parties simultanees. Les parties
gardent les memes graines et budgets de noeuds ; seul leur temps de calcul
observe peut changer. Le lanceur refuse plusieurs jobs lorsqu'une horloge
est active, verifie les placements apparies et ecrit les rapports depuis un
collecteur unique. Les lignes du classement restent ordonnees par placement
et camp, independamment de l'ordre de fin des processus.
