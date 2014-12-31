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
#include "utils/ConfigManager.h"
#include "utils/Random.h"
#include "utils/LogManager.h"

#include <cmath>

RoomPortal::RoomPortal(GameMap* gameMap) :
        Room(gameMap),
        mSpawnCreatureCountdown(0),
        mXCenter(0),
        mYCenter(0),
        mPortalObject(nullptr)
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
        mPortalObject = nullptr;
}

void RoomPortal::createMeshLocal()
{
    Room::createMeshLocal();

    // Don't recreate the portal if it's already done.
    if (mPortalObject != nullptr)
        return;

    // The client game map should not load building objects. They will be created
    // by the messages sent by the server because some of them are randomly created
    if(!getGameMap()->isServerGameMap())
        return;

    mPortalObject = loadBuildingObject(getGameMap(), "PortalObject", getCentralTile(), 0.0, false);
    addBuildingObject(getCentralTile(), mPortalObject);

    mPortalObject->setAnimationState("Idle");
}

void RoomPortal::destroyMeshLocal()
{
    Room::destroyMeshLocal();
    mPortalObject = nullptr;
}

void RoomPortal::addCoveredTile(Tile* t, double nHP)
{
    Room::addCoveredTile(t, nHP);
    recomputeCenterPosition();
}

bool RoomPortal::removeCoveredTile(Tile* t)
{
    return Room::removeCoveredTile(t);
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
        mPortalObject->setAnimationState("Idle");
        return;
    }

    // Randomly choose to spawn a creature.
    const double maxCreatures = ConfigManager::getSingleton().getMaxCreaturesPerSeat();
    // Count how many creatures are controlled by this seat
    double numCreatures = getGameMap()->getCreaturesBySeat(getSeat()).size();
    double targetProbability = powl((maxCreatures - numCreatures) / maxCreatures, 1.5);
    if (Random::Double(0.0, 1.0) <= targetProbability)
        spawnCreature();
}

void RoomPortal::spawnCreature()
{
    if (mPortalObject != nullptr)
        mPortalObject->setAnimationState("Triggered", false);

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

    newCreature->setSeat(getSeat());

    getGameMap()->addCreature(newCreature);
    Ogre::Vector3 spawnPosition(static_cast<Ogre::Real>(mXCenter), static_cast<Ogre::Real>(mYCenter), 0.0f);
    newCreature->createMesh();
    newCreature->setPosition(spawnPosition, false);

    mSpawnCreatureCountdown = Random::Uint(15, 30);
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
        mXCenter += tempTile->getX();
        mYCenter += tempTile->getY();
    }

    mXCenter /= static_cast<double>(mCoveredTiles.size());
    mYCenter /= static_cast<double>(mCoveredTiles.size());
}
