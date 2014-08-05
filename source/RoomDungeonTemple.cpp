/*
 *  Copyright (C) 2011-2014  OpenDungeons Team
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "RoomDungeonTemple.h"

#include "GameMap.h"
#include "Creature.h"
#include "Weapon.h"
#include "CreatureAction.h"
#include "CreatureSound.h"
#include "RoomObject.h"
#include "ODServer.h"
#include "ServerNotification.h"

RoomDungeonTemple::RoomDungeonTemple(GameMap* gameMap) :
    Room(gameMap),
    mWaitTurns(0),
    mTempleObject(NULL)
{
    mType = dungeonTemple;
}

void RoomDungeonTemple::absorbRoom(Room* room)
{
    Room::absorbRoom(room);

    // Get back the temple mesh reference
    mTempleObject = getFirstRoomObject();
}

void RoomDungeonTemple::createMesh()
{
    Room::createMesh();

    // Don't recreate the portal if it's already done.
    if (mTempleObject != NULL)
        return;

    // The client game map should not load room objects. They will be created
    // by the messages sent by the server because some of them are randomly
    // created
    if(!getGameMap()->isServerGameMap())
        return;

    mTempleObject = loadRoomObject(getGameMap(), "DungeonTempleObject");
    createRoomObjectMeshes();
}

void RoomDungeonTemple::destroyMesh()
{
    Room::destroyMesh();
}

void RoomDungeonTemple::produceKobold()
{
    if (mWaitTurns > 0)
    {
        --mWaitTurns;
        return;
    }

    mWaitTurns = 30;

    // If the room has been destroyed, or has not yet been assigned any tiles, then we
    // cannot determine where to place the new creature and we should just give up.
    if (mCoveredTiles.empty())
        return;

    // Create a new creature and copy over the class-based creature parameters.
    CreatureDefinition *classToSpawn = getGameMap()->getClassDescription("Kobold");
    if (classToSpawn == NULL)
    {
        std::cout << "Error: No 'Kobold' creature definition" << std::endl;
        return;
    }

    //TODO: proper assignemt of creature definition through constrcutor
    Creature* newCreature = new Creature(getGameMap(), true);
    newCreature->setCreatureDefinition(classToSpawn);
    newCreature->setPosition(Ogre::Vector3((Ogre::Real)mCoveredTiles[0]->x,
                                            (Ogre::Real)mCoveredTiles[0]->y,
                                            (Ogre::Real)0));
    newCreature->setColor(getColor());

    // Default weapon is empty
    newCreature->setWeaponL(new Weapon(getGameMap(), "none", 0.0, 1.0, 0.0, "L", newCreature));
    newCreature->setWeaponR(new Weapon(getGameMap(), "none", 0.0, 1.0, 0.0, "R", newCreature));

    newCreature->createMesh();
    newCreature->getWeaponL()->createMesh();
    newCreature->getWeaponR()->createMesh();
    getGameMap()->addCreature(newCreature);

    // Inform the clients
    if (getGameMap()->isServerGameMap())
    {
        try
        {
           ServerNotification *serverNotification = new ServerNotification(
               ServerNotification::addCreature, newCreature->getControllingPlayer());
           serverNotification->packet << newCreature;
           ODServer::getSingleton().queueServerNotification(serverNotification);
        }
        catch (std::bad_alloc&)
        {
            Ogre::LogManager::getSingleton().logMessage("ERROR: bad alloc in RoomDungeonTemple::produceKobold", Ogre::LML_CRITICAL);
            exit(1);
        }
    }
}
