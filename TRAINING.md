# Modèle appris par autojeu

Le jeu propose **IA entraînée / Expérimental** dans le choix de l'adversaire. Le mode Expert reste indépendant. Le fichier `assets/models/selfplay.policy` est chargé au démarrage ; sans modèle valide, le mode entraîné n'est pas proposé. `build/stratego.exe --ml` sélectionne ce mode au lancement.

## Ce qui apprend

Il s'agit d'une politique linéaire de 24 paramètres, entraînée par REINFORCE en C. Ce n'est ni un réseau neuronal profond, ni un entraînement du modèle ChatGPT. La politique ajoute un score appris aux évaluations tactiques de l'IA rapide. Les caractéristiques décrivent le gain attendu d'une attaque, les menaces, la reconnaissance, l'avancement, la défense et le rôle de la pièce alliée.

Chaque partie oppose deux copies du modèle courant avec exploration. Le gagnant reçoit +1, le perdant −1 ; les gradients des décisions sont pondérés selon leur éloignement de la fin. Les poids sont modifiés après la partie. Les parties interrompues à 800 demi-coups n'apportent pas de récompense et ne sont pas comptées comme des matchs nuls établis par les règles. Aucun coup de l'Expert ni aucune valeur de son évaluation ne sert d'étiquette d'entraînement.

Les rangs adverses non révélés ne sont jamais des caractéristiques : les probabilités utilisent les pertes publiques, les rangs révélés et le fait qu'une pièce ait bougé. L'arbitre connaît nécessairement les vrais rangs pour résoudre les combats, mais ceux-ci ne sont pas transmis à la politique. Un test échange des rangs cachés et vérifie l'identité des caractéristiques, des scores et des décisions.

## Premier entraînement livré

- 4 096 parties d'autojeu, graines 1 à 4 096 : 1 548 terminées et 2 548 interrompues à la limite.
- Huit checkpoints, sélection sur 32 parties de validation par checkpoint, graines 100 000 à 100 015, camps alternés.
- Le checkpoint retenu est celui obtenu après 512 parties : continuer l'entraînement n'a pas amélioré le score de sélection.
- Test final séparé : 256 parties, graines 300 000 à 300 127, camps alternés ; **53 victoires, 71 défaites, 132 parties interrompues**.
- L'adversaire de comparaison utilise les mêmes évaluations tactiques mais des poids appris nuls. Ce n'est pas le moteur Expert +. Les deux politiques utilisent la même température d'exploration de 0,35 en évaluation et le modèle chargé utilise également cette température dans le jeu.

Ce résultat ne démontre pas de progrès. Le nombre élevé de parties interrompues limite le signal d'apprentissage et la petite validation rend la sélection fragile. Ce premier modèle constitue une chaîne d'entraînement fonctionnelle, pas une IA imbattable. Le mode est explicitement expérimental.

Le bilan lisible par machine accompagne les poids dans `assets/models/selfplay.policy.json`. Le journal de cette exécution est conservé dans `training/selfplay_run_001.log`. Un essai préparatoire de 256 parties avait montré le blocage de politiques entièrement déterministes ; ses poids ont été remplacés par ceux de l'exécution décrite ici. Les graines du test final sont distinctes de cet essai préparatoire.

## Reproduire ou entraîner une nouvelle version

Après compilation :

```powershell
.\build\train_selfplay.exe assets/models/selfplay_next.policy 8 512
```

Arguments : fichier de sortie, nombre de rounds, parties par round. Chaque exécution repart de zéro avec des graines déterministes. Utiliser un nouveau nom préserve le modèle livré. Ne pas sélectionner des réglages sur le test final puis présenter ce même test comme indépendant : réserver de nouvelles graines lors d'une nouvelle étude.

Pour utiliser un autre modèle, copier ses poids sous `assets/models/selfplay.policy` et son rapport associé, puis recopier les assets dans `build/assets` ou reconstruire le jeu et le relancer. Les poids d'un jeu déjà ouvert ne changent pas pendant une partie.

## Vérifications

`ctest --test-dir build --output-on-failure` couvre les règles, les positions des parties humaines enregistrées pour le mode Expert, le calcul en arrière-plan, l'invariance aux rangs cachés, les gradients de politique comparés à des différences finies, la sauvegarde/relecture des poids et le rejet d'un modèle invalide. Les succès tactiques de l'Expert ne sont pas attribués au modèle appris.
