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

#include "entities/BuildingObject.h"
#include "entities/Creature.h"
#include "entities/CreatureDefinition.h"
#include "entities/PersistentObject.h"
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

const std::string RoomPortalName = "Portal";
const std::string RoomPortalNameDisplay = "Portal room";
const RoomType RoomPortal::mRoomType = RoomType::portal;

namespace
{
class RoomPortalFactory : public RoomFactory
{
    RoomType getRoomType() const override
    { return RoomPortal::mRoomType; }

    const std::string& getName() const override
    { return RoomPortalName; }

    const std::string& getNameReadable() const override
    { return RoomPortalNameDisplay; }

    int getCostPerTile() const override
    { return 0; }

    void checkBuildRoom(GameMap* gameMap, const InputManager& inputManager, InputCommand& inputCommand) const override
    {
        // Not buildable on game mode
    }

    bool buildRoom(GameMap* gameMap, Player* player, ODPacket& packet) const override
    {
        // Not buildable on game mode
        return false;
    }

    void checkBuildRoomEditor(GameMap* gameMap, const InputManager& inputManager, InputCommand& inputCommand) const override
    {
        return checkBuildRoomDefaultEditor(gameMap, RoomPortal::mRoomType, inputManager, inputCommand);
    }

    bool buildRoomEditor(GameMap* gameMap, ODPacket& packet) const override
    {
        RoomPortal* room = new RoomPortal(gameMap);
        return buildRoomDefaultEditor(gameMap, room, packet);
    }

    Room* getRoomFromStream(GameMap* gameMap, std::istream& is) const override
    {
        RoomPortal* room = new RoomPortal(gameMap);
        if(!Room::importRoomFromStream(*room, is))
        {
            OD_LOG_ERR("Error while building a room from the stream");
        }
        return room;
    }

    bool buildRoomOnTiles(GameMap* gameMap, Player* player, const std::vector<Tile*>& tiles) const override
    {
        // Not buildable in game mode
        return false;
    }
};

// Register the factory
static RoomRegister reg(new RoomPortalFactory);
}

static const double CLAIMED_VALUE_PER_TILE = 1.0;
static const Ogre::Vector3 SCALE(0.7,0.7,0.7);

RoomPortal::RoomPortal(GameMap* gameMap) :
        Room(gameMap),
        mSpawnCreatureCountdown(0),
        mPortalObject(nullptr),
        mClaimedValue(0),
        mNbCreatureMaxIncrease(0)
{
   setMeshName("");
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
    return !getSeat()->isAlliedSeat(seat);
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
                for(auto& p : mBuildingObjects)
                {
                    if(p.second == nullptr)
                        continue;

                    // We take the first BuildingObject. Note that we cannot use
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

    mPortalObject = new PersistentObject(getGameMap(), true, getName(), "PortalObject",
        centralTile, 0.0, SCALE, false, 1.0f, "Idle", true);
    addBuildingObject(centralTile, mPortalObject);
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

    if(mSpawnCreatureCountdown > 0)
    {
        --mSpawnCreatureCountdown;
        return;
    }
    mSpawnCreatureCountdown = Random::Uint(ConfigManager::getSingleton().getRoomConfigUInt32("PortalCooldownSpawnMin"),
        ConfigManager::getSingleton().getRoomConfigUInt32("PortalCooldownSpawnMax"));

    if (mCoveredTiles.empty())
        return;

    // Rogue seat cannot spawn creatures through normal portal (they won't spawn
    // anyway since there is no spawning list for rogue seat but no need to compute
    // everything in this case)
    if(getSeat()->isRogueSeat())
        return;

    if(getSeat()->getPlayer() == nullptr)
        return;

    if(getSeat()->getPlayer()->getHasLost())
        return;

    // Randomly choose to spawn a creature.
    const double maxCreatures = getSeat()->getNumCreaturesFightersMax();
    // Count how many creatures are controlled by this seat
    double numCreatures = getSeat()->getNumCreaturesFighters();
    if(numCreatures >= maxCreatures)
        return;

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
        + " spawns a creature class=" + classToSpawn->getClassName()
        + ", name=" + newCreature->getName() + ", seatId=" + Helper::toString(getSeat()->getId()));

    newCreature->addToGameMap();
    newCreature->createMesh();
    newCreature->setPosition(newCreature->getPosition());
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

bool RoomPortal::importFromStream(std::istream& is)
{
    if(!Room::importFromStream(is))
        return false;

    if(!(is >> mClaimedValue))
        return false;
    if(!(is >> mNbCreatureMaxIncrease))
        return false;

    return true;
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
