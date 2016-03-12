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

#include "rooms/RoomTorture.h"

#include "creatureaction/CreatureActionUseRoom.h"
#include "entities/BuildingObject.h"
#include "entities/Creature.h"
#include "entities/GameEntityType.h"
#include "entities/Tile.h"
#include "game/Player.h"
#include "game/Seat.h"
#include "gamemap/GameMap.h"
#include "network/ODServer.h"
#include "network/ServerNotification.h"
#include "rooms/RoomManager.h"
#include "utils/ConfigManager.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"
#include "utils/MakeUnique.h"
#include "utils/Random.h"

const std::string RoomTortureName = "Torture";
const std::string RoomTortureNameDisplay = "Torture room";
const RoomType RoomTorture::mRoomType = RoomType::torture;

namespace
{
class RoomTortureFactory : public RoomFactory
{
    RoomType getRoomType() const override
    { return RoomTorture::mRoomType; }

    const std::string& getName() const override
    { return RoomTortureName; }

    const std::string& getNameReadable() const override
    { return RoomTortureNameDisplay; }

    int getCostPerTile() const override
    { return ConfigManager::getSingleton().getRoomConfigInt32("TortureCostPerTile"); }

    void checkBuildRoom(GameMap* gameMap, const InputManager& inputManager, InputCommand& inputCommand) const override
    {
        checkBuildRoomDefault(gameMap, RoomTorture::mRoomType, inputManager, inputCommand);
    }

    bool buildRoom(GameMap* gameMap, Player* player, ODPacket& packet) const override
    {
        std::vector<Tile*> tiles;
        if(!getRoomTilesDefault(tiles, gameMap, player, packet))
            return false;

        int32_t pricePerTarget = RoomManager::costPerTile(RoomTorture::mRoomType);
        int32_t price = static_cast<int32_t>(tiles.size()) * pricePerTarget;
        if(!gameMap->withdrawFromTreasuries(price, player->getSeat()))
            return false;

        RoomTorture* room = new RoomTorture(gameMap);
        return buildRoomDefault(gameMap, room, player->getSeat(), tiles);
    }

    void checkBuildRoomEditor(GameMap* gameMap, const InputManager& inputManager, InputCommand& inputCommand) const override
    {
        checkBuildRoomDefaultEditor(gameMap, RoomTorture::mRoomType, inputManager, inputCommand);
    }

    bool buildRoomEditor(GameMap* gameMap, ODPacket& packet) const override
    {
        RoomTorture* room = new RoomTorture(gameMap);
        return buildRoomDefaultEditor(gameMap, room, packet);
    }

    Room* getRoomFromStream(GameMap* gameMap, std::istream& is) const override
    {
        RoomTorture* room = new RoomTorture(gameMap);
        if(!Room::importRoomFromStream(*room, is))
        {
            OD_LOG_ERR("Error while building a room from the stream");
        }
        return room;
    }

    bool buildRoomOnTiles(GameMap* gameMap, Player* player, const std::vector<Tile*>& tiles) const override
    {
        int32_t pricePerTarget = RoomManager::costPerTile(RoomTorture::mRoomType);
        int32_t price = static_cast<int32_t>(tiles.size()) * pricePerTarget;
        if(!gameMap->withdrawFromTreasuries(price, player->getSeat()))
            return false;

        RoomTorture* room = new RoomTorture(gameMap);
        return buildRoomDefault(gameMap, room, player->getSeat(), tiles);
    }
};

// Register the factory
static RoomRegister reg(new RoomTortureFactory);
}

static const Ogre::Vector3 SCALE_ACTIVESPOT_CENTER(0.3,0.3,0.3);
static const Ogre::Vector3 SCALE_ACTIVESPOT_WALL(0.7,0.7,0.7);

RoomTorture::RoomTorture(GameMap* gameMap) :
    Room(gameMap)
{
    setMeshName("TortureGround");
}

BuildingObject* RoomTorture::notifyActiveSpotCreated(ActiveSpotPlace place, Tile* tile)
{
    switch(place)
    {
        case ActiveSpotPlace::activeSpotCenter:
        {
            Ogre::Real x = static_cast<Ogre::Real>(tile->getX());
            Ogre::Real y = static_cast<Ogre::Real>(tile->getY());
            mCreaturesSpots.emplace(std::make_pair(tile, RoomTortureCreatureInfo()));
            return loadBuildingObject(getGameMap(), "Roulette", tile, x, y, 0.0, SCALE_ACTIVESPOT_CENTER, false);
        }
        case ActiveSpotPlace::activeSpotLeft:
        {
            return loadBuildingObject(getGameMap(), "Chimney", tile, 90.0, SCALE_ACTIVESPOT_WALL, false);
        }
        case ActiveSpotPlace::activeSpotRight:
        {
            return loadBuildingObject(getGameMap(), "Chimney", tile, 270.0, SCALE_ACTIVESPOT_WALL, false);
        }
        case ActiveSpotPlace::activeSpotTop:
        {
            return loadBuildingObject(getGameMap(), "Chimney", tile, 0.0, SCALE_ACTIVESPOT_WALL, false);
        }
        case ActiveSpotPlace::activeSpotBottom:
        {
            return loadBuildingObject(getGameMap(), "Chimney", tile, 180.0, SCALE_ACTIVESPOT_WALL, false);
        }
        default:
            break;
    }
    return nullptr;
}

