# Fins de partie : référence ISF

Source consultée le 8 septembre 2026 : [ISF Game Rules, article 12](https://isfstratego.kleier.net/docs/rulreg/isfgamerules.pdf), fédération internationale de Stratego.

L'article 12 distingue la capture du drapeau, l'immobilisation d'un seul camp, l'abandon, et les nulles : immobilisation des deux camps, accord mutuel, fin de la période de jeu et certaines décisions d'arbitrage ou situations de pendules. Les restrictions de répétition des articles 10 et 11 ne sont pas une règle de nulle automatique.

## Application dans ce projet

- Le moteur vérifie les déplacements légaux des **deux** armées après un coup. Si aucune ne peut jouer, le résultat est `GAME_DRAW`. Si une seule le peut, elle gagne immédiatement, quel que soit le camp dont le tour arrive.
- Une prise de drapeau reste prioritaire. Un accord ou une expiration de période ne remplace jamais une issue déjà acquise.
- Le bouton **Proposer nulle** est disponible à votre tour, hors animation. La partie ne s'arrête que si l'IA accepte. Une offre refusée reste sans effet sur le résultat ; une nouvelle offre est possible vingt demi-coups plus tard.
- La politique d'acceptation de l'IA est propre au logiciel : à partir du demi-coup 120, matériel public approximativement équilibré, et aucune capture immédiatement disponible d'un drapeau révélé ou d'un officier supérieur révélé. Ce ne sont **pas** des conditions réglementaires de nulle. La règle reste l'accord des deux joueurs.
- Le jeu interactif reste sans pendules et sans durée maximale. Aucune nulle pour absence de capture ou nombre de coups n'a été ajoutée.
- L'IA peut aussi proposer une nulle à son tour : au moins 200 demi-coups joués, 80 demi-coups sans combat, matériel public équilibré selon sa politique d'acceptation, aucune attaque disponible, et 100 demi-coups depuis la dernière offre de l'un des joueurs. Ce sont des critères heuristiques, pas une preuve de blocage ni une règle de nulle automatique. Une boîte de dialogue permet d'accepter ou refuser (Échap refuse également) ; le jeu attend la réponse, sans expiration de l'offre.
- Les journaux nouveaux portent `rules: ISF-endings-v1` et une raison de fin. Les anciens journaux sont rejoués avec leur ancien traitement de l'immobilisation pour préserver leurs résultats historiques.

## Tournoi automatique

Le dernier argument facultatif, après le dossier de sortie, fixe une période commune de jeu **en secondes**, annoncée avant le départ. Exemple pour deux manches avec une période de dix minutes chacune :

```powershell
New-Item -ItemType Directory -Path build/timed_matches -Force
./build/ai_benchmark.exe 1 100000 601 -1 1 build/timed_matches 600
```

Une expiration produit `DRAW` avec `END_PLAYING_PERIOD`. Sans cet argument, aucune période ne s'applique. La limite technique de demi-coups produit toujours `unfinished` et reste distincte. Les résultats du pilote de dix parties ne sont pas requalifiés.

Cette mise à jour concerne les fins de partie : elle n'implémente pas les pendules personnelles avec réclamations, l'arbitrage humain ni l'intégralité des règles ISF de poursuite.

## Vérification

`official_endings` couvre la double immobilisation après égalité, les pièces bloquées, l'immobilisation du camp venant de jouer, l'accord refusé/accepté, la période de jeu, l'abandon, la priorité d'une issue acquise et la relecture des raisons de fin. `--draw-demo` vérifie et affiche l'écran de nulle.
