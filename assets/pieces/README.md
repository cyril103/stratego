# Illustrations des pieces

`napoleonic-ranks.png` est un atlas original genere avec l'outil ImageGen integre.
Les douze panneaux representent, de gauche a droite puis de haut en bas :
drapeau, espion, eclaireur, demineur, sergent, lieutenant, capitaine, commandant,
colonel, general, marechal et bombe. Direction artistique : portraits peints
du Premier Empire, fond charbon, uniformes sombres et accents dores.

`src/piece_art.h` extrait les panneaux en conservant leurs proportions et
compose les douze textures au chargement. Les reperes D, 1 a 10 et B sont
composes avec la police Cinzel du jeu, independamment des images generees.
La plaque UV est appliquee sur l'avant du modele, avec un cadre metallique.
Les deux camps utilisent les memes illustrations et restent distingues par
leurs corps bleus ou rouges. Aucune illustration de grade cache n'est soumise
au rendu, meme si la camera passe derriere une piece ennemie.

Les textures sont reutilisees pour le placement, les pieces capturees et les
animations de combat. Aucun appel reseau n'est necessaire dans le jeu.

Le prompt original est conserve dans `generation-prompt.txt`.
