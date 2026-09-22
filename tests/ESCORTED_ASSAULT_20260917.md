# Attaque de drapeau avec démineur escorté

`src/ai_assault.h` ajoute une mission offensive unique au classement des coups
d'Expert+ Improved. L'officier doit dominer les grades numériques ennemis encore
en vie, selon les pertes publiques. La présence possible d'un espion reste
traitée par les protections du maréchal.

La cible est la case de drapeau la plus probable dans les croyances publiques.
La paire démineur/officier est choisie en tenant compte du trajet vers cette
cible et de la possibilité pour l'officier de rejoindre le démineur. Les lacs,
bombes amies et bloqueurs sont pris en compte. La mission est recalculée pour
réagir aux captures, révélations et urgences défensives.

Les deux pièces reçoivent des incitations complémentaires : progression du
démineur, rapprochement de l'escorte, pénalité si le démineur prend trop d'avance.
Lorsque le démineur ne peut pas progresser en sécurité à portée de l'escorte,
l'officier peut dégager le chemin devant lui. Un officier requis pour un rappel
défensif n'est pas affecté à l'assaut.
La mission ne récompense pas une avancée dangereuse du démineur, même si une
recapture par l'escorte serait possible : cela perdrait quand même le démineur.
Elle ne crédite pas un combat incertain comme un déplacement réussi. Une
menace détectée contre notre drapeau suspend la mission.

Le test `escorted_flag_assault` couvre :

- constitution de la paire et progression de ses deux membres ;
- absence de mission sans supériorité numérique des grades ;
- priorité à la défense contre un démineur envahisseur ;
- refus de récompenser un démineur exposé malgré une escorte adjacente ;
- indépendance du choix et de l'état aléatoire après permutation d'un drapeau
  et d'une bombe cachés ;
- quatre parties complètes sur une position réduite avec défense mobile et
  écran de bombes, contre des coups adverses légaux pseudo-aléatoires.

Ces scénarios vérifient le fonctionnement de la mission et ses priorités.
Ils ne constituent pas un benchmark contre un humain ni une garantie de gain.
Les primes sont bornées ; la recherche tactique peut retenir un autre plan.

Résultats des quatre scénarios réduits : captures de drapeau en 39, 35 et
35 demi-coups, puis victoire par immobilisation en 21 demi-coups. Les trois
captures comprennent le déminage d'une protection du drapeau. Le replay
`human_win_161.jsonl` contrôle aussi que le regroupement de l'escorte ne bloque
pas la progression du général lorsque le démineur ne peut pas avancer avec lui.

Validation finale : compilation Release, 14 tests CTest réussis (142,08 s),
et lancement graphique `--smoke --battle` terminé avec le code 0.
Le binaire installé dans `build/stratego.exe` est identique au binaire testé
(SHA-256 `F69C43525EAD2FECAD27750CAE6328936C9DC8D580793835E7385F718B697F1E`).
L'ancien exécutable est conservé dans
`reports/ai_safety_20260917/stratego_before_assault.exe`.
