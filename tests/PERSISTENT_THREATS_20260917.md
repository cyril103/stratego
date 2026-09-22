# Protection persistante des hauts grades

Source : `fixtures/human_win_369.jsonl`, partie du 17 septembre à 10:59.

## Problème

L'évaluation supplémentaire des menaces inconnues ne regardait que la dernière
pièce adverse déplacée. Jouer une autre pièce suffisait à supprimer cette
prudence alors que l'assaillant ou le soutien caché restait au contact.

## Changement

L'évaluation parcourt les voisins de chaque officier révélé, du colonel au
maréchal. Elle ne retient que les grades attaquants encore possibles selon
l'information publique. Le risque de base demeure tant que le contact et
l'incertitude existent. Une approche présente dans les huit derniers coups
adverses renforce ce risque, sans être nécessaire à son existence. Le niveau
de base est plus faible pour une pièce qui n'a jamais bougé.

La menace maximale est retenue par officier, pour ne pas compter sa perte
plusieurs fois. L'effet est évalué sur toute l'armée avant et après le coup,
y compris pour une capture qui révèle notre officier. Les attaques incertaines
ne reçoivent toujours pas un bénéfice de sauvetage garanti.

## Vérifications ciblées

- Demi-coup 48, huit graines : pas de retour du général A5-B5.
- Demi-coup 176, huit graines : pas de capture piégée du maréchal D7-C7.
- Pour ces positions, permuter les identités cachées conserve le coup et
  l'état du générateur aléatoire.
- Le changement du dernier coup et l'écoulement de vingt demi-coups ne font
  pas disparaître le risque d'un contact inconnu inchangé.
- Les anciens tests de la partie en 293 demi-coups passent : piège évité,
  général replié et éclaireur discret. Les cas d'espion éliminé et d'éclaireur
  identifié ne déclenchent pas de faux risque d'espion.

Le rejeu complet est validé par le moteur de règles. Ces tests démontrent des
décisions et invariants précis, pas une victoire forcée ou un taux de victoire.
L'évaluation reste heuristique et sensible aux bluffs ; elle n'accède pas aux
identités adverses cachées. Aucun résultat d'invincibilité n'est revendiqué.
`13/13` groupes de tests reussis sur le code final : officer_safety_tests execute directement, puis les douze autres tests CTest (108,22 s). Le controle --smoke --battle termine avec le code 0. Le binaire teste est installe dans build/stratego.exe avec verification SHA-256 ; version precedente conservee dans reports/ai_safety_20260917/stratego_before_persistent.exe.
