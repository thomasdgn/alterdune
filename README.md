# ALTERDUNE - TD11-12

## Presentation

ALTERDUNE est un mini-jeu console RPG en C++ realise pour un projet de Programmation Orientee Objet.

Cette version correspond uniquement au mini-suivi TD11-12 :

- architecture propre
- code compilable
- classes bien separees
- chargement CSV fonctionnel
- menu principal fonctionnel
- debut de combat simple

Le projet n'essaie pas encore de livrer la version finale complete.

## Organisation du dossier

```text
alterdune/
|-- data/
|   |-- items.csv
|   `-- monsters.csv
|-- assets/
|   `-- README.md
|-- docs/
|   `-- DESIGN.md
|-- include/
|   |-- ActAction.h
|   |-- BestiaryEntry.h
|   |-- Entity.h
|   |-- FrontendViewModels.h
|   |-- Game.h
|   |-- Item.h
|   |-- Monster.h
|   `-- Player.h
|-- src/
|   |-- ActAction.cpp
|   |-- BestiaryEntry.cpp
|   |-- Entity.cpp
|   |-- Game.cpp
|   |-- Item.cpp
|   |-- Monster.cpp
|   |-- Player.cpp
|   `-- main.cpp
|-- .gitignore
|-- README.md
```

### Role de chaque dossier

- `src/` : tous les fichiers source `.cpp`
- `include/` : tous les headers `.h`
- `data/` : les fichiers CSV du projet
- `docs/` : les documents de conception

Cette organisation est simple, classique et propre pour un projet C++ etudiant.

## Ce qui est implemente

- saisie du nom du joueur
- chargement de `data/items.csv`
- chargement de `data/monsters.csv`
- creation du joueur
- affichage d'un resume au demarrage
- menu principal
- affichage des stats du joueur
- affichage et utilisation simple des items
- affichage du bestiaire
- base de combat minimaliste
- catalogue ACT code en dur

## Architecture POO

### Classes principales

- `Entity` : classe de base commune a `Player` et `Monster`
- `Player` : represente le joueur et son inventaire
- `Monster` : represente un monstre, sa categorie et sa mercy
- `Item` : represente un objet de soin
- `ActAction` : represente une action ACT
- `BestiaryEntry` : represente une entree du bestiaire
- `Game` : pilote l'application

### Notions de POO utilisees

- encapsulation : chaque classe gere son propre etat
- heritage : `Player` et `Monster` derivent de `Entity`
- polymorphisme : `printStatus()` et `getEntityType()` sont virtuelles
- composition : `Player` contient des `Item`, `Game` contient les objets centraux

## UML textuel rapide

```text
Entity
|-- Player
`-- Monster

Game
|-- Player
|-- vector<Monster>
|-- vector<BestiaryEntry>
`-- map<string, ActAction>

Player
`-- vector<Item>
```

Le detail se trouve dans `docs/DESIGN.md`.

## Comment lancer le projet

## Option 1 - Visual Studio

1. Ouvrir Visual Studio
2. Creer un projet `Console App` en C++
3. Ajouter tous les fichiers de `src/`
4. Ajouter tous les fichiers de `include/`
5. Verifier que le dossier de travail contient bien le dossier `data/`
6. Compiler puis executer

### Point important pour Visual Studio

Le programme lit les fichiers :

- `data/items.csv`
- `data/monsters.csv`

Donc Visual Studio doit lancer l'executable depuis la racine du projet, ou bien il faut copier le dossier `data/` dans le dossier d'execution.

## Option 2 - g++

Depuis la racine du projet :

```bash
g++ -std=c++17 -Iinclude src/main.cpp src/Entity.cpp src/Player.cpp src/Monster.cpp src/Item.cpp src/ActAction.cpp src/BestiaryEntry.cpp src/Game.cpp -o alterdune
```

Puis lancer :

```bash
./alterdune
```

Sous Windows PowerShell, si besoin :

