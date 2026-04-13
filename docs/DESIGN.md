# ALTERDUNE - UML Textuel

## 1. Objectif du document

Ce document resume le diagramme UML de la version actuelle du projet ALTERDUNE.

Le but est de montrer clairement :

- les classes principales
- leurs attributs
- leurs methodes essentielles
- les relations entre elles
- les notions de POO mobilisees

Il s'agit d'un UML textuel volontairement simple, lisible et defendable a l'oral.

## 2. Vue globale

```text
Entity
|-- Player
`-- Monster (abstraite)

Monster
|-- NormalMonster
|-- MiniBossMonster
`-- BossMonster

Player
`-- vector<Item>

Monster
`-- vector<string> actIds

Game
|-- Player
|-- vector<unique_ptr<Monster>> monsterCatalog
|-- vector<BestiaryEntry> bestiary
`-- map<string, ActAction> actCatalog
```

## 3. Detail des classes

### 3.1 `Entity`

Classe de base commune aux entites de combat.

```text
Entity
--------------------------------------------
- m_name : string
- m_hp : int
- m_maxHp : int
- m_atk : int
- m_def : int
--------------------------------------------
+ Entity(name = "", maxHp = 0, atk = 0, def = 0)
+ virtual ~Entity()
+ virtual getEntityType() : string
+ virtual printStatus(os) : void
+ takeDamage(rawDamage) : void
+ heal(amount) : void
+ isAlive() : bool
+ getName() : const string&
+ getHp() : int
+ getMaxHp() : int
+ getAtk() : int
+ getDef() : int
+ setName(name) : void
+ setHp(hp) : void
+ setMaxHp(maxHp) : void
+ setAtk(atk) : void
+ setDef(def) : void
```

Role :

- factoriser les attributs communs au joueur et aux monstres
- centraliser la logique de points de vie
- servir de base a l'heritage et au polymorphisme

### 3.2 `Player`

Classe derivee de `Entity`.

```text
Player : Entity
--------------------------------------------
- m_inventory : vector<Item>
- m_kills : int
- m_spares : int
- m_victories : int
--------------------------------------------
+ Player(name = "Traveler", maxHp = 100, atk = 12, def = 3)
+ getEntityType() : string
+ printStatus(os) : void
+ addItem(item) : void
+ getInventory() : vector<Item>&
+ getInventory() const : const vector<Item>&
+ displayStats(os) : void
+ displayItems(os) : void
+ useItem(index, os) : bool
+ getKills() : int
+ getSpares() : int
+ getVictories() : int
+ recordKill() : void
+ recordSpare() : void
+ recordVictory() : void
```

Role :

- representer le joueur
- gerer l'inventaire
- stocker les statistiques de progression

### 3.3 `Monster`

Classe abstraite derivee de `Entity`.

```text
Monster : Entity <<abstract>>
--------------------------------------------
- m_mercy : int
- m_mercyGoal : int
- m_actIds : vector<string>
--------------------------------------------
+ Monster(name = "", maxHp = 0, atk = 0, def = 0, mercyGoal = 100, actIds = {})
+ getEntityType() : string
+ printStatus(os) : void
+ getCategory() : MonsterCategory = 0
+ getMaxActChoices() : int = 0
+ clone() : unique_ptr<Monster> = 0
+ getMercy() : int
+ getMercyGoal() : int
+ getActIds() : const vector<string>&
+ getAvailableActIds() : vector<string>
+ addMercy(amount) : void
+ isSpareable() : bool
+ categoryFromString(value) : MonsterCategory
+ categoryToString(category) : string
```

Role :

- representer un ennemi du jeu
- gerer la progression vers un spare via la mercy
- memoriser quelles actions ACT sont autorisees
- servir de base polymorphe aux differentes categories de monstres

### 3.4 `NormalMonster`

```text
NormalMonster : Monster
--------------------------------------------
+ NormalMonster(name = "", maxHp = 0, atk = 0, def = 0, mercyGoal = 100, actIds = {})
+ getCategory() : MonsterCategory
+ getMaxActChoices() : int
+ clone() : unique_ptr<Monster>
```

Role :

- representer un monstre normal
- limiter le nombre d'actions ACT disponibles a 2

### 3.5 `MiniBossMonster`

```text
MiniBossMonster : Monster
--------------------------------------------
+ MiniBossMonster(name = "", maxHp = 0, atk = 0, def = 0, mercyGoal = 100, actIds = {})
+ getCategory() : MonsterCategory
+ getMaxActChoices() : int
+ clone() : unique_ptr<Monster>
```

Role :

- representer un mini-boss
- limiter le nombre d'actions ACT disponibles a 3

### 3.6 `BossMonster`

```text
BossMonster : Monster
--------------------------------------------
+ BossMonster(name = "", maxHp = 0, atk = 0, def = 0, mercyGoal = 100, actIds = {})
+ getCategory() : MonsterCategory
+ getMaxActChoices() : int
+ clone() : unique_ptr<Monster>
```

Role :

- representer un boss
- limiter le nombre d'actions ACT disponibles a 4

### 3.7 `Item`

```text
Item
--------------------------------------------
- m_name : string
- m_type : ItemType
- m_value : int
- m_quantity : int
--------------------------------------------
+ Item(name = "", type = UNKNOWN, value = 0, quantity = 0)
+ getName() : const string&
+ getType() : ItemType
+ getValue() : int
+ getQuantity() : int
+ setQuantity(quantity) : void
+ addQuantity(amount) : void
+ consumeOne() : bool
+ itemTypeFromString(value) : ItemType
+ itemTypeToString(type) : string
```

Role :

- representer un objet de l'inventaire
- gerer son type, sa valeur et sa quantite

### 3.8 `ActAction`

```text
ActAction
--------------------------------------------
- m_id : string
- m_text : string
- m_mercyImpact : int
--------------------------------------------
+ ActAction(id = "", text = "", mercyImpact = 0)
+ getId() : const string&
+ getText() : const string&
+ getMercyImpact() : int
```

Role :

- representer une action du catalogue ACT
- associer un identifiant, un texte et un effet sur la mercy

### 3.9 `BestiaryEntry`

```text
BestiaryEntry
--------------------------------------------
- m_name : string
- m_category : MonsterCategory
- m_maxHp : int
- m_atk : int
- m_def : int
- m_result : string
--------------------------------------------
+ BestiaryEntry(name = "", category = NORMAL, maxHp = 0, atk = 0, def = 0, result = "")
+ getName() : const string&
+ getCategory() : MonsterCategory
+ getMaxHp() : int
+ getAtk() : int
+ getDef() : int
+ getResult() : const string&
```

Role :

- memoriser les monstres vaincus
- stocker un resultat simple : `Killed` ou `Spared`

### 3.10 `Game`

Classe centrale qui pilote le programme.

```text
Game
---------------------------------------------------------
- m_player : Player
- m_monsterCatalog : vector<unique_ptr<Monster>>
- m_bestiary : vector<BestiaryEntry>
- m_actCatalog : map<string, ActAction>
- m_rng : mt19937
---------------------------------------------------------
+ Game()
+ initialize() : bool
+ run() : void
---------------------------------------------------------
- initializeActCatalog() : void
- promptPlayerName() : bool
- loadItemsFromCsv(filePath) : bool
- loadMonstersFromCsv(filePath) : bool
- displayStartupSummary() const : void
- displayMainMenu() const : void
- readIntChoice(minValue, maxValue) const : int
- handleMenuChoice(choice) : void
- showBestiary() const : void
- showPlayerStats() const : void
- showItems() : void
- startBattle() : void
- hasReachedEnding() const : bool
- displayEndingAndExit() : void
- createRandomMonster() : unique_ptr<Monster>
- randomInt(minValue, maxValue) : int
- recordBattleResult(monster, result) : void
- trim(value) : string
- split(line, delimiter) : vector<string>
```

Role :

- centraliser la logique globale
- charger les donnees CSV
- initialiser les objets du jeu
- afficher le menu principal
- lancer un combat minimal

## 4. Enumerations utilisees

### `MonsterCategory`

```text
MonsterCategory
--------------------------------------------
+ NORMAL
+ MINIBOSS
+ BOSS
```

Role :

- distinguer les types de monstres
- enrichir l'affichage et la structure du bestiaire

### `ItemType`

```text
ItemType
--------------------------------------------
+ HEAL
+ UNKNOWN
```

Role :

- typer les objets de l'inventaire
- preparer l'ajout de nouveaux types d'items plus tard

## 5. Relations UML

### Heritage

```text
Entity <|-- Player
Entity <|-- Monster
Monster <|-- NormalMonster
Monster <|-- MiniBossMonster
Monster <|-- BossMonster
```

Interpretation :

- `Player` est une `Entity`
- `Monster` est une `Entity`

### Composition

```text
Player *-- Item
Game *-- Player
Game *-- Monster
Game *-- BestiaryEntry
Game *-- ActAction
```

Interpretation :

- le joueur possede un inventaire d'objets
- le jeu possede les objets centraux necessaires a son execution

### Association logique

```text
Monster --> ActAction
```

Interpretation :

- un monstre ne contient pas directement les objets `ActAction`
- il stocke seulement des identifiants d'actions ACT
- ces identifiants pointent vers le catalogue ACT stocke dans `Game`

## 6. Explication simple du diagramme


- `Entity` contient tout ce qui est commun aux combattants
- `Player` herite de `Entity`
- `Monster` est une classe abstraite qui herite de `Entity`
- `NormalMonster`, `MiniBossMonster` et `BossMonster` heritent de `Monster`
- `Player` ajoute l'inventaire et les statistiques globales
- `Monster` ajoute la mercy et les ACT autorises
- les classes derivees de `Monster` definissent un comportement different selon la categorie
- `Game` orchestre tout le programme
- les donnees du jeu sont chargees depuis des CSV
- le polymorphisme est montre par les methodes virtuelles de `Entity` et surtout par les monstres derives
