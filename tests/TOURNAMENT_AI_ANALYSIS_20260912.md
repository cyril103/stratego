# Analyse du duel Improved / Classique et correction du 12 septembre 2026

Reference : `reports/tournaments/improved_classique_10_sans_horloge_20260912`.
Dix parties, cinq placements aller-retour, sans horloge : Improved 3 victoires,
2 nulles et 5 defaites, contre 5 victoires pour Classique. Les deux nulles sont
liees a une immobilisation des deux camps, pas au temps. Les replays ont ete
valides par le moteur.

## Lecture des cinq defaites

| Placement, camp Improved | Fin | Constat dans le replay |
|---|---|---|
| 912, camp 0 | Drapeau, demi-coup 512 | Des officiers sont elimines puis deux demineurs ouvrent la defense. Le general capture encore ailleurs pendant que le demineur arrive a cote du drapeau. Au dernier coup, la defense est deja trop loin pour intervenir. |
| 913, camp 0 | Drapeau, 296 | Les echanges des defenseurs laissent une colonne ouverte. Un eclaireur se repositionne au coup 294 puis prend le drapeau a longue distance au coup 296. A 295, les defenseurs restants ne peuvent deja plus couper cette ligne en un coup. |
| 914, camp 1 | Immobilisation, 540 | Le general est pris tres tot par le marechal adverse, encore inconnu avant ce combat. La fin se joue avec un dernier eclaireur qui attaque une bombe. Cela ne prouve pas qu'une issue gagnante restait disponible. |
| 915, camp 0 | Drapeau, 452 | Les derniers demineurs disparaissent tandis qu'un commandant reste loin du drapeau. Le demineur adverse ouvre la derniere bombe au coup 450 ; le capitaine arrive trop tard. |
| 916, camp 0 | Immobilisation, 764 | Le general est pris au coup 758. Le dernier marechal se rapproche ensuite d'une piece inconnue qui vient vers lui : c'est l'espion qui le prend au coup 764. L'identite de l'espion n'etait pas connue avant le combat. |

Ces observations distinguent les informations publiques des identites connues
uniquement apres coup. Elles ne prouvent pas que chaque partie pouvait etre
sauvee a partir de sa position finale. Le rappel des reserves reste un axe a mesurer separement. La conservation des
derniers demineurs fait l'objet d'une correction ciblee ci-dessous.

## Correction retenue dans Improved

- Une verification tactique de tous les coups candidats elimine une defaite
  immediate publiquement certaine lorsqu'une alternative existe : drapeau
  capture au coup suivant ou disparition de la derniere mobilite. Une attaque
  pouvant prendre un drapeau inconnu immediatement reste autorisee.
- Avec une seule piece mobile, les candidats sont compares d'abord selon le
  risque immediat de perdre cette piece. L'evaluation utilise les rangs encore
  possibles. Une piece inconnue qui s'approche volontairement du marechal
  augmente le risque d'espion (facteur heuristique 4, pas une probabilite calibree).
  Si aucun deplacement n'est sans risque, le moteur compare les risques au lieu
  de renoncer a cette protection.
- Les tests publics de lignes d'eclaireur respectent l'occupation des cases meme
  quand leur rang a ete masque. Une piece inconnue ne devient pas transparente.
- Les attaques exploratoires des un ou deux derniers demineurs contre une
  piece inconnue ayant deja bouge recoivent une penalite de perte de capacite
  de deminage, si des bombes adverses restent en jeu. Les attaques de bombes,
  les tentatives contre un drapeau potentiel et les recaptures connues ne sont
  pas penalisees par ce terme.
- Les restrictions de va-et-vient sont mises a jour dans les positions projetees.
  Une capture gagnante du drapeau et une double immobilisation certaine gardent
  leur statut de victoire ou de nulle.

Le moteur Classique, les poids de la politique entrainee et les budgets de
recherche n'ont pas ete modifies. Il ne s'agit pas d'un nouvel entrainement ML.

## Verification des corrections

