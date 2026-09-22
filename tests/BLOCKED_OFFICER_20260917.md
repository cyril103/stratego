# Officier bloqué et dernier défenseur

Source : `fixtures/human_win_429.jsonl`, partie du 17 septembre à 11:29.

## Changements

- Une capture par un colonel, général ou maréchal reçoit un coût distinct si
  une pièce inconnue voisine peut le reprendre. Ce coût ne s'annule pas sous
  prétexte que la case de départ était déjà menacée. Les réponses connues
  restent évaluées par les protections tactiques existantes.
- Un coéquipier peut dégager la première sortie utilisable d'un officier
  révélé menacé. Une sortie doit être libre, légale et raisonnablement sûre
  selon les informations publiques. Une seconde sortie ne donne pas ce bonus.
- Une attaque qui risque d'éliminer le dernier défenseur proche du drapeau
  reçoit une pénalité proportionnelle au risque de perte et à la possibilité
  d'un démineur proche. La pénalité exclut la capture du démineur lui-même.
  Les distances locales sont estimées sur le terrain, pas par une preuve
  exhaustive de course au drapeau.

## Vérification ciblée

Le rejeu entier est validé par le moteur. Sur les graines 1 à 8 :

- Avant le 322, l'IA dégage J5 ou I6 pour ouvrir une sortie au maréchal I5,
  au lieu de prendre le capitaine I4 protégé par l'espion H4.
- Avant le 420, elle conserve son lieutenant H10 plutôt que de l'échanger
  contre le lieutenant G10 tandis que le démineur E10 approche.
- Une permutation de grades cachés préserve les décisions et l'état aléatoire.

Les anciennes positions de protection du général et du maréchal restent dans
le même test, ainsi que la discrétion de l'éclaireur. Ces assertions portent
sur des décisions ciblées ; elles ne prouvent pas la victoire contre un joueur
qui adapte ses réponses et ne mesurent pas un taux de victoire global.
Validation finale : les 13 tests passent. Les 12 autres tests ont passe dans la suite complete ; le test de protection a ete relance avec succes apres correction de sa selection de pieces a permuter (33,56 s). Le code du moteur est identique entre ces executions. Le controle --smoke --battle termine avec le code 0. Binaire installe dans build/stratego.exe avec empreinte identique au binaire teste ; sauvegarde dans reports/ai_safety_20260917/stratego_before_clearance.exe.
