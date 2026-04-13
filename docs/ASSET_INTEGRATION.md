# Asset Integration Plan

## Overview

The imported packs in `assets/import/` are usable for three different layers of the project:

- combat monsters and bosses
- hero visual variants
- combat effects and polish

The frontend is currently a 2D SFML renderer, so the most useful source files are:

- `PNG` frame sequences
- `PNG` spritesheets
- static `PNG` battlers
- optional `GIF` previews only as reference for choosing the correct animation set

## Pack Summary

### `free-rpg-monster-sprites-pixel-art`

Strong fit for direct combat monsters.

Notable folders:

- `PNG/demon`
- `PNG/dragon`
- `PNG/jinn_animation`
- `PNG/lizard`
- `PNG/medusa`
- `PNG/small_dragon`

Each folder contains useful frame sets such as:

- `Idle`
- `Attack`
- `Hurt`
- `Death`
- `Walk`

This pack is the fastest path to animated boss or enemy battlers.

### `Legacy Collection`

Much broader pack with several sub-libraries.

Most promising sections for ALTERDUNE:

- `Assets/TinyRPG/Characters/Battle Sprites`
- `Assets/Gothicvania/Characters`
- `Assets/Warped/Characters`
- `Assets/Explosions and Magic`

## Recommended Hero Sources

### Best immediate candidates

1. `Assets/Gothicvania/Characters/Bridge Heroine/Heroine base`
Good for a fantasy / action heroine with idle, run and player-attack animations.

2. `Assets/Warped/Characters/cyberpunk-detective`
Good for a sharper sci-fi / pulse-oriented hero look.

3. `Assets/TinyRPG/Characters/Top-Down-16-bit-fantasy/Characters pack 1/Guy`
Good fallback for portraits, menu identity or future top-down exploration mode.

### Suggested mapping to current hero styles

- `wanderer` -> `Bridge Heroine`
- `vanguard` -> `Bridge Heroine` with stronger attack stance, or `Guy` if we want a simpler classic RPG look
- `mystic` -> `cyberpunk-detective` for now, or later another arcane-themed character if we import one

## Recommended Monster And Boss Sources

### Best immediate candidates

1. `free-rpg-monster-sprites-pixel-art/PNG/dragon`
Very strong fit for a Fire boss. Includes dedicated fire attack frames.

2. `free-rpg-monster-sprites-pixel-art/PNG/demon`
Good fit for Shadow / Fire miniboss or boss.

3. `Assets/Warped/Characters/Grotto-escape-2-snake`
Good fit for Nature / Venom boss or miniboss.

4. `Assets/Gothicvania/Characters/Fire-Skull-Files`
Good fit for Fire or Shadow miniboss.

5. `Assets/TinyRPG/Characters/Battle Sprites/Monster Pack Files/static`
Fast source of static battlers:

- `demon.png`
- `Fire-haunt.png`
- `Jumping-Demon.png`
- `Treant.png`
- `vampire.png`
- `witch.png`

### Good thematic mapping to current game

- `QueenByte` -> `dragon` or `Fire-Skull` if we want a more elemental boss identity
- `Archivore` -> `demon`, `vampire`, or `witch` depending on whether we want ancient-shadow, cursed-royal or arcane-librarian vibes
- `BloomCobra` -> `Grotto-escape-2-snake`
- `Dustling` -> `vampire` or `enemy-ghost`
- `GlimmerMoth` -> can stay procedural for now, unless we import a flying spectral asset later

## Recommended Effect Sources

Very strong candidates from `Legacy Collection`:

- `Assets/Explosions and Magic/Grotto-escape-2-FX/sprites/fire-ball`
- `Assets/Explosions and Magic/Grotto-escape-2-FX/sprites/electro-shock`
- `Assets/Explosions and Magic/Grotto-escape-2-FX/sprites/slash-horizontal`
- `Assets/Explosions and Magic/Grotto-escape-2-FX/sprites/slash-circular`
- `Assets/Explosions and Magic/Hit/Sprites`
- `Assets/Explosions and Magic/Water splash/Sprites`

These are ideal for:

- `EMBER`
- `PULSE`
- `BLADE`
- `TIDE`
- hit flashes
- death / impact feedback

## Integration Priority

### Phase 1

- assign one real sprite to each hero appearance
- assign one strong visual boss for `QueenByte`
- assign one strong visual boss for `Archivore`
- add one animated snake-like monster for `BloomCobra`

## First Pass Already Integrated

These files are now copied into the live frontend asset folders and are already usable by the game:

- `player_wanderer` -> `Bridge Heroine / player-idle-1.png`
- `player_vanguard` -> `space-marine-lite / idle-gun1.png`
- `player_mystic` -> `cyberpunk-detective / Layer-1.png`
- `queenbyte` -> `dragon / Idle1.png`
- `archivore` -> `demon / Idle1.png`
- `bloomcobra` -> `Grotto-escape-2-snake / idle / _0000_Layer-1.png`

They have been copied into:

- `assets/sprites/`
- `assets/portraits/`

This means the frontend can already display real PNG art for these characters instead of relying only on procedural fallback silhouettes.

## First Animation Pass Already Integrated

The frontend now also reads frame sequences from `assets/animations/` for battle rendering.

Current clips prepared:

- `player_wanderer`: `idle`, `attack`
- `player_vanguard`: `idle`, `attack`
- `player_mystic`: `idle`
- `queenbyte`: `idle`, `attack`, `hurt`
- `archivore`: `idle`, `attack`, `hurt`
- `bloomcobra`: `idle`, `attack`, `hurt`

Current behavior in the frontend:

- idle loops are displayed automatically when a battle starts
- player `FIGHT` plays an attack animation when available
- the target monster plays a `hurt` animation when available

This is intentionally a lightweight first step. It gives visible motion now, while keeping the code simple enough to extend later with richer boss patterns and elemental FX.

### Phase 2

- add elemental FX overlays for `EMBER`, `PULSE`, `TIDE`, `BLADE`
- replace more procedural silhouettes with real battlers

### Phase 3

- animate idle / attack / hurt by reading frame folders or spritesheets
- make boss fights visually unique per element

## Practical Advice

- keep `assets/import/` as source material only
- copy or export cleaned files into:
  - `assets/sprites/`
  - `assets/portraits/`
  - `assets/ui/`
- keep original licenses and pack readmes untouched in `assets/import/`

## Best Next Step

The most efficient next implementation step is:

1. add frame-based animation support for `idle`, `attack`, `hurt`
2. promote `dragon` and `demon` into full elemental bosses with dedicated FX
3. assign at least one real sprite to the remaining base monsters
4. wire one imported FX family into `EMBER`, `PULSE`, `BLADE`, `TIDE`

That gives immediate visual payoff without forcing us to animate the whole roster at once.
