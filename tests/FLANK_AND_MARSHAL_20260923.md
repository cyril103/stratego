# Maréchal exposé et percée couverte — partie de 13 h 03

Source : `fixtures/human_win_213_20260923.jsonl`, ExpertPlusImproved,
victoire humaine par capture du drapeau au coup 213.

## Diagnostic

Le maréchal de l'IA, révélé au coup 115, reste en B6. Au coup 162,
le capitaine A5–B5 lui ferme sa retraite vers B5. L'inconnu C7–B7 au
coup 163 est en réalité l'espion humain. Au coup 164, l'IA déplace son
démineur A6–A5 ; l'espion prend le maréchal au coup 165.

L'identité de l'espion n'était pas accessible à l'IA. En revanche, son
déplacement, son contact avec le maréchal révélé et la survie de l'espion
adverse étaient publics. Le maréchal pouvait gagner le combat en B7
contre toutes les identités possibles de cette pièce mobile : le maréchal
humain était déjà capturé. Il restait toutefois un risque de contre-attaque
par un autre espion possible en B8, dont le rang réel était général.
Il faut distinguer combat certainement gagné et sécurité après capture.

La perte du maréchal permet la combinaison suivante : le général humain
contrôle l'aile gauche, tandis que le démineur rejoint A2, ouvre la bombe
B2 au coup 211 et prend le drapeau C2 au coup 213. L'espion de l'IA en C4
encombre le passage de son propre général D4. Une protection du dernier
défenseur ne couvre pas cette situation, où plusieurs officiers restent.

## Changements

- Réévaluer le contact avec un espion possible pour tous les coups racine,
  y compris les déplacements d'une autre pièce. Retenir les retraites et les
  captures très probablement gagnantes qui suppriment le contact, sans
  exposer le maréchal à un autre espion possible ni forcer un échange coûteux.
  Si aucune réponse de ce type n'existe, conserver l'évaluation des risques.
- Préserver une retraite libre du maréchal exposé lorsqu'un espion possible
  est à deux cases. Les urgences terminales restent prioritaires.
- Face à un officier identifié couvrant un démineur possible près du drapeau,
  réévaluer l'accès de notre officier après chaque déplacement. Cela récompense
  aussi le dégagement d'une pièce alliée qui bloque le passage.
- Autoriser le sacrifice précis d'un espion devenu inactif pour dégager ce
  passage. L'exception ne s'applique pas si le maréchal adverse vit encore,
  ni à un sacrifice d'officier, ni sans menace de démineur possible.

## Vérification ciblée

- Huit reprises au coup 162 : une issue reste disponible, ou le maréchal
  s'éloigne de la menace. La poursuite humaine C7–B7 puis B7–B6, lorsqu'elle
  reste légale, ne capture plus le maréchal.
- Variante du coup 164 avec le général B8 identifié : le maréchal B6 prend
  l'inconnu B7 sur huit graines. Échanger les rangs cachés de B7 et E5
  conserve la décision et la consommation aléatoire.
- Variante avec B5 libre : la retraite B6–B5 est conservée comme réponse
  sûre. Dans la position historique déjà enfermée au coup 164, le soutien
  inconnu B8 empêche de certifier la sécurité de la capture ; la correction
  préventive au coup 162 évite de créer ce dilemme.
- Quatre reprises depuis le coup 198, avec les réponses humaines enregistrées :
  espion C4–B4 au coup 198, général D4–C4 puis échange C4–B4 au coup 202,
  colonel E4–D4–C4–B4–B3, puis capture du démineur en B2 au coup 212.
  Le drapeau reste intact.
- Cas négatifs : pas de dégagement sacrificiel avec un espion encore actif,
  pas de plan sans démineur possible ou contre un officier non identifié.

Ces continuations vérifient la réponse au plan enregistré. Elles ne prouvent
pas une victoire contre toute adaptation humaine et ne constituent pas une
nouvelle mesure du taux de victoire en tournoi.

## Validation finale

Reconstruction complète des cibles utilisant le moteur, sans avertissement.
`ctest --test-dir build-napoleonic --output-on-failure -j 4` : **35/35 tests
réussis**, 297,79 secondes. Aucun critère des tests préexistants n'a été modifié.

L'exécutable testé est installé dans `build/stratego.exe`, utilisé par
`Jouer.cmd`. Les deux copies ont le même SHA-256 :
`DAD8EEEEC98F57428504C2C5BD5DDB4287BB4AF8D19145D5C526F0FDF95D758C`.
