# Stratego 3D — Atelier de stratégie

Jeu local jouable contre l'ordinateur, écrit en **C99 avec raylib 5.5**. Plateau 10 × 10, 40 pièces par camp, modèles originaux créés dans **Blender**, deux niveaux d'IA, placement libre par échange, animation des déplacements, journal des combats et caméra orbitale.

## Jouer sous Windows

L'interface impériale associe des titres Cinzel à la police Barlow pour les commandes et les informations de jeu. Ces polices sont fournies sous licence SIL OFL. Les boutons en bronze et les panneaux bleu nuit sont dessinés à la résolution du jeu, avec survol animé, relief d'appui et état désactivé. Un aigle doré et le panorama napoléonien complètent l'accueil.

Lancer `build/stratego.exe`, ou double-cliquer `Jouer.cmd` après compilation. Les assets sont copiés à côté de l'exécutable. OpenGL 3.3 est nécessaire. Aucun serveur ni compte.

**Linux Mint 22.x (Intel/AMD 64 bits)** : un installateur `.deb` et un lanceur
dans le menu Jeux sont disponibles. Voir [l'installation et la reconstruction du paquet](docs/LINUX_MINT.md).

Le jeu démarre en plein écran réel, à la résolution native du moniteur. Sur les écrans larges (16:10, 16:9 et ultralarges), l'interface adapte sa largeur : le plateau dispose de plus d'espace, le panneau reste ancré à droite et le fond remplit l'écran sans déformation ni bandes noires. `F11` bascule entre plein écran et fenêtre. Le bouton **Quitter**, en bas à droite, ferme le jeu depuis tous les écrans, y compris l'aide et les dialogues.

1. Choisir Découverte ou Expert + (sélectionné par défaut), puis « Préparer mon armée ».
2. Dans le placement en 2D, choisir un grade dans la réserve à droite puis une case des quatre rangées de votre camp. Cliquer une pièce posée puis une case pour la déplacer ou l'échanger ; clic droit pour la remettre en réserve. « Vider » recommence le placement et « Mélanger » pose une formation complète. Échap annule la sélection.
3. Une fois les 40 pièces placées, cliquer « Engager la bataille » : retour automatique en 3D. Vous jouez en premier.
4. Sélectionner une pièce, puis une destination dorée. Un marqueur orange désigne une attaque possible.

Pendant la préparation, **Sauvegarder** mémorise votre placement lorsque les 40 pièces sont posées. **Charger** restaure ce placement, que vous pouvez ensuite modifier avant de lancer la bataille. Une seule sauvegarde est conservée : une nouvelle sauvegarde remplace la précédente. Elle persiste après fermeture du jeu dans `saves/placement.txt` (répertoire de lancement). Un fichier invalide est refusé sans modifier votre placement courant.

| Commande | Action |
|---|---|
| Clic gauche | Sélection, échange pendant le placement, déplacement / attaque |
| Clic droit maintenu + déplacement | Orbite de la caméra |
| Molette | Zoom progressif vers le pointeur, jusqu'au gros plan sur une pièce |
| Clic molette maintenu + déplacement | Déplacer la vue sur le plateau |
| C | Recentrer la caméra |
| F1 / bouton Règles | Manuel intégré |
| Échap | Fermer l'aide ou annuler la sélection |
| M | Activer / couper le son |
| Fermer la fenêtre | Quitter |

Les marqueurs D et B représentent le drapeau et les bombes. Les autres sont les rangs 1 à 10. Un point doré sur une pièce alliée indique que son rang a été révélé à l'ordinateur. Les pièces ennemies inconnues utilisent toutes **le même modèle**, pour ne rien dévoiler par leur silhouette.

La caméra utilise une perspective douce de 28°, centrée face au plateau, avec une inclinaison initiale de 58°. La molette rapproche la caméra jusqu'au gros plan ; le clic molette déplace la vue et `C` réinitialise le cadrage. Le plateau forme une maquette de champ de bataille : prairie et terre texturées, chemin discret, deux lacs irréguliers avec berges en relief, galets et roseaux. L'eau anime ses rides, sa profondeur apparente et ses reflets. Le socle en noyer texturé reçoit des bordures en laiton et une ombre de contact. Le placement utilise les mêmes matériaux en vue du dessus. Les textures générées et leurs prompts sont documentés dans `assets/textures/README.md`.

Le présentoir **Pièces sorties**, à droite du plateau, regroupe les pertes dans douze emplacements par grade. Les onglets **Bleues** et **Rouges** sélectionnent l'armée et indiquent ses pertes totales. Chaque emplacement affiche une miniature de la pièce et le nombre sorti sur l'effectif initial ; survolez-le pour lire son nom et le nombre restant en jeu. Le dernier grade éliminé est encadré en doré. Les compteurs changent à la fin de l'animation du combat, y compris pour les deux camps en cas d'égalité. Ils ne dévoilent aucune position ou identité des pièces encore cachées. Le journal reste accessible dans l'onglet voisin.

## Compiler

### Campagne IA

La version corrigée `2bc16bf` est validée par les 38 tests CTest et installée
comme version habituelle à la demande du joueur : `Jouer.cmd` lance désormais
la correction de gestion de l'avantage. `Tester-IA-campagne.cmd` lance la même
version. La sauvegarde locale de `09936c8` reste dans
`reports/stratego-before-campaign-09936c8.exe`.
Cette validation technique n'établit pas un gain global de force. La campagne antérieure
du candidat `994ff5a` avait donné 1 victoire, 2 défaites et 1 partie inachevée
au budget natif ; ces chiffres ne mesurent pas les corrections suivantes.
Les résultats et les limites sont détaillés dans
[le bilan de campagne](tests/AI_CAMPAIGN_20260923.md).
Les exécutables ne sont pas versionnés ; compiler les sources actuelles
reconstruit cette nouvelle version, pas l'ancienne référence.
La [correction des sondages de bombes et de la conversion de l'avantage](tests/BREACH_CONVERSION.md)
utilise les deux dernières défaites humaines en 474 et 505 demi-coups.

Pour reconstruire un candidat séparément du lancement habituel avec MinGW :

```powershell
cmake -S . -B build-campaign-ui -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build-campaign-ui -j 6
ctest --test-dir build-campaign-ui --output-on-failure
New-Item -ItemType Directory -Force build
Copy-Item build-campaign-ui/stratego.exe build/stratego-campaign.exe
.\Tester-IA-campagne.cmd
```

### Compilation habituelle

CMake 3.20+, un compilateur C et Git sont nécessaires. Au premier lancement de CMake, raylib est récupéré automatiquement si `vendor/raylib` n'existe pas. Sa révision est figée. Les assets exportés sont inclus : Blender n'est pas nécessaire pour jouer ou compiler.

```powershell
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build -j 6
ctest --test-dir build --output-on-failure
.\build\stratego.exe
```

Avec Visual Studio, omettre le générateur MinGW, puis utiliser `cmake --build build --config Release` et `ctest --test-dir build -C Release`. L'exécutable se trouve alors dans `build/Release`.

Linux Mint 22.x : suivre [la procédure du paquet Debian](docs/LINUX_MINT.md), compilé et testé dans un environnement Ubuntu 24.04. macOS : utiliser CMake avec les outils de compilation Xcode ; cette plateforme n'a pas été validée sur cette machine.

Tests sans dépendance graphique :

```sh
cmake -S . -B build-tests -DSTRATEGO_TESTS_ONLY=ON
cmake --build build-tests
ctest --test-dir build-tests --output-on-failure
```

## Assets Blender

Le rendu temps réel utilise trois éclairages de studio, des reflets dépendant de la caméra, une réponse distincte pour le plastique, les plaques métalliques et le bois, ainsi qu'une correction gamma et une compression douce des hautes lumières. Des textures d'ombres de contact diffuses remplacent les disques opaques ; elles suivent les déplacements sans révéler les grades cachés. La scène 3D est calculée en 2032 × 1408 puis réduite pour lisser les contours. C'est une approximation temps réel du style de la planche Blender, sans calcul complet d'éclairage indirect ni ombres géométriques entre les pièces.

`assets/stratego.blend` contient le plateau, ses incrustations et les modèles de pièces. `tools/create_assets.py` recrée la scène et exporte les OBJ triangulés, en coordonnées Y vertical pour raylib. Compatible Blender 2.93 et versions ultérieures ; génération vérifiée avec Blender 2.93.1.

```powershell
& 'D:\Program\blender2.93\blender.exe' --background --python tools/create_assets.py
cmake --build build
```

Les cases, marqueurs et ombres de contact sont dessinés dans raylib. Le terrain, le bois, les rives et l'eau disposent de matériaux GLSL distincts, avec des reflets HDR et une eau animée par du bruit. Le quadrillage reste discret et les huit cases des lacs restent infranchissables, y compris leurs berges décoratives. Les pièces conservent leurs corps bleus ou rouges et leurs créneaux. Leurs plaques dorées portent douze illustrations originales du Premier Empire : portraits des grades, drapeau et bombe, générés avec ImageGen puis appliqués comme textures UV. Les chiffres sont composés séparément ; un petit repère au pied des pièces connues conserve leur lisibilité en vue éloignée. Les mêmes textures apparaissent dans le placement, les captures et les combats. Les grades cachés restent absents du rendu. Voir [les illustrations et leur prompt](assets/pieces/README.md).

`tools/boardgame_pieces.py` construit les coques et les douze emblèmes dans Blender. `assets/pieces_catalog.png` et `assets/pieces_catalog.blend` présentent la bibliothèque ; ils sont régénérés avec `tools/render_piece_catalog.py`. Dans le jeu, les ennemis cachés utilisent uniquement la coque commune, même quand la caméra tourne. Les chiffres flottants sont limités à la pièce sélectionnée ; le zoom permet de lire les faces. La police Barlow est distribuée sous SIL OFL, licence incluse dans `assets/fonts/OFL.txt`. Les effets sonores sont synthétisés au démarrage.

## Règles et IA

### Entraînement contre Expert+

`tools/start_expert_training.ps1` lance en arrière-plan 100 000 épisodes contre le véritable Expert+ (`ai_choose`, difficulté 1, recherche inchangée), en alternant les camps. Compiler d'abord la cible `train_expert`. Le modèle linéaire existant sert de point de départ ; seul le camp apprenant reçoit les mises à jour REINFORCE. La limite est de 1 600 demi-coups : les épisodes interrompus à cette limite sont comptés séparément dans le journal et ne produisent pas de récompense de victoire.

`training/expert100k/latest.policy` est sauvegardé après chaque épisode, avec une archive tous les 1 000 épisodes. Relancer le script reprend au dernier épisode sauvegardé, jusqu'au total de 100 000. Les journaux horodatés donnent les victoires, défaites, épisodes limités et le temps écoulé de chaque session. L'entraînement conserve le modèle actuellement utilisé par le jeu ; le résultat doit être évalué sur des parties indépendantes avant de le remplacer.

Un camp sans coup légal à son tour perd automatiquement, même si son drapeau est intact. Le bouton **Capituler**, en haut pendant la bataille, permet aussi de concéder la victoire à l'ordinateur ; une confirmation évite les clics accidentels. Il est indisponible pendant une animation de déplacement ou de combat. La victoire affiche un étendard doré et des confettis ; la défaite, un étendard abaissé et une transition plus sobre. Le motif de fin est indiqué. **Voir le plateau** ferme l'annonce et **Rejouer** prépare une nouvelle partie. Les scénarios `--resign-demo` et `--immobile-demo` vérifient respectivement le bouton de confirmation et une défaite avec drapeau intact, puis exportent une capture du résultat.

Les grades adverses ne restent plus affichés définitivement : ils sont visibles pendant le combat et jusqu'à la fin du demi-tour suivant (le prochain déplacement terminé, quel que soit le camp). La pièce reprend ensuite son dos anonyme. Un nouveau combat relance cette visibilité temporaire ; un long déplacement d'éclaireur utilise la même durée. Vos propres grades restent visibles. L'IA conserve les rangs observés dans sa mémoire, indépendamment de leur affichage. Le journal à l'écran indique le résultat et les cases du combat sans conserver les noms des grades, pour laisser au joueur le travail de mémorisation.

Les combats sont présentés en plusieurs temps : approche, retournement des pièces ennemies cachées, pause de lecture des deux grades, retrait du perdant puis occupation de la case par l'attaquant victorieux. En cas d'égalité, les deux pièces disparaissent ; si le défenseur gagne, il reste sur place. La séquence dure environ 2,55 secondes et respecte les exceptions démineur/bombe et espion/maréchal. L'état des règles et le journal ne sont mis à jour qu'à la fin ; l'aide met aussi l'animation en pause.

Les vérifications visuelles automatisées sont accessibles avec `build/stratego.exe --combat-demo=6,5` (victoire), `--combat-demo=6,6` (égalité), `--combat-demo=4,8` (défaite), `--combat-demo=3,11` (démineur), `--combat-demo=1,10` (espion) et `--combat-demo=6,0` (drapeau). Ajouter `--combat-enemy` pour vérifier le retournement d'un attaquant adverse. Ces démonstrations exportent une capture pendant la révélation et une après résolution, puis comparent le plateau final au résultat du moteur de règles.

Composition classique : drapeau ×1, espion ×1, éclaireur ×8, démineur ×5, sergent ×4, lieutenant ×4, capitaine ×4, commandant ×3, colonel ×2, général ×1, maréchal ×1, bombe ×6.

Règles prises en charge : mouvements orthogonaux, lacs infranchissables, éclaireurs à longue portée sans saut, rang supérieur gagnant, élimination mutuelle à égalité, espion attaquant le maréchal, démineur contre bombe, drapeau et bombes immobiles, victoire par capture du drapeau ou absence de coup légal. Un quatrième trajet consécutif d'une même pièce entre deux cases est interdit. Les rangs révélés restent mémorisés.

Adaptations de cette édition : le joueur ivoire commence ; pas d'arbitrage de la règle avancée de poursuite sur plusieurs cases des tournois, ni de règle de partie nulle automatique. Pas de sauvegarde, jeu réseau ou multijoueur local.

Le niveau **Expert +** compare jusqu'à **24 coups sur 16 formations ennemies plausibles**, puis approfondit six finalistes. Après le coup candidat, la recherche vise progressivement 2, 4 et 6 demi-coups, voire 8 en finale ou en crise autour du drapeau. Elle utilise un cache privé de positions et ne conserve que les itérations terminées. Dans chaque monde hypothétique, les candidats sont comparés à la même profondeur effectivement atteinte : les maxima ne sont pas garantis. Une extension tactique examine jusqu'à trois captures successives, y compris les échanges à égalité, avec six captures candidates par niveau. La recherche alpha-bêta conserve jusqu'à dix réponses par nœud et limite le calcul par branche : ce n'est pas une recherche exhaustive ni une preuve de jeu parfait. Voir [la validation de la recherche](tests/SEARCH_VALIDATION.md).

Les rangs ennemis cachés sont effacés avant la construction des formations simulées. Les échantillons respectent les effectifs restants et n'attribuent jamais une bombe ou un drapeau à une pièce ayant bougé. Les positions reculées et les bombes révélées constituent des indices probabilistes pour chercher le drapeau, pas des certitudes. L'IA ne consulte pas les rangs cachés réels. Dans chaque simulation, la recherche utilise les rangs supposés ; cette approximation ne modélise pas parfaitement toutes les décisions sous incertitude.

Expert+ utilise également l'immobilité prolongée : après 40 demi-coups, l'indice augmente progressivement jusqu'à 280 demi-coups. Un groupe de pièces adjacentes n'ayant jamais bougé renforce l'hypothèse d'un drapeau protégé par des bombes, en complément de la profondeur de placement et des bombes déjà identifiées. Une pièce voisine révélée comme officier ne renforce pas cet indice. Les officiers en réserve restent possibles ; aucune pièce cachée n'est transformée en certitude. Ces poids heuristiques, non calibrés sur un grand corpus de parties, servent à la fois aux probabilités de combat et aux formations simulées. Les pertes connues continuent de limiter les grades possibles.

L'évaluation tient compte des menaces sur toute l'armée et des possibilités de recapture, de la valeur des pièces selon les forces restantes, du soutien entre unités et d'une réserve près du drapeau. La valeur d'un espion diminue après la disparition du maréchal adverse ; les derniers démineurs deviennent plus précieux. Les probabilités immédiates de combat sont évaluées analytiquement pour réduire les paris dus à un échantillon chanceux. Un drapeau seulement supposé n'est plus évalué comme une victoire certaine ; les scénarios défavorables contribuent au classement des plans.

Les menaces publiques restent prises en compte dans le classement final des coups. Une avancée qui livre une pièce à un adversaire identifié sans possibilité de recapture reçoit une forte pénalité. Quand aucun officier adverse de rang égal ou supérieur ne subsiste, l'IA accorde plus de priorité à l'activation de son officier dominant. La défaite enregistrée en 161 demi-coups fournit des tests de mobilisation du général et de protection d'un démineur exposé ; cela vérifie ces situations précises, sans garantir une victoire sur toute la partie.

Les officiers poursuivent les pièces mobiles et les menaces révélées, au lieu de se précipiter sur les défenses fixes. Quand une unité moins précieuse est disponible, le maréchal et le général évitent de sonder une pièce potentiellement piégée. Les éclaireurs explorent, les démineurs ouvrent les défenses et les officiers peuvent les escorter. Les incursions près du drapeau, surtout celles des démineurs, déclenchent une priorité d'interception. La mémoire des huit derniers coups de chaque camp décourage les déplacements circulaires. Le mode Découverte conserve l'ancienne IA simplifiée.

Après un désamorçage observé, l'IA peut poursuivre vers un drapeau probable avec un démineur, ou un attaquant dont le risque public de bombe est négligeable. Une réserve matérielle ne justifie plus le sacrifice d'un officier sur cette hypothèse. Lorsqu'une alternative viable existe, les unités utiles évitent les sondages immobiles risqués et privilégient les captures ou poursuites sûres de pièces mobiles. Les urgences du drapeau et les paris de dernier recours restent prioritaires. Voir [les règles et tests de conversion](tests/BREACH_CONVERSION.md).

Le dernier démineur privilégie une route sûre vers le drapeau le plus probable lorsqu'un sondage voisin risque de le perdre. En fin de partie, la réserve rejoint le garde menacé par un chemin réellement praticable et évite les échanges sans soutien. Ces missions réduisent les allers-retours qui retardaient l'attaque ou la défense ; elles sont vérifiées sur la défaite enregistrée en 547 demi-coups (`tests/CONTINUITY.md`).

Le maréchal conserve la mémoire des contacts qu'il a déjà évités : un nouveau suspect ne doit pas le faire revenir vers un ancien poursuivant. La reprise sûre d'un espion révélé menaçant une pièce stratégique devient prioritaire, même avec une armée encore complète. L'évaluation tient compte de la perte du maréchal malgré une reprise ultérieure de l'espion et de la valeur de l'espion allié tant que le maréchal ennemi survit (`tests/MARSHAL_GUARD.md`).

La défense compare les temps d'arrivée sur les voies d'invasion et pénalise les menaces proches qui ne peuvent pas recevoir de défenseurs distincts. Cette affectation est une estimation prudente, pas une preuve d'interception. Les officiers favorisent une escorte à proximité d'un démineur avancé si la case est sûre face aux ennemis connus. L'historique des quatre derniers tours encourage la poursuite d'une progression sans imposer un plan devenu dangereux. Les deux derniers démineurs reçoivent une protection supplémentaire tant que des bombes adverses subsistent.

En présence d'une invasion proche, les officiers à partir du capitaine recherchent aussi un chemin comprenant le dégagement de leurs propres pièces. Le coût inclut les déplacements des bloqueurs ; les chaînes de dégagement sont limitées à trois pièces et les chemins à sept unités de coût. Le bonus ne s'applique qu'aux chemins nécessitant un dégagement ou venant d'être ouverts, pour préserver les interceptions directes déjà disponibles. Le rejeu de la défaite en 251 demi-coups vérifie, sur quatre graines, le dégagement de H8, la progression du commandant et l'interception du démineur en I10 avant la capture du drapeau. Des tests vérifient également une position symétrique et l'indépendance vis-à-vis des grades cachés. Ce rejeu garde les coups adverses enregistrés et ne constitue pas une preuve de victoire contre toute réponse.

Une exception cible désormais les officiers envahisseurs identifiés, du commandant au maréchal : les routes d'interception peuvent être favorisées même sans bloqueur, afin de mobiliser une pièce capable de les arrêter avant de nouvelles pertes. Un échange à égalité peut terminer cette interception, mais le chemin ne continue pas fictivement avec la pièce détruite. La défaite en 335 demi-coups vérifie sur quatre graines la mobilisation du maréchal et la capture du général envahisseur après son entrée en C8. La recherche jusqu'à sept demi-coups est aussi déclenchée lorsqu'un intrus mobile dispose d'un chemin d'au plus quatre cases vers le drapeau ; les lacs, les pièces de son camp et les bombes qu'il ne peut pas désamorcer bloquent ce chemin. Une position révélée indépendante vérifie qu'un échange égal de maréchaux ouvrant une attaque forcée de démineur sur le drapeau est évité.

L'ordinateur génère ses placements par mutations de six familles initiales. Il varie aussi les positions des bombes et du drapeau, conserve une poche servant de leurre et autorise zéro à deux bombes en première ligne. Les trois voies disposent de démineurs et d'officiers d'appui, avec un espion adjacent au maréchal et des sorties pour les pièces mobiles. Les six anciens modèles restent disponibles pour les comparaisons reproductibles. Voir [la campagne de progression et ses contrôles de placement](tests/AI_CAMPAIGN_20260923.md). Le joueur conserve son placement libre. Le calcul se fait en arrière-plan ; quatre branches indépendantes peuvent être évaluées simultanément sans changer le résultat déterministe à graine identique.

La campagne du 23 septembre ajoute une mémoire des approches et retraites publiques, des probabilités compatibles avec les effectifs ennemis restants, un risque de raid adapté au rang et aux réserves, ainsi que la continuité des missions de démineurs. Le maréchal vérifie aussi si une approche adverse peut fermer ses retraites, en conservant les possibilités de capture sûre ou d'attente avec une autre pièce. Une ligue fige les anciennes versions et compare les mêmes placements dans les deux camps ; les parties au plafond restent inachevées. Les résultats automatiques ne constituent pas une mesure de niveau humain. Voir [le protocole et la validation](tests/AI_CAMPAIGN_20260923.md).

Le comparateur affronte **l'Expert+ livré juste avant les changements de recherche du 8 septembre**, conservé dans `tests/ai_previous.c`. Il utilise le même placement initial pour chaque paire, inverse le camp de la nouvelle IA et compte explicitement les parties inachevées. Le troisième argument choisit la première graine, le quatrième peut limiter le test à un camp (0 ou 1). Le cinquième active les formations structurées pour les deux armées. Les temps moyens des deux moteurs sont affichés.

Résultats historiques du comparateur, antérieurs aux dernières corrections tactiques : **4 victoires / 2 défaites** sur le banc de développement à placements identiques, puis **1 victoire / 1 défaite** sur un placement distinct avec la nouvelle formation structurée. Détails, conditions et limites dans [tests/AI_VALIDATION.md](tests/AI_VALIDATION.md). Ces petits échantillons ne garantissent aucun niveau contre un humain. Le cinquième argument du comparateur active la formation structurée de la nouvelle IA.

```powershell
.\build\ai_benchmark.exe 3 700 2
```

Les tests couvrent aussi la capture du drapeau, l'espion contre le maréchal, le démineur contre une bombe, le refus d'une capture protégée, le repli d'un général menacé, les paris sur une bombe inconnue et l'interdiction d'ouvrir une ligne d'éclaireur sur son propre drapeau. Un solveur exhaustif indépendant vérifie une attaque gagnante en cinq demi-coups sur une petite position révélée. La permutation des seuls rangs cachés doit préserver le coup choisi et l'état du générateur aléatoire. Les instantanés, l'arrêt et le redémarrage du calcul en arrière-plan sont également testés.

Corrections issues de la partie terminée en 227 demi-coups le 8 septembre : risque d'espion renforcé pour le maréchal, retraite anticipée des hauts grades face à un poursuivant identifié et couverture distincte des accès au drapeau. La défense peut envisager de sacrifier une pièce pour éliminer un démineur escorté. Le journal `tests/fixtures/human_win_227.jsonl` vérifie le piège de l'espion, la sortie du général en deux mouvements et le maintien de la couverture du drapeau sur huit graines. Ces tests ciblés ne mesurent pas un taux de victoire global.

Chaque partie jouée est enregistrée dans `reports/partie_*.jsonl`, sous le répertoire de travail du jeu. Le fichier contient le placement initial, les coups, les combats et l'issue. Ces journaux permettent de reproduire et d'analyser une défaite ; ils ne sont jamais lus par l'IA pendant la partie et ne constituent pas un apprentissage automatique. Ils ne permettent pas encore de reprendre une partie dans l'interface.

L'invincibilité n'est pas garantie : le moteur conserve des approximations, une profondeur bornée et l'incertitude sur les pièces ennemies. Il n'accède pas aux rangs cachés réels et n'embarque pas de modèle entraîné de type DeepNash.

La correction du 12 septembre dans **Expert+ Improved** compare les trajets des menaces et des défenseurs pour rappeler plus tôt une réserve capable d'intervenir. Le protocole, les tests et le comparatif avec Classique sont décrits dans [le bilan d'interception](tests/INTERCEPTION_AI_20260912.md).

La correction du 17 septembre prend aussi en compte une pièce inconnue qui vient au contact d'un officier révélé, du colonel au maréchal. Si un grade capable de le battre reste possible, cette approche augmente la priorité de protection ; une capture qui révèle le maréchal à côté d'un espion possible est également pénalisée. Le terme porte sur toute l'armée et ne crédite pas une attaque incertaine comme un sauvetage réussi. Il utilise uniquement les déplacements publics, les grades observés et les pertes connues. La partie `tests/fixtures/human_win_293.jsonl` sert de régression pour le piège de l'espion et le repli du général, avec huit graines et permutation des identités cachées. Cette prudence reste heuristique : elle peut être exploitée par un bluff et ne garantit pas la victoire.

Expert+ Improved valorise aussi le secret de ses éclaireurs : leur premier déplacement de plusieurs cases sans attaque reçoit une pénalité, conservée dans le classement final des coups. Avancer d'une case préserve leur possibilité de bluff. Cette pénalité diminue avec la proportion d'éclaireurs parmi les grades mobiles encore inconnus de l'adversaire ; elle est nulle si leur identité est déjà connue ou si seuls des éclaireurs restent possibles. Les attaques ne reçoivent pas cette pénalité. Les longs déplacements restent autorisés pour une défense urgente ou un avantage estimé suffisant : ce n'est pas une interdiction de leur mobilité. Le demi-coup 210 du rejeu, avec la graine 519, vérifie le choix D9-C9 à la place de D9-B9 ; des tests conservent une capture lointaine de drapeau et un long déplacement sans attaque indispensable à sa défense.

La défaite en 259 demi-coups a révélé un rappel trop limité : il ignorait les commandants et s'arrêtait dès que le défenseur quittait les trois dernières rangées adverses. Le rappel considère désormais les intrus identifiés à partir du commandant, jusqu'à huit pas terrestres du drapeau, et continue après cette frontière. Les coups de rappel ne reçoivent aucun bonus s'ils impliquent un combat incertain ou une exposition à une capture connue. La poursuite des pièces mobiles plus faibles s'applique aussi aux colonels, généraux et maréchaux dominants ; pour le maréchal, elle attend l'élimination de l'espion adverse. Le test `dominant_officer_recall` vérifie l'interception de la percée enregistrée sur quatre graines, la continuité du retour, une position symétrique et l'indépendance des grades cachés. Ce rejeu est une régression tactique, pas une mesure de taux de victoire global.

La défaite en 369 demi-coups étend la protection des officiers aux menaces persistantes : toutes les pièces inconnues adjacentes sont considérées, et pas seulement celle du dernier coup. Une approche récente conserve une prime de danger, mais le risque de contact subsiste ensuite. La plus forte menace inconnue est comptée par officier ; les pertes connues et les grades révélés limitent toujours les possibilités. Les positions 48 et 176 de `human_win_369.jsonl` vérifient sur huit graines que le général ne revient pas en B5 et que le maréchal ne prend pas le général piégé en C7, avec permutation des identités cachées. Les anciens tests de repli et de piège à l'espion restent applicables. Voir [la validation des menaces persistantes](tests/PERSISTENT_THREATS_20260917.md).

La défaite en 429 demi-coups ajoute le dégagement d'un haut grade menacé : une pièce amie peut recevoir un bonus pour ouvrir sa première sortie utilisable. Les captures des hauts grades sont aussi pénalisées lorsqu'un soutien inconnu peut les reprendre, indépendamment du danger déjà présent sur la case de départ. En finale, une attaque susceptible de perdre le dernier défenseur à trois pas du drapeau est pénalisée si un démineur possible approche ; capturer ce démineur lui-même reste autorisé. Le rejeu vérifie sur huit graines le dégagement du maréchal au demi-coup 322 et la conservation du dernier lieutenant au 420, sans consulter les grades cachés. Ces évaluations restent heuristiques et ne prouvent pas la victoire sur toute la partie.

Quand un officier à partir du capitaine dépasse tous les grades numériques adverses restants, Expert+ Improved peut lui assigner un démineur pour une attaque de drapeau. La paire vise une case de drapeau probable, choisie avec les informations publiques. Le chemin du démineur tient compte des lacs, des bloqueurs et du coût estimé des combats ; l'officier doit pouvoir rejoindre son partenaire. La progression du démineur et le rapprochement de l'escorte sont récompensés, tandis qu'un démineur prenant plus de deux pas d'avance reçoit une pénalité. La mission n'encourage ni l'exposition du démineur à une capture ni un pari de l'officier sur une bombe ou un espion. Elle est suspendue en cas de menace détectée contre le drapeau de l'IA. Ce guidage intervient dans le classement initial et final des coups, sans remplacer la recherche tactique.

L'audit du 17 septembre étend cette mission aux grades égaux lorsque l'avance en matériel mobile vaut au moins l'officier engagé, avec au moins deux pièces mobiles supplémentaires. Les alliés peuvent aussi libérer la première avancée sûre d'un démineur bloqué. Les officiers encore cachés bénéficient désormais d'une protection contre les contacts inconnus, moins forte que celle des officiers révélés. Un calcul local des courses au drapeau examine les réponses légales et les restrictions d'aller-retour. Le protocole, les résultats avant/après et les limites sont détaillés dans [l'audit du tournoi](tests/TOURNAMENT_AUDIT_20260917.md).

Le cycle stratégique suivant améliore l'évaluation des grades selon les forces restantes, les échanges, la prudence en position favorable et la reconnaissance avec dégagement des éclaireurs. Les grades certains sont déduits des pertes publiques sans lire les identités cachées ; l'interception considère tous les grades mobiles possibles. La référence évaluée obtient **6 victoires, 2 défaites et 2 inachevées** en développement, puis **6–4 sur dix parties de contrôle avec cinq nouveaux placements joués dans les deux camps**. Les essais rejetés, les défaites restantes et les preuves sont détaillés dans [l'audit stratégique complet](tests/STRENGTH_AUDIT_20260917.md). Ce résultat ne rend pas l'IA invincible.

Le correctif suivant, issu des parties humaines de 18 h 50 et 19 h 17, retire un faux bonus défensif lorsqu'une pièce déplacée peut être capturée immédiatement sans reprise possible. Sur seize relances de la position critique, le colonel offert au maréchal adverse passe de 11 choix à zéro ; les 15 tests passent et les prises décisives de la partie gagnée sont conservées. Ce correctif est installé, avec sauvegarde de la référence. Son taux de victoire global n'a pas été remesuré : voir [l'audit des deux parties](reports/analyses/duel_20260917_1933/analyse.md).

L'audit des trois parties humaines suivantes (20 h 04, 20 h 21 et 20 h 49 : victoires de l'IA, dont une par abandon humain) corrige l'abandon d'un démineur menacé pendant qu'une autre pièce joue. Les deux derniers démineurs sont désormais inclus dans la pénalité de capture connue sans reprise lorsqu'il reste des bombes adverses. Sur la position enregistrée au demi-coup 470, la retraite sûre passe de 4/16 à 16/16 essais ; les 15 tests passent. Aucun nouveau taux de victoire global n'est établi : voir [l'audit des trois parties](reports/analyses/latest_20260917_2110/analyse.md).

## Structure

La correction issue de la défaite en 383 demi-coups améliore le repli du maréchal, affecte un officier à l'interception d'un haut grade identifié et vérifie les pièges qui éliminent la dernière pièce libre autour d'un drapeau fortifié. Les trois positions critiques sont rejouées sur quatre graines, avec permutation des grades cachés. Voir [la validation de la protection des officiers et du dernier défenseur](tests/DEFENSIVE_TEMPO.md).

La partie suivante, terminée en 379 demi-coups, ajoute l'anticipation des retraites dans une impasse, le dégagement des sorties, l'interception locale d'un général par le maréchal et la couverture du général par l'espion. Une évaluation supplémentaire protège le dernier haut grade contre les échanges qui laisseraient les officiers adverses dominants. Voir [les comportements et les tests de coordination des officiers](tests/OFFICER_TEAM.md).

La défaite en 625 demi-coups corrige la confusion entre les grades des pièces alliées et leur révélation à l'adversaire dans les trajets vers le drapeau. La protection des pièces menacées inclut l'espion et les gardiens de faible grade ; les captures inconnues sont évaluées avec la reprise connue et les rôles restants. Les dernières réserves sont rappelées vers la zone défensive avant la percée. Voir [la validation des rôles et de la défense](tests/DEFENSE_ROLES.md).

La défaite en 479 demi-coups étend les missions défensives aux grades inférieurs devenus décisifs en finale. Les alliés dégagent le passage des réserves, l'espion bénéficie de la recherche d'encerclement, et les échanges directs tiennent compte des forces restantes. La recherche de capture forcée du drapeau couvre aussi les capitaines et autres pièces mobiles après ouverture des bombes. Voir [la validation de la défense adaptée aux forces restantes](tests/ADAPTIVE_DEFENSE.md).

La défaite en 577 demi-coups ajoute la relève des gardiens bloqués par une ligne d'éclaireur : un allié vient couvrir la colonne avant l'interception du démineur. La recherche locale reste active même avec une grande armée défensive lorsque les attaquants sont peu nombreux. Voir [la défense coordonnée et son rejeu](tests/GUARD_RELIEF.md).

Les fins de partie suivent les cas applicables de l'article 12 du règlement ISF : immobilisation d'un camp = défaite, immobilisation des deux = nulle, possibilité de proposer une nulle à l'IA. Le tournoi distingue les vraies nulles des interruptions techniques. Voir [les règles de fin de partie et leurs limites](tests/OFFICIAL_ENDINGS.md). Le jeu interactif reste sans pendules.

| Fichier | Rôle |
|---|---|
| `src/game.c`, `src/game.h` | État du jeu, placement, coups légaux, combats, victoire |
| `src/ai.c` | Adversaire fondé sur les observations publiques |
| `src/ai_intercept.h` | Trajets d'approche et priorité du défenseur capable d'intercepter une menace |
| `src/ai_basic.c` | Ancienne IA conservée pour Découverte et la comparaison |
| `src/ai_deploy.c` | Placement initial structuré de l'adversaire |
| `src/ai_worker.c`, `src/ai_parallel.c` | Recherche en arrière-plan et branches parallèles |
| `src/match_log.c` | Journal local reproductible des parties |
| `src/main.c` | Rendu, interface, animation, son et entrées |
| `tools/create_assets.py` | Création Blender et export des modèles |
| `tests/test_game.c` | Tests de règles, invariance des informations cachées, parties simulées |
| `tests/benchmark_ai.c`, `tests/ai_previous.c` | Comparaison avec la précédente version Expert, camps alternés |
| `assets/` | Fichier Blender, OBJ, shaders et police |

Vérification graphique automatisée : `stratego --smoke` produit `preview.png`, `stratego --smoke --battle` joue quelques coups puis produit `battle.png`. Ces modes ouvrent brièvement une fenêtre et se ferment seuls.

## Références Web

- [Règles officielles Stratego Original, Royal Jumbo / PlayMonster (2019)](https://www.playmonster.com/wp-content/uploads/2019/07/StrategoOR-_Rules_2019.pdf) : composition, placement, plateau illustré, mouvements et exceptions de combat.
- [Notice officielle Hasbro](https://instructions.hasbro.com/en-us/instruction/stratego-game) : référence complémentaire de l'édition classique.
- [Exemple officiel raylib : chargement de modèles](https://www.raylib.com/examples/models/loader.html?name=models_loading) : intégration des modèles OBJ.
- [Sources officielles raylib 5.5](https://github.com/raysan5/raylib/tree/5.5) : API de rendu 3D, picking et shaders, licence zlib.
- [Barlow, dépôt Google Fonts](https://github.com/google/fonts/tree/main/ofl/barlow) : police et licence.
- [DeepNash : recherche de Google DeepMind sur Stratego](https://deepmind.google/blog/mastering-stratego-the-classic-game-of-imperfect-information/) : référence sur l'information cachée, la reconnaissance et l'apprentissage par autojeu. Ce projet utilise un moteur de recherche fait maison, pas DeepNash.

Projet non officiel. Stratego est une marque de Royal Jumbo. Aucun modèle ou visuel du jeu commercial n'a été copié dans les assets.
