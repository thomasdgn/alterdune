# ALTERDUNE - UML Textuel

## Vue d'ensemble

Le projet repose sur une classe de base `Entity` qui regroupe les attributs communs aux combattants.

- `Entity`
  - attributs : `name`, `hp`, `maxHp`, `atk`, `def`
  - methodes : `takeDamage()`, `heal()`, `isAlive()`, `printStatus()`

- `Player : public Entity`
  - contient `vector<Item> inventory`
  - stocke `kills`, `spares`, `victories`
  - methodes : `displayStats()`, `displayItems()`, `useItem()`

- `Monster : public Entity`
  - stocke `category`, `mercy`, `mercyGoal`, `vector<string> actIds`
  - methodes : `addMercy()`, `isSpareable()`

- `Item`
  - stocke `name`, `type`, `value`, `quantity`

- `ActAction`
  - stocke `id`, `text`, `mercyImpact`

- `BestiaryEntry`
  - stocke `name`, `category`, `maxHp`, `atk`, `def`, `result`

- `Game`
  - contient un `Player`
  - contient un `vector<Monster>`
  - contient un `vector<BestiaryEntry>`
  - contient un `map<string, ActAction>`

## Relations

- Heritage :
  - `Player` herite de `Entity`
  - `Monster` herite de `Entity`

- Composition :
  - `Player` contient des `Item`
  - `Game` contient les objets centraux du programme

- Polymorphisme :
  - `Entity` declare des methodes virtuelles
  - `Player` et `Monster` les redefinissent