La position precedant le demi-coup 763 de la defaite 916 est conservee dans
`tests/fixtures/tournament_loss_916.jsonl`. Avec huit graines de recherche,
l'ancien executable choisit quatre fois le deplacement fatal 51 -> 61.
Le nouveau test exige un risque inferieur a celui de ce deplacement et verifie
que permuter les identites cachees ne change ni le coup choisi ni l'etat aleatoire.
La defaite 915 est egalement conservee. Avant son demi-coup 441, les huit
variantes renoncent a sacrifier le dernier demineur contre la piece mobile
inconnue en 72. Les estimations publiques de perte dans les attaques 337 et
441 de cette partie sont respectivement 66,7 % et 50 % ; dans la defaite 912,
le coup 417 a un risque estime de 88,9 %. Ce sont les estimations de la
fonction de probabilite, pas une connaissance de la piece cachee.
Des positions synthetiques couvrent egalement le blocage d'une attaque du drapeau
par un eclaireur et l'occupation d'une case de rang masque.

Une extension des evasions dans la recherche tactique a ete essayee puis ecartee :
elle degradait des positions de reference (course au drapeau et retrait d'un general).
Elle ne fait pas partie de la correction retenue. Les deux premieres executions
experimentales ont ete arretees apres echec de tests et ne constituent pas des
resultats de tournoi utilisables.

## Evaluation comparative

Le comparatif de la version retenue est enregistre dans
`reports/tournaments/improved_safety_miners_v4_20260912`. Meme serie de dix parties,
memes placements, camps alternes, aucun chronometre et plafond technique de
5000 demi-coups. Les inachevees restent distinctes des nulles.
Les 11 tests passent sur cette version, y compris les anciennes defaites et
victoires, les nouveaux cas de finale et les permutations des rangs caches.
Le test graphique `--smoke --battle` retourne 0 sur l'executable du jeu remplace.

La version intermediaire safety_v3 a ete ecartee : elle perdait une nulle
sur le placement 915, camp 1. Des tests supplementaires ont corrige le traitement des echanges egaux : la
double immobilisation est reconnue quand le dernier rang adverse se deduit
du bilan des captures, et quand c'est l'adversaire qui effectue cet echange.
Cela ne suffit pas a retablir l'ancienne nulle sur le placement 915, camp 1 :
la nouvelle trajectoire diverge des le demi-coup 282, bien avant la finale.
Le resultat de la partie entiere reste une regression sur cet exemple.

| Version Improved | Victoires | Nulles | Defaites | Points / 10 |
|---|---|---|---|---|
| Avant correction | 3 | 2 | 5 | 4 |
| Apres correction | 4 | 1 | 5 | 4.5 |

Meme reference Classique, memes cinq placements aller-retour, memes graines de recherche initiales et budgets, sans horloge. Aucune partie inachevee. Les 10 replays de la nouvelle serie sont valides.

| Placement | Camp Improved | Avant | Apres |
|---|---|---|---|
| 912 | 0 | Defaite | Defaite |
| 912 | 1 | Victoire | Victoire |
| 913 | 0 | Defaite | Defaite |
| 913 | 1 | Victoire | Victoire |
| 914 | 0 | Victoire | Victoire |
| 914 | 1 | Defaite | Defaite |
| 915 | 0 | Defaite | Victoire |
| 915 | 1 | Nulle | Defaite |
| 916 | 0 | Defaite | Defaite |
| 916 | 1 | Nulle | Nulle |

Le gain net est de 0,5 point. Le placement 915, camp 0, passe de defaite a victoire ; le camp 1 du meme placement passe de nulle a defaite. Classique reste devant, 5,5 a 4,5.

Au demi-coup 337 du placement 915, camp 0, le premier changement consiste a remplacer une attaque perdante du demineur 15 -> 14 par le repositionnement du lieutenant 95 -> 94. Le demineur est ensuite pris par le sergent adverse au coup 338 : le gain est ici une meilleure priorite de defense et de tempo, pas la survie de tous les demineurs.

La serie a servi au diagnostic : ce resultat est une verification sur les positions travaillees, pas une validation independante ni une estimation Elo. Des reserves defensives trop tardives restent visibles.

Les details et empreintes de la version sont dans le dossier du comparatif. Le jeu habituel `build/stratego.exe` contient cette version ; `Jouer.cmd` le lance. La reference Classique est conservee.
