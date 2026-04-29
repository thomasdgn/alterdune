# ALTERDUNE

ALTERDUNE est un mini RPG en C++ realise pour un projet de Programmation Orientee Objet.

Le jeu existe en deux versions :

- une version console, stable et jouable ;
- un frontend SFML bonus, plus visuel, branche sur la meme logique de jeu.

Le coeur du projet reste le moteur C++ : progression, combats, inventaire, bestiaire, mercy, monstres, regions et fins multiples.

## Fonctionnalites

- joueur avec nom, apparence, statistiques et inventaire ;
- chargement des objets depuis `data/items.csv` ;
- chargement des monstres depuis `data/monsters.csv` ;
- campagne organisee par regions ;
- systeme de cles regionales pour debloquer la suite ;
- combats avec `FIGHT`, `ACT`, `ITEM`, `MERCY` et fuite ;
- actions ACT qui augmentent ou diminuent la mercy ;
- bestiaire rempli au fil des rencontres ;
- recompenses, objets et ameliorations de progression ;
- trois fins selon les choix de combat :
  - `FIN GENOCIDAIRE` : tous les monstres sont tues ;
  - `FIN PACIFISTE` : tous les monstres sont epargnes avec mercy ;
  - `FIN NEUTRE` : le parcours melange au moins un kill et une mercy.

En console comme dans le frontend, le type de fin est affiche dans un bloc dedie.

## Organisation

```text
alterdune/
|-- assets/                 assets optionnels du frontend
|-- data/
|   |-- items.csv
|   `-- monsters.csv
|-- docs/
|   |-- DESIGN.md
|   |-- FRONTEND.md
|   `-- UI_MOCKUP.txt
|-- include/
|   |-- ActAction.h
|   |-- BestiaryEntry.h
|   |-- Entity.h
|   |-- FrontendApp.h
|   |-- FrontendViewModels.h
|   |-- Game.h
|   |-- Item.h
|   |-- Monster.h
|   `-- Player.h
|-- src/
|   |-- ActAction.cpp
|   |-- BestiaryEntry.cpp
|   |-- Entity.cpp
|   |-- FrontendApp.cpp
|   |-- frontend_main.cpp
|   |-- Game.cpp
|   |-- Item.cpp
|   |-- main.cpp
|   |-- Monster.cpp
|   `-- Player.cpp
|-- CMakeLists.txt
`-- README.md
```

## Architecture POO

Classes principales :

- `Entity` : base commune du joueur et des monstres ;
- `Player` : joueur, inventaire, statistiques de run ;
- `Monster` : classe abstraite des ennemis, avec mercy et actions ACT ;
- `NormalMonster`, `MiniBossMonster`, `BossMonster` : categories concretes de monstres ;
- `Item` : objets utilisables, principalement les soins ;
- `ActAction` : action ACT et impact sur la mercy ;
- `BestiaryEntry` : entree de bestiaire ;
- `Game` : moteur central, progression, combats, chargement CSV et fins ;
- `FrontendApp` : interface SFML bonus.

Notions utilisees :

- encapsulation ;
- heritage ;
- polymorphisme ;
- composition ;
- utilisation de collections STL (`vector`, `map`, `set`, `unique_ptr`) ;
- chargement de donnees externes en CSV.

Le diagramme UML complet est dans `docs/DESIGN.md`.

## Lancer la version deja compilee

Si le dossier `build/Release/` est present, les executables principaux sont :

```powershell
.\build\Release\alterdune_console.exe
.\build\Release\alterdune_frontend.exe
```

Le frontend peut aussi etre lance en mode test court :

```powershell
.\build\Release\alterdune_frontend.exe --ending-preview
```

Dans ce mode, le jeu affiche une fin apres 2 combats gagnes. Cela permet de verifier rapidement le rendu des fins sans refaire toute la campagne.

## Compiler avec CMake

Commande recommandee depuis la racine du projet :

```powershell
cmake -S . -B build
cmake --build build --config Release
```

CMake produit :

- `build/Release/alterdune_console.exe` ;
- `build/Release/alterdune_frontend.exe` si SFML 3 est detecte.

Le frontend utilise SFML 3 avec les modules :

- `graphics` ;
- `window` ;
- `system` ;
- `audio`.

## Compiler seulement la console avec g++

Cette commande compile uniquement la version console. Elle ne compile pas le frontend SFML.

```powershell
g++ -std=c++17 -Iinclude src/main.cpp src/Entity.cpp src/Player.cpp src/Monster.cpp src/Item.cpp src/ActAction.cpp src/BestiaryEntry.cpp src/Game.cpp -o alterdune.exe
```

Puis :

```powershell
.\alterdune.exe
```

## Tester les fins sans refaire toute la campagne

### Test direct

Ces commandes simulent directement une route complete et affichent le bloc final :

```powershell
.\build\Release\alterdune_console.exe --test-ending genocide fr
.\build\Release\alterdune_console.exe --test-ending pacifist fr
.\build\Release\alterdune_console.exe --test-ending neutral fr
```

Le dernier argument peut etre `fr` ou `en`.

### Test en 2 combats simules

Ces commandes affichent le rendu d'une fin apres seulement 2 combats simules :

```powershell
.\build\Release\alterdune_console.exe --test-ending-2 genocide fr
.\build\Release\alterdune_console.exe --test-ending-2 pacifist fr
.\build\Release\alterdune_console.exe --test-ending-2 neutral fr
```

Resultats attendus :

- `genocide` : 2 kills, bloc `FIN GENOCIDAIRE` ;
- `pacifist` : 2 mercy, bloc `FIN PACIFISTE` ;
- `neutral` : 1 kill et 1 mercy, bloc `FIN NEUTRE`.

### Preview jouable

Pour tester le vrai deroulement, mais avec une campagne raccourcie a 2 victoires :

```powershell
.\build\Release\alterdune_console.exe --ending-preview
.\build\Release\alterdune_frontend.exe --ending-preview
```

Dans ce mode :

- 2 monstres tues donnent `FIN GENOCIDAIRE` ;
- 2 monstres epargnes donnent `FIN PACIFISTE` ;
- 1 monstre tue et 1 monstre epargne donnent `FIN NEUTRE`.

## Controles du frontend

- menus : clic souris ou fleches + `Entree` ;
- selection de monstre : clic souris ou fleches + `Entree` ;
- combat : clic souris ou fleches gauche/droite + `Entree` ;
- sous-menus ACT / ITEM : clic souris ou fleches haut/bas + `Entree` ;
- retour ou fermeture d'un panneau : `Esc` ;
- plein ecran / fenetre : `F11`.

## Notes pour la soutenance

Points defendables :

- le modele console est separe en classes claires ;
- le frontend reutilise le moteur existant au lieu de dupliquer la logique ;
- les donnees principales sont externalisees dans les CSV ;
- les fins dependent directement des choix du joueur ;
- des modes de test permettent de verifier les fins rapidement ;
- le projet illustre bien heritage, polymorphisme, composition et encapsulation.

## Documents utiles

- `docs/DESIGN.md` : UML et description du modele console ;
- `docs/FRONTEND.md` : notes de frontend SFML ;
- `docs/ASSET_INTEGRATION.md` : integration des assets ;
- `assets/README.md` : organisation des ressources visuelles et audio.
