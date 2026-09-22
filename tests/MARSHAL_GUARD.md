# Protection du maréchal et de l'espion

Régression : human_win_429_153557.jsonl (distincte de l'ancienne partie human_win_429.jsonl). Le maréchal révélé revient au contact d'un poursuivant inconnu au demi-coup 86, est pris au 87, puis l'IA manque au 88 la capture sûre de l'espion révélé par son lieutenant. Les deux espions disparaissent au 89 et le maréchal humain devient impossible à battre.

## Corrections

La mémoire des huit derniers tours est utilisée pour reconnaître un contact déjà évité par le maréchal. L'analyse remonte d'abord le coup adverse qui a suivi chaque mouvement allié ; elle distingue ainsi l'ancien poursuivant d'une autre pièce qui vient seulement d'approcher. Les identités cachées ne sont jamais consultées : un suspect reste une hypothèse.

Après les filtres de défaite immédiate et de risque terminal, l'IA évite de revenir au contact de ce suspect, ou d'y laisser son maréchal pendant un coup sans rapport, lorsqu'un déplacement calme sans perte connue est disponible. Cette alternative ne doit pas exposer le maréchal à un espion déjà révélé. Les tentatives de capture d'un drapeau caché restent possibles.

Un espion révélé qui peut prendre le maréchal ou échanger l'espion allié encore utile reçoit une priorité tactique, quelle que soit la taille de l'armée. La reprise doit être gagnante, ne pas abandonner d'autre pièce à une perte connue, ne pas aggraver le risque évalué du drapeau et ne permettre aucune contre-attaque gagnante ou égale parmi les grades publiquement possibles. La pièce la moins chère est préférée.

L'évaluation reconnaît deux pertes stratégiques auparavant sous-estimées : échanger son espion quand le maréchal adverse vit encore, et perdre son maréchal contre un espion même si un allié peut reprendre cet espion ensuite.

## Vérifications

Le test marshal_spy_protection couvre seize graines et des permutations d'identités cachées. Depuis la position 86, les coups humains enregistrés ne prennent plus le maréchal ni l'espion allié. Depuis la position 88 déjà compromise, le lieutenant 37 capture l'espion en 38 et préserve l'espion allié en 28. Les contrôles portent également sur la mémoire de poursuite après inversion des camps, l'absence de conclusion sans historique ou sans espion possible, les contre-attaques et le coût d'un échange maréchal/espion.

Ces reprises vérifient les scénarios enregistrés, pas une victoire contre toutes les réponses humaines possibles. Les tests généraux et la comparaison de parties doivent également passer avant installation.

Validation finale : 23 tests validés. La première exécution (reports/marshal-guard-tests.log) a validé 22 tests ; le test historique des officiers avait lu le nouveau journal à cause d'une collision de nom. L'ancien fichier a été restauré sans différence de contenu et le nouveau porte le suffixe _153557. Le test historique relancé sur son bon journal passe (reports/marshal-guard-tests-rerun.log). Le nouveau test à seize graines passe également après séparation des journaux.

La comparaison Expert+ Improved / Expert+ Classique, graine 204 dans les deux camps, conserve exactement les séquences de coups de la référence continuity-tournament-v3 : une victoire en 603 demi-coups, aucune défaite et une partie inachevée à 1200. Cela ne démontre pas un gain général de force, mais ne révèle pas de régression sur cette comparaison. Le test graphique rend 150 images et valide le pointage des 100 cases (reports/marshal-guard-smoke.log).

L'exécutable build/stratego.exe est installé et identique à build-napoleonic/stratego.exe : SHA256 E0B562B53B85D7527CA8160403FAE750AC11D049EB94C5D82A5A00BFECEE3DF5. Sauvegarde précédente : reports/stratego-before-marshal-guard.exe.
