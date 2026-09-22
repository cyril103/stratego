# Déploiements variés — 8 septembre 2026

## Recherche et choix de conception

Le générateur précédent utilisait toujours un drapeau sur la dernière ligne, entouré de trois bombes, et un maréchal dans les deux colonnes centrales. Les permutations ne changeaient donc pas sa structure principale.

Sources consultées :

- [A. F. C. Arts, Competitive Play in Stratego, université de Maastricht, 2010](https://project.dke.maastrichtuniversity.nl/games/files/msc/Arts_thesis.pdf), section 2.3 : protection du drapeau près d'un bord, structures leurres et conservation de démineurs. Le document présente plusieurs déploiements en annexe ; nos formations sont originales, sans reprise de ses grilles.
- [UltraBoardGames — Flag Placement](https://www.ultraboardgames.com/stratego/flag-placement.php) : compromis entre positions de bord, arrière et positions inhabituelles. Guide de pratique, pas étude comparative contrôlée.
- [UltraBoardGames — Bomb Placement](https://www.ultraboardgames.com/stratego/bomb-placement.php) : protection par bombes, prévisibilité des regroupements et appui mobile après désamorçage.
- [PlayMonster — Stratego Masters, règles](https://www.playmonster.com/wp-content/uploads/2018/06/StrategoMasters_Rules.pdf) : règles et conseils de déploiement de l'éditeur.

Il ne ressort pas de ces sources une formation universellement optimale. Les contraintes ci-dessous sont nos choix de mise en œuvre, pas une garantie de niveau ou une reproduction de formations championnes.

## Six familles originales

| Famille | Principe | Compromis |
|---|---|---|
| Coin fortifié | Deux bombes autour d'un drapeau de coin, garde et leurre opposé | Moins de directions d'approche, emplacement classique |
| Abri derrière le lac | Drapeau arrière décalé et écran de trois bombes | Accès indirect, silhouette de forteresse reconnaissable |
| Centre protégé | Fortification arrière centrale et capitaine en réserve | Défense des deux ailes nécessaire |
| Aile protégée | Drapeau arrière sur une aile et fausse poche opposée | Risque d'une percée latérale |
| Deuxième ligne | Drapeau avancé d'une ligne, entouré de quatre bombes | Moins conventionnel, davantage de bombes immobilisées dans la défense |
| Défense mobile et leurres | Drapeau sans enceinte complète, couvert par des officiers ; bombes déportées | Moins prévisible, nécessite de conserver ses gardes |

Chaque famille a une variante miroir. Les sondes de première ligne sont permutées dans les voies ouvertes, et les pièces restantes sont mélangées dans les emplacements libres. Les effectifs réglementaires, les positions du squelette défensif, l'espion adjacent au maréchal et les appuis par voie restent contraints.

En partie interactive, les six familles sont tirées dans un sac mélangé sans remise. Le renouvellement du sac évite de reprendre la dernière famille ; son identifiant est conservé dans `reports/last_formation.txt` pour éviter une répétition immédiate au redémarrage. Le choix ne dépend pas du placement caché du joueur et n'est pas affiché pendant la bataille. Les outils d'entraînement utilisent le choix déterministe basé sur la graine de `Game`.

## Vérifications

`formation_tests` vérifie 6 familles × 200 graines × 2 camps = 2 400 déploiements :

- Effectifs exacts, 40 identités uniques par camp et zone de placement réglementaire.
- Espion adjacent au maréchal.
- Démineur dans chacune des trois voies et officier d'appui dans les deux lignes avant de chacune.
- Connexion de chaque pièce mobile à une sortie : les autres pièces mobiles peuvent devoir bouger, mais aucune bombe alliée ne doit disparaître.
- Reproductibilité à graine et famille identiques ; dix cases de drapeau différentes observées, dont la deuxième ligne.

Ces contrôles valident la légalité, la diversité et certaines propriétés de mobilité. Ils ne mesurent pas le taux de victoire des familles : une comparaison sur de nombreuses parties reste nécessaire avant de leur attribuer des niveaux relatifs.
