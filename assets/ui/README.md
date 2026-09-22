# Interface imperiale

Les boutons et panneaux sont dessines par `src/ui_theme.h` : email bleu nuit,
cadres en laiton, doubles filets, coins graves et ombres. Le survol s'anime
progressivement ; l'appui inverse le relief, les commandes indisponibles
sont desaturees. Les textes restent rendus par le jeu et les cadres s'adaptent
a la taille des commandes.

`imperial-emblem.png` est un embleme a fond transparent genere avec l'outil
ImageGen integre. Le prompt final utilise est :

> Use case: stylized-concept. Asset type: transparent emblem asset for a premium Napoleonic strategy game user interface. One single symmetrical French First Empire inspired imperial eagle emblem, wings spread horizontally, perched on a small central shield, framed by two delicate symmetrical laurel branches forming an open wreath. Entire object sculpted in aged warm brass and antique gold, subtly worn raised metal relief, soft dimensional lighting from upper left, exquisite restrained neoclassical craftsmanship. Straight-on view, centered isolated complete silhouette, square composition, generous transparent empty margin, truly transparent background. No words, no letters, no numbers, no crown, no flags, no pedestal, no scene, no background, no frame. Crisp elegant silhouette readable at 140 pixels, limited small detail, suitable for overlay on dark navy game UI.

Titres : Cinzel Bold, depuis https://github.com/google/fonts/tree/main/ofl/cinzel,
licence SIL OFL dans `assets/fonts/Cinzel-OFL.txt`.
`Cinzel-Bold.ttf` est une instance statique de `Cinzel.ttf` avec l'axe `wght=700`,
exportee avec fontTools.varLib.instancer.
Interface courante : Barlow SemiBold et Bold, licence dans `assets/fonts/OFL.txt`.
Les informations sont rendues a 18 unites minimum et les commandes a 18-20
(16 minimum si un bouton etroit l'exige). Le journal revient a la ligne au lieu
de reduire la police. Les couleurs secondaires conservent un contraste eleve.

Verification visuelle : `stratego --models-demo` (accueil), `--ui-hover-demo`
(survol du bouton principal), `--ui-help-demo` (manuel), `--smoke` (placement),
`--smoke --battle` (bataille), `--resign-demo` (fin de partie), `--quit-demo`
(plein ecran et fermeture). Les modes de capture se ferment automatiquement.
