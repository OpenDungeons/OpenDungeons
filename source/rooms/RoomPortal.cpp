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

#include "entities/Creature.h"
#include "entities/CreatureDefinition.h"
#include "entities/PersistentObject.h"
#include "entities/RenderedMovableEntity.h"
#include "entities/Tile.h"
#include "game/Player.h"
#include "game/Seat.h"
#include "gamemap/GameMap.h"
#include "rooms/RoomManager.h"
#include "utils/ConfigManager.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"
#include "utils/Random.h"

#include <cmath>

static RoomManagerRegister<RoomPortal> reg(RoomType::portal, "Portal");

const double CLAIMED_VALUE_PER_TILE = 1.0;

RoomPortal::RoomPortal(GameMap* gameMap) :
        Room(gameMap),
        mSpawnCreatureCountdown(0),
        mPortalObject(nullptr),
        mClaimedValue(0),
        mNbCreatureMaxIncrease(0)
{
   setMeshName("Portal");
}

void RoomPortal::absorbRoom(Room *r)
{
    if(r->getType() != getType())
    {
        OD_LOG_ERR("Trying to merge incompatible rooms: " + getName() + ", type=" + RoomManager::getRoomNameFromRoomType(getType()) + ", with " + r->getName() + ", type=" + RoomManager::getRoomNameFromRoomType(r->getType()));
        return;
    }
    RoomPortal* oldRoom = static_cast<RoomPortal*>(r);
    mClaimedValue += oldRoom->mClaimedValue;
    // We keep the number of creatures increased by this portal
    mNbCreatureMaxIncrease = oldRoom->mNbCreatureMaxIncrease;

    Room::absorbRoom(r);
}

bool RoomPortal::removeCoveredTile(Tile* t)
{
    if(mClaimedValue > CLAIMED_VALUE_PER_TILE)
        mClaimedValue -= CLAIMED_VALUE_PER_TILE;

    return Room::removeCoveredTile(t);
}

bool RoomPortal::isClaimable(Seat* seat) const
{
    if(getSeat()->canBuildingBeDestroyedBy(seat))
        return false;

    return true;
}

void RoomPortal::claimForSeat(Seat* seat, Tile* tile, double danceRate)
{
    if(mClaimedValue > danceRate)
    {
        mClaimedValue-= danceRate;
        return;
    }

    mClaimedValue = static_cast<double>(numCoveredTiles());
    setSeat(seat);

    for(Tile* tile : mCoveredTiles)
        tile->claimTile(seat);
}

void RoomPortal::updateActiveSpots()
{
    // Room::updateActiveSpots(); <<-- Disabled on purpose.
    // We don't update the active spots the same way as only the central tile is needed.
    if (getGameMap()->isInEditorMode())
        updatePortalPosition();
    else
    {
        if(mPortalObject == nullptr)
        {
            // We check if the portal already exists (that can happen if it has
            // been restored after restoring a saved game)
            if(mBuildingObjects.empty())
                updatePortalPosition();
            else
            {
                for(std::pair<Tile* const, RenderedMovableEntity*>& p : mBuildingObjects)
                {
                    if(p.second == nullptr)
                        continue;

                    // We take the first RenderedMovableEntity. Note that we cannot use
                    // the central tile because after saving a game, the central tile may
                    // not be the same if some tiles have been destroyed
                    mPortalObject = p.second;
                    break;
                }
            }
        }
    }
}

void RoomPortal::updatePortalPosition()
{
    // Delete all previous rooms meshes and recreate a central one.
    removeAllBuildingObjects();
    mPortalObject = nullptr;

    Tile* centralTile = getCentralTile();
    if (centralTile == nullptr)
        return;

    mPortalObject = new PersistentObject(getGameMap(), true, getName(), "PortalObject", centralTile, 0.0, false);
    addBuildingObject(centralTile, mPortalObject);

    mPortalObject->setAnimationState("Idle");
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

    if(getSeat()->getPlayer() == nullptr)
        return;

    if(getSeat()->getPlayer()->getHasLost())
        return;

    // Randomly choose to spawn a creature.
    const double maxCreatures = getSeat()->getNumCreaturesFightersMax();
    // Count how many creatures are controlled by this seat
    double numCreatures = getGameMap()->getNbFightersForSeat(getSeat());
    if(numCreatures >= maxCreatures)
        return;

    double targetProbability = powl((maxCreatures - numCreatures) / maxCreatures, 1.5);
    if (Random::Double(0.0, 1.0) <= targetProbability)
        spawnCreature();
}

