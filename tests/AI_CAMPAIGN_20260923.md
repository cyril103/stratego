# Campagne de progression de l'IA

Référence figée : commit `09936c8`. Objectif : améliorer la robustesse et
la force de jeu face à plusieurs styles, sans exploiter les rangs cachés.
Les résultats contre des programmes ne démontrent pas une supériorité sur
la majorité des humains : cette conclusion nécessiterait des parties humaines.

## Bilan et décision de livraison

La campagne est terminée : **336 parties jouées ou arrêtées au plafond**, sur
les différentes versions et protocoles. Les [mesures archivées](benchmarks/campaign_20260923/README.md)
conservent les résultats individuels, les empreintes des sources et binaires,
les placements initiaux, les comparaisons appariées et les audits.

| Série | Victoires | Défaites | Inachevées |
|---|---:|---:|---:|
| Référence, développement à 25 % | 68 | 5 | 7 |
| Candidat final, mêmes 80 parties | 68 | 6 | 6 |
| Candidat final contre la référence, budget natif | 1 | 2 | 1 |

Le gain global n'est **pas démontré**. Sur les 69 parties terminées dans les deux
versions, deux défaites deviennent des victoires et deux victoires deviennent
des défaites : l'écart moyen de score est nul. Cette sélection exclut les parties
inachevées. En tenant compte de toutes les censures, l'écart de score reste
compris entre −8,75 et +7,5 points de pourcentage. Ces bornes ne sont pas un
intervalle de confiance. Le petit échantillon natif ne permet pas davantage
de conclure à une supériorité.

L'audit des mêmes 80 parties relève 31 pertes de maréchal contre 34, et
7 pertes d'espion avec le maréchal ennemi encore vivant contre 11. Ces événements
incluent des échanges et sacrifices favorables ; ce ne sont pas des nombres
automatiques de fautes. Les parties avec au moins 200 demi-coups consécutifs
sans combat passent de 8 à 4. Aucun des deux moteurs n'attaque une bombe connue
sans démineur et aucun ne perd son drapeau contre un éclaireur dans cette série.

Les 37 tests CTest et les 6 tests Python passent. Cela valide les situations
testées, sans établir la force générale du moteur. Le candidat est donc livré
**séparément pour essais**, sans promotion automatique au lancement habituel :

L'état de livraison ci-dessous décrit la fin de cette campagne. Le lanceur
candidat reçoit ensuite les [corrections issues des parties humaines suivantes](BREACH_CONVERSION.md) ;
les mesures archivées et empreintes de cette campagne restent celles de `994ff5a`.

- `Tester-IA-campagne.cmd` lance le candidat local `build/stratego-campaign.exe`,
  issu de `994ff5a`, SHA-256
  `08682BB8230FF0E3E31A191455AC5752FD62E94E0DC9E69DC73BF5F45B58F180`.
- `Jouer.cmd` conserve sur ce poste la référence `09936c8`, SHA-256
  `DAD8EEEEC98F57428504C2C5BD5DDB4287BB4AF8D19145D5C526F0FDF95D758C`.
- Les sources versionnées sont celles du candidat ; une nouvelle compilation
  dans `build/` remplace volontairement la version habituelle. Le README
  décrit la compilation séparée. Aucun exécutable n'est ajouté à Git.

Les prochaines pistes motivées par ces résultats sont la conservation du
dernier allié mobile en défense et la conversion des finales qui stagnent.
Le replay de la défaite en 503 demi-coups est archivé comme limite non résolue.
La validation native finale n'a servi à aucun nouveau réglage.

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

Validation de l'infrastructure : six tests Python ; candidat et référence
identiques sur les 40 demi-coups du test de parité (deux camps, graine 16000).
Le premier dépistage utilise 80 parties à 25 % du budget, 800 demi-coups maximum,
contre les quatre adversaires distincts de la référence elle-même.

## Changements du moteur final `994ff5a`

- Mémoire par identité des approches et retraites face à des rangs révélés.
  Les vraisemblances restent faibles, non nulles et s'estompent sur 120 demi-coups.
  Une attaque évitée est enregistrée sans en déduire un rang : une autre urgence
  peut expliquer ce choix. La relecture reconstruit la mémoire ; les simulations
  n'y ajoutent pas d'observations imaginaires.
- Ajustement itératif des probabilités aux effectifs publics restants. Chaque
  case conserve une distribution normalisée ; les effectifs attendus de drapeaux,
  bombes et rangs mobiles correspondent à l'inventaire. Cette approximation
  marginale n'est pas un posterior bayésien exact, et les mondes échantillonnés
  conservent leur méthode d'affectation sans remise.
- Coût de fermeture de la dernière retraite d'un commandant, colonel ou général
  face à une approche plausible et traversable. Il complète les secours déjà
  prioritaires du maréchal, de l'espion et des derniers démineurs.