```powershell
.\alterdune.exe
```

## Option 3 - Frontend SFML bonus

Le projet contient maintenant un premier frontend bonus avec :

- une fenetre SFML
- un choix de langue au demarrage du frontend (`English` / `Francais`)
- un menu principal retro
- un choix d'apparence du heros
- une selection de monstres
- un ecran de combat interactif branche sur les vraies donnees du jeu
- des types elementaires de monstres et d'attaques
- des assets optionnels et des effets audio optionnels

Fichiers concernes :

- `include/FrontendApp.h`
- `src/FrontendApp.cpp`
- `src/frontend_main.cpp`
- `include/FrontendViewModels.h`
- `docs/FRONTEND.md`

### Build recommande avec CMake

Si SFML est installe sur la machine :

```bash
cmake -S . -B build
cmake --build build --config Release
```

Le projet cree :

- `alterdune_console`
- `alterdune_frontend` si SFML est detecte

### Lancer le frontend

Sous Windows avec CMake :

```powershell
cmake -S . -B build
cmake --build build --config Release
.\build\Release\alterdune_frontend.exe
```

Si le generateur n'utilise pas le dossier `Release`, le binaire peut aussi se trouver ici :

```powershell
.\build\alterdune_frontend.exe
```

### Controles du frontend

- menu : clic souris ou fleches `Haut/Bas` + `Entree`
- bestiary : clic souris ou fleches `Haut/Bas`
- items : clic souris ou fleches `Haut/Bas`
- selection de monstre : clic souris ou fleches `Haut/Bas` + `Entree`
- combat : clic souris ou fleches `Gauche/Droite` + `Entree`
- sous-menus ACT / ITEM : clic souris ou fleches `Haut/Bas` + `Entree`
- retour / fermeture d'un panneau : `Esc`
- plein ecran / fenetre : `F11`

### Notes SFML

Le frontend utilise SFML 3 avec les modules :

- `graphics`
- `window`
- `system`
- `audio`

Le code essaie de charger une police locale dans `assets/fonts/`, puis bascule sur des polices Windows classiques si besoin.

Le frontend peut aussi charger des assets optionnels depuis `assets/` :

- fonds de regions
- portraits du heros et des monstres
- sprites
- panneaux UI
- effets sonores courts

S'ils ne sont pas encore presents, des placeholders retro sont utilises automatiquement.

## Packages necessaires

Il n'y a aucun package externe a installer pour faire tourner ce projet.

Le projet utilise uniquement :

- le compilateur C++
- la bibliotheque standard C++

Exemples de compilateurs possibles :

- Visual Studio / MSVC
- g++

Pour le frontend bonus, il faut aussi :

- SFML 3

Le projet inclut deja un premier pack de sons retro placeholder dans `assets/sfx/`, donc aucun telechargement audio supplementaire n'est necessaire pour tester cette partie.

## Preparation du frontend bonus

Le projet commence maintenant a preparer un futur frontend style vieux Pokemon.

Fichiers ajoutes pour cela :

- `docs/FRONTEND.md` : direction artistique et plan d'integration
- `docs/UI_MOCKUP.txt` : maquette textuelle d'un ecran de combat
- `include/FrontendViewModels.h` : structures de donnees prevues pour une future UI
- `assets/README.md` : organisation recommandee des assets

Le moteur du jeu reste console pour l'instant, mais la structure est pensee pour brancher plus tard une interface graphique, idealement avec SFML.

## Choix defendables a l'oral

- le projet est volontairement limite au perimetre TD11-12
- l'architecture est deja claire
- le code est deja separé en classes avec `.h` / `.cpp`
- les donnees sont deja externalisees en CSV
- le polymorphisme est present sans rendre le code trop complexe
- la structure du dossier est propre et lisible

## Ce qu'il reste pour la suite

- enrichir le combat
- ajouter la progression complete
- gerer les fins multiples
- enrichir le bestiaire et les interactions ACT
