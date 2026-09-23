# Mesures de la campagne du 23 septembre 2026

Les fichiers JSON conservent les protocoles, empreintes des binaires et sources, graines, camps, empreintes des placements initiaux et résultats de chaque partie.
Les replays complets restent dans `reports/` localement ; les régressions du piège et de l’attente du maréchal sont versionnées dans `tests/fixtures/campaign_marshal_pocket_507.jsonl` et `tests/fixtures/campaign_marshal_standoff_783.jsonl`.

`remaining_marshal_pocket_503.jsonl` conserve une défaite du moteur final : le dernier allié disparaît après la défense du drapeau et le maréchal reste piégé. Ce cas est une limite observée, pas une régression déclarée résolue.

| Série | Budget | Parties | Victoires | Défaites | Nulles | Inachevées |
|---|---:|---:|---:|---:|---:|---:|
| baseline | 25 % | 80 | 68 | 5 | 0 | 7 |
| intermediate | 25 % | 80 | 67 | 6 | 0 | 7 |
| strict_guard | 25 % | 80 | 65 | 7 | 0 | 8 |
| final | 25 % | 80 | 68 | 6 | 0 | 6 |
| native_previous | 100 % | 4 | 2 | 1 | 0 | 1 |
| native | 100 % | 4 | 1 | 2 | 0 | 1 |
| evolved | 25 % | 8 | 6 | 2 | 0 | 0 |

## Comparaison appariée à budget réduit

| Adversaire | Référence V/D/I | Intermédiaire V/D/I | Garde strict V/D/I | Final V/D/I |
|---|---|---|---|---|
| classic | 10/4/6 | 12/4/4 | 10/5/5 | 12/4/4 |
| raider | 19/1/0 | 19/1/0 | 18/1/1 | 19/1/0 |
| miner | 19/0/1 | 18/0/2 | 18/0/2 | 19/0/1 |
| cautious | 20/0/0 | 18/1/1 | 19/1/0 | 18/1/1 |

## Audit des replays

| Série | Maréchaux perdus* | Espions perdus avec maréchal adverse vivant* | Attaques non démineur sur bombe connue | Drapeaux pris par éclaireur | Parties avec au moins 200 demi-coups consécutifs sans combat |
|---|---:|---:|---:|---:|---:|
| baseline | 34 | 11 | 0 | 0 | 8 |
| intermediate | 29 | 9 | 0 | 0 | 4 |
| strict_guard | 27 | 9 | 0 | 1 | 5 |
| final | 31 | 7 | 0 | 0 | 4 |
| native_previous | 2 | 0 | 0 | 0 | 0 |
| native | 2 | 0 | 0 | 0 | 2 |
| evolved | 4 | 2 | 0 | 0 | 0 |

*Ces pertes incluent des échanges et sacrifices parfois favorables. Les comptes ne sont pas des nombres automatiques de fautes.

## Limites

- Les séries baseline, intermediate, strict_guard et final comparent les mêmes 80 tâches, sur cinq graines, deux placements et les deux camps. Les bornes tenant compte de la censure sont dans `comparisons.json`.
- Une partie atteignant le plafond reste inachevée. Les intervalles calculés seulement sur les parties terminées peuvent être biaisés par cette sélection.
- Les intervalles Wilson conservés dans les JSON supposent des observations indépendantes ; les camps et adversaires partageant une graine sont ici corrélés. La comparaison rééchantillonne les cinq groupes de graines, mais ses intervalles restent exploratoires sur un échantillon aussi petit.
- Les trois spécialistes sont des adversaires de stress, pas des joueurs experts. Leurs scores élevés ne mesurent pas un niveau humain.
- Native utilise les quotas de recherche du jeu, contre la référence figée 09936c8, sur deux graines nouvelles (31000 et 31001) et les deux camps. Native_previous mesure le garde strict sur les graines 28000 et 28001. Quatre parties par version ne suffisent pas à établir une supériorité statistique.
- Evolved teste séparément les placements finaux, avec le moteur d9bdc59, sur deux graines nouvelles contre Classique et le spécialiste de raid. Ce protocole ne permet pas de comparer directement sa force aux placements précédents.
- Le moteur de décision final est 994ff5a. Le générateur est 1673de0. Les manifestes final et native correspondent à toutes les sources livrées ; le générateur ne sert pas dans leurs placements stable/random.
- Les temps de décision ont été relevés sous des charges concurrentes, priorités variables et une interruption prolongée de la session. Ils ne permettent pas une comparaison fiable des performances.
- Aucun classement humain, Elo humain ni pourcentage de victoire contre les humains ne peut être déduit de ces adversaires automatiques.
