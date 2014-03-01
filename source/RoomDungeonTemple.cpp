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

void RoomDungeonTemple::createMesh()
{
    Room::createMesh();

    loadRoomObject("DungeonTempleObject");
    createRoomObjectMeshes();
}

void RoomDungeonTemple::destroyMesh()
{
    Room::destroyMesh();
}

/*! \brief Counts down a timer until it reaches 0, then it spawns a kobold of the color of this dungeon temple at the center of the dungeon temple, and resets the timer.
 *
 */
void RoomDungeonTemple::produceKobold()
{
    // If the game map is trying to load the next level it deletes any creatures on the map, spawning new ones prevents it from finishing.
    if (getGameMap()->loadNextLevel)
        return;

    if (waitTurns <= 0)
    {
        waitTurns = 30;

        // If the room has been destroyed, or has not yet been assigned any tiles, then we
        // cannot determine where to place the new creature and we should just give up.
        if (coveredTiles.size() == 0)
            return;

        // Create a new creature and copy over the class-based creature parameters.
        CreatureDefinition *classToSpawn = getGameMap()->getClassDescription("Kobold");
        if (classToSpawn != NULL)
        {
            //TODO: proper assignemt of creature definition through constrcutor
            Creature* newCreature = new Creature( getGameMap());
            newCreature->setCreatureDefinition(classToSpawn);
            newCreature->setName(newCreature->getUniqueCreatureName());
            newCreature->setPosition(Ogre::Vector3((Ogre::Real)coveredTiles[0]->x,
                                                   (Ogre::Real)coveredTiles[0]->y,
                                                   (Ogre::Real)0));
            newCreature->setColor(getColor());

            //NOTE:  This needs to be modified manually when the level file weapon format changes.
            newCreature->setWeaponL(new Weapon("none", 5, 4, 0, newCreature, "L"));
            newCreature->setWeaponR(new Weapon("none", 5, 4, 0, newCreature, "R"));

            newCreature->createMesh();
            getGameMap()->addCreature(newCreature);
        }
    }
    else
    {
        --waitTurns;
    }
}

