/*
 *  Copyright (C) 2011-2015  OpenDungeons Team
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
#include "entities/PersistentObject.h"
#include "entities/RenderedMovableEntity.h"
#include "gamemap/GameMap.h"
#include "entities/Weapon.h"
#include "entities/CreatureAction.h"
#include "utils/ConfigManager.h"
#include "utils/Random.h"
#include "utils/LogManager.h"
#include "render/ODFrameListener.h"
#include "modes/ModeManager.h"

#include <cmath>

RoomPortal::RoomPortal(GameMap* gameMap) :
        Room(gameMap),
        mSpawnCreatureCountdown(0),
        mPortalObject(nullptr)
{
   setMeshName("Portal");
}

void RoomPortal::updateActiveSpots()
{
    // Room::updateActiveSpots(); <<-- Disabled on purpose.
    // We don't update the active spots the same way as only the central tile is needed.
    if (ODFrameListener::getSingleton().getModeManager()->getCurrentModeType() == ModeManager::ModeType::EDITOR)
        updatePortalPosition();
}

void RoomPortal::updatePortalPosition()
{
    // Only the server game map should load objects.
    if (!getGameMap()->isServerGameMap())
        return;

    // Delete all previous rooms meshes and recreate a central one.
    removeAllBuildingObjects();
    mPortalObject = nullptr;

    Tile* centralTile = getCentralTile();
    if (centralTile == nullptr)
        return;

    mPortalObject = new PersistentObject(getGameMap(), getName(), "PortalObject", centralTile, 0.0, false);
    addBuildingObject(centralTile, mPortalObject);

    mPortalObject->setAnimationState("Idle");
}

void RoomPortal::createMeshLocal()
{
    Room::createMeshLocal();
    updatePortalPosition();
}

void RoomPortal::destroyMeshLocal()
{
    Room::destroyMeshLocal();
    mPortalObject = nullptr;
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
    // If the room has been destroyed, or has not yet been assigned any tiles, then we
    // cannot determine where to place the new creature and we should just give up.
    if (mCoveredTiles.empty())
        return;

    // We check if a creature can spawn
    Seat* seat = getSeat();
    const CreatureDefinition* classToSpawn = seat->getNextCreatureClassToSpawn();
    if (classToSpawn == nullptr)
        return;

    Tile* centralTile = getCentralTile();
    if (centralTile == nullptr)
        return;

    if (mPortalObject != nullptr)
        mPortalObject->setAnimationState("Triggered", false);

    // Create a new creature and copy over the class-based creature parameters.
    Creature* newCreature = new Creature(getGameMap(), classToSpawn);

    LogManager::getSingleton().logMessage("Spawning a creature class=" + classToSpawn->getClassName()
        + ", name=" + newCreature->getName() + ", seatId=" + Ogre::StringConverter::toString(getSeat()->getId()));

    Ogre::Real xPos = static_cast<Ogre::Real>(centralTile->getX());
    Ogre::Real yPos = static_cast<Ogre::Real>(centralTile->getY());

    newCreature->setSeat(getSeat());
    getGameMap()->addCreature(newCreature);
    Ogre::Vector3 spawnPosition(xPos, yPos, 0.0f);
    newCreature->createMesh();
    newCreature->setPosition(spawnPosition, false);

    mSpawnCreatureCountdown = Random::Uint(15, 30);
}
