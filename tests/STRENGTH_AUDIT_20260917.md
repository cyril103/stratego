# Amélioration stratégique d'Improved — 17 septembre 2026

**Historique de la référence livrée à 17 h 33.** Un correctif tactique ultérieur
est décrit dans [l'audit des deux parties humaines](../reports/analyses/duel_20260917_1933/analyse.md).
Les scores de tournoi ci-dessous concernent la référence, pas ce correctif.

La version livrée (itération 6) gagne son contrôle indépendant **6–4**, sans
partie inachevée, après un développement à **6 victoires, 2 défaites et
2 inachevées**. Le critère fixé est atteint. L'exécutable de `Jouer.cmd` a été
remplacé par cette version, avec sauvegarde du précédent.

| Série de la version livrée | Victoires | Défaites | Inachevées |
|---|---:|---:|---:|
| Développement, placements 4917–4921 | 6 | 2 | 2 |
| Contrôle indépendant, placements 5917–5921 | 6 | 4 | 0 |
| Total des deux séries | 12 | 6 | 2 |

Le progrès est mesuré sur ces séries. Improved reste battable ; plusieurs
contrôles ont été tentés sur des versions successives. Le dernier 6–4 ne
constitue donc pas une preuve statistique de supériorité générale.

## Protocole

Chaque série comprend cinq placements, joués dans les deux camps : dix parties,
mais cinq placements indépendants. Improved affronte Expert+ Classique, dont le
code est inchangé. Aucun camp ne connaît les grades adverses encore cachés.
Les budgets de recherche sont ceux de chaque modèle ; le calcul n'est donc pas
égalisé entre modèles. Les matchs n'ont pas de pendule. Les expériences finales
s'arrêtent à 2 000 demi-coups : une partie interrompue à cette limite est
inachevée, sans point attribué, et non une nulle. Aucune nulle n'est proposée.

Les exécutables, politiques, sources, empreintes SHA-256, résultats et replays
sont conservés dans `reports/tournaments/strength_cycle_20260917/`.
Les séries de développement servent aux corrections. Un contrôle ultérieur
utilise des placements réservés avant de les examiner, sans modifier le programme
pendant la série. Le critère fixé est six victoires sur dix en développement,
puis six sur dix au contrôle. Ce petit échantillon ne mesure pas un classement
Elo et ne prouve pas une supériorité universelle.

## Stratégies modifiées

- La valeur d'un grade dépend de l'armée restante : supérieurs adverses encore
  vivants, nombre d'égaux, grades inférieurs dominés et réserves amies. Un échange
  de généraux n'a pas la même valeur si notre colonel devient ensuite dominant.
- Le dernier démineur vaut davantage lorsque des bombes adverses subsistent.
  L'espion conserve sa valeur particulière tant que le maréchal adverse existe.
- L'évaluation des captures tient compte de l'équilibre des forces après échange.
  Cette correction est bornée pour ne pas effacer une menace tactique immédiate.
- Lorsque l'IA domine, la recherche donne davantage de poids aux mauvaises
  issues plausibles. Elle conserve au moins sa prudence antérieure lorsqu'elle
  est en retard. La récompense d'information diminue avec un avantage net.
- Les interceptions, la défense escortée et le soulagement d'une menace publique
  contre le drapeau pèsent davantage. Une capture incertaine ne reçoit pas un
  crédit de défense comme si son succès était garanti.
- La recherche de finale compte les grades mobiles survivants à partir des
  pertes publiques, au lieu de compter les bombes cachées comme des unités mobiles.
- Une mission de reconnaissance valorise la progression des éclaireurs et le
  dégagement de leur trajet par un allié. Elle compare des cartes de trajet avant
  et après le coup, avec un bonus borné. Elle est suspendue en présence d'une
  menace contre un officier ou le drapeau. La pénalité existante pour dévoiler un
  éclaireur par un long déplacement sans attaque est conservée.
- L'itération 6 propage les grades logiquement certains grâce aux pertes publiques
  et aux déplacements. Une pièce ainsi identifiée est retirée du sac des grades
  encore à échantillonner. Une probabilité simplement élevée ne suffit pas.
- Son interception considère tous les grades mobiles possibles d'un inconnu.
  L'ancienne approximation le traitait uniquement comme un démineur et pouvait
  ignorer complètement une menace lorsque cette probabilité devenait nulle.

La reconnaissance n'est pas une connaissance cachée : les cartes utilisent
uniquement les observations publiques et les probabilités de grades. Les règles,
les formations, le modèle appris et Expert+ Classique ne sont pas modifiés.

## Historique des expériences

La version installée au début de ce travail perdait 4–6 sur les placements
2917–2921, joués dans les deux camps (audit précédent).

| Expérience | Résultats enregistrés | Décision |
|---|---|---|
| Itération 1 | 6 victoires, 3 défaites ; dernière partie arrêtée avant son plafond | Rejetée : stagnation et régressions pendant la préparation |
| Itération 2 | 1 victoire, 1 défaite ; 2 parties arrêtées, 6 non lancées | Rejetée : stagnations malgré 15 tests réussis |
| Itération 3, développement | 7 victoires, 3 défaites | Progrès sur les placements connus ; 15 tests réussis |
| Itération 3, contrôle 3917–3921 | 5 victoires, 3 défaites, 2 inachevées | Critère de six victoires non atteint ; nouvelle correction |
| Itération 4 | 1 victoire, 1 défaite ; 2 parties arrêtées, 6 non lancées | Rejetée : la reconnaissance perturbait des urgences défensives |
| Itération 5, développement 3917–3921 | 8 victoires, 2 défaites, aucune inachevée | Cinq anciennes victoires conservées, une défaite et deux inachevées transformées en victoires |
| Itération 5, contrôle 4917–4921 | 3 victoires, 5 défaites, 2 inachevées | Échec du contrôle ; ne justifie pas une livraison comme version supérieure |
| Itération 6, développement 4917–4921 | 6 victoires, 2 défaites, 2 inachevées | Trois anciennes défaites deviennent des victoires ; les trois anciennes victoires sont conservées |
| Itération 6, contrôle 5917–5921 | 6 victoires, 4 défaites, aucune inachevée | Critère atteint ; version retenue et livrée |

Les séries arrêtées ne sont jamais présentées comme dix parties complètes et
leurs parties interrompues ne sont pas comptées comme des nulles. Les fichiers
`rejected.json` donnent les résultats et les derniers demi-coups effectivement
enregistrés. Le contrôle 3917–3921, une fois analysé, devient un jeu de
développement : il n'est plus indépendant pour les versions suivantes.

| Placement / camp Improved | Itération 3 | Itération 5 | Demi-coups de l'itération 5 |
|---|---|---|---:|
| 3917 / 0 | Défaite | Défaite | 500 |
| 3917 / 1 | Défaite | Défaite | 450 |
| 3918 / 0 | Défaite | Victoire | 1257 |
| 3918 / 1 | Victoire | Victoire | 552 |
| 3919 / 0 | Victoire | Victoire | 583 |
| 3919 / 1 | Victoire | Victoire | 487 |
| 3920 / 0 | Inachevée | Victoire | 459 |
| 3920 / 1 | Inachevée | Victoire | 458 |
| 3921 / 0 | Victoire | Victoire | 500 |
| 3921 / 1 | Victoire | Victoire | 486 |

Ces huit victoires correspondent à sept immobilisations adverses et une capture
de drapeau. Les dix replays ont été relus et validés par le moteur de règles.

### Deuxième série de développement

| Placement / camp Improved | Itération 5 | Itération 6 | Demi-coups de l'itération 6 |
|---|---|---|---:|
| 4917 / 0 | Défaite | Défaite | 572 |
| 4917 / 1 | Défaite | Victoire | 398 |
| 4918 / 0 | Inachevée | Inachevée | 2000 |
| 4918 / 1 | Défaite | Défaite | 479 |
| 4919 / 0 | Défaite | Victoire | 440 |
| 4919 / 1 | Inachevée | Inachevée | 2000 |
| 4920 / 0 | Défaite | Victoire | 515 |
| 4920 / 1 | Victoire | Victoire | 312 |
| 4921 / 0 | Victoire | Victoire | 463 |
| 4921 / 1 | Victoire | Victoire | 394 |

Les six victoires sont trois captures de drapeau et trois immobilisations.
Les dix replays sont validés par le moteur. Les deux blocages persistent : cette
version améliore les résultats mais ne résout pas toutes les stagnations.
Une longue pause de la machine a contaminé certaines durées murales ; celles-ci
ne servent ni au résultat ni à une comparaison de performance.

### Contrôle indépendant de la version livrée

| Placement / camp Improved | Résultat | Demi-coups | Fin |
|---|---|---:|---|
| 5917 / 0 | Défaite | 544 | Immobilisation |
| 5917 / 1 | Victoire | 928 | Drapeau |
| 5918 / 0 | Victoire | 531 | Immobilisation |
| 5918 / 1 | Défaite | 507 | Immobilisation |
| 5919 / 0 | Victoire | 572 | Immobilisation |
| 5919 / 1 | Défaite | 508 | Immobilisation |
| 5920 / 0 | Défaite | 702 | Drapeau |
| 5920 / 1 | Victoire | 338 | Immobilisation |
| 5921 / 0 | Victoire | 722 | Immobilisation |
| 5921 / 1 | Victoire | 678 | Immobilisation |

Les cinq placements sont distincts de tous les placements précédemment examinés.
Chaque paire utilise exactement la même disposition en échangeant les camps.
Le benchmark, la politique et les sources sont identiques entre développement
et contrôle. Les vingt replays de ces deux séries sont validés par le moteur.

Dans 5917/camp 1, le commandant gagne au demi-coup 928 alors que l'adversaire
conserve un général : atteindre le drapeau prime sur un duel perdu. Dans
5918/camp 0, l'échange des sergents au 531 élimine le dernier mobile adverse.
Dans 5921/camp 1, le capitaine capture le dernier démineur au 678 ; le colonel
et l'éclaireur alliés sont encore présents.

La limite la plus nette est 5920/camp 0 : le matériel mobile a culminé à +107
et reste à +56, mais un espion adverse capture le drapeau au 702. Cette victoire
matérielle théorique ne compense pas une mauvaise couverture du drapeau. Les
autres défaites terminent par immobilisation, après des avantages provisoires.
La poursuite, la couverture des voies d'accès et la conversion d'avantage restent
perfectibles. La mission d'interception peut encore suivre un démineur par
derrière au lieu de couper sa route, comme dans 4918/camp 1 en développement.

Les éclaireurs peuvent encore effectuer un long déplacement sans attaque si le
gain évalué le justifie : leur discrétion est un coût stratégique, pas une
interdiction de déplacement. Le test de déplacement discret enregistré reste
réussi. Les divulgations constatées sont listées dans les audits JSON.

## Enseignements des replays

L'itération 3 préserve les quatre victoires de la référence et transforme trois
défaites en victoires sur 2917–2921. Elle illustre notamment une attaque de
drapeau coordonnée dans 2919/camp 0 : échange des capitaines au demi-coup 395,
déminage au 397, capture du drapeau au 401. Dans 2919/camp 1, un démineur ouvre
la bombe au 376 et prend le drapeau au 378 avant l'arrivée du poursuivant.

Son contrôle révèle deux blocages différents : une finale presque sans pièces
mobiles (3920/camp 0), et une armée encore nombreuse qui reste jusqu'à 1 409
demi-coups sans combat (3920/camp 1). Une simple pénalité de répétition ne suffit
pas à organiser la reconnaissance. Dans la défaite 3917/camp 0, le maréchal n'a
qu'un coup légal au demi-coup 359 : il serait faux de lui reprocher de ne pas
avoir attendu sur place. La prévention de la percée doit intervenir plus tôt.

Avec l'itération 5, 3920/camp 0 se termine par une victoire au demi-coup 459 :
un démineur ouvre une bombe au 455 ; le maréchal adverse le capture au 458,
puis notre maréchal échange le dernier mobile adverse au 459. La partie
3918/camp 0, auparavant perdue, est gagnée au 1257 après les échanges des
maréchaux et généraux, avec deux colonels restant dominants. Cette victoire
reste lente et ne signifie pas que toutes les poursuites sont optimales.

## Vérification et livraison

L'itération 6 passe les mêmes 15 tests : trois tests ciblés dans
`iteration6/ctest_preflight.log`, puis les douze autres dans
`iteration6/ctest_remaining.log`. Son contrôle graphique réussit également.
Le diagnostic de 4917/camp 1 avant le demi-coup 724 montre le défaut corrigé :
l'ancien plan évaluait la pression sur le drapeau à zéro ; le capitaine déduit
porte cette pression à 67,5 et le retour du commandant reçoit un bonus
d'interception. Cela prouve la correction de l'estimation locale, pas que toute
la position était forcée gagnante. Le nouveau match sur ce placement est gagné
au demi-coup 398 ; la trajectoire de partie a changé avant cette position.

Empreinte SHA-256 du benchmark final :
`4d0d7547d21e9a5e7ca3163a0f9e6426108338c559d846040d5e4b33a58a27ff`.

Empreinte de `build/stratego.exe`, identique à `build-improved/stratego.exe` :
`c73f79804d4d26dd08c542ea65902bc46c945083c1728ac5fd5ea67fa9f1dff5`.

Le lancement graphique de l'exécutable installé a également réussi.
L'ancienne version est conservée dans
`reports/tournaments/strength_cycle_20260917/delivery/stratego_before.exe`.
Lancer `Jouer.cmd` et sélectionner **Expert+ Improved** pour utiliser la version
livrée. Expert+ Classique, les règles, les formations et les poids appris sont
inchangés ; les modifications de jeu portent sur `src/ai.c`, `src/ai_force.h`
et `src/ai_intercept.h`.

Les preuves sont dans `iteration6/`, `validation3/`, `board_verification.json`
et `delivery/verification.json` sous le dossier des expériences. Les commandes
de reproduction sont :

```powershell
cmake --build build-improved -j 3
ctest --test-dir build-improved --output-on-failure -j 2
python tools/tournament.py --binary build-improved/ai_benchmark.exe --output reports/tournaments/reproduction_strength --models 1 3 --pairs 5 --seed 5917 --seconds 0 --plies 2000 --jobs 2
```

Le dossier de sortie doit être nouveau. Le parallélisme change les durées,
pas les budgets de recherche ni les graines de chaque partie.

### Vérification historique de l'itération 5

L'itération 5 passe les 15 tests CTest, y compris les défaites humaines
enregistrées, la discrétion des éclaireurs, les officiers menacés, les attaques
escortées, les courses au drapeau et l'invariance aux identités cachées.
Le lancement graphique `stratego.exe --smoke --battle` réussit également.
Les nouveaux tests couvrent les valeurs relatives des grades, les échanges
selon les réserves et le dégagement d'un trajet de reconnaissance.

Empreinte historique du benchmark de l'itération 5 :
`e173cc3277ea543cd8fa6e573dc879386610a23b925d8741f4d0bfd042b37bba`.

Empreinte historique de l'exécutable graphique de l'itération 5 :
`0b9a9c5d410766d206a4c4e795f16f16211d8fdf20fbade42d38d0e3e2ede1af`.

Empreinte d'Expert+ Classique (`tests/ai_previous.c`), inchangée :
`3b37b5829615f8b47fc2b9e04da7f1418bcee763b87add95353d532179aec971`.
