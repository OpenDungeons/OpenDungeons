## OpenDungeons Release Notes

This file contains the subsequent release notes for each version of OpenDungeons since version 0.4.9, in reverse chronological order.

The numbers preceded by a hash (e.g. #9) reference GitHub issue numbers on our repository at https://github.com/OpenDungeons/OpenDungeons


### Version 0.5.0

**Highlights:**

* Fog of War implemented
* Spell system implemented with two spells: Summon Worker and Call to War
* Research logic and UI implemented: rooms, spells and traps should now be researched in the library
* Saving/loading feature
* Creatures pay day implemented
* Portals can be claimed and can generate rogue waves that attack keepers
* Traps must be crafted in the workshop
* Creatures mood implemented
* Creature overlays show relevant information on hover (or when pressing ALT)
* Many GUI improvements

**General:**

* Saving/loading feature (#531 #534)
* Many hardcoded values were made configurable via config files
* Rooms and traps prices are configurable (#346)
* Tileset configuration and biomes support (#14)
* Test levels are noted as such (#381)
* Backup the last three game logs (#567)
* Packaging/installation improvements:
  - Provide an appdata.xml file (#356)
  - Drop bundled FreeMono.ttf font (#366)
  - Install engine config files in the proper arch-dependent path (#400)
  - Link icon with the game window and executable on Windows (#202)
* Removed the broken Culling Manager (#170)
* Disable angelscript as it is not used currently (#504)
* Code abstration to prepare unit tests (#592)
* Lots of bug fixes

**Gameplay:**

* Implemented the Fog of War (#295)
  - Implemented trap vision logic (#408)
* Implemented creatures pay day (#431)
* Implemented spells support and made workers spawned only via a spell (#16)
* Implemented the Call to War spell (#484)
* Implemented the library's research logic + research tree (#411 #508)
* Portals can be claimed (#588)
* The max number of creatures is configurable and influenced by the number of claimed portals (#588)
* Implemented rogue portal waves (#594 #607)
* Gold dropped on the floor follows the same mesh logic as the one in treasury rooms (#279)
* Improved the camera perspective (#345)
* Readded default viewpoint support and made the V key cycle the viewpoints (#494)
* AI repairs damaged rooms (#375)
* Revised the creature spawning logic (#568)
* Updated the help window content (#548)

**Rooms:**

* The workshop replaces the forge for traps. The forge will be used later to craft creature's equipment (#459)
* Traps are crafted in the workshop only when needed (#469)
* Add support to disable a room for a specific player in a specific level (#86)

**Creatures:**

* Workers: Gold dug from gold tiles is now visually carried (#407)
* Improved creature vision processing (#396)
* Added support to make creature prefer certain jobs (#354)
* Implemented creature mood (#11)
* Improved wall claiming order (#282)
* Research entities should be brought to the temple center (#576)
* Many fixes to workers and fighters behaviour

**UI:**

* Added the research tree GUI, currently not interactive (#517)
* Skull flag system to see current creatures health + level + team color (#519)
* Console code cleaned up and using CEGUI (#134 #299)
* Chat and event messages handling upgrade (#638 #642 #647 #654)
* Fixed tooltips behaviour (#337 #397)
* Added a check box, permitting to disable replay creation when quitting the game (#406)
* Improved the UI of Combo-boxes/Check-boxes, ... from generic to OD-like (#419)
* Display the number of controlled fighting creatures (#562)
* The keeper hand is used as an icon and is above the GUI (#547 #420)

**Graphics:**

* Better light handling (#446 #453 #456)
* Support for normals (#514)
* Added normals for many materials
* Use more keeper hand animations (#350)
* New workshop and corresponding elements (#609 #612)
* New training dummy and wall models for the training hall (#656)
* New troll bed (#503)
* New goblin + bed (#438 #449)
* New grimoire model (#544)
* New gold ground tile (#536)
* Fixed material errors (#529 #570 #606)
* Spring cleaning: unused textures, models and materials dropped (#492 #579)

**Editor:**

* Fixed adding/moving lights (#19 #20 #486)
* Fixed seeing traps in the map editor (#455)
* Made adding portals/temple possible via the editor (#192)

**Balancing:**

* Decreased amount of gold stored in each treasury tile to 1000
* Reduced creature walking speed by 10%
* Set armor increase/level to 0.1 for all creatures
* Raised experience gained from battle
* Creature regain mood points when hitting other creatures (#649)
* Reduced workers claiming rate (#650)
* Increased portal cooldown (+25%)


### Version 0.4.9 - 22 December 2014

**Highlights:**

* Multiplayer mode implemented using a server/client logic
* Functional AI implemented
* Core gameplay better defined
* Traps support
* Gameplay elements are configurable via ini-style files: levels, creatures characteristics, equipment, rooms and traps behaviour
* Lots of new rooms
* Revamped UI
* Replay mode

**General:**

* The game is stable and does not crash at exit
* The game does use threads only when needed (through sf::Thread)
* The code has been reviewed, is following conventions and does not use not GPL'ed code (#91)
* The license of every files is clear and defined in the CREDITS file (#93)
* Fix CMake basic invocation flags handling (#97 #42 #57 #58)
* Lots of bug fixes

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
* Crypt: spawns gnomes when you let enemy creatures decay there (#322)
* Add support to destroy rooms or traps using a new button to get a refund of half their price (#108)

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
