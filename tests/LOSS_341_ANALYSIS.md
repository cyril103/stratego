# Corrections après la défaite en 341 demi-coups

Source conservée : `fixtures/human_win_341.jsonl`. Les changements ci-dessous concernent uniquement `src/ai.c`, donc **Expert+ Improved**. Le moteur Classique et le modèle appris restent inchangés.

## Général

Au demi-coup 44, le général révélé attaque C4→D4 et rencontre le maréchal encore inconnu. Une pénalité de perte probable seule ne suffisait pas à éviter ce pari. Désormais, un général révélé ne sonde pas une pièce inconnue tant qu'un maréchal reste parmi les rangs non identifiés, lorsqu'une autre action non exclue est disponible. Les attaques inconnues des officiers supérieurs reçoivent également une pénalité calculée à partir des rangs encore possibles. Aucun rang caché réel n'est consulté.

## Espion

L'espion I9 ne s'était pas déplacé de toute la partie. Le nouveau calcul cherche une position à deux cases d'un maréchal identifié, par un parcours évitant les attaques connues. Il distingue une bombe alliée infranchissable d'un allié mobile pouvant libérer le passage. Déplacer cet allié peut donc améliorer le plan de l'espion. Cette préparation n'est pas récompensée au détriment d'une urgence de préservation des officiers. Une attaque de l'espion sur un maréchal connu reçoit une priorité tactique supplémentaire.

Ce calcul est un objectif de positionnement réévalué à chaque coup, pas un piège garanti ni une stratégie complète de bluff. Les menaces inconnues restent possibles.

## Drapeau et escorte

Au demi-coup 328, l'IA préférait encore certaines captures lointaines pendant qu'un démineur humain approchait sous la protection du général. Un défenseur proche reçoit maintenant une priorité d'approche si une pièce mobile pouvant être un démineur est près du drapeau et accompagnée d'un haut gradé révélé. Les cases immédiatement perdantes contre un ennemi connu ne reçoivent pas cette prime. Le calcul d'interception existant demeure en place ; cette correction ajoute une priorité locale et ne prétend pas résoudre exhaustivement la course.

## Vérifications

Les positions précédant 44 et 328 sont ajoutées aux régressions avec huit graines chacune : éviter l'attaque aveugle du général et mobiliser la défense au lieu de poursuivre la sortie du maréchal. Des positions synthétiques vérifient le dégagement du couloir de l'espion, les cases dangereuses et l'attaque du maréchal identifié. Les anciennes défaites et la victoire par démineur restent dans la suite.

Aucun entraînement automatique n'est effectué et aucun gain d'Elo n'est déduit de ces seuls tests.