void RoomTorture::absorbRoom(Room* room)
{
    if(room->getType() != getType())
    {
        OD_LOG_ERR("Trying to merge incompatible rooms: " + getName() + ", type=" + RoomManager::getRoomNameFromRoomType(getType()) + ", with " + room->getName() + ", type=" + RoomManager::getRoomNameFromRoomType(room->getType()));
        return;
    }

    Room::absorbRoom(room);

    RoomTorture* roomAbs = static_cast<RoomTorture*>(room);
    OD_ASSERT_TRUE_MSG(roomAbs->mCreaturesSpots.empty(), "room=" + getName() + ", roomAbs=" + roomAbs->getName());
}

void RoomTorture::notifyActiveSpotRemoved(ActiveSpotPlace place, Tile* tile)
{
    Room::notifyActiveSpotRemoved(place, tile);

    if(place != ActiveSpotPlace::activeSpotCenter)
        return;

    auto it = mCreaturesSpots.find(tile);
    if(it == mCreaturesSpots.end())
    {
        OD_LOG_ERR("room=" + getName() + ", tile=" + Tile::displayAsString(tile));
        return;
    }

    RoomTortureCreatureInfo& info = it->second;
    if(info.mCreature != nullptr)
    {
        // We check if another active spot is free. If yes, use it
        for(std::pair<Tile* const,RoomTortureCreatureInfo>& p : mCreaturesSpots)
        {
            if(p.second.mCreature != nullptr)
                continue;

            // We found a spot
            p.second.mCreature = info.mCreature;
            p.second.mIsReady = false;
            p.second.mState = 0;
            info.mCreature = nullptr;
        }

        if(info.mCreature != nullptr)
            info.mCreature->clearActionQueue();
    }

    // clearActionQueue should have released mCreaturesSpots
    OD_ASSERT_TRUE_MSG(info.mCreature == nullptr, "room=" + getName() + ", tile=" + Tile::displayAsString(tile) + ", creature=" + info.mCreature->getName());

    mCreaturesSpots.erase(it);
}

bool RoomTorture::hasOpenCreatureSpot(Creature* creature)
{
    for(std::pair<Tile* const,RoomTortureCreatureInfo>& p : mCreaturesSpots)
    {
        if(p.second.mCreature == nullptr)
            return true;
    }

    return false;
}

bool RoomTorture::addCreatureUsingRoom(Creature* creature)
{
    RoomTortureCreatureInfo* infoToUse = nullptr;
    for(std::pair<Tile* const,RoomTortureCreatureInfo>& p : mCreaturesSpots)
    {
        if(p.second.mCreature != nullptr)
            continue;

        infoToUse = &p.second;
        break;
    }

    if(infoToUse == nullptr)
    {
        OD_LOG_ERR("room=" + getName() + ", creature=" + creature->getName());
        return false;
    }

    if(!Room::addCreatureUsingRoom(creature))
        return false;

    creature->setInJail(this);

    infoToUse->mCreature = creature;
    infoToUse->mIsReady = false;
    infoToUse->mState = 0;
    return true;
}

void RoomTorture::removeCreatureUsingRoom(Creature* creature)
{
    Room::removeCreatureUsingRoom(creature);

    // If the creature leaving the casino left another one playing alone, we
    // should search for another creature alone and try to make them match
    RoomTortureCreatureInfo* infoToUse = nullptr;
    for(std::pair<Tile* const,RoomTortureCreatureInfo>& p : mCreaturesSpots)
    {
        if(p.second.mCreature != creature)
            continue;

        infoToUse = &p.second;
        break;
    }

    if(infoToUse == nullptr)
    {
        OD_LOG_ERR("room=" + getName() + ", creature=" + creature->getName());
        return;
    }

    creature->setInJail(nullptr);

    infoToUse->mCreature = nullptr;
    infoToUse->mIsReady = false;
    infoToUse->mState = 0;
}

void RoomTorture::doUpkeep()
{
    Room::doUpkeep();

    if (mCoveredTiles.empty())
        return;

    ConfigManager& config = ConfigManager::getSingleton();
    for(std::pair<Tile* const,RoomTortureCreatureInfo>& p : mCreaturesSpots)
    {
        if(p.second.mCreature == nullptr)
            continue;
        if(!p.second.mIsReady)
            continue;

        Creature* creature = p.second.mCreature;
        Tile* tileCreature = creature->getPositionTile();
        if(tileCreature == nullptr)
        {
            OD_LOG_ERR("room=" + getName() + ", creature=" + creature->getName() + ", pos=" + Helper::toString(creature->getPosition()));
            break;
        }
        creature->increaseTurnsTorture();
        double damage = config.getRoomConfigDouble("TortureDamagePerTurn");
        creature->takeDamage(this, damage, 0.0, 0.0, 0.0, tileCreature, false);
        break;
    }
}

