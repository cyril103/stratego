# Installer Stratego 3D sur Linux Mint 22.x

Le paquet `stratego3d_1.0.20260923_amd64.deb` vise les PC Intel/AMD 64 bits
sous Linux Mint 22.x. Il contient le jeu, l'IA corrigée et les ressources
graphiques : aucune compilation ni installation de Blender n'est nécessaire.
Une carte graphique et un pilote compatibles OpenGL 3.3 sont nécessaires.

## Installation

Copier le fichier `.deb` sur la machine Linux, puis l'ouvrir par double-clic
avec l'installateur de paquets. Autre possibilité, depuis le dossier du fichier :

```bash
sudo apt install ./stratego3d_1.0.20260923_amd64.deb
```

APT peut télécharger les bibliothèques manquantes. Le mot de passe demandé
est celui du compte Linux. Ensuite, lancer **Stratego 3D** dans le menu **Jeux**,
ou utiliser `/usr/games/stratego3d` dans un terminal. F11 quitte le plein écran.

Les placements et replays sont conservés dans
`~/.local/share/stratego3d/saves/` et `~/.local/share/stratego3d/reports/`
(ou dans `$XDG_DATA_HOME/stratego3d/` si ce réglage est défini).
Pour transférer un placement depuis Windows, copier `saves/placement.txt`
dans ce dossier `saves/`, une fois le jeu fermé.

## Mise à jour et désinstallation

Installer le nouveau `.deb` de la même façon. Les données personnelles restent
en place. Pour désinstaller le programme :

```bash
sudo apt remove stratego3d
```

La désinstallation ne supprime pas les placements ni les replays.

## Reconstruire le paquet

Construire sur **Ubuntu 24.04 ou Mint 22.x**, pas sur une distribution plus
récente : les versions de bibliothèques pourraient sinon être incompatibles.
Le jeu utilise raylib à la révision figée dans CMake, avec le backend X11.

```bash
sudo apt update
sudo apt install build-essential cmake git dpkg-dev file ca-certificates \
  libasound2-dev libgl1-mesa-dev libx11-dev libxrandr-dev libxi-dev \
  libxcursor-dev libxinerama-dev
git clone https://github.com/cyril103/stratego.git
cd stratego
bash tools/build_linux_mint.sh build-linux-mint --test
```

Le paquet se trouve dans `build-linux-mint/packages/`. Sans `--test`, seul le
jeu est compilé. `STRATEGO_BUILD_JOBS=4` permet de régler le parallélisme.
Le premier build télécharge raylib si `vendor/raylib` n'est pas fourni.
Pour une nouvelle livraison, changer `STRATEGO_PACKAGE_VERSION` dans
`cmake/LinuxPackage.cmake` ou le régler dans le cache CMake avant reconstruction.

Les `.blend`, les images de référence et les fichiers de développement ne
font pas partie du paquet. Les licences de raylib, GLFW et des polices sont
fournies avec leurs ressources.

## Validation de cette livraison

Le paquet a été compilé avec GCC 13 dans un environnement Ubuntu 24.04
isolé sous WSL. Les 38 tests CTest du moteur ont réussi (348,91 secondes).
L'installation et le lancement graphique ont été vérifiés
avec un compte sans privilèges, Xvfb et le rendu logiciel Mesa : sélection
des 100 cases, rendu de 150 images, sauvegarde et chargement du placement.
Le lanceur fonctionne aussi avec un chemin `XDG_DATA_HOME` contenant des
espaces. La réinstallation et la purge du paquet conservent les données
personnelles. Le raccourci respecte le format Desktop Entry.

Ce contrôle ne remplace pas un essai sur le bureau et le pilote graphique
de la machine Mint réelle ; les performances du rendu logiciel de test
ne préjugent pas de celles de la carte graphique. L'environnement isolé
ne dispose pas de périphérique audio : la lecture du son reste à vérifier
sur la machine Mint.

Sources de compatibilité et dépendances :
[base Ubuntu 24.04 de Mint 22](https://www.linuxmint.com/rel_wilma.php),
[construction Linux de raylib](https://github.com/raysan5/raylib/wiki/Working-on-GNU-Linux).
