# ALTERDUNE - UML Console

## Perimetre

Ce document decrit le modele UML de la partie console d'ALTERDUNE.

Le frontend SFML n'est pas represente ici, meme si certaines methodes `Frontend*` existent dans `Game.h`. Le diagramme ci-dessous se concentre sur le moteur console compile dans `alterdune_console` :

- `src/main.cpp`
- `include/Entity.h` / `src/Entity.cpp`
- `include/Player.h` / `src/Player.cpp`
- `include/Monster.h` / `src/Monster.cpp`
- `include/Item.h` / `src/Item.cpp`
- `include/ActAction.h` / `src/ActAction.cpp`
- `include/BestiaryEntry.h` / `src/BestiaryEntry.cpp`
- `include/Game.h` / `src/Game.cpp`

## Diagramme UML

```mermaid
classDiagram
direction LR

class Entity {
  #string m_name
  #int m_hp
  #int m_maxHp
  #int m_atk
  #int m_def
  +Entity(name, maxHp, atk, def)
  +getEntityType() string
  +printStatus(os) void
  +takeDamage(rawDamage) int
  +heal(amount) int
  +isAlive() bool
  +getName() const string&
  +getHp() int
  +getMaxHp() int
  +getAtk() int
  +getDef() int
  +setName(name) void
  +setHp(hp) void
  +setMaxHp(maxHp) void
  +setAtk(atk) void
  +setDef(def) void
}

class Player {
  -vector~Item~ m_inventory
  -string m_appearanceId
  -int m_kills
  -int m_spares
  -int m_victories
  +Player(name, maxHp, atk, def)
  +getEntityType() string
  +printStatus(os) void
  +addItem(item) void
  +getInventory() vector~Item~&
  +displayStats(os) void
  +displayItems(os) void
  +useItem(index, os) bool
  +getAppearanceId() const string&
  +setAppearanceId(appearanceId) void
  +recordKill() void
  +recordSpare() void
  +recordVictory() void
}

class Monster {
  <<abstract>>
  -int m_mercy
  -int m_mercyGoal
  -vector~string~ m_actIds
  +Monster(name, maxHp, atk, def, mercyGoal, actIds)
  +getEntityType() string
  +printStatus(os) void
  +getCategory()* MonsterCategory
  +getMaxActChoices()* int
  +clone()* unique_ptr~Monster~
  +getMercy() int
  +getMercyGoal() int
  +getActIds() const vector~string~&
  +getAvailableActIds() vector~string~
  +addMercy(amount) void
  +isSpareable() bool
  +categoryFromString(value) MonsterCategory
  +categoryToString(category) string
}

class NormalMonster {
  +getCategory() MonsterCategory
  +getMaxActChoices() int
  +clone() unique_ptr~Monster~
}

class MiniBossMonster {
  +getCategory() MonsterCategory
  +getMaxActChoices() int
  +clone() unique_ptr~Monster~
}

class BossMonster {
  +getCategory() MonsterCategory
  +getMaxActChoices() int
  +clone() unique_ptr~Monster~
}

class Item {
  -string m_name
  -ItemType m_type
  -int m_value
  -int m_quantity
  +Item(name, type, value, quantity)
  +getName() const string&
  +getType() ItemType
  +getValue() int
  +getQuantity() int
  +setQuantity(quantity) void
  +addQuantity(amount) void
  +consumeOne() bool
  +itemTypeFromString(value) ItemType
  +itemTypeToString(type) string
}

class ActAction {
  -string m_id
  -string m_text
  -int m_mercyImpact
  +ActAction(id, text, mercyImpact)
  +getId() const string&
  +getText() const string&
  +getMercyImpact() int
}

class BestiaryEntry {
  -string m_name
  -MonsterCategory m_category
  -int m_maxHp
  -int m_atk
  -int m_def
  -string m_description
  -string m_result
  +BestiaryEntry(name, category, maxHp, atk, def, description, result)
  +getName() const string&
  +getCategory() MonsterCategory
  +getMaxHp() int
  +getAtk() int
  +getDef() int
  +getDescription() const string&
  +getResult() const string&
  +setResult(result) void
}

class Game {
  -Player m_player
  -vector~Monster~ m_monsterCatalog
  -vector~BestiaryEntry~ m_bestiary
  -map~string, ActAction~ m_actCatalog
  -mt19937 m_rng
  -string m_languageCode
  -set~string~ m_regionKeys
  -set~string~ m_clearedEncounters
  +Game()
  +initialize() bool
  +setLanguage(languageCode) void
  +getLanguage() const string&
  +setPlayerAppearance(appearanceId) void
  +getPlayerAppearance() const string&
  +run() void
  -initializeActCatalog() void
  -promptPlayerName() bool
  -promptPlayerAppearance() void
  -loadItemsFromCsv(filePath) bool
  -loadMonstersFromCsv(filePath) bool
  -displayMainMenu() void
  -handleMenuChoice(choice) void
  -showBestiary() void
  -showPlayerStats() void
  -showItems() void
  -startBattle() void
  -handleFightAction(monster) BattleTurnResult
  -handleActAction(monster, actUsage) BattleTurnResult
  -handleItemAction(monster) BattleTurnResult
  -handleMercyAction(monster) BattleTurnResult
  -handleMonsterTurn(monster) BattleTurnResult
  -concludeBattle(monster, result) void
  -grantBattleReward(monster, result) void
  -recordBattleResult(monster, result) void
}

class MonsterCategory {
  <<enumeration>>
  NORMAL
  MINIBOSS
  BOSS
}

class ItemType {
  <<enumeration>>
  HEAL
  UNKNOWN
}

class BattleTurnResult {
  <<enumeration>>
  CONTINUE
  PLAYER_WON_KILL
  PLAYER_WON_SPARE
  PLAYER_FLED
  PLAYER_LOST
  NO_TURN_SPENT
}

Entity <|-- Player
Entity <|-- Monster
Monster <|-- NormalMonster
Monster <|-- MiniBossMonster
Monster <|-- BossMonster

Player "1" *-- "0..*" Item : inventory
Game "1" *-- "1" Player
Game "1" *-- "0..*" Monster : catalog
Game "1" *-- "0..*" BestiaryEntry : bestiary
Game "1" *-- "0..*" ActAction : ACT catalog
Game ..> BattleTurnResult : uses
Monster "1" --> "0..*" ActAction : actIds resolve through Game
Monster --> MonsterCategory
Item --> ItemType
BestiaryEntry --> MonsterCategory
```

