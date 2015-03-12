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
#include "entities/ResearchEntity.h"
#include "game/Player.h"
#include "network/ODServer.h"
#include "network/ServerNotification.h"
#include "modes/ModeManager.h"
#include "utils/LogManager.h"
#include "utils/ConfigManager.h"

RoomDungeonTemple::RoomDungeonTemple(GameMap* gameMap) :
    Room(gameMap),
    mTempleObject(nullptr)
{
    setMeshName("DungeonTemple");
}

void RoomDungeonTemple::updateActiveSpots()
{
    // Room::updateActiveSpots(); <<-- Disabled on purpose.
    // We don't update the active spots the same way as only the central tile is needed.
    if (getGameMap()->isInEditorMode())
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

bool RoomDungeonTemple::hasCarryEntitySpot(GameEntity* carriedEntity)
{
    if(carriedEntity->getObjectType() != GameEntityType::researchEntity)
        return false;

    // We accept any researchEntity
    return true;
}

Tile* RoomDungeonTemple::askSpotForCarriedEntity(GameEntity* carriedEntity)
{
    OD_ASSERT_TRUE_MSG(carriedEntity->getObjectType() == GameEntityType::researchEntity,
        "room=" + getName() + ", entity=" + carriedEntity->getName());
    if(carriedEntity->getObjectType() != GameEntityType::researchEntity)
        return nullptr;

    // We accept any researchEntity
    return getCoveredTile(0);
}

void RoomDungeonTemple::notifyCarryingStateChanged(Creature* carrier, GameEntity* carriedEntity)
{
    OD_ASSERT_TRUE_MSG(carriedEntity->getObjectType() == GameEntityType::researchEntity,
        "room=" + getName() + ", entity=" + carriedEntity->getName());
    if(carriedEntity->getObjectType() != GameEntityType::researchEntity)
        return;

    // We check if the carrier is at the expected destination. If not on the first tile,
    // we don't accept the researchEntity
    // Note that if the room is being destroyed, during the transport, the researchEntity
    // will be dropped at its original place and will become available again so there
    // should be no problem
    Tile* carrierTile = carrier->getPositionTile();
    if(carrierTile != getCoveredTile(0))
        return;

    // We notify the player that the research is now available and we delete the researchEntity
    ResearchEntity* researchEntity = static_cast<ResearchEntity*>(carriedEntity);
    getSeat()->addResearch(researchEntity->getResearchType());
    researchEntity->removeFromGameMap();
    researchEntity->deleteYourself();
}
