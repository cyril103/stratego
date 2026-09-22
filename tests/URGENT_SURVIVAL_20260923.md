# Survie des pièces clés — défaite du 23 septembre (463 demi-coups)

Fixture : `fixtures/human_win_463_20260923.jsonl`.

Le lieutenant perdu au coup 296 attaquait J8 depuis I8 ; le sergent perdu au
coup 438 attaquait I9 depuis I8. Les bombes étaient différentes et inconnues
avant leur premier combat. Le problème n'était pas un oubli de la bombe J8.

Corrections :

- Au coup 412, une retraite sûre de l'espion doit primer sur les estimations
  lointaines de recherche tant que le maréchal adverse vit. Les captures
  certaines de l'agresseur et l'élimination du maréchal restent possibles.
  Les filtres de défaite immédiate et de protection du drapeau passent avant.
  Le pari anti-maréchal déjà prévu en cas de déficit extrême reste possible
  contre un mobile inconnu qui pourrait réellement être le maréchal.
- Avec exactement deux mobiles, une attaque non démineuse sur une cible
  majoritairement bombe et minoritairement drapeau est écartée lorsqu'un coup
  calme sans perte matérielle identifiée existe parmi les options retenues.
  Le dernier mobile conserve ses paris de dernier recours. Cela ne garantit
  pas que la position restante soit gagnante.

`urgent_army_survival` rejoue les positions 412 et 438 sur huit graines,
vérifie les réponses légales, la disparition de la capture immédiate enregistrée,
la conservation de l'espion, l'absence d'attaque sur la bombe connue J8 et
l'abstention du pari I9. Une permutation des grades ennemis cachés doit produire
les mêmes coups et le même état aléatoire. Les exceptions démineur, dernier
mobile, maréchal déjà capturé et prise immédiate du drapeau sont aussi vérifiées.

Sur les huit graines : J3-I3 au coup 412 et I8-H8 au coup 438. Le problème
distinct du maréchal au coup 142 n'est pas modifié par cette correction.

Validation : compilation Release complète sans avertissement, puis 31 tests
CTest réussis (271,90 s), dont les protections historiques du maréchal, la
discrétion des officiers, la défense contre l'éclaireur et le pari de l'espion
en déficit extrême.
