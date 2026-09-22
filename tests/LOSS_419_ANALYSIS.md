# Défaite : général abandonné et espion inactif

Source : `fixtures/human_win_419.jsonl`, ExpertPlusImproved, victoire humaine par capture du drapeau au demi-coup 419.

Le général IA arrive en F6 au demi-coup 200. Le maréchal humain, connu depuis le combat 17, revient par G8, G7 puis F7. Au demi-coup 206, l'IA déplace son commandant E7 vers D7 ; le général est capturé en F6 au demi-coup 207. Une retraite en E6 était possible. L'espion IA reste en A7 entre les demi-coups 100 et 222 : il ne vient pas soutenir cette phase de défense. Cela ne prouve pas qu'il aurait pu capturer le maréchal, qui conserve l'initiative s'il attaque l'espion.

Corrections dans Expert+ Improved :

- Le calcul de perte certaine vérifie aussi les officiers supérieurs laissés ailleurs sur le plateau, même si le coup choisi est une attaque. Une capture inconnue n'est pas supposée victorieuse pour justifier leur abandon.
- Une reprise légale par un défenseur, notamment l'espion contre le maréchal, peut justifier de conserver la position plutôt que de fuir automatiquement.
- L'espion bénéficie d'un objectif de rapprochement vers les maréchaux révélés, par un chemin ouvert. Il se place à au moins deux cases de distance et évite les cases immédiatement attaquables par un ennemi révélé. Les ennemis cachés restent une incertitude ; cette heuristique ne garantit pas une approche sûre contre eux.

La position avant 206 est rejouée avec huit graines pour vérifier la retraite du général hors de portée immédiate du maréchal. Des positions synthétiques couvrent l'approche de l'espion, l'absence de cible connue, une case dangereuse et la possibilité de reprendre le maréchal. La partie complète est conservée pour les régressions futures. Aucun entraînement ni gain d'Elo mesuré n'est revendiqué.
