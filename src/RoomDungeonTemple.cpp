#include "Globals.h"
#include "RoomDungeonTemple.h"

RoomDungeonTemple::RoomDungeonTemple()
	: Room()
{
	type = dungeonTemple;
	waitTurns = 0;
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
	if(waitTurns <= 0)
	{
		waitTurns = 30;

		// If the room has been destroyed, or has not yet been assigned any tiles, then we
		// cannot determine where to place the new creature and we should just give up.
		if(coveredTiles.size() == 0)
			return;

		// Create a new creature and copy over the class-based creature parameters.
		CreatureClass *classToSpawn = gameMap.getClassDescription("Kobold");
		if(classToSpawn != NULL)
		{
			Creature *newCreature = new Creature;
			*newCreature = *classToSpawn;
			newCreature->name = newCreature->getUniqueCreatureName();
			newCreature->setPosition(coveredTiles[0]->x, coveredTiles[0]->y, 0);
			newCreature->setColor(color);

			//NOTE:  This needs to be modified manually when the level file weapon format changes.
			newCreature->weaponL = new Weapon;
			newCreature->weaponL->name = "none";
			newCreature->weaponL->damage = 5;
			newCreature->weaponL->range = 4;
			newCreature->weaponL->defense = 0;
			newCreature->weaponL->parentCreature = newCreature;
			newCreature->weaponL->handString = "L";

			newCreature->weaponR = new Weapon;
			newCreature->weaponR->name = "none";
			newCreature->weaponR->damage = 5;
			newCreature->weaponR->range = 4;
			newCreature->weaponR->defense = 0;
			newCreature->weaponR->parentCreature = newCreature;
			newCreature->weaponR->handString = "R";

			newCreature->createMesh();
			gameMap.addCreature(newCreature);
		}
	}
	else
	{
		waitTurns--;
	}
}