void RoomPortal::spawnCreature()
{
    // We check if a creature can spawn
    const CreatureDefinition* classToSpawn = getSeat()->getNextFighterClassToSpawn(*getGameMap(), ConfigManager::getSingleton());
    if (classToSpawn == nullptr)
        return;

    Tile* centralTile = getCentralTile();
    if (centralTile == nullptr)
        return;

    if (mPortalObject != nullptr)
        mPortalObject->setAnimationState("Triggered", false);

    Ogre::Real xPos = static_cast<Ogre::Real>(centralTile->getX());
    Ogre::Real yPos = static_cast<Ogre::Real>(centralTile->getY());

    // Create a new creature and copy over the class-based creature parameters.
    Creature* newCreature = new Creature(getGameMap(), true, classToSpawn, getSeat(), Ogre::Vector3(xPos, yPos, 0.0f));

    OD_LOG_INF("RoomPortal name=" + getName()
        + "spawns a creature class=" + classToSpawn->getClassName()
        + ", name=" + newCreature->getName() + ", seatId=" + Helper::toString(getSeat()->getId()));

    newCreature->addToGameMap();
    newCreature->createMesh();
    newCreature->setPosition(newCreature->getPosition(), false);

    mSpawnCreatureCountdown = Random::Uint(30, 50);
}

void RoomPortal::setupRoom(const std::string& name, Seat* seat, const std::vector<Tile*>& tiles)
{
    Room::setupRoom(name, seat, tiles);
    mClaimedValue = static_cast<double>(numCoveredTiles()) * CLAIMED_VALUE_PER_TILE;
    // By default, we allow some more creatures per portal
    mNbCreatureMaxIncrease = 5;
}

void RoomPortal::exportToStream(std::ostream& os) const
{
    Room::exportToStream(os);

    os << mClaimedValue << "\t" << mNbCreatureMaxIncrease << "\n";
}

void RoomPortal::importFromStream(std::istream& is)
{
    Room::importFromStream(is);

    OD_ASSERT_TRUE(is >> mClaimedValue);
    OD_ASSERT_TRUE(is >> mNbCreatureMaxIncrease);
}

void RoomPortal::restoreInitialEntityState()
{
    // We need to use seats with vision before calling Room::restoreInitialEntityState
    // because it will empty the list
    if(mPortalObject == nullptr)
    {
        OD_LOG_ERR("roomPortal=" + getName());
        return;
    }

    Tile* tilePortalObject = mPortalObject->getPositionTile();
    if(tilePortalObject == nullptr)
    {
        OD_LOG_ERR("roomPortal=" + getName() + ", mPortalObject=" + mPortalObject->getName());
        return;
    }
    TileData* tileData = mTileData[tilePortalObject];
    if(tileData == nullptr)
    {
        OD_LOG_ERR("roomPortal=" + getName() + ", tile=" + Tile::displayAsString(tilePortalObject));
        return;
    }

    if(!tileData->mSeatsVision.empty())
        mPortalObject->notifySeatsWithVision(tileData->mSeatsVision);

    // If there are no covered tile, the temple object is not working
    if(numCoveredTiles() == 0)
        mPortalObject->notifyRemoveAsked();

    Room::restoreInitialEntityState();
}

void RoomPortal::checkBuildRoom(GameMap* gameMap, const InputManager& inputManager, InputCommand& inputCommand)
{
    // Not buildable on game mode
}

bool RoomPortal::buildRoom(GameMap* gameMap, Player* player, ODPacket& packet)
{
    // Not buildable on game mode
    return false;
}

void RoomPortal::checkBuildRoomEditor(GameMap* gameMap, const InputManager& inputManager, InputCommand& inputCommand)
{
    return checkBuildRoomDefaultEditor(gameMap, RoomType::portal, inputManager, inputCommand);
}

bool RoomPortal::buildRoomEditor(GameMap* gameMap, ODPacket& packet)
{
    RoomPortal* room = new RoomPortal(gameMap);
    return buildRoomDefaultEditor(gameMap, room, packet);
}

Room* RoomPortal::getRoomFromStream(GameMap* gameMap, std::istream& is)
{
    return new RoomPortal(gameMap);
}
