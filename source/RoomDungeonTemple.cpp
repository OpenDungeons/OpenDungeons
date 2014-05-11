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
    mType = dungeonTemple;
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
    if (waitTurns <= 0)
    {
        waitTurns = 30;

        // If the room has been destroyed, or has not yet been assigned any tiles, then we
        // cannot determine where to place the new creature and we should just give up.
        if (mCoveredTiles.size() == 0)
            return;

        // Create a new creature and copy over the class-based creature parameters.
        CreatureDefinition *classToSpawn = getGameMap()->getClassDescription("Kobold");
        if (classToSpawn != NULL)
        {
            //TODO: proper assignemt of creature definition through constrcutor
            Creature* newCreature = new Creature( getGameMap());
            newCreature->setCreatureDefinition(classToSpawn);
            newCreature->setName(newCreature->getUniqueCreatureName());
            newCreature->setPosition(Ogre::Vector3((Ogre::Real)mCoveredTiles[0]->x,
                                                   (Ogre::Real)mCoveredTiles[0]->y,
                                                   (Ogre::Real)0));
            newCreature->setColor(getColor());

            // Default weapon is empty
            newCreature->setWeaponL(new Weapon("none", 0.0, 1.0, 0.0, "L", newCreature));
            newCreature->setWeaponR(new Weapon("none", 0.0, 1.0, 0.0, "R", newCreature));

            newCreature->createMesh();
            newCreature->getWeaponL()->createMesh();
            newCreature->getWeaponR()->createMesh();
            getGameMap()->addCreature(newCreature);
        }
        else
        {
            std::cout << "Error: No 'Kobold' creature definition" << std::endl;
        }
    }
    else
    {
        --waitTurns;
    }
}
