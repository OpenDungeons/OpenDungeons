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

#include "game/Seat.h"
#include "gamemap/GameMap.h"
#include "entities/Creature.h"
#include "entities/GiftBoxEntity.h"
#include "entities/PersistentObject.h"
#include "entities/ResearchEntity.h"
#include "entities/Tile.h"
#include "rooms/RoomManager.h"
#include "utils/LogManager.h"

static RoomManagerRegister<RoomDungeonTemple> reg(RoomType::dungeonTemple, "DungeonTemple");

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
    else
    {
        if(mTempleObject == nullptr)
        {
            // We check if the temple already exists (that can happen if it has
            // been restored after restoring a saved game)
            if(mBuildingObjects.empty())
                updateTemplePosition();
            else
            {
                for(std::pair<Tile* const, RenderedMovableEntity*>& p : mBuildingObjects)
                {
                    if(p.second == nullptr)
                        continue;

                    // We take the first RenderedMovableEntity. Note that we cannot use
                    // the central tile because after saving a game, the central tile may
                    // not be the same if some tiles have been destroyed
                    mTempleObject = p.second;
                    break;
                }
            }
        }
    }
}

void RoomDungeonTemple::updateTemplePosition()
{
    // Only the server game map should load objects.
    if (!getIsOnServerMap())
        return;

    // Delete all previous rooms meshes and recreate a central one.
    removeAllBuildingObjects();
    mTempleObject = nullptr;

    Tile* centralTile = getCentralTile();
    if (centralTile == nullptr)
        return;

    mTempleObject = new PersistentObject(getGameMap(), true, getName(), "DungeonTempleObject", centralTile, 0.0, false);
    addBuildingObject(centralTile, mTempleObject);
}

void RoomDungeonTemple::destroyMeshLocal()
{
    Room::destroyMeshLocal();
    mTempleObject = nullptr;
}

bool RoomDungeonTemple::hasCarryEntitySpot(GameEntity* carriedEntity)
{
    switch(carriedEntity->getObjectType())
    {
        case GameEntityType::giftBoxEntity:
        case GameEntityType::researchEntity:
            return true;
        default:
            return false;
    }
}

Tile* RoomDungeonTemple::askSpotForCarriedEntity(GameEntity* carriedEntity)
{
    switch(carriedEntity->getObjectType())
    {
        case GameEntityType::giftBoxEntity:
        case GameEntityType::researchEntity:
            return getCentralTile();
        default:
            OD_LOG_ERR("room=" + getName() + ", entity=" + carriedEntity->getName());
            return nullptr;
    }
}

void RoomDungeonTemple::notifyCarryingStateChanged(Creature* carrier, GameEntity* carriedEntity)
{
    // We check if the carrier is at the expected destination. If not on the wanted tile,
    // we don't accept the entity
    // Note that if the wanted tile were to move during the transport, the carried entity
    // will be dropped at its original destination and will become available again so there
    // should be no problem
    Tile* carrierTile = carrier->getPositionTile();
    if(carrierTile != getCentralTile())
        return;

    switch(carriedEntity->getObjectType())
    {
        case GameEntityType::giftBoxEntity:
        {
            // We apply the gift box effect
            GiftBoxEntity* giftBox = static_cast<GiftBoxEntity*>(carriedEntity);
            giftBox->applyEffect();
            giftBox->removeEntityFromPositionTile();
            giftBox->removeFromGameMap();
            giftBox->deleteYourself();
            return;
        }
        case GameEntityType::researchEntity:
        {
            // We notify the player that the research is now available and we delete the researchEntity
            ResearchEntity* researchEntity = static_cast<ResearchEntity*>(carriedEntity);
            getSeat()->addResearchPoints(researchEntity->getResearchPoints());
            researchEntity->removeEntityFromPositionTile();
            researchEntity->removeFromGameMap();
            researchEntity->deleteYourself();
            return;
        }
        default:
            OD_LOG_ERR("room=" + getName() + ", entity=" + carriedEntity->getName());
            return;
    }
}

void RoomDungeonTemple::restoreInitialEntityState()
{
    // We need to use seats with vision before calling Room::restoreInitialEntityState
    // because it will empty the list
    if(mTempleObject == nullptr)
    {
        OD_LOG_ERR("roomDungeonTemple=" + getName());
        return;
    }

    Tile* tileTempleObject = mTempleObject->getPositionTile();
    if(tileTempleObject == nullptr)
    {
        OD_LOG_ERR("roomDungeonTemple=" + getName() + ", mTempleObject=" + mTempleObject->getName());
        return;
    }
    TileData* tileData = mTileData[tileTempleObject];
    if(tileData == nullptr)
    {
        OD_LOG_ERR("roomDungeonTemple=" + getName() + ", tile=" + Tile::displayAsString(tileTempleObject));
        return;
    }

    if(!tileData->mSeatsVision.empty())
        mTempleObject->notifySeatsWithVision(tileData->mSeatsVision);

    // If there are no covered tile, the temple object is not working
    if(numCoveredTiles() == 0)
        mTempleObject->notifyRemoveAsked();

    Room::restoreInitialEntityState();
}

void RoomDungeonTemple::checkBuildRoom(GameMap* gameMap, const InputManager& inputManager, InputCommand& inputCommand)
{
    // Not buildable in game mode
}

bool RoomDungeonTemple::buildRoom(GameMap* gameMap, Player* player, ODPacket& packet)
{
    // Not buildable in game mode
    return false;
}

void RoomDungeonTemple::checkBuildRoomEditor(GameMap* gameMap, const InputManager& inputManager, InputCommand& inputCommand)
{
    checkBuildRoomDefaultEditor(gameMap, RoomType::dungeonTemple, inputManager, inputCommand);
}

bool RoomDungeonTemple::buildRoomEditor(GameMap* gameMap, ODPacket& packet)
{
    RoomDungeonTemple* room = new RoomDungeonTemple(gameMap);
    return buildRoomDefaultEditor(gameMap, room, packet);
}

Room* RoomDungeonTemple::getRoomFromStream(GameMap* gameMap, std::istream& is)
{
    return new RoomDungeonTemple(gameMap);
}