bool RoomTorture::useRoom(Creature& creature, bool forced)
{
    Tile* tileCreature = creature.getPositionTile();
    if(tileCreature == nullptr)
    {
        OD_LOG_ERR("room=" + getName() + ", creature=" + creature.getName() + ", pos=" + Helper::toString(creature.getPosition()));
        return false;
    }

    ConfigManager& config = ConfigManager::getSingleton();
    for(std::pair<Tile* const,RoomTortureCreatureInfo>& p : mCreaturesSpots)
    {
        if(p.second.mCreature != &creature)
            continue;

        if(tileCreature != p.first)
        {
            creature.setDestination(p.first);
            return false;
        }

        // The creature is on the good tile
        p.second.mIsReady = true;

        if((getSeat() != creature.getSeat()) &&
           (Random::Double(0.0, 1.0) <= config.getRoomConfigDouble("TortureRallyPercent")))
        {
            // The creature changes side
            creature.changeSeat(getSeat());
            creature.clearActionQueue();

            // We return false because we don't want to choose an action before a next complete turn
            if((getSeat()->getPlayer() != nullptr) &&
               getSeat()->getPlayer()->getIsHuman() &&
               !getSeat()->getPlayer()->getHasLost())
            {
                ServerNotification *serverNotification = new ServerNotification(
                    ServerNotificationType::chatServer, getSeat()->getPlayer());
                std::string msg = "Your tormentors have convinced another creature how sweet it is to live under your rule";
                serverNotification->mPacket << msg << EventShortNoticeType::aboutCreatures;
                ODServer::getSingleton().queueServerNotification(serverNotification);
            }
            return false;
        }

        // We start the fire effect and we set job cooldown
        uint32_t nbTurns = Random::Uint(config.getRoomConfigUInt32("TortureSessionLengthMin"),
            config.getRoomConfigUInt32("TortureSessionLengthMax"));
        creature.setJobCooldown(nbTurns);

        BuildingObject* obj = getBuildingObjectFromTile(tileCreature);
        if(obj == nullptr)
        {
            OD_LOG_ERR("room=" + getName() + ", tile=" + Tile::displayAsString(tileCreature));
            return false;
        }

        // We set the flame during half the session
        ++p.second.mState;
        switch(p.second.mState)
        {
            case 1:
            {
                obj->addParticleEffect("Flame", nbTurns / 2);
                obj->fireRefresh();
                creature.setAnimationState(EntityAnimation::flee_anim);
                break;
            }
            case 2:
            {
                creature.setAnimationState(EntityAnimation::idle_anim);
                p.second.mState = 0;
                break;
            }
            default:
                p.second.mState = 0;
                break;
        }
        return false;
    }

    // We should not come here
    OD_LOG_ERR("room=" + getName() + ", creature=" + creature.getName());
    creature.clearActionQueue();
    return false;

}

bool RoomTorture::isInContainment(Creature& creature)
{
    if(creature.getSeatPrison() != getSeat())
        return false;

    return true;
}

void RoomTorture::creatureDropped(Creature& creature)
{
    // Owned and enemy creatures can be tortured
    if((getSeat() != creature.getSeat()) && (getSeat()->isAlliedSeat(creature.getSeat())))
        return;

    if(!hasOpenCreatureSpot(&creature))
        return;

    // We only push the use room action. We do not want this creature to be
    // considered as searching for a job
    creature.clearActionQueue();
    creature.pushAction(Utils::make_unique<CreatureActionUseRoom>(creature, *this, true));
}
void RoomTorture::exportToStream(std::ostream& os) const
{
    Room::exportToStream(os);

    std::vector<Creature*> creatures;
    for(Tile* tile : mCoveredTiles)
    {
        for(GameEntity* entity : tile->getEntitiesInTile())
        {
            if(entity->getObjectType() != GameEntityType::creature)
                continue;

            Creature* creature = static_cast<Creature*>(entity);
            if(creature->getSeatPrison() != getSeat())
                continue;

            creatures.push_back(creature);
        }
    }

    uint32_t nb = creatures.size();
    os << nb;
    for(Creature* creature : creatures)
    {
        os << "\t" << creature->getName();
    }
    os << "\n";
}

bool RoomTorture::importFromStream(std::istream& is)
{
    if(!Room::importFromStream(is))
        return false;

    uint32_t nb;
    if(!(is >> nb))
        return false;
    while(nb > 0)
    {
        std::string creatureName;
        if(!(is >> creatureName))
            return false;

        mPrisonersLoad.push_back(creatureName);
        nb--;
    }

    return true;
}

void RoomTorture::restoreInitialEntityState()
{
    for(const std::string& creatureName : mPrisonersLoad)
    {
        Creature* creature = getGameMap()->getCreature(creatureName);
        if(creature == nullptr)
        {
            OD_LOG_ERR("creatureName=" + creatureName);
            continue;
        }

        creature->clearActionQueue();
        creature->pushAction(Utils::make_unique<CreatureActionUseRoom>(*creature, *this, true));
    }
}
