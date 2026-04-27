# Assets Folder

Ce dossier est pret a recevoir de vrais assets pour le frontend SFML.

Organisation recommandee :

```text
assets/
|-- backgrounds/
|-- fonts/
|-- animations/
|-- portraits/
|-- sprites/
|-- ui/
|-- sfx/
`-- music/
```

## Backgrounds reconnus

Le frontend cherche directement :

- `assets/backgrounds/sunken_mire.png`
- `assets/backgrounds/glass_dunes.png`
- `assets/backgrounds/signal_wastes.png`
- `assets/backgrounds/ancient_vault.png`

Ces images servent de fond principal selon la region ou l'ecran.

## Portraits reconnus

Tous les `.png` dans `assets/portraits/` sont charges automatiquement.

Exemples utiles :

- `assets/portraits/player_default.png`
- `assets/portraits/player_wanderer.png`
- `assets/portraits/player_vanguard.png`
- `assets/portraits/player_mystic.png`
- `assets/portraits/froggit.png`
- `assets/portraits/mimicbox.png`
- `assets/portraits/queenbyte.png`
- `assets/portraits/archivore.png`

## Sprites reconnus

Tous les `.png` dans `assets/sprites/` sont charges automatiquement.

Exemples utiles :

- `assets/sprites/player_default.png`
- `assets/sprites/player_wanderer.png`
- `assets/sprites/player_vanguard.png`
- `assets/sprites/player_mystic.png`
- `assets/sprites/froggit.png`
- `assets/sprites/dustling.png`
- `assets/sprites/queenbyte.png`
- `assets/sprites/archivore.png`

Si un sprite existe pour le heros ou un monstre, il remplace la silhouette dessinee en code.

Formats conseilles pour le frontend actuel :

- portraits statiques en `PNG` avec fond transparent
- sprites plein corps en `PNG`
- spritesheets `PNG` si vous voulez qu'on ajoute ensuite une vraie animation par frames

Le frontend actuel est un renderer 2D SFML. Donc les meilleurs assets a importer maintenant sont des sprites 2D, portraits, battlers, backgrounds et UI. Les modeles 3D (`FBX`, `GLTF`, `OBJ`) ne sont pas affiches directement dans cette version.

## Animations reconnues

Le frontend peut maintenant charger des sequences de frames dans :

- `assets/animations/<slug>/idle/`
- `assets/animations/<slug>/attack/`
- `assets/animations/<slug>/hurt/`

Exemples actuellement branches :

- `assets/animations/player_wanderer/idle/`
- `assets/animations/player_wanderer/attack/`
- `assets/animations/player_vanguard/idle/`
- `assets/animations/player_vanguard/attack/`
- `assets/animations/player_mystic/idle/`
- `assets/animations/queenbyte/idle/attack/hurt/`
- `assets/animations/archivore/idle/attack/hurt/`
- `assets/animations/bloomcobra/idle/attack/hurt/`

Si une animation existe, elle est utilisee en priorite pendant le combat avant le sprite statique.

## Sons reconnus

Le frontend essaie aussi de charger ces effets sonores optionnels :

- `assets/sfx/ui_move.wav`
- `assets/sfx/ui_confirm.wav`
- `assets/sfx/battle_hit.wav`
- `assets/sfx/battle_act.wav`
- `assets/sfx/battle_item.wav`
- `assets/sfx/battle_mercy.wav`
- `assets/sfx/battle_cast.wav`
- `assets/sfx/battle_heal.wav`
- `assets/sfx/battle_victory.wav`
- `assets/sfx/battle_defeat.wav`
- `assets/sfx/battle_blade.wav`
- `assets/sfx/battle_pulse.wav`
- `assets/sfx/battle_dust.wav`
- `assets/sfx/battle_ember.wav`
- `assets/sfx/battle_tide.wav`
- `assets/sfx/battle_fire.wav`
- `assets/sfx/battle_water.wav`
- `assets/sfx/battle_shadow.wav`
- `assets/sfx/battle_storm.wav`
- `assets/sfx/battle_metal.wav`
- `assets/sfx/battle_nature.wav`
- `assets/sfx/battle_light.wav`
- `assets/sfx/battle_void.wav`
- `assets/sfx/battle_arcane.wav`
- `assets/sfx/battle_buff.wav`
- `assets/sfx/battle_debuff.wav`
- `assets/sfx/battle_guard.wav`
- `assets/sfx/battle_spare.wav`
- `assets/sfx/monster_queenbyte.wav`
- `assets/sfx/monster_archivore.wav`
- `assets/sfx/monster_cinderhex.wav`
- `assets/sfx/monster_tidewarden.wav`
- `assets/sfx/monster_solaraith.wav`
- `assets/sfx/monster_nullsaint.wav`

Le projet contient maintenant un premier pack de sons retro placeholder deja genere dans `assets/sfx/`.

Le frontend peut s'en servir pour differencier :

- les styles d'attaque du heros
- les ripostes elementaires des monstres
- les signatures sonores des monstres importants
- les soins, buffs, debuffs et spare

Si certains fichiers n'existent pas, le jeu reste jouable sans son.

## Musiques reconnues

Le frontend peut maintenant charger des musiques optionnelles pour le menu, les regions et certains combats de boss :

- `assets/music/menu_theme.wav`
- `assets/music/menu_theme.ogg`
- `assets/music/sunken_mire.wav`
- `assets/music/sunken_mire.ogg`
- `assets/music/glass_dunes.wav`
- `assets/music/glass_dunes.ogg`
- `assets/music/signal_wastes.wav`
- `assets/music/signal_wastes.ogg`
- `assets/music/ancient_vault.wav`
- `assets/music/ancient_vault.ogg`
- `assets/music/battle_generic.wav`
- `assets/music/battle_generic.ogg`
- `assets/music/boss_fire.wav`
- `assets/music/boss_fire.ogg`
- `assets/music/boss_water.wav`
- `assets/music/boss_water.ogg`
- `assets/music/boss_shadow.wav`
- `assets/music/boss_shadow.ogg`
- `assets/music/boss_light.wav`
- `assets/music/boss_light.ogg`
- `assets/music/boss_nature.wav`
- `assets/music/boss_nature.ogg`
- `assets/music/boss_metal.wav`
- `assets/music/boss_metal.ogg`
- `assets/music/boss_storm.wav`
- `assets/music/boss_storm.ogg`
- `assets/music/boss_void.wav`
- `assets/music/boss_void.ogg`
- `assets/music/boss_queenbyte.ogg`
- `assets/music/boss_archivore.ogg`
- `assets/music/boss_cinderhex.ogg`
- `assets/music/boss_tidewarden.ogg`
- `assets/music/boss_solaraith.ogg`
- `assets/music/boss_nullsaint.ogg`

Le projet contient maintenant un premier pack de pistes placeholder en `wav` pour tester tout de suite :

- menu
- ambiance de region
- combat generique
- themes de boss elementaires

Le frontend privilegie les fichiers `ogg` quand ils existent, puis retombe sur `wav`.

Si vous remplacez ces fichiers par de vraies musiques, gardez simplement les memes noms.

## Convention de nommage

Le frontend convertit automatiquement les noms en minuscules avec underscores.

Exemples :

- `QueenByte` devient `queenbyte`
- `Signal Wastes` devient `signal_wastes`
- `Player Vanguard` devient `player_vanguard`

## Fallback actuel

- si un background existe : il est affiche
- si un portrait existe : il remplace le portrait procedural
- si un sprite existe : il remplace la silhouette de combat
- si un son existe : il est joue dans le frontend
- sinon : le frontend garde son fallback retro dessine en code

## Bon point de depart pour vos assets

Pour une premiere passe propre, vous pouvez deja fournir :

- un portrait et un sprite par apparence de heros
- un portrait et un sprite par boss
- un background par region
- 4 a 6 petits sons retro tres courts
