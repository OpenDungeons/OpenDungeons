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

#include "rooms/RoomPortal.h"

#include "network/ODServer.h"
#include "network/ServerNotification.h"
#include "game/Seat.h"
#include "game/Player.h"
#include "entities/Creature.h"
#include "entities/Tile.h"
#include "entities/RenderedMovableEntity.h"
#include "gamemap/GameMap.h"
#include "entities/Weapon.h"
#include "entities/CreatureAction.h"
#include "utils/Random.h"
#include "utils/LogManager.h"

#include <cmath>

RoomPortal::RoomPortal(GameMap* gameMap) :
        Room(gameMap),
        mSpawnCreatureCountdown(0),
        mXCenter(0),
        mYCenter(0),
        mPortalObject(NULL)
{
   setMeshName("Portal");
}

void RoomPortal::notifyActiveSpotRemoved(ActiveSpotPlace place, Tile* tile)
{
    // This Room keeps its building object until it is destroyed (they will be released when
    // the room is destroyed)
}

void RoomPortal::absorbRoom(Room* room)
{
    Room::absorbRoom(room);

    // Get back the portal mesh reference
    if (!mBuildingObjects.empty())
        mPortalObject = mBuildingObjects.begin()->second;
    else
        mPortalObject = NULL;
}

void RoomPortal::createMeshLocal()
{
    Room::createMeshLocal();

    // Don't recreate the portal if it's already done.
    if (mPortalObject != NULL)
        return;

    // The client game map should not load building objects. They will be created
    // by the messages sent by the server because some of them are randomly created
    if(!getGameMap()->isServerGameMap())
        return;

    mPortalObject = loadBuildingObject(getGameMap(), "PortalObject", getCentralTile(), 0.0);
    addBuildingObject(getCentralTile(), mPortalObject);
    createBuildingObjectMeshes();

    mPortalObject->setAnimationState("Idle");
}

void RoomPortal::destroyMeshLocal()
{
    Room::destroyMeshLocal();
    mPortalObject = NULL;
}

void RoomPortal::addCoveredTile(Tile* t, double nHP, bool isRoomAbsorb)
{
    Room::addCoveredTile(t, nHP, isRoomAbsorb);
    recomputeCenterPosition();
}

bool RoomPortal::removeCoveredTile(Tile* t, bool isRoomAbsorb)
{
    return Room::removeCoveredTile(t, isRoomAbsorb);
    // Don't recompute the position.
    // Removing a portal tile usually means some creatures are attacking it.
    // The portal shouldn't move in that case.
    //recomputeCenterPosition();
}

void RoomPortal::doUpkeep()
{
    // Call the super class Room::doUpkeep() function to do any generic upkeep common to all rooms.
    Room::doUpkeep();

    if (mCoveredTiles.empty())
        return;

    if (mSpawnCreatureCountdown > 0)
    {
        --mSpawnCreatureCountdown;
        return;
    }

    // Randomly choose to spawn a creature.
    const double maxCreatures = 15;
    //TODO:  Improve this probability calculation.
    // Count how many creatures are controlled by this seat
    double numCreatures = getGameMap()->getCreaturesBySeat(getSeat()).size();
    double targetProbability = powl((maxCreatures - numCreatures) / maxCreatures, 1.5);
    if (Random::Double(0.0, 1.0) <= targetProbability)
        spawnCreature();
}

void RoomPortal::spawnCreature()
{
    std::cout << "Portal: " << getName() << "  spawn creature..." << std::endl;

    if (mPortalObject != NULL)
        mPortalObject->setAnimationState("Spawn", false);

    // If the room has been destroyed, or has not yet been assigned any tiles, then we
    // cannot determine where to place the new creature and we should just give up.
    if (mCoveredTiles.empty())
        return;

    // We check if a creature can spawn
    Seat* seat = getSeat();
    const CreatureDefinition* classToSpawn = seat->getNextCreatureClassToSpawn();
    if (classToSpawn == nullptr)
        return;

    // Create a new creature and copy over the class-based creature parameters.
    Creature *newCreature = new Creature(getGameMap(), classToSpawn);

    LogManager::getSingleton().logMessage("Spawning a creature class=" + classToSpawn->getClassName()
        + ", name=" + newCreature->getName() + ", seatId=" + Ogre::StringConverter::toString(getSeat()->getId()));

    // Set the creature specific parameters.
    //NOTE:  This needs to be modified manually when the level file creature format changes.
    newCreature->setPosition(Ogre::Vector3((Ogre::Real)mXCenter, (Ogre::Real)mYCenter, (Ogre::Real)0.0));
    newCreature->setSeat(getSeat());

    // Add the creature to the gameMap and create meshes so it is visible.
    getGameMap()->addCreature(newCreature);
    newCreature->createMesh();

    mSpawnCreatureCountdown = Random::Uint(15, 30);

    // Inform the clients
    if (getGameMap()->isServerGameMap())
    {
        try
        {
           ServerNotification *serverNotification = new ServerNotification(
               ServerNotification::addCreature, newCreature->getGameMap()->getPlayerBySeat(newCreature->getSeat()));
           newCreature->exportToPacket(serverNotification->mPacket);
           ODServer::getSingleton().queueServerNotification(serverNotification);
        }
        catch (std::bad_alloc&)
        {
            Ogre::LogManager::getSingleton().logMessage("ERROR: bad alloc in RoomDungeonTemple::produceKobold", Ogre::LML_CRITICAL);
            exit(1);
        }
    }
}

void RoomPortal::recomputeCenterPosition()
{
    mXCenter = mYCenter = 0.0;

    if (mCoveredTiles.empty())
        return;

    // Loop over the covered tiles and compute the average location (i.e. the center) of the portal.
    for (unsigned int i = 0; i < mCoveredTiles.size(); ++i)
    {
        Tile *tempTile = mCoveredTiles[i];
        mXCenter += tempTile->x;
        mYCenter += tempTile->y;
    }

    mXCenter /= static_cast<double>(mCoveredTiles.size());
    mYCenter /= static_cast<double>(mCoveredTiles.size());
}
