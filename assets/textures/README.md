# Materiaux du champ de bataille

Trois textures de couleur creees avec ImageGen integre, conservees dans ce dossier :

- `battlefield-meadow.png` : herbe, mousse et terre pietinee.
- `lake-shore.png` : terre humide, galets et sediment des rives.
- `campaign-walnut.png` : noyer verni du cadre du plateau.

Le shader `assets/shaders/battlefield.fs` eclaire les materiaux en fonction de la camera, utilise les variations de couleur pour un micro-relief de normale, melange un chemin de terre au gazon et distingue les reflets de l'eau du fini du bois. Les images ne sont pas des captures pre-eclairees du plateau. Les reflets utilisent une vraie carte HDR de studio (voir `assets/lighting/README.md`). L'eau dispose d'un maillage dense deplace par trois couches de bruit anime avec deformation du domaine, courants et echelles distincts, de normales analytiques synchronisees, d'absorption coloree selon une profondeur decorative et de caustiques procedurales. Le bump du terrain et du bois est derive des textures de couleur ; il ne s'agit pas de scans de hauteur separes. Les moulures dorees, les biseaux et les rosaces sont modelises en 3D. Aucune table sous le plateau. Les textures sont filtrees avec mipmaps et anisotropie.

`src/battlefield.h` construit deux lacs differents, leurs rives inclinees, leurs raccords au terrain, leurs galets et quelques roseaux. Les huit cases interdites restent identiques aux regles. L'ecran de placement utilise les memes materiaux et geometries vus du dessus. Le relief des rives est decoratif ; les pieces et les coordonnees des cases ne changent pas.

Verification visuelle : `stratego --smoke --battle` (vue jouable), `stratego --terrain-demo` (detail des lacs), `stratego --showcase-demo` (cadre en perspective), `stratego --smoke` (placement).

## Prompts finaux

### Prairie

Use case: stylized-concept. Asset type: seamless physically based base-color terrain texture for a premium Napoleonic battlefield miniature. Create one square tileable 2048x2048 texture of naturally trampled European meadow: fine olive green grass tufts, moss, exposed earthy umber soil, subtle dry ochre grass, tiny sparse pale gravel, irregular soft patches of vegetation. Orthographic directly overhead, material scan quality, realistic micro detail, neutral flat diffuse illumination with NO cast shadows, no baked directional lighting, no perspective. Entire image uniformly filled edge to edge with natural terrain, seamless edges in both directions, no single focal point. Grass scale fine enough for a tabletop strategic battlefield. Earth occupies about 35%, low saturated olive and sage vegetation 65%. No objects, no trees, no buildings, no paths, no water, no text, no grid, no borders, no UI.

### Rives

Use case: stylized-concept. Asset type: seamless square PBR base-color texture for a realistic lake shoreline in a Napoleonic tabletop battlefield. Directly overhead orthographic material scan, uniformly flat neutral lighting, no cast shadows. Small worn limestone and slate pebbles embedded in damp dark earth and sandy sediment, scattered subtle moss in crevices. Low contrast, charcoal grey, warm stone grey, muted brown and a little olive. Finely detailed, physically believable wet shoreline earth, stones mostly 1 to 5 centimeters in appearance, not a pile of boulders. Seamlessly tileable on every edge, consistent scale, edge to edge full material only. No water surface, no large rocks, no objects, no foliage clumps, no border, no letters, no grid, no UI. 2048x2048.

### Noyer

Use case: stylized-concept. Asset type: seamless square PBR albedo material for a premium antique Napoleonic military campaign gaming table. Rich dark French walnut wood, elegant close parallel natural flowing grain, subtle pores, restrained lightly worn polished finish. Straight wood grain running vertically across image, deep chocolate brown, charcoal brown and muted warm amber streaks. Real scanned wood veneer appearance. Orthographic directly overhead, perfectly flat diffuse neutral lighting, no highlights, no cast shadows, no perspective, no planks or seams. Seamlessly tileable in both directions. Full material edge to edge. No furniture, no brass decorations, no objects, no text, no border, no UI. 2048x2048.
