# Défense et raids après la partie de 647 demi-coups

Replay : `fixtures/human_win_647_20260923.jsonl`. Victoire humaine par prise du
drapeau en B1 au demi-coup 647. Les positions de test sont chargées avant le
demi-coup indiqué.

## Corrections défensives

- **98 et 102 : conserver l'écran de l'espion.** La menace d'éclaireur possible
  est évaluée après chaque déplacement, même lorsque l'espion est actuellement
  protégé. Un déplacement ne doit plus rouvrir une ligne que l'IA vient de
  fermer. Les captures connues de maréchal mettent à jour l'inventaire utilisé
  par cette protection ; les filtres de survie du drapeau restent prioritaires.
- **468 : préserver l'avant-dernier défenseur.** Le capitaine ne sonde plus C10,
  dont les probabilités publiques sont d'environ 46 % de bombe et 39 % de
  drapeau, lorsqu'une alternative sûre existe. Le seuil de risque de bombe est
  de 25 % pour une armée réduite à deux unités mobiles. Les démineurs, le
  dernier mobile et les tentatives de drapeau majoritaires gardent leurs
  exceptions.
- **634 : intercepter avant l'identification.** Le dernier défenseur dominant
  peut poursuivre une pièce mobile inconnue qui pourrait être un démineur
  proche du drapeau. Cette extension exige des démineurs encore cachés et la
  supériorité du défenseur sur tous les rangs mobiles adverses restants. Elle
  ne transforme pas une identité possible en information certaine.
- **164 : rappeler un officier capable de contrer le général.** Une invasion
  connue de général ou de maréchal dans les lignes alliées peut recevoir une
  réponse à plus longue distance. La pression augmente avec le nombre de
  pièces faibles exposées. Les menaces immédiates et la défense du drapeau
  restent prioritaires ; le bonus de rappel renforcé est suspendu s'il laisse
  un haut officier allié directement prenable.

## Raids offensifs

Le bonus de raid concerne les commandants, colonels et les hauts officiers
déjà révélés. Il récompense une capture à bonnes probabilités, ou une approche
courte vers une pièce mobile vulnérable. Les pièces mobiles ne peuvent être
des bombes. Pour une cible immobile, seules les deux premières rangées du
camp adverse sont admissibles et ses probabilités publiques doivent satisfaire
les mêmes limites : au plus 8 % de bombe, au plus 12 % de combat non gagnant,
et au moins 85 % de capture gagnante.

La profondeur seule ne rend donc jamais une cible sûre. Le bonus reste borné,
conserve le coût de divulgation des hauts officiers, tient compte des reprises
adverses et n'encourage pas une exposition supplémentaire à un espion. Il est
désactivé pour l'officier assigné à une contre-invasion et pour un mouvement
augmentant sensiblement le risque du drapeau. Une approche ne reçoit pas ce
bonus lorsqu'un haut officier est déjà au contact d'une menace incertaine.

## Tests

Validation finale : compilation sans avertissement du compilateur et
**34 tests CTest réussis sur 34**, en 307,51 secondes.

`raid_and_hidden_invasion` vérifie les positions 98, 102, 468 et 634 sur huit
graines, avec permutation de rangs ennemis cachés. À partir de 634, il rejoue
la percée humaine avec les nouvelles réponses : le démineur doit être capturé
et le drapeau conservé.

La suite de l'invasion du général est également rejouée depuis 164 sur quatre
graines : le général humain est neutralisé au demi-coup 190 dans les quatre
reprises. Un scénario offensif vérifie l'enchaînement de deux captures d'un
colonel contre des adversaires mobiles encore inconnus. Des cas négatifs
couvrent le risque de bombe, un officier supérieur possible et une cible
immobile dans les rangées profondes ; une cible de première ligne à faible
risque reste admissible.

Ces continuations suivent les coups humains enregistrés tant qu'ils restent
légaux. Elles démontrent l'arrêt de ces séquences, pas une victoire forcée
contre toute nouvelle réponse humaine ni une amélioration chiffrée en tournoi.
