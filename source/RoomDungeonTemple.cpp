#include "RoomDungeonTemple.h"
#include "GameMap.h"
#include "Creature.h"
#include "Weapon.h"
#include "CreatureAction.h"
#include "CreatureSound.h"
#include "RoomObject.h"

RoomDungeonTemple::RoomDungeonTemple() :
        waitTurns(0)
{
    type = dungeonTemple;
}

void RoomDungeonTemple::createMeshes()
{
    Room::createMeshes();

    loadRoomObject("DungeonTempleObject");
    createRoomObjectMeshes();
}

void RoomDungeonTemple::destroyMeshes()
{
    Room::destroyMeshes();
}

/*! \brief Counts down a timer until it reaches 0, then it spawns a kobold of the color of this dungeon temple at the center of the dungeon temple, and resets the timer.
 *
 */
void RoomDungeonTemple::produceKobold()
{
    // If the game map is trying to load the next level it deletes any creatures on the map, spawning new ones prevents it from finishing.
    if (gameMap->loadNextLevel)
        return;

    if (waitTurns <= 0)
    {
        waitTurns = 30;

        // If the room has been destroyed, or has not yet been assigned any tiles, then we
        // cannot determine where to place the new creature and we should just give up.
        if (coveredTiles.size() == 0)
            return;

        // Create a new creature and copy over the class-based creature parameters.
        CreatureClass *classToSpawn = gameMap->getClassDescription("Kobold");
        if (classToSpawn != NULL)
        {
            Creature *newCreature = new Creature(gameMap);
            *newCreature = *classToSpawn;
            newCreature->name = newCreature->getUniqueCreatureName();
            newCreature->setPosition(coveredTiles[0]->x, coveredTiles[0]->y, 0);
            newCreature->setColor(color);

            //NOTE:  This needs to be modified manually when the level file weapon format changes.
            newCreature->weaponL = new Weapon("none", 5, 4, 0, newCreature, "L");
            newCreature->weaponR = new Weapon("none", 5, 4, 0, newCreature, "R");

            newCreature->createMesh();
            gameMap->addCreature(newCreature);
        }
    }
    else
    {
        --waitTurns;
    }
}

