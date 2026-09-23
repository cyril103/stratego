# Audit Expert+ Improved / Expert+ Classique — 23 septembre 2026

## Résultat apparié

| Version d'Improved | Victoires | Défaites | Nulles | Inachevées |
|---|---:|---:|---:|---:|
| Référence `41f8987` | 5 | 2 | 1 | 2 |
| Après correction | 8 | 2 | 0 | 0 |

Les cinq victoires initiales conservent exactement leurs séquences de coups.
La nulle et les deux inachevées deviennent des victoires. Aucune ancienne
victoire ne devient une défaite, mais les deux défaites restent des défaites.

| Graine / camp Improved | Avant | Après | Premier coup différent |
|---|---|---|---:|
| 9231 / 0 | Victoire 685 | Victoire 685 | Aucun |
| 9231 / 1 | Inachevée 2000 | Victoire 914 | 572 |
| 9232 / 0 | Victoire 309 | Victoire 309 | Aucun |
| 9232 / 1 | Nulle 458 | Victoire 480 | 420 |
| 9233 / 0 | Victoire 364 | Victoire 364 | Aucun |
| 9233 / 1 | Inachevée 2000 | Victoire 480 | 196 |
| 9234 / 0 | Victoire 411 | Victoire 411 | Aucun |
| 9234 / 1 | Défaite 505 | Défaite 507 | 328 |
| 9235 / 0 | Défaite 319 | Défaite 333 | 261 |
| 9235 / 1 | Victoire 454 | Victoire 454 | Aucun |

Les nombres sont des demi-coups. Résultats et replays finaux :
`reports/tournaments/audit_20260923_after/`. Comparaison des séquences :
`reports/audit923-comparison.json`. Les vingt journaux ont passé le contrôle
de cohérence des combats de `tools/audit_tournament.py`.

## Protocole et référence

Référence : commit `41f8987`. Cinq placements automatiques, graines 9231 à
9235, chacun joué dans les deux camps : dix parties. Budgets natifs des deux
moteurs, sans horloge, sans proposition de nulle. Plafond technique de 2 000
demi-coups, signalé comme partie inachevée et non comme nulle. Les placements
appariés sont vérifiés par le lanceur. Ce petit échantillon ne mesure pas un Elo.

Les dix parties de référence donnent à Improved **5 victoires, 2 défaites,
1 nulle et 2 inachevées**. La nulle est une double immobilisation après
l'échange des deux derniers maréchaux. Les inachevées atteignent le plafond
avec de longues répétitions de positions, après la perte des derniers démineurs.

Les journaux, binaires archivés, sources et empreintes sont conservés dans
`reports/tournaments/audit_20260923_before/`. Le lanceur utilise sa propre
copie de l'exécutable : compiler une correction ne change aucune partie de
référence en cours.

## Erreurs reproduites

- Graine 9231, camp 1, avant le demi-coup 612 : le dernier démineur D2 est
  menacé par le commandant connu D3. Trois replis immédiats sont légaux.
  Improved joue pourtant son général B3-C3 et perd le démineur au 613.
  Les six bombes adverses sont encore présentes. La partie termine inachevée
  après 1 211 demi-coups sans combat.
- Graine 9232, camp 1, avant le 420 : le dernier démineur H4 est attaquable
  par le lieutenant connu I4. Improved déplace son lieutenant G3-H3 pour
  préparer une reprise. Le démineur est perdu au 421 et les lieutenants
  s'échangent au 422. La reprise masque la perte de capacité dans le calcul
  de perte matérielle immédiate. La partie finit par double immobilisation.
- Graine 9233, camp 1, avant le 196 : trois replis sont possibles pour le
  dernier démineur I4 menacé par un commandant connu H4. Les quatre essais de
  diagnostic de l'ancienne version choisissent néanmoins G3-G4. La partie
  perd le démineur au 197 et atteint finalement le plafond technique.
- Graine 9234, camp 1, avant le 342 : Improved échange son maréchal B3-B4
  et laisse son seul capitaine face à une armée plus nombreuse. La protection
  précédente des derniers officiers ne s'appliquait pas : aucun ennemi restant
  après l'échange ne dépassait le capitaine en grade. Un seul défenseur ne
  couvre toutefois pas toutes les voies d'accès au drapeau. Celui-ci tombe
  finalement au 505.

Les autres pertes ne sont pas toutes des fautes démontrées. Par exemple,
l'attaque du colonel au 102 de 9231/camp 1 perd cet officier mais prend une
pièce et libère l'autre colonel, les deux étant coincés face au maréchal.
La perte du général au 92 de 9231/camp 0 reste un cas de poursuite par une
pièce cachée ; ce correctif ne lui attribue pas rétroactivement un grade connu.

## Corrections

