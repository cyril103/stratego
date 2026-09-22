# Relève du défenseur bloqué par une ligne d'éclaireur

Régression : `fixtures/human_win_577.jsonl`, partie humaine du 22 septembre
2026 à 12:52. Six pièces mobiles IA, dont le maréchal et le général, perdaient
contre deux éclaireurs et le dernier démineur humain.

Le sergent C9 couvrait le drapeau C10 contre l'éclaireur C7. Il ne pouvait
prendre le démineur en B9 sans ouvrir cette ligne. Le général poursuivait le
démineur par derrière, sans pouvoir le rattraper avant la prise du drapeau.

## Comportement ajouté

`ai_guardrelief.h` détecte un défenseur dont le départ permettrait à un
éclaireur identifié de prendre directement le drapeau. Si un autre attaquant
approche, il recherche un remplaçant et un trajet pour couvrir la ligne ou
prendre l'éclaireur. Il privilégie les trajets courts et les pièces moins
coûteuses, garde les obstacles alliés et évite les attaques connues sur les
cases du trajet. Le premier pas doit être légal et ne pas ouvrir une autre
prise immédiate du drapeau. Le maréchal évite aussi les contacts avec une
pièce inconnue pouvant être l'espion.

La mission est recalculée à chaque tour. Elle peut donc répartir les rôles
autrement si le sergent avance sur sa colonne pendant que le lieutenant
prend sa place. Une capture sûre de l'attaquant prend priorité sur la
préparation d'un nouveau remplacement.

La recherche tactique de perte du drapeau ne se désactive plus parce qu'il
reste beaucoup de défenseurs lorsqu'il ne reste que trois attaquants mobiles
adverses ou moins. Elle examine les approches à cinq cases dans ces nouveaux
cas de grande armée ; les petites finales conservent leur déclenchement
précédent à deux cases et leur ordre de recherche. Épuiser le budget reste
un résultat inconnu, jamais une preuve de
défaite. Cette extension seule ne suffisait pas à résoudre le cas enregistré :
la mission de remplacement est nécessaire pour préparer la défense en amont.

## Vérifications

Le test `pinned_guard_relief` rejoue l'attaque depuis le demi-coup 568 avec
les graines 1, 2, 3 et 519. Dans les quatre cas :

| Demi-coup IA | Coup choisi | Rôle |
|---|---|---|
| 568 | Lieutenant D10–D9 | Préparer le soutien |
| 570 | Sergent C9–C8 | Avancer en continuant à bloquer l'éclaireur |
| 572 | Lieutenant D9–C9 | Couvrir la colonne |
| 574 | Lieutenant C9–B9 | Prendre le dernier démineur |

Les coups humains intermédiaires sont ceux de la partie enregistrée. Tous
les coups sont vérifiés par le moteur de règles. Le drapeau survit et la
prise C7–C10 reste illégale, car le sergent couvre C8. Cela arrête cette attaque,
sans prétendre démontrer une victoire contre toutes les réponses possibles.

Le test vérifie également :

- l'identification du dernier démineur par les pertes publiques ;
- l'invariance du premier choix après permutation de grades ennemis cachés ;
- la même affectation après rotation du plateau et inversion des camps ;
- la libération du sergent quand un autre défenseur couvre déjà la ligne ;
- le maintien de la recherche locale avec quatorze défenseurs mobiles,
  en réintroduisant huit éclaireurs capturés dans des cases éloignées.
- l'absence de la nouvelle fourchette et du sacrifice perdant observés aux
  762 et 764 pendant le tournoi de développement.

Journaux de validation : `reports/defense577-*`. Le comparatif complet utilise
le placement 204 dans les deux camps contre Expert+ Classique, avec une limite
de 1 200 demi-coups. Une partie atteignant cette limite reste « inachevée ».

Le tournoi de développement avait révélé une régression : avec une fenêtre
élargie aussi dans les petites finales, la recherche épuisait son budget sur
une ligne de sacrifice et la préférait aux retraites dont elle voyait la perte
du drapeau. Le fixture `guard_search_loss_767.jsonl` conserve cette régression.
L'extension est désormais limitée aux grandes armées auparavant exclues ;
la mission de relève reste disponible quel que soit le nombre de défenseurs.

## Validation finale

Les 20 tests passent après cette correction supplémentaire
(`reports/defense577-tests-final.log`, 320,75 secondes avec le tournoi en
parallèle). Le test graphique termine 150 images, deux coups et le contrôle
de pointage des 100 cases, sans erreur (`reports/defense577-smoke-final.log`).

SHA256 identique du binaire final compilé et installé dans `build/stratego.exe` :
`A2EF4EB3E0BFBC86708E96549F1BD6AB3B4959FBC837B3BCC5E0E411EEA1C89D`.
La sauvegarde précédente est `reports/stratego-before-defense577.exe`.
Les résultats du comparatif final sont conservés dans
`reports/defense577-final-tournament/results.log`.

Le contrôle final donne une victoire par immobilisation au 603, zéro défaite
et une partie inachevée au 1 200 : le même résultat que la référence précédente.
Les deux suites de coups sont identiques à celles de cette référence. La
régression de développement est donc absente de ce comparatif. Les temps
moyens sont de 378,52 ms pour l'IA modifiée et 234,28 ms pour Classique, avec
un maximum de 3 201,94 ms ; compilation et tests ont partagé le processeur
pendant une partie du tournoi, donc ce n'est pas une mesure isolée.

Ce comparatif vérifie le maintien du comportement sur un placement joué
dans les deux camps. Il ne démontre pas une hausse générale du niveau.
L'amélioration démontrée ici est l'interception du démineur dans la dernière
partie humaine, avec le drapeau protégé malgré la ligne de l'éclaireur.
