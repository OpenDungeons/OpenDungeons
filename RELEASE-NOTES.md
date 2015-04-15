## OpenDungeons Release Notes

This file contains the subsequent release notes for each version of OpenDungeons since version 0.4.9, in reverse chronological order.

The numbers preceded by a hash (e.g. #9) reference GitHub issue numbers on our repository at https://github.com/OpenDungeons/OpenDungeons


### Version 0.4.9 - 22 December 2014

**Highlights:**

* Multiplayer mode implemented using a server/client logic
* Functional AI implemented
* Core gameplay better defined
* Gameplay elements are configurable via ini-style files: levels, creatures characteristics, equipment, rooms and traps behaviour
* Lots of new rooms
* Revamped UI
* Replay mode

**Code-related:**

* The game is stable and does not crash at exit
* The game does use threads only when needed (through sf::Thread)
* The code has been reviewed, is following conventions and does not use not GPL'ed code (#91)
* The license of every files is clear and defined in the CREDITS file (#93)
* Fix CMake basic invocation flags handling (#97 #42 #57 #58)

**Game logic:**

* Spawn pools. Each faction has got their own configurable set of spawnable creatures
* Support for configurable alliances
* Support for three functional traps
* Optimize pickup/dig mouse handling speed (#103)
* Upgrade and completion of the creature stats format (#151 #152 #245)
* Creatures are spawned based on the available rooms and their active spots number and it is configurable (#257)
* Better balancing through the use of a global balance sheet (#153)
* Creature definitions are overrideable in levels to create special creatures, or bosses
* Equipment is now defined in the config/equipment.cfg file
* Creatures can spawn with default equipment
* Rooms and traps behaviour are now defined in config/rooms.cfg and config/traps.cfg files
* Creatures can be slapped to death. And more importantly, those creepy chickens can be slapped while on a claimed tile

**Rooms:**

* Treasure room
  - New Ground texture
  - New Gold stacks
  - First treasury tile owned costs 0 to avoid being stuck (#87)
* Portal
* Dungeon Temple
* Training Room
  - Class & file rename (RoomDojo -> RoomTrainingHall) (#83)
  - Better training dummy model
* Dormitory
  - Class & file rename (RoomQuarters -> RoomDormitory) (#83)
  - Beds are persistent on room expansion.
  - Beds are removed when creatures die.
* Hatchery & food system (#13)
  - All models integrated and the room can placed in game (#106)
  - The room spawns chickens wandering within the room limits.
  - Creatures can eat them visually and calm their hunger.
* Forge (not actually useful in this release)
* Library (not actually useful in this release)
* Add support to destroy a room using a new button. Destroying a room gives back half the tiles price (#108)
* Add support to disable a room for a specific player in a specific level (#86)

**Creature behaviour:**

* Creature hunger system (#13)
* Creature behaving correctly in narrow passages (1-tile wide) (#4)
* Separate water/lava and flying collision handling (#101)
* Better creature fighting behaviour (#154)
* Make workers flee on first enemy blow (#90)
* Workers drop mined gold in the form of a pickable gold stack if no treasury room

**Modes and menus:**

* Game mode and editor mode functional with a level loading menu
* Minimap functional
* The basic menus are functional and look definite (#6)
* Factions and players are configurable in a lobby
* The remote clients shouldn't have to select a level (#70)
* Show room pricing (#89)
* Added buttons to grab workers and fighters easily in the menu (#84 #85)

**Basic AI (#15):**

* The AI can build every useful room
* The AI takes care of building rooms to get the maximum "active spots" (represented by room models)
* The AI can drop fighters nearby other enemy creatures to trigger some fun
* AI takes care of having at least one treasury tile before searching for gold

**Graphics & audio:**

* All models are displayed fine even if not yet using normals (#76 #100 #129)
* New and improved models (including a new set of beds)
* Added missing music/sounds (#95 #23)