## Script PlantUML

Copier-coller ce script dans un editeur PlantUML en ligne, par exemple PlantUML Online Server ou PlantText.

```plantuml
@startuml
title ALTERDUNE - UML Console
left to right direction
skinparam classAttributeIconSize 0
skinparam shadowing false
skinparam backgroundColor #FFFFFF
skinparam class {
  BackgroundColor #F8FAFC
  BorderColor #334155
  ArrowColor #334155
}
skinparam enum {
  BackgroundColor #ECFEFF
  BorderColor #0891B2
}

abstract class Entity {
  # m_name : string
  # m_hp : int
  # m_maxHp : int
  # m_atk : int
  # m_def : int
  --
  + Entity(name : string, maxHp : int, atk : int, def : int)
  + getEntityType() : string
  + printStatus(os : ostream&) : void
  + takeDamage(rawDamage : int) : int
  + heal(amount : int) : int
  + isAlive() : bool
  + getName() : const string&
  + getHp() : int
  + getMaxHp() : int
  + getAtk() : int
  + getDef() : int
  + setName(name : string) : void
  + setHp(hp : int) : void
  + setMaxHp(maxHp : int) : void
  + setAtk(atk : int) : void
  + setDef(def : int) : void
}

class Player {
  - m_inventory : vector<Item>
  - m_appearanceId : string
  - m_kills : int
  - m_spares : int
  - m_victories : int
  --
  + Player(name : string, maxHp : int, atk : int, def : int)
  + getEntityType() : string
  + printStatus(os : ostream&) : void
  + addItem(item : Item) : void
  + getInventory() : vector<Item>&
  + displayStats(os : ostream&) : void
  + displayItems(os : ostream&) : void
  + useItem(index : size_t, os : ostream&) : bool
  + getAppearanceId() : const string&
  + setAppearanceId(appearanceId : string) : void
  + recordKill() : void
  + recordSpare() : void
  + recordVictory() : void
}

abstract class Monster {
  - m_mercy : int
  - m_mercyGoal : int
  - m_actIds : vector<string>
  --
  + Monster(name : string, maxHp : int, atk : int, def : int, mercyGoal : int, actIds : vector<string>)
  + getEntityType() : string
  + printStatus(os : ostream&) : void
  + {abstract} getCategory() : MonsterCategory
  + {abstract} getMaxActChoices() : int
  + {abstract} clone() : unique_ptr<Monster>
  + getMercy() : int
  + getMercyGoal() : int
  + getActIds() : const vector<string>&
  + getAvailableActIds() : vector<string>
  + addMercy(amount : int) : void
  + isSpareable() : bool
  + {static} categoryFromString(value : string) : MonsterCategory
  + {static} categoryToString(category : MonsterCategory) : string
}

class NormalMonster {
  + getCategory() : MonsterCategory
  + getMaxActChoices() : int
  + clone() : unique_ptr<Monster>
}

class MiniBossMonster {
  + getCategory() : MonsterCategory
  + getMaxActChoices() : int
  + clone() : unique_ptr<Monster>
}

class BossMonster {
  + getCategory() : MonsterCategory
  + getMaxActChoices() : int
  + clone() : unique_ptr<Monster>
}

class Item {
  - m_name : string
  - m_type : ItemType
  - m_value : int
  - m_quantity : int
  --
  + Item(name : string, type : ItemType, value : int, quantity : int)
  + getName() : const string&
  + getType() : ItemType
  + getValue() : int
  + getQuantity() : int
  + setQuantity(quantity : int) : void
  + addQuantity(amount : int) : void
  + consumeOne() : bool
  + {static} itemTypeFromString(value : string) : ItemType
  + {static} itemTypeToString(type : ItemType) : string
}

class ActAction {
  - m_id : string
  - m_text : string
  - m_mercyImpact : int
  --
  + ActAction(id : string, text : string, mercyImpact : int)
  + getId() : const string&
  + getText() : const string&
  + getMercyImpact() : int
}

class BestiaryEntry {
  - m_name : string
  - m_category : MonsterCategory
  - m_maxHp : int
  - m_atk : int
  - m_def : int
  - m_description : string
  - m_result : string
  --
  + BestiaryEntry(name : string, category : MonsterCategory, maxHp : int, atk : int, def : int, description : string, result : string)
  + getName() : const string&
  + getCategory() : MonsterCategory
  + getMaxHp() : int
  + getAtk() : int
  + getDef() : int
  + getDescription() : const string&
  + getResult() : const string&
  + setResult(result : string) : void
}

class Game {
  - m_player : Player
  - m_monsterCatalog : vector<unique_ptr<Monster>>
  - m_bestiary : vector<BestiaryEntry>
  - m_actCatalog : map<string, ActAction>
  - m_rng : mt19937
  - m_languageCode : string
  - m_regionKeys : set<string>
  - m_clearedEncounters : set<string>
  --
  + Game()
  + initialize() : bool
  + setLanguage(languageCode : string) : void
  + getLanguage() : const string&
  + setPlayerAppearance(appearanceId : string) : void
  + getPlayerAppearance() : const string&
  + run() : void
  - initializeActCatalog() : void
  - promptPlayerName() : bool
  - promptPlayerAppearance() : void
  - loadItemsFromCsv(filePath : string) : bool
  - loadMonstersFromCsv(filePath : string) : bool
  - displayMainMenu() : void
  - handleMenuChoice(choice : int) : void
  - showBestiary() : void
  - showPlayerStats() : void
  - showItems() : void
  - startBattle() : void
  - handleFightAction(monster : Monster&) : BattleTurnResult
  - handleActAction(monster : Monster&, actUsage : map<string,int>&) : BattleTurnResult
  - handleItemAction(monster : Monster&) : BattleTurnResult
  - handleMercyAction(monster : Monster&) : BattleTurnResult
  - handleMonsterTurn(monster : Monster&) : BattleTurnResult
  - concludeBattle(monster : Monster, result : BattleTurnResult) : void
  - grantBattleReward(monster : Monster, result : BattleTurnResult) : void
  - recordBattleResult(monster : Monster, result : string) : void
}

enum MonsterCategory {
  NORMAL
  MINIBOSS
  BOSS
}

enum ItemType {
  HEAL
  UNKNOWN
}

enum BattleTurnResult {
  CONTINUE
  PLAYER_WON_KILL
  PLAYER_WON_SPARE
  PLAYER_FLED
  PLAYER_LOST
  NO_TURN_SPENT
}

Entity <|-- Player
Entity <|-- Monster
Monster <|-- NormalMonster
Monster <|-- MiniBossMonster
Monster <|-- BossMonster

Player "1" *-- "0..*" Item : inventory
Game "1" *-- "1" Player
Game "1" *-- "0..*" Monster : catalog
Game "1" *-- "0..*" BestiaryEntry : bestiary
Game "1" *-- "0..*" ActAction : ACT catalog
Game ..> BattleTurnResult
Monster "1" --> "0..*" ActAction : actIds via Game
Monster --> MonsterCategory
Item --> ItemType
BestiaryEntry --> MonsterCategory

note right of Game
  Diagramme limite a la console.
  Les methodes Frontend* de Game.h
  sont volontairement exclues.
end note
@enduml
```

## Lecture rapide

- `Entity` factorise les attributs et comportements communs aux combattants.
- `Player` herite de `Entity` et possede un inventaire de `Item`.
- `Monster` herite de `Entity`, reste abstraite, gere la mercy et expose `clone()` pour creer des combats a partir du catalogue.
- `NormalMonster`, `MiniBossMonster` et `BossMonster` specialisent le nombre d'actions ACT disponibles.
- `Game` orchestre la boucle console, le chargement CSV, le catalogue ACT, le catalogue de monstres, les combats, les recompenses et le bestiaire.
- `Monster` ne contient pas directement des `ActAction` : il stocke des identifiants (`m_actIds`) resolus dans `Game::m_actCatalog`.