Le dernier démineur menacé par un attaquant identifié est sauvé lorsqu'un
coup garantit sa survie au combat initial et évite une capture connue au tour
suivant, sans créer une nouvelle perte matérielle ailleurs. Une reprise de
l'attaquant ne remplace pas le démineur. La règle s'applique seulement si des
bombes adverses restent. Elle ne consulte pas les grades cachés. La protection
du drapeau et la prise immédiate d'un drapeau connu restent prioritaires.

L'évaluation des échanges du dernier officier fort tient également compte
du nombre de défenseurs : dans une armée déjà en difficulté, laisser un seul
mobile contre au moins trois ennemis après l'échange coûte une capacité de
défense, même si ce mobile les dépasse individuellement en grade. Lorsqu'une
retraite calme sans perte connue existe parmi les coups conservés par les
filtres de sécurité, les attaques qui engagent directement un tel échange
sont écartées. Déplacer un défenseur sur une case où il risque une reprise
de même grade reste évalué normalement : cette interception peut être
nécessaire à la survie du drapeau, comme le vérifie `strategic_defense_roles`.

Les tests rejouent 612, 420 et 342 sur huit graines, avec permutation des
grades adverses cachés et comparaison de l'état aléatoire final. Les réponses
obtenues sont D2-C2, H4-G4 et B3-A3. Les exceptions sans bombe adverse, avec
un autre démineur, sans menace identifiée et avec prise immédiate du drapeau
sont aussi contrôlées.

Sur 9232/camp 1, la comparaison complète confirme directement l'utilité du
sauvetage : la première différence est H4-G4 au demi-coup 420. Ce même dernier
démineur désamorce ensuite G10 au 478 puis capture le drapeau H10 au 480,
transformant la double immobilisation de référence en victoire.

Sur 9231/camp 1, la nouvelle trajectoire gagne par immobilisation au 914,
au lieu d'atteindre le plafond. Le premier écart visible est toutefois un
déplacement de sergent au 572, avant la position de régression 612. Le dernier
démineur reste vivant jusqu'au 757, puis meurt face à un capitaine encore
inconnu. La victoire complète ne prouve pas que le seul coup testé au 612
était une victoire forcée.

## Limites restantes

Sur 9234/camp 1, préserver le maréchal ne suffit pas à tenir toute la base.
Le capitaine est capturé au 469, et l'unique maréchal restant ne couvre pas
la dernière attaque du démineur adverse : le drapeau tombe au 507. La
conservation d'un officier n'est donc pas une preuve de défense gagnante.

Sur 9235/camp 0, le premier changement est le refus d'un échange de capitaines
au 261. La perte préalable de matériel et la disparition des capacités de
défense restent trop lourdes : défaite par immobilisation au 333. Ces deux
positions restent des pistes pour un prochain audit, sans prétendre qu'un
simple changement local garantit une victoire.

## Validation technique

Compilation Release complète sans avertissement. Les 32 tests CTest passent,
ainsi que les cinq tests Python du lanceur de tournoi. Les empreintes des
sources archivées confirment que seuls `src/ai.c`, `src/ai_endgamecare.h` et
`CMakeLists.txt` diffèrent entre les deux expériences. Expert+ Classique, les
formations, les règles et la politique apprise sont identiques.

SHA-256 du benchmark de référence :
`09159fef8d06424be2412823c094ff99b5ec10ed10a57de9aec628f446e0756b`.
SHA-256 du benchmark final :
`9432bf48cf510fd24c708bbd0e648f2973228a875a8eed2e31694e29e0e7359c`.
SHA-256 de l'exécutable de jeu compilé :
`516f18179700f6fb7960394b3bbb220e2246c9034919af5544fea8df9f27523c`.
La vérification croisée des vingt replays, des cinq placements distincts,
des sources et des binaires est conservée dans
`reports/tournaments/audit_20260923_after/verification.json`.

Les durées observées ne sont pas un comparatif de vitesse : certaines
exécutions ont connu de longues interruptions et des calculs concomitants.
Les budgets de recherche sont fixes et aucune partie n'utilise l'horloge.

## Reproduction

```powershell
cmake --build build-napoleonic -j 6
ctest --test-dir build-napoleonic --output-on-failure -j 4
python tests/test_tournament.py
python tools/tournament.py --binary build-napoleonic/ai_benchmark.exe --output reports/tournaments/reproduce_audit923 --models 1 3 --pairs 5 --seed 9231 --seconds 0 --plies 2000 --jobs 2
python tools/audit_tournament.py reports/tournaments/reproduce_audit923
```

Le dossier de sortie doit être nouveau. Les dix parties utilisées pour
développer ces corrections servent aussi à la comparaison appariée ; une
amélioration sur cette série n'établit pas à elle seule la généralisation.
