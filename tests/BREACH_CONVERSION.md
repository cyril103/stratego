# Conversion d'une brèche vers le drapeau

L'IA conserve les coordonnées des bombes effectivement désamorcées. Cette mémoire publique est reconstruite automatiquement par les replays, sans accéder aux rangs cachés.

Une mission de conversion peut reprendre après la mort du démineur : une pièce mobile située à un ou deux pas attaque une pièce immobile inconnue du fond de camp, voisine d'une bombe désamorcée, lorsque sa probabilité de drapeau atteint 25 %. Le choix favorise la proximité et les attaquants moins coûteux.

La perte complète de l'attaquant doit laisser un avantage matériel d'au moins un capitaine et une autre pièce plus forte que toutes les unités mobiles adverses encore possibles. Les chemins traversent uniquement des cases libres. Les risques publics pour notre drapeau, les pertes tactiques connues, l'exposition supplémentaire à un espion et les filtres de défaite de l'IA restent prioritaires. Une bombe connue n'est jamais une cible de cette mission.

Cette décision de conversion précède la recherche par échantillonnage : des positions cachées hypothétiques ne doivent pas faire systématiquement abandonner cette courte occasion de victoire. Il s'agit d'une politique de risque calculé, pas d'une preuve que la cible est le drapeau.

Le test breached_flag_conversion reprend human_win_577 avant le coup 418 : maréchal E2-D2, puis D2-D1. Il vérifie quatre graines, toutes les réponses humaines légales sur le plateau enregistré, l'invariance après permutation cachée du drapeau avec une bombe, la perte effective du maréchal dans cette variante et le maintien du général. Il couvre aussi l'absence de brèche, les cibles connues ou mobiles, l'absence de réserve et l'urgence défensive.

Validation : les 21 tests CTest passent (reports/breach-tests.log). Le test graphique termine 150 images sans erreur. La comparaison contre Expert+ Classique, graine 204 dans les deux camps, donne une victoire en 603 demi-coups et une partie inachevée à 1200 ; les séquences de coups sont identiques à la référence defense577-final. Ces deux parties ne suffisent pas à établir une hausse globale de force. L'exécutable installé dans build/stratego.exe correspond à build-napoleonic/stratego.exe (SHA256 4B3FB057878BC8CF9E23FD3B54F2A7F0A1B073C03C7848FF7345219233BFFD58).