- Budget de risque des raids selon le rang, les remplaçants et l'avance matérielle.
  Préférence modérée pour un démineur ayant réellement progressé vers l'objectif,
  afin de stabiliser le couple démineur/escorte. Les urgences restent prioritaires.
- Mutations déterministes de placements, avec conservation de l'armée, des
  couloirs, de la proximité espion/maréchal et des sorties des pièces mobiles.
  Les six anciens modèles restent accessibles séparément pour les comparaisons.
- Audit automatique des replays : bombe connue attaquée sans démineur, espion
  perdu avec maréchal adverse vivant, circonstances des pertes de maréchaux,
  drapeau pris par éclaireur, première révélation des deux officiers supérieurs.
  Un événement est un signal à examiner, pas une preuve automatique d'erreur.

Les adversaires de la ligue sont compilés avec leurs propres anciens fichiers
de croyances, de stratégie et de politique : une modification du candidat ne
doit pas changer silencieusement l'adversaire.

La première intégration a été rejetée par les régressions avant de finir son
tournoi. L'inventaire cohérent abaissait les probabilités du drapeau, ce qui
désactivait deux missions utilisant des seuils absolus anciens et sous-estimait
une ouverture urgente de bombe. Les objectifs sont maintenant sélectionnés
selon leur concentration relative ; ouvrir une porte sous la poursuite d'un
officier et terminer la tentative derrière une porte déjà ouverte ont une valeur
explicite. Un test d'éclaireur exigeait exactement l'ancienne probabilité de 0,3 :
il vérifie maintenant la cohérence entre la probabilité estimée et le risque de
capture, tout en conservant ses scénarios de défense et de permutation cachée.

