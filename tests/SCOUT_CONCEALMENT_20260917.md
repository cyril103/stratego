# Discretion des eclaireurs — 17 septembre 2026

Expert+ Improved comptabilise maintenant la divulgation de son propre eclaireur dans le score initial et final des coups. Un premier long deplacement sans combat coute jusqu'a 20 points heuristiques, ponderes par la proportion de grades mobiles non eclaireurs encore inconnus de l'adversaire. La composition de l'armee, les pertes et les propres pieces deja revelees suffisent : aucune identite adverse cachee n'est consultee.

Un pas simple, une attaque, un eclaireur deja revele ou une armee dont les seuls grades mobiles inconnus sont des eclaireurs ne recoivent pas cette penalite. La penalite reste finie pour permettre une defense ou une fuite urgente.

Rejeu human_win_293, avant le demi-coup 210, graine 519 : avant correction D9-B9 ; apres correction D9-C9. Il s'agit d'un exemple de decision, pas d'une garantie sur toutes les graines : les gains tactiques estimes dans les mondes simules peuvent encore justifier un long trajet.

Tests ajoutes : cout de revelation selon la connaissance publique ; absence de cout sur les attaques ; capture lointaine du drapeau ; blocage obligatoire d'une ligne d'eclaireur menacant notre drapeau, par un long deplacement sans attaque ; regression D9-C9 dans la partie enregistree.

Le controle de lancement --smoke --battle termine avec le code 0.
Validation finale : 12 tests CTest sur 12 reussis (114,56 s). Executable installe dans build/stratego.exe, empreinte SHA-256 identique au binaire teste. Version precedente conservee dans reports/ai_safety_20260917/stratego_before_scout.exe.
