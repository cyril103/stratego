# Campagne de progression de l'IA

Référence figée : commit `09936c8`. Objectif : améliorer la robustesse et
la force de jeu face à plusieurs styles, sans exploiter les rangs cachés.
Les résultats contre des programmes ne démontrent pas une supériorité sur
la majorité des humains : cette conclusion nécessiterait des parties humaines.

## Protocole

- Ligue contre la référence figée, Expert+ Classique et trois politiques
  spécialisées : raids, démineurs et prudence. Ces trois politiques sont des
  adversaires de stress, pas des approximations de joueurs experts.
- Placements aléatoires et anciens modèles figés ; camps inversés sur chaque
  graine. L'option `evolved` évalue séparément les nouveaux placements.
- Deux plages disjointes : développement à partir de 17000 ; validation à
  partir de 27000. Ne pas régler les paramètres sur la validation.
- Par défaut : 10 graines × 2 placements × 2 camps × 5 adversaires = 200 parties.
- Une partie arrêtée au plafond reste inachevée. Afficher les bornes de score
  et la proportion de parties terminées ; ne pas convertir l'avantage matériel
  en victoire. Comparer également le coût de calcul.
- Le paramètre `--scale` du constructeur applique les mêmes budgets de recherche
  à la référence et au candidat. 25/50 % servent au dépistage ; 100 % conserve
  les budgets natifs. Les adversaires Classique et spécialisés ont leurs propres
  algorithmes et ne constituent pas une comparaison à coût strictement égal.
- Exécutable copié et haché au début de chaque tournoi, protocole et replays
  persistants, reprise explicite des parties manquantes avec `--resume`.
- Comparaison appariée des versions : vérifier les plateaux initiaux, comparer
  séparément les résultats censurés et rééchantillonner les groupes de graines
  entiers pour ne pas considérer les deux camps comme indépendants.

## Chantiers

1. Ligue reproductible et référence indépendante du code modifié.
2. Mémoire publique de comportement, probabilités prudentes et cohérentes.
3. Sécurité de toute l'armée et vérification des réponses adverses.
4. Continuité des missions et gestion du risque selon les rôles.
5. Placements générés avec leurres et contrôle des voies de sortie.
6. Évaluation, sélection, rapport et installation des changements validés.

## Travaux consultés

- Perolat et al., *Mastering the Game of Stratego with Model-Free Multiagent
  Reinforcement Learning*, 2022 : https://arxiv.org/abs/2206.15378.
  Idées retenues : diversité des adversaires et des placements, préservation
  de l'information, éviter les stratégies facilement exploitables. Cette
  campagne ne reproduit ni R-NaD ni l'entraînement massif de DeepNash.
- Présentation des auteurs : https://deepmind.google/blog/mastering-stratego-the-classic-game-of-imperfect-information/.
- Goodman, *Re-determinizing Information Set Monte Carlo Tree Search in Hanabi*,
  2019 : https://arxiv.org/abs/1902.06075. Point de vigilance : les simulations
  ne doivent pas prêter à l'adversaire une connaissance des rangs qu'il n'a
  pas observés. Ce travail porte sur Hanabi ; son résultat chiffré n'est pas
  transposable directement à Stratego.

## Exécution

```powershell
python tools/build_league.py --output build-campaign --reference 09936c8 --scale 100
python tools/league.py --build build-campaign --output reports/campaign-native --pairs 10 --seed 27000 --jobs 2
python -m unittest discover -s tests -p "test_league.py"
```

Validation de l'infrastructure : quatre tests Python ; candidat et référence
identiques sur les 40 demi-coups du test de parité (deux camps, graine 16000).
Le premier dépistage utilise 80 parties à 25 % du budget, 800 demi-coups maximum,
contre les quatre adversaires distincts de la référence elle-même.