Le générateur final mélange également la première ligne, garde au moins trois
éclaireurs pour les sondages et conserve l'espion et les officiers supérieurs
en retrait. Chaque mutation admise conserve une fausse poche à drapeau (pièce
mobile voisine d'au moins deux bombes). Le lancement du jeu appelle ce générateur,
au lieu de tirer uniquement parmi les six anciens modèles.

Une vérification supplémentaire a retiré une régularité exploitable : l'exclusion
des officiers supérieurs en première ligne excluait aussi les bombes, dont le
rang numérique est supérieur. Jusqu'à deux bombes y sont maintenant permises,
avec les mêmes contraintes de sorties, éclaireurs, défense et leurre. Sur les
200 graines ordinateur du test, 32 placements n'en ont aucune en première ligne,
89 en ont une et 79 en ont deux. Ce contrôle porte sur la diversité, sans
prétendre établir que chaque placement est stratégiquement optimal.

Les tournois de moteur utilisent les placements `stable` et `random` : cette
modification de `ai_deploy` n'y est pas appelée. Les manifestes des premières
séries précèdent donc le dernier générateur ; cette différence n'affecte pas
leurs placements.
Une série distincte `reports/campaign-evolved` teste les placements générés
finaux, avec le moteur `d9bdc59` : graines 29000 et 29001, camps inversés,
Classique et raider, plafond 800, budget 25 %. Bilan : 6 victoires et 2 défaites.
Elle ne remplace pas la comparaison appariée du moteur.

## Validation technique

- Version finale : `994ff5a` ; générateur de placements : `1673de0`.
- Compilation Release complète sans avertissement ; 37/37 tests CTest réussis
  (450,47 secondes, sous charge de tournoi). Ces temps ne constituent pas une
  mesure comparative de la latence du moteur.
- Six tests Python de protocole, censure, appariement et audit réussis.
- Le test supplémentaire d'ouverture gardée, ajouté à `campaign_army_and_missions`,
  passe sur trois graines : le dernier démineur n'est pas livré au général qui
  protège la bombe devant un drapeau connu.
- 2 800 placements contrôlés, dont 400 générés dans les deux camps. Sur les
  200 graines du camp ordinateur : 200 topologies bombes/drapeau distinctes,
  19 positions de drapeau observées. Ce décompte mesure la diversité, pas la force.
- Le code source du binaire de ligue au budget natif correspond aux fichiers
  `src/` de la version validée (vérification des SHA-256 du manifeste).
- Les replays de la référence reconstruite restent identiques au contrôle
  initial pendant les 40 demi-coups de la graine de parité 16000.

## Itérations du moteur

Le premier candidat corrigé utilise `reports/campaign-v2-dev`, comparé aux mêmes
80 tâches de `reports/campaign-baseline-dev`. Son audit de développement a
identifié une perte évitable du maréchal : au demi-coup 468 de
`classic_17004_stable_1.jsonl`, le retour de 14 à 4 laisse les deux sorties
couvertes après l'approche adverse de 25 à 15. Le rang de cet adversaire est
encore inconnu. Une retraite vers 13 reste disponible.

Le contrôle des retraites simule maintenant une approche légale d'un espion
possible, puis les réponses publiques disponibles : fuite, capture certaine,
ou dégagement par un allié sans perte connue. Une case libre au moment de la
retraite ne suffit donc plus à déclarer celle-ci sûre. Le contrôle conserve les
priorités terminales et n'élimine pas toutes les options si aucune n'est sûre.
Le replay complet rejoint les fixtures ; le test vérifie aussi l'invariance du
choix après permutation des rangs cachés, sur trois graines.

Après cette correction : nouvelle compilation Release complète sans
avertissement, 37/37 tests CTest réussis (472,51 secondes sous charge), et
six tests Python réussis. Le manifeste du tournoi natif correspond alors
exactement aux sources du moteur validé.

Cette correction exige une nouvelle validation. La première validation native
`reports/campaign-native-holdout` a été interrompue avant tout résultat complet :
ses parties partielles ne comptent ni comme nulles ni comme victoires. Le candidat
intermédiaire utilise `reports/campaign-final-dev` (les mêmes 80 tâches de développement)
et `reports/campaign-final-native` (graines nouvelles 28000 et 28001, camps
inversés, placements figés, contre la référence, budget natif, plafond de
1 000 demi-coups). Cette série native se termine à 2 victoires, 1 défaite et
1 partie inachevée ; elle ne suffit pas à démontrer une supériorité statistique.

La comparaison de développement révèle ensuite un excès de prudence de ce
contrôle. Au demi-coup 403 de `classic_17002_stable_0.jsonl`, il écarte le trajet
du maréchal de 68 vers 67, alors que d'autres pièces peuvent jouer sans danger.
Un suspect à distance deux doit encore s'approcher au contact avant de pouvoir
attaquer : le maréchal n'est pas obligé de bouger immédiatement. Attendre avec
une autre pièce est maintenant une réponse admise si elle survit avec certitude,
ne subit pas de perte connue sans reprise et ne livre ni une défaite immédiate
publique ni un tir d'éclaireur sur le drapeau. Le cas initial à deux pièces
mobiles reste protégé : le mouvement d'attente du sergent y serait perdant.
Les deux replays sont conservés en tests, dont l'invariance aux rangs cachés.

La version corrigée est réévaluée dans `reports/campaign-v4-dev`, sur les mêmes
80 tâches, et dans `reports/campaign-v4-native`, sur deux graines encore jamais
utilisées (31000 et 31001), deux camps, référence figée, placements `stable`,
budget natif et plafond 1 000. Les séries précédentes restent archivées ; leurs
résultats ne sont pas attribués au nouveau moteur. Aucun paramètre n'est réglé
sur les résultats de cette nouvelle validation hors développement.

La compilation Release complète de cette dernière correction est sans
avertissement. Les 37 tests CTest passent (450,47 secondes sous charge), ainsi
que les six tests Python. Les empreintes des sources des deux exécutables V4
correspondent exactement aux sources validées, générateur compris.

## Limites observées sur le candidat final

La validation native finale se termine à **1 victoire, 2 défaites et 1 partie
inachevée** contre `09936c8`. Sur quatre parties, ce résultat ne démontre pas
un gain de force. L'audit n'y trouve ni attaque de bombe connue sans démineur,
ni espion perdu avec le maréchal adverse vivant, ni drapeau pris par éclaireur.
Les deux pertes de maréchaux sont des échanges entre maréchaux : l'un est connu,
l'autre encore inconnu avant le combat. Deux parties comportent néanmoins plus
de 200 demi-coups consécutifs sans combat, dont la partie plafonnée à 1 000.

Le développement conserve une faiblesse plus précise dans
`classic_17004_stable_1.jsonl` : le moteur final perd au demi-coup 503, malgré
la réussite du test sur la position historique du demi-coup 468. Dans la
nouvelle suite, le sergent capture le démineur en 11 au demi-coup 498, puis
le commandant adverse le reprend. Le maréchal, dernière pièce mobile, se
retrouve en 3 ; l'espion inconnu en 14 couvre ses deux sorties, 4 et 13.
Le correctif du piège immédiat ne garantit donc pas la conservation durable
d'une pièce permettant d'attendre, notamment quand la défense du drapeau
absorbe le dernier allié. Ce cas reste une limite, pas une erreur déclarée
résolue par le seul passage des tests.

À l'inverse, les trois parties `classic_17002_stable_0`,
`classic_17002_stable_1` et `classic_17002_random_0`, dégradées par le garde
strict (deux défaites et une partie inachevée), sont gagnées par le moteur
final. La seconde était l'unique drapeau perdu contre un éclaireur dans
la série complète du garde strict. Contre Classique, le bilan final est
12/4/4 (victoires/défaites/inachevées), contre 10/4/6 pour la référence.
