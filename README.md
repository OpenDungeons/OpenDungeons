## OpenDungeons

OpenDungeons is an open source, real time strategy game sharing game elements
with the Dungeon Keeper series and Evil Genius. Players build an underground
dungeon which is inhabited by creatures. Players fight each other for control
of the underground by indirectly commanding their creatures, directly casting
spells in combat, and luring enemies into sinister traps.

The game is developed by a friendly community of developers and artists, and
has now reached a quite playable and enjoyable status after more than 6 years
of development.

### How to play

Future versions will have an in-game tutorial, but for now, you can use the
following resources to learn the basic gameplay concepts:

- In-game help screen, toggled with the F1 key
- Video tutorial (version 0.5.0): https://www.youtube.com/watch?v=P4MClQUdb0E
- Wiki page: https://github.com/OpenDungeons/OpenDungeons/wiki/Gameplay

You can play singleplayer levels using the Skirmish menu, or host/join a
multiplayer game by using the corresponding menus.

### Be part of the community

As free software aficionados, we value community-based development and
welcome any willing contributor regardless of their skills. Giving us
feedback about the gameplay, or reporting bugs on our tracker, is already
a very relevant way of contributing to the development of this game,
so please get in touch!

You will find us on the following channels:

- Forum: http://forum.freegamedev.net/viewforum.php?f=15
- GitHub: https://github.com/OpenDungeons/OpenDungeons
- IRC: #opendungeons channel on Freenode

### Build instructions

If you retrieve the source code of OpenDungeons and want to have a go at
building it yourself, have a look at platform-specific build instructions
on our wiki: https://github.com/OpenDungeons/OpenDungeons/wiki/Compile

In a few words, to build OpenDungeons, you need the following libraries:

- OGRE SDK (1.9.x)
- Boost (same version that OGRE was linked against)
- CEGUI SDK (0.8.x)
- SFML (2.x)

You will also need a recent CMake version (2.8 or newer) and a compiler
that supports C++11 features reasonably well, i.e.:

- Linux: GCC 4.8+
- Windows: MSVS 2013 Express or MinGW 4.8+

On an UNIX system, you can then run:
```
mkdir build && cd build
cmake ..
make -jX    // X is the number of CPU cores that you want to allocate  
```
And run the *opendungeons* output binary.

### Contributing code

If you want to contribute code, you should take a look at our coding
guidelines: https://github.com/OpenDungeons/OpenDungeons/wiki/Code-Guidelines

It contains a rather deep introduction on how we name, indent, structure and
extend our code. It also has some performance optimisation tips.

#### Repository organisation

**Data files**
```
config/          - Several game config files
dist/            - Icons and linux desktop entry file.
gui/             - CEGUI files + corresponding Gui images
levels/          - Game levels
licenses/        - License files used for game data and code
materials/       - Materials (models texturing scripts and textures)
models/          - Model files
music/           - Music files
scripts/         - Our AngelScript code files
sounds/          - Game Sounds
AUTHORS          - List of past and current contributors
CREDITS          - Detailed listing of licenses and credits for our assets
LICENSE.md       - General information about the code and assets licenses
README.md        - The file you are currently reading
RELEASE-NOTES.md - What's new in OpenDungeons
```

**Code files**
```
cmake/           - Helper files for CMake
 |- config/      - Variable input files for the CMake script
 |- modules/     - Addon scripts for CMake to find dependencies
dependencies/    - All external projects
 |- angelscript/ - AngelScript SDK, our scripting language
sources/         - All our own .cpp and .h files of the game
tools/           - Some developers shell scripts
.gitignore       - The files and folders that are ignored by git locally
CMakeLists.txt   - CMake script for generating the Makefile and IDE projects
```
