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

#include "rooms/RoomDungeonTemple.h"

#include "gamemap/GameMap.h"
#include "entities/Creature.h"
#include "entities/Weapon.h"
#include "entities/CreatureAction.h"
#include "entities/CreatureSound.h"
#include "network/ODServer.h"
#include "network/ServerNotification.h"

RoomDungeonTemple::RoomDungeonTemple(GameMap* gameMap) :
    Room(gameMap),
    mWaitTurns(0),
    mTempleObject(NULL)
{
    setMeshName("DungeonTemple");
}

void RoomDungeonTemple::notifyActiveSpotRemoved(ActiveSpotPlace place, Tile* tile)
{
    // This Room keeps its building object until it is destroyed (they will be released when
    // the room is destroyed)
}

void RoomDungeonTemple::absorbRoom(Room* room)
{
    Room::absorbRoom(room);

    // Get back the temple mesh reference
    if (!mBuildingObjects.empty())
        mTempleObject = mBuildingObjects.begin()->second;
    else
        mTempleObject = NULL;
}

void RoomDungeonTemple::createMeshLocal()
{
    Room::createMeshLocal();

    // Don't recreate the portal if it's already done.
    if (mTempleObject != NULL)
        return;

    // The client game map should not load building objects. They will be created
    // by the messages sent by the server because some of them are randomly
    // created
    if(!getGameMap()->isServerGameMap())
        return;

    mTempleObject = loadBuildingObject(getGameMap(), "DungeonTempleObject", getCentralTile(), 0.0, false);
    addBuildingObject(getCentralTile(), mTempleObject);
    createBuildingObjectMeshes();
}

void RoomDungeonTemple::destroyMeshLocal()
{
    Room::destroyMeshLocal();
    mTempleObject = NULL;
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
    const CreatureDefinition *classToSpawn = getGameMap()->getClassDescription("Kobold");
    if (classToSpawn == NULL)
    {
        std::cout << "Error: No 'Kobold' creature definition" << std::endl;
        return;
    }

    Creature* newCreature = new Creature(getGameMap(), classToSpawn);
    newCreature->setPosition(Ogre::Vector3((Ogre::Real)mCoveredTiles[0]->x,
                                            (Ogre::Real)mCoveredTiles[0]->y,
                                            (Ogre::Real)0));
    newCreature->setSeat(getSeat());

    newCreature->createMesh();
    getGameMap()->addCreature(newCreature);

    // Inform the clients
    if (getGameMap()->isServerGameMap())
    {
        ServerNotification *serverNotification = new ServerNotification(
           ServerNotification::addCreature, getGameMap()->getPlayerBySeat(getSeat()));
        newCreature->exportToPacket(serverNotification->mPacket);
        ODServer::getSingleton().queueServerNotification(serverNotification);
    }
}
