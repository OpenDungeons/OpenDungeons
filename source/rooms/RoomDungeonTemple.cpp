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

#include "rooms/RoomDungeonTemple.h"

#include "gamemap/GameMap.h"
#include "entities/Creature.h"
#include "entities/Weapon.h"
#include "entities/CreatureAction.h"
#include "entities/CreatureSound.h"
#include "entities/PersistentObject.h"
#include "network/ODServer.h"
#include "network/ServerNotification.h"
#include "render/ODFrameListener.h"
#include "modes/ModeManager.h"
#include "utils/LogManager.h"

RoomDungeonTemple::RoomDungeonTemple(GameMap* gameMap) :
    Room(gameMap),
    mWaitTurns(0),
    mTempleObject(nullptr)
{
    setMeshName("DungeonTemple");
}

void RoomDungeonTemple::updateActiveSpots()
{
    // Room::updateActiveSpots(); <<-- Disabled on purpose.
    // We don't update the active spots the same way as only the central tile is needed.
    if (ODFrameListener::getSingleton().getModeManager()->getCurrentModeType() == ModeManager::ModeType::EDITOR)
        updateTemplePosition();
}

void RoomDungeonTemple::updateTemplePosition()
{
    // Only the server game map should load objects.
    if (!getGameMap()->isServerGameMap())
        return;

    // Delete all previous rooms meshes and recreate a central one.
    removeAllBuildingObjects();
    mTempleObject = nullptr;

    Tile* centralTile = getCentralTile();
    if (centralTile == nullptr)
        return;

    mTempleObject = new PersistentObject(getGameMap(), getName(), "DungeonTempleObject", centralTile, 0.0, false);
    addBuildingObject(centralTile, mTempleObject);
}

void RoomDungeonTemple::createMeshLocal()
{
    Room::createMeshLocal();
    updateTemplePosition();
}

void RoomDungeonTemple::destroyMeshLocal()
{
    Room::destroyMeshLocal();
    mTempleObject = nullptr;
}

void RoomDungeonTemple::produceKobold()
{
    // If the room has been destroyed, or has not yet been assigned any tiles, then we
    // cannot determine where to place the new creature and we should just give up.
    if (mCoveredTiles.empty())
        return;

    Tile* centralTile = getCentralTile();
    if (centralTile == nullptr)
        return;

    if (mWaitTurns > 0)
    {
        --mWaitTurns;
        return;
    }

    mWaitTurns = 30;

    // Create a new creature and copy over the class-based creature parameters.
    const CreatureDefinition *classToSpawn = getGameMap()->getClassDescription("Kobold");
    Creature* newCreature = new Creature(getGameMap(), classToSpawn);
    if (classToSpawn == nullptr)
    {
        LogManager::getSingleton().logMessage("Error: No worker creature definition, class=" + classToSpawn->getClassName()
            + ", name=" + newCreature->getName() + ", seatId=" + Ogre::StringConverter::toString(getSeat()->getId()));

        delete newCreature;
        return;
    }

    LogManager::getSingleton().logMessage("Spawning a creature class=" + classToSpawn->getClassName()
        + ", name=" + newCreature->getName() + ", seatId=" + Ogre::StringConverter::toString(getSeat()->getId()));

    newCreature->setSeat(getSeat());
    getGameMap()->addCreature(newCreature);
    Ogre::Vector3 spawnPosition(static_cast<Ogre::Real>(centralTile->getX()),
                                static_cast<Ogre::Real>(centralTile->getY()),
                                static_cast<Ogre::Real>(0.0));
    newCreature->createMesh();
    newCreature->setPosition(spawnPosition, false);
}
