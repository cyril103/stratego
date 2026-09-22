# Protection des officiers — 17 septembre 2026

La partie human_win_293.jsonl est relue integralement par le moteur (victoire humaine par drapeau, 293 demi-coups).

Avant correction, replay_analyze avec la graine 519 reproduit les deux erreurs : au demi-coup 38, marechal F6-F5 dans le piege de l'espion ; au 44, espion E9-E8 au lieu de sauver le general en A4.

Apres correction, sur les graines 1 a 8 :
- au 38, capitaine D8-E8, sans offrir le marechal ;
- au 44, general A4-A5 ;
- les memes coups et le meme etat aleatoire apres permutation de deux grades ennemis caches.

La nouvelle penalite porte sur l'ensemble des officiers reveles (colonel, general, marechal). Elle exploite l'approche au contact de la derniere piece adverse deplacee, si un grade capable de battre l'officier reste possible. Les captures qui revelent l'officier sont incluses. Une attaque incertaine ne recoit pas le benefice d'un sauvetage garanti.

Validation : les 12 tests CTest passent, y compris les anciennes defaites, les victoires enregistrees, la recherche, les simulations de regles et le nouveau test approaching_hidden_officers. Le nouveau test inclut une symetrie du plateau, l'absence de risque d'espion lorsque celui-ci est elimine, et un eclaireur connu.

Limites : coefficient heuristique, sensibilite au bluff, pas de preuve de victoire sur le reste de la partie ni de mesure d'un gain de taux de victoire. La correction cible les erreurs initiales ; elle ne rejoue pas les coups humains apres une divergence comme s'ils restaient optimaux.

Construction : cmake -S . -B build-improved -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release puis cmake --build build-improved -j 6. Le cache historique build reference un ancien chemin de travail. L'ancien executable est sauvegarde dans reports/ai_safety_20260917/stratego_before.exe.
`build-improved/stratego.exe --smoke --battle` termine avec le code 0. Le nouvel executable a ete copie dans `build/stratego.exe` ; empreintes SHA-256 identiques apres copie. Le lancement habituel par `Jouer.cmd` utilise donc la correction.
