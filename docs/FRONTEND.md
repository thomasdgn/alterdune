# ALTERDUNE - Frontend Preparation

## Goal

Le frontend bonus vise une presentation plus visuelle, inspirée des anciens jeux Pokemon :

- scene de combat lisible et pixel-art
- gros boutons d'action en bas d'ecran
- zone de texte pour les dialogues et retours ACT
- sprites fixes ou legerement animes
- progression par regions avec une ambiance visuelle differente

L'idee n'est pas de remplacer tout de suite la logique du jeu, mais de preparer une interface qui pourra lire l'etat du moteur actuel.

## Recommended tech choice

Pour ce projet C++ etudiant, la meilleure option reste **SFML** :

- simple a installer sur Windows / Visual Studio
- bonne gestion des fenetres 2D
- sprites, texte, sons, clavier / souris
- suffisante pour un rendu propre sans sur-complexifier le projet

## Visual direction

Le style vise :

- resolution retro type `1280x720` dans une fenetre redimensionnable
- gros pixels assumés
- palettes par region
- interfaces encadrees, tres lisibles
- personnages / monstres avec silhouettes fortes

### Region palettes

- `Sunken Mire` : vert marecage, brun humide, jaune pale
- `Glass Dunes` : sable clair, cyan, blanc chaud
- `Signal Wastes` : rouge poussiere, orange neon, gris sombre
- `Ancient Vault` : bronze, ivoire, bleu nuit

## Combat screen target

```text
 ------------------------------------------------------------
| Region background                                           |
|                                                            |
|                           Monster sprite                    |
|                           Name / Category / Mercy           |
|                                                            |
| Player sprite                                               |
| Name / HP / bonuses                                         |
|------------------------------------------------------------|
| Dialogue log / ACT reaction / battle hint                   |
|------------------------------------------------------------|
|  FIGHT   |   ACT   |   ITEM   |   MERCY                    |
 ------------------------------------------------------------
```

## Planned screens

### 1. Title screen

- logo ALTERDUNE
- bouton `Start`
- bouton `Quit`

### 2. Main menu

- resume de progression
- regions debloquees
- boutons :
  - `Bestiary`
  - `Start Battle`
  - `Player Stats`
  - `Items`
  - `Quit`

### 3. Monster selection screen

- cartes par monstre
- affichage du nom, region, categorie, menace
- cartes verrouillees pour les contrées lointaines non debloquees

### 4. Battle screen

- sprite joueur
- sprite monstre
- HP joueur
- HP monstre
- barre Mercy
- texte d'ambiance et hint
- boutons FIGHT / ACT / ITEM / MERCY

### 5. Bestiary screen

- cartes ou panneaux de monstres debloques
- nom
- categorie
- statistiques
- resultat du dernier combat
- petite description

## Data already available in the game

Le moteur actuel fournit deja presque tout ce qu'il faut pour une UI :

- nom du joueur
- HP / max HP
- inventaire
- progression : victories / kills / spares
- lands debloquees
- nom du monstre
- categorie
- mercy
- hint, mood, intro, texte de reaction
- statistiques
- bestiaire enrichi

Le fichier [`include/FrontendViewModels.h`](../include/FrontendViewModels.h) sert maintenant de contrat simple pour une future couche d'affichage.

## Suggested integration plan

### Step 1

Creer un petit module SFML se contentant d'afficher :

- une fenetre
- un fond
- des boutons
- du texte

Sans encore connecter la logique du jeu.

### Step 2

Construire un `BattleViewData` a partir de l'etat courant de `Game`, puis afficher :

- joueur
- monstre
- HP
- mercy
- boutons

### Step 3

Connecter les clics souris / clavier aux memes actions que la version console :

- FIGHT
- ACT
- ITEM
- MERCY

### Step 4

Ajouter les assets :

- sprites des monstres
- portraits du joueur
- backgrounds par region
- police pixel
- sons de validation

## Asset folder proposal

```text
assets/
|-- backgrounds/
|-- fonts/
|-- portraits/
|-- sprites/
|-- ui/
`-- sfx/
```

## First frontend MVP

Le premier frontend bonus raisonnable pourrait se limiter a :

- ecran menu principal
- ecran de selection des monstres
- ecran de combat
- boutons cliquables
- sprites statiques
- fond par region

Cela suffirait deja a produire un bonus visuel fort pendant la soutenance.

## Current implementation status

Une premiere version de ce frontend est maintenant preparee dans le projet :

- `FrontendApp` ouvre une fenetre SFML
- un menu principal retro est rendu
- un ecran de combat statique lit les donnees exposees par `Game`
- les regions changent la couleur d'ambiance
- les actions `FIGHT / ACT / ITEM / MERCY` sont affichees comme vrais boutons d'interface

Ce n'est pas encore un jeu graphique complet, mais c'est deja une base concrete pour la suite.

## Recent improvements

Depuis cette base initiale, le frontend a aussi gagne :

- un choix de langue `FR / EN` au lancement
- un choix d'apparence du heros avec affichage visuel direct
- une selection de monstres plus lisible avec :
  - sprite ou silhouette
  - type elementaire
  - region
  - menace
  - physique
  - description
- une premiere couche d'animations par frames pour certains heros et monstres
- des fonds de combat plus thematiques selon :
  - la region
  - le type elementaire du monstre
  - sa categorie

Les boss recoivent maintenant un traitement visuel plus dramatique que les monstres normaux.

La passe de polish la plus recente ajoute aussi :

- des profils de rendu par hero et par monstre pour mieux normaliser :
  - l'echelle
  - le cadrage
  - l'ancrage au sol
- des cadres et portraits accentues par type elementaire pour garder les informations lisibles sans que le sprite les cache
- des arenes de boss plus marquees selon le type et le boss rencontre
- des transitions et FX un peu plus spectaculaires pour rendre le combat moins statique
- un mode plein ecran avec `F11`

## Why this is a good next step

- le moteur console reste la base fiable
- le frontend devient une couche d'affichage au-dessus
- on peut avancer progressivement
- la version soutenance reste stable meme si le frontend bonus n'est pas fini
