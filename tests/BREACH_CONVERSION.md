# Conversion des brèches et conservation de l'armée

Depuis la correction du 23 septembre 2026, une réserve matérielle ne justifie plus de sacrifier un officier contre un drapeau seulement supposé.

Une bombe désamorcée reste une information publique utile. La mission courte de brèche, limitée à deux pas, utilise désormais un démineur ou un attaquant dont la probabilité publique de rencontrer une bombe ne dépasse pas 2 %. Les conditions de réserve, de défense du drapeau et de sécurité tactique restent actives. Un drapeau identifié est toujours pris immédiatement.

Cette règle remplace la politique précédente : le test historique exigeait explicitement la mort du maréchal après une permutation cachée bombe/drapeau. Cette attente est supprimée à la demande du joueur. Le test vérifie maintenant le refus du pari, l'invariance aux rangs cachés, l'exploitation par un démineur et la capture d'un drapeau révélé.

Un filtre complémentaire évite les sondages immobiles risqués par les autres unités utiles, lorsqu'une alternative survivante existe après les filtres d'urgence. Il fonctionne aussi sans avantage matériel. Le budget de risque diminue encore avec une avance : 8 % pour les capitaines et grades supérieurs, 25 % pour les autres unités concernées, divisés par deux au-delà de 10 % d'avantage dans l'évaluation des forces. Les éclaireurs gardent leur rôle de reconnaissance ; le dernier mobile et la recherche finale après élimination de l'armée adverse restent possibles. Ces seuils sont des choix prudents, pas des probabilités calibrées expérimentalement.

La conversion de l'avantage favorise les captures et les routes ouvertes vers des unités mobiles dont la probabilité de capture gagnante atteint 98 %. Elle tient compte des reprises, des menaces connues, des gardes assignés et de la défense du drapeau. Les maréchaux et généraux encore cachés ne reçoivent pas ce bonus. Les trajets traversent seulement des cases libres et sont limités à six pas. Une mission sûre du dernier démineur déjà engagée reste prioritaire : le bonus de chasse est désactivé tant qu'elle fournit une étape viable. Le test historique `scarce_piece_continuity` conserve sa victoire sur les quatre graines, sans modification de ses attentes.

## Dernières parties utilisées

- `human_win_474.jsonl`, avant le coup 362 : lieutenant vers 80, bombe estimée à 34,4 %, drapeau à 3,0 %. Le sondage est évité.
- Même partie, avant le coup 372 : colonel vers 83, bombe estimée à 29,5 %, drapeau à 2,8 %. Un capitaine connu en 17 menace déjà le drapeau en 7 sans défense possible ; le pari de dernier recours reste autorisé. La perte d'avantage précède cette position.
- `human_win_505.jsonl`, avant le coup 424 : sergent vers 79, bombe estimée à 32,2 %, drapeau à 2,9 %. Le sondage est évité.

Les fixtures complètes sont versionnées. Les tests vérifient trois graines par position, les permutations des rangs cachés, les drapeaux connus, les démineurs et une capture mobile sûre avec une cible immobile tentante à côté. Ils ne démontrent pas une victoire contre toutes les réponses d'un humain.

## Validation et livraison

Compilation Release complète sans avertissement. Les **38/38 tests CTest**
passent après correction de la priorité du dernier démineur (321,65 secondes).
Sur les trois graines du coup 362, le colonel joue 23 vers 22 et se rapproche
du capitaine connu en 21 ; le lieutenant ne sonde plus la bombe en 80.
Au coup 424, le sergent se retire de 69 vers 59 au lieu de sonder 79.

Le candidat local `build/stratego-campaign.exe` est mis à jour après cette
validation, via le même lanceur `Tester-IA-campagne.cmd`. Le lancement habituel
`Jouer.cmd` conserve la version précédente. Les nouvelles parties du candidat
portent l'identifiant `ExpertPlusImprovedConversionV1` pour distinguer les
replays des anciennes parties `ExpertPlusImprovedCampaignV1`.
Les scores de la campagne précédente ne sont pas des mesures de cette correction.
