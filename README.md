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
|-- docs/
|   `-- DESIGN.md
|-- include/
|   |-- ActAction.h
|   |-- BestiaryEntry.h
|   |-- Entity.h
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

## Packages necessaires

Il n'y a aucun package externe a installer pour faire tourner ce projet.

Le projet utilise uniquement :

- le compilateur C++
- la bibliotheque standard C++

Exemples de compilateurs possibles :

- Visual Studio / MSVC
- g++

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
