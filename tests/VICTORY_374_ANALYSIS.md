# Enseignements de la victoire du 8 septembre 2026

La partie `fixtures/ai_win_374.jsonl` se termine au demi-coup 374 par la capture du drapeau humain H1. Expert+ Improved avait encore un démineur ; le général humain revenait vers sa base. Les deux bombes I2 et I1 ont été désamorcées aux demi-coups 368 et 370. Au demi-coup 372, l'IA a pourtant déplacé un lieutenant au lieu de sonder immédiatement H1.

Le problème est reproductible avec l'ancien moteur, graine 519 : il choisit également un déplacement sans attaque (sergent H7 vers G7). Le choix historique exact peut varier avec l'échantillonnage.

## Correction conservée

L'évaluation des attaques de démineurs immédiatement jouables accorde une valeur supplémentaire à la probabilité publique que la cible immobile et inconnue soit le drapeau. Cette valeur est renforcée lorsqu'un adversaire révélé capable de capturer le démineur peut le rejoindre en un ou deux déplacements, sans traverser un lac ni une pièce intermédiaire. Il s'agit de profiter d'une occasion avant l'interception, pas de certifier une victoire cachée. La prime ne s'applique pas aux autres grades : son extension aux officiers perturbait une manœuvre utile dans une ancienne partie.

Un démineur reçoit également un bonus mesuré pour ouvrir une bombe probable adjacente à un drapeau probable. Les protections contre les pertes connues et les calculs de défense restent appliqués. L'ancien Expert+ Classique n'est pas modifié.

Ces bonus restent heuristiques : 80 fois la probabilité de drapeau, 320 en cas d'urgence locale du démineur, et jusqu'à 12 pour l'ouverture d'une protection probable. Ils ne constituent pas un calcul exhaustif de course au drapeau. Aucun rang ennemi caché n'est utilisé et aucun apprentissage automatique n'a été lancé.

## Vérification

Le test `recorded_ai_victory` rejoue les positions précédant 368, 370 et 372 avec huit graines chacune. Il exige la poursuite du démineur I3→I2→I1, puis l'attaque immédiate I1→H1. À la dernière position, il échange aussi les rangs cachés de H1 et G1 : le choix et l'état aléatoire doivent rester identiques.

Ces contrôles vérifient le défaut observé et l'absence de fuite d'information. Ils ne mesurent pas un gain d'Elo et ne prouvent pas que toutes les finales sont résolues.
