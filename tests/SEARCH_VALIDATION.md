# Recherche progressive — 8 septembre 2026

## Modification du moteur

- Cache privé de 4 096 entrées pour chaque variante et chaque monde hypothétique. Les clés comprennent les grades simulés, les identités, les révélations, les captures et l'historique des répétitions. Aucun partage entre mondes cachés ou décisions concurrentes.
- Stockage séparé des valeurs exactes et des bornes alpha/bêta. Une recherche interrompue ne publie pas de valeur exacte.
- Approfondissement 1, 2, 4, 6, puis 8 demi-coups après le coup candidat lorsque la position justifie un budget supérieur. La profondeur 1 sert de secours.
- Pour chaque monde hypothétique, tous les candidats sont comparés à la profondeur commune effectivement terminée. Les maxima configurés ne sont pas des profondeurs garanties.
- Budget de présélection : 600 nœuds par candidat et monde ; finalistes : 2 400 normalement, 6 400 en finale ou en crise autour du drapeau. La prolongation tactique consomme le même budget.
- Les replis des hauts grades face à un supérieur révélé sont mieux classés dans les réponses simulées, afin de ne pas être éliminés trop tôt par la sélection des branches.

## Vérifications

`completed_search_and_cache` compare une recherche avec et sans cache, vérifie les bornes après une coupure, la distinction des historiques et des révélations, la conservation du dernier résultat terminé et la comparaison à horizon commun.

Les tests des neuf journaux humains conservent notamment la défense du drapeau, les contre-attaques et les huit essais de retraite du général de la partie en 227 demi-coups. Sur la position 120 de la partie en 175 demi-coups, le test accepte également le sauvetage du démineur attaqué par le colonel révélé ; la chaîne de dégagement du maréchal est vérifiée séparément.

## Comparaison

La référence `tests/ai_previous.c` est désormais l'Expert+ livré immédiatement avant cette évolution. L'ancienne référence est conservée dans `tests/ai_legacy_early.c`. Aucun entraînement n'est lancé.

Le comparateur utilise le même placement initial dans les deux manches et inverse le camp de la nouvelle IA. Avec les formations activées, les deux armées sont préparées avant cette inversion. Il affiche aussi le temps moyen de chaque moteur. Les parties limitées en nombre de demi-coups sont signalées comme inachevées, pas comme des nulles.

Une première version sans comparaison à horizon commun a obtenu 0 victoire, 1 défaite et 1 partie inachevée sur la graine 41 (limite 600). Temps moyens : 313 ms pour cette version, 175 ms pour la référence. Cette expérience ne constitue pas une validation de la version finale ; elle a motivé la correction des horizons.

Version finale, `ai_benchmark 1 700 42 -1 1` : **1 victoire, 1 défaite, 0 partie inachevée**. Défaite avec HUMAN au demi-coup 464, victoire avec COMPUTER au demi-coup 510. Moyenne nouvelle IA : **528,33 ms**, maximum **1 860,85 ms**, sur 487 décisions ; référence : **238,04 ms** sur 487 décisions. Le score ne démontre pas de gain Elo et le coût de calcul a augmenté. Il faut davantage de placements indépendants pour établir un changement de force global.

Les **9 suites CTest passent** (182,34 secondes avec la confrontation en parallèle). Le test graphique `stratego --smoke --battle` termine 150 images et 4 mouvements, puis ferme proprement le jeu. Sur la position 60 de la partie en 227 demi-coups, le diagnostic de la graine 519 rapporte une profondeur commune de 2 à 4 demi-coups après le candidat, moyenne 2,75 ; cela illustre la différence entre profondeur configurée et atteinte.

Les résultats d'un petit nombre de parties ne permettent pas d'attribuer un gain Elo fiable. Les temps mesurés pendant d'autres tests simultanés dépendent aussi de la charge du PC.

## Références

- A. F. C. Arts, [Competitive Play in Stratego, 2010](https://project.dke.maastrichtuniversity.nl/games/files/msc/Arts_thesis.pdf) : approfondissement, ordre des coups, tables de transposition.
- Schadd et Winands, [Quiescence Search for Stratego, 2009](https://dke.maastrichtuniversity.nl/m.winands/documents/bnaic2009Schadd.pdf) : coût et limites de la prolongation tactique sous information cachée.
- Cowling, Powley et Whitehouse, [Information Set Monte Carlo Tree Search, 2012](https://eprints.whiterose.ac.uk/id/eprint/75048/1/CowlingPowleyWhitehouse2012.pdf) : limites de la déterminisation. Le moteur actuel reste une recherche sur des mondes échantillonnés ; il n'implémente pas ISMCTS.

Le cache et l'approfondissement ne suppriment ni les erreurs de croyance sur les pièces cachées, ni les limites de la sélection des branches.
