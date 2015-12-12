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

#include "rooms/RoomCasino.h"

#include "creatureaction/CreatureActionFightFriendly.h"
#include "entities/Creature.h"
#include "entities/CreatureDefinition.h"
#include "entities/GameEntityType.h"
#include "entities/RenderedMovableEntity.h"
#include "entities/SkillEntity.h"
#include "entities/Tile.h"
#include "game/Player.h"
#include "game/Skill.h"
#include "game/Seat.h"
#include "gamemap/GameMap.h"
#include "gamemap/Pathfinding.h"
#include "rooms/RoomManager.h"
#include "utils/ConfigManager.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"
#include "utils/MakeUnique.h"
#include "utils/Random.h"

const std::string RoomCasinoName = "Casino";
const std::string RoomCasinoNameDisplay = "Casino room";
const RoomType RoomCasino::mRoomType = RoomType::casino;

namespace
{
class RoomCasinoFactory : public RoomFactory
{
    RoomType getRoomType() const override
    { return RoomCasino::mRoomType; }

    const std::string& getName() const override
    { return RoomCasinoName; }

    const std::string& getNameReadable() const override
    { return RoomCasinoNameDisplay; }

    int getCostPerTile() const override
    { return ConfigManager::getSingleton().getRoomConfigInt32("CasinoCostPerTile"); }

    void checkBuildRoom(GameMap* gameMap, const InputManager& inputManager, InputCommand& inputCommand) const override
    {
        checkBuildRoomDefault(gameMap, RoomCasino::mRoomType, inputManager, inputCommand);
    }

    bool buildRoom(GameMap* gameMap, Player* player, ODPacket& packet) const override
    {
        std::vector<Tile*> tiles;
        if(!getRoomTilesDefault(tiles, gameMap, player, packet))
            return false;

        int32_t pricePerTarget = RoomManager::costPerTile(RoomCasino::mRoomType);
        int32_t price = static_cast<int32_t>(tiles.size()) * pricePerTarget;
        if(!gameMap->withdrawFromTreasuries(price, player->getSeat()))
            return false;

        RoomCasino* room = new RoomCasino(gameMap);
        return buildRoomDefault(gameMap, room, player->getSeat(), tiles);
    }

    void checkBuildRoomEditor(GameMap* gameMap, const InputManager& inputManager, InputCommand& inputCommand) const override
    {
        checkBuildRoomDefaultEditor(gameMap, RoomCasino::mRoomType, inputManager, inputCommand);
    }

    bool buildRoomEditor(GameMap* gameMap, ODPacket& packet) const override
    {
        RoomCasino* room = new RoomCasino(gameMap);
        return buildRoomDefaultEditor(gameMap, room, packet);
    }

    Room* getRoomFromStream(GameMap* gameMap, std::istream& is) const override
    {
        RoomCasino* room = new RoomCasino(gameMap);
        if(!Room::importRoomFromStream(*room, is))
        {
            OD_LOG_ERR("Error while building a room from the stream");
        }
        return room;
    }

    bool buildRoomOnTiles(GameMap* gameMap, Player* player, const std::vector<Tile*>& tiles) const override
    {
        int32_t pricePerTarget = RoomManager::costPerTile(RoomCasino::mRoomType);
        int32_t price = static_cast<int32_t>(tiles.size()) * pricePerTarget;
        if(!gameMap->withdrawFromTreasuries(price, player->getSeat()))
            return false;

        RoomCasino* room = new RoomCasino(gameMap);
        return buildRoomDefault(gameMap, room, player->getSeat(), tiles);
    }
};

// Register the factory
static RoomRegister reg(new RoomCasinoFactory);
}

const Ogre::Real OFFSET_CREATURE = 0.3;

RoomCasino::RoomCasino(GameMap* gameMap) :
    Room(gameMap)
{
    setMeshName("Casino");
}

RenderedMovableEntity* RoomCasino::notifyActiveSpotCreated(ActiveSpotPlace place, Tile* tile)
{
    switch(place)
    {
        case ActiveSpotPlace::activeSpotCenter:
        {
            Ogre::Real x = static_cast<Ogre::Real>(tile->getX());
            Ogre::Real y = static_cast<Ogre::Real>(tile->getY());
            mCreaturesSpots.emplace(std::make_pair(tile, RoomCasinoGame()));
            return loadBuildingObject(getGameMap(), "Anvil", tile, x, y, 0.0, false);
        }
        case ActiveSpotPlace::activeSpotLeft:
        {
            return loadBuildingObject(getGameMap(), "Bookshelf", tile, 90.0, false);
        }
        case ActiveSpotPlace::activeSpotRight:
        {
            return loadBuildingObject(getGameMap(), "Bookshelf", tile, 270.0, false);
        }
        case ActiveSpotPlace::activeSpotTop:
        {
            return loadBuildingObject(getGameMap(), "Bookshelf", tile, 0.0, false);
        }
        case ActiveSpotPlace::activeSpotBottom:
        {
            return loadBuildingObject(getGameMap(), "Bookshelf", tile, 180.0, false);
        }
        default:
            break;
    }
    return nullptr;
}

void RoomCasino::absorbRoom(Room* room)
{
    if(room->getType() != getType())
    {
        OD_LOG_ERR("Trying to merge incompatible rooms: " + getName() + ", type=" + RoomManager::getRoomNameFromRoomType(getType()) + ", with " + room->getName() + ", type=" + RoomManager::getRoomNameFromRoomType(room->getType()));
        return;
    }

    Room::absorbRoom(room);
}

void RoomCasino::notifyActiveSpotRemoved(ActiveSpotPlace place, Tile* tile)
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

    RoomCasinoGame& game = it->second;
    Creature* creature;
    creature = game.mCreature1.mCreature;
    if(creature != nullptr)
        creature->clearActionQueue();

    creature = game.mCreature2.mCreature;
    if(creature != nullptr)
        creature->clearActionQueue();

    // clearActionQueue should have released mCreaturesSpots
    OD_ASSERT_TRUE_MSG(game.mCreature1.mCreature == nullptr, "room=" + getName() + ", tile=" + Tile::displayAsString(tile) + ", creature=" + game.mCreature1.mCreature->getName());
    OD_ASSERT_TRUE_MSG(game.mCreature2.mCreature == nullptr, "room=" + getName() + ", tile=" + Tile::displayAsString(tile) + ", creature=" + game.mCreature2.mCreature->getName());
}

bool RoomCasino::hasOpenCreatureSpot(Creature* creature)
{
    // Broke creatures cannot play
    if(creature->getGoldCarried() <= 0)
        return false;

    for(std::pair<Tile* const,RoomCasinoGame>& p : mCreaturesSpots)
    {
        if(p.second.mCreature1.mCreature == nullptr)
            return true;

        if(p.second.mCreature2.mCreature == nullptr)
            return true;
    }

    return false;
}

bool RoomCasino::addCreatureUsingRoom(Creature* creature)
{
    const CreatureRoomAffinity& creatureRoomAffinity = creature->getDefinition()->getRoomAffinity(getType());
    OD_ASSERT_TRUE_MSG(creatureRoomAffinity.getRoomType() == getType(), "name=" + getName() + ", creature=" + creature->getName()
        + ", creatureRoomAffinityType=" + Helper::toString(static_cast<int>(creatureRoomAffinity.getRoomType())));

    OD_ASSERT_TRUE_MSG(creature->getGoldCarried() > 0, "name=" + getName() + ", creature=" + creature->getName());

    if(!Room::addCreatureUsingRoom(creature))
        return false;

    // We try to have 2 creatures on the same spot if possible
    RoomCasinoGameCreatureInfo* firstAvailableSpace = nullptr;
    for(std::pair<Tile* const,RoomCasinoGame>& p : mCreaturesSpots)
    {
        // We don't care if already 2 creatures
        if((p.second.mCreature1.mCreature != nullptr) && (p.second.mCreature2.mCreature != nullptr))
            continue;

        // If spot 2 is used, we can use spot 1
        if(p.second.mCreature2.mCreature != nullptr)
        {
            p.second.mCreature1.mCreature = creature;
            p.second.mCreature1.mIsReady = false;
            return true;
        }
        // If spot 1 is used, we can use spot 2
        if(p.second.mCreature1.mCreature != nullptr)
        {
            p.second.mCreature2.mCreature = creature;
            p.second.mCreature2.mIsReady = false;
            return true;
        }

        // No spot used. If we have not already found an available spot, we use spot 1
        if(firstAvailableSpace == nullptr)
            firstAvailableSpace = &p.second.mCreature1;
    }

    // We could not found any creature waiting for another one. We use the first available
    // we found if any
    if(firstAvailableSpace != nullptr)
    {
        firstAvailableSpace->mCreature = creature;
        firstAvailableSpace->mIsReady = false;
        return true;
    }

    return false;
}

void RoomCasino::removeCreatureUsingRoom(Creature* creature)
{
    Room::removeCreatureUsingRoom(creature);

    // If the creature leaving the casino left another one playing alone, we
    // should search for another creature alone and try to make them match
    RoomCasinoGameCreatureInfo* opponentAlone = nullptr;
    bool isCreatureFound = false;
    for(std::pair<Tile* const,RoomCasinoGame>& p : mCreaturesSpots)
    {
        if(p.second.mCreature1.mCreature == creature)
        {
            isCreatureFound = true;
            p.second.mCreature1.mCreature = nullptr;
            if(p.second.mCreature2.mCreature != nullptr)
                opponentAlone = &p.second.mCreature2;

            break;
        }
        if(p.second.mCreature2.mCreature == creature)
        {
            isCreatureFound = true;
            p.second.mCreature2.mCreature = nullptr;
            if(p.second.mCreature1.mCreature != nullptr)
                opponentAlone = &p.second.mCreature1;

            break;
        }
    }

    if(!isCreatureFound)
    {
        OD_LOG_ERR("room=" + getName() + ", creature=" + creature->getName());
        return;
    }

    // If no opponent alone, no need to search
    if(opponentAlone == nullptr)
        return;

    for(std::pair<Tile* const,RoomCasinoGame>& p : mCreaturesSpots)
    {
        if((&p.second.mCreature1 == opponentAlone) || (&p.second.mCreature2 == opponentAlone))
            continue;

        // We don't care if already 2 creatures
        if((p.second.mCreature1.mCreature != nullptr) && (p.second.mCreature2.mCreature != nullptr))
            continue;

        // We don't care if no creatures
        if((p.second.mCreature1.mCreature == nullptr) && (p.second.mCreature2.mCreature == nullptr))
            continue;

        if(p.second.mCreature1.mCreature == nullptr)
        {
            p.second.mCreature1.mCreature = opponentAlone->mCreature;
            opponentAlone->mCreature = nullptr;
            p.second.mCreature1.mIsReady = false;
            break;
        }

        if(p.second.mCreature2.mCreature == nullptr)
        {
            p.second.mCreature2.mCreature = opponentAlone->mCreature;
            opponentAlone->mCreature = nullptr;
            p.second.mCreature2.mIsReady = false;
            break;
        }
    }
}

void RoomCasino::doUpkeep()
{
    Room::doUpkeep();

    if (mCoveredTiles.empty())
        return;

    for(std::pair<Tile* const,RoomCasinoGame>& p : mCreaturesSpots)
    {
        if(p.second.mCooldown > 0)
        {
            --p.second.mCooldown;
            continue;
        }

        if(p.second.mCreature1.mCreature == nullptr)
            continue;
        if(!p.second.mCreature1.mIsReady)
            continue;
        if(p.second.mCreature2.mCreature == nullptr)
            continue;
        if(!p.second.mCreature2.mIsReady)
            continue;

        Tile* tileSpot = p.first;
        RenderedMovableEntity* ro = getBuildingObjectFromTile(tileSpot);
        if(ro == nullptr)
        {
            OD_LOG_ERR("unexpected null building object");
            return;
        }

        ro->setAnimationState("Triggered", false);

        // TODO: we could use the wall active spots to change feePercent/bets

        // We set anim for both creatures
        uint32_t cooldown = Random::Uint(ConfigManager::getSingleton().getRoomConfigUInt32("CasinoCooldownWorkMin"),
            ConfigManager::getSingleton().getRoomConfigUInt32("CasinoCooldownWorkMax"));
        double feePercent = std::min(ConfigManager::getSingleton().getRoomConfigDouble("CasinoFee"), 1.0);
        double wakefullness = ConfigManager::getSingleton().getRoomConfigDouble("CasinoWakefulnessPerWork");
        int32_t creatureBet = ConfigManager::getSingleton().getRoomConfigInt32("CasinoBet");
        creatureBet = std::min(creatureBet, p.second.mCreature1.mCreature->getGoldCarried());
        creatureBet = std::min(creatureBet, p.second.mCreature2.mCreature->getGoldCarried());
        int32_t totalBet = 0;
        Creature* creature;

        creature = p.second.mCreature1.mCreature;
        totalBet += creatureBet;
        creature->addGoldCarried(-creatureBet);
        creature->jobDone(wakefullness);
        creature->setJobCooldown(cooldown);
        const CreatureRoomAffinity& creature1RoomAffinity = creature->getDefinition()->getRoomAffinity(getType());

        creature = p.second.mCreature2.mCreature;
        totalBet += creatureBet;
        creature->addGoldCarried(-creatureBet);
        creature->jobDone(wakefullness);
        creature->setJobCooldown(cooldown);
        const CreatureRoomAffinity& creature2RoomAffinity = creature->getDefinition()->getRoomAffinity(getType());

        p.second.mCooldown = cooldown;

        // We take our fee
        int32_t totalFee = static_cast<int32_t>(static_cast<double>(totalBet) * feePercent);
        getGameMap()->addGoldToSeat(totalFee, getSeat()->getId());
        totalBet -= totalFee;

        // We give the total amount to the winning creature
        double totalWinPercent = creature1RoomAffinity.getEfficiency()
                + creature2RoomAffinity.getEfficiency();
        if(Random::Double(0, totalWinPercent) <= creature1RoomAffinity.getEfficiency())
        {
            setCreatureWinning(*p.second.mCreature1.mCreature, ro->getPosition());
            setCreatureLoosing(*p.second.mCreature2.mCreature, ro->getPosition());
            p.second.mCreature1.mCreature->addGoldCarried(totalBet);
        }
        else
        {
            setCreatureWinning(*p.second.mCreature2.mCreature, ro->getPosition());
            setCreatureLoosing(*p.second.mCreature1.mCreature, ro->getPosition());
            p.second.mCreature2.mCreature->addGoldCarried(totalBet);
        }
    }
}

bool RoomCasino::useRoom(Creature& creature, bool forced)
{
    bool isGamblePossible = false;
    Tile* tileSpot = nullptr;
    RoomCasinoGameCreatureInfo* creatureInfo = nullptr;
    RoomCasinoGameCreatureInfo* opponentInfo = nullptr;
    Ogre::Real creaturePositionOffset = 0;
    for(std::pair<Tile* const,RoomCasinoGame>& p : mCreaturesSpots)
    {
        if(p.second.mCreature1.mCreature == &creature)
        {
            tileSpot = p.first;
            creatureInfo = &p.second.mCreature1;
            opponentInfo = &p.second.mCreature2;
            creaturePositionOffset = OFFSET_CREATURE;

            if(p.second.mCreature1.mIsReady && (p.second.mCreature2.mCreature != nullptr))
                isGamblePossible = true;

            break;
        }
        if(p.second.mCreature2.mCreature == &creature)
        {
            tileSpot = p.first;
            creatureInfo = &p.second.mCreature2;
            opponentInfo = &p.second.mCreature1;
            creaturePositionOffset = -OFFSET_CREATURE;

            if(p.second.mCreature2.mIsReady && (p.second.mCreature1.mCreature != nullptr))
                isGamblePossible = true;

            break;
        }
    }

    if(tileSpot == nullptr)
    {
        OD_LOG_ERR("room=" + getName() + ", creature=" + creature.getName());
        return false;
    }

    if((creatureInfo == nullptr) || (opponentInfo == nullptr))
    {
        OD_LOG_ERR("room=" + getName() + ", creature=" + creature.getName());
        return false;
    }

    // If the creature has no gold, it should stop gambling
    if(creature.getGoldCarried() <= 0)
    {
        // We save the opponent here because it could be released by popAction
        // if there is a gambler waiting
        Creature* opponent = opponentInfo->mCreature;
        creature.popAction();
        // We randomly engage the creature we are playing with if any
        if((opponent != nullptr) && (Random::Uint(0,100) <= 50))
        {
            // We fight for KO
            creature.pushAction(Utils::make_unique<CreatureActionFightFriendly>(creature, opponent, true, getCoveredTiles()));
            opponent->pushAction(Utils::make_unique<CreatureActionFightFriendly>(*opponent, &creature, true, getCoveredTiles()));
        }
        return true;
    }

    // If the creature can play (if it is ready and has an opponent), it should
    // play (or at least wait for the opponent)
    if(isGamblePossible)
        return false;

    Tile* tileCreature = creature.getPositionTile();
    if(tileCreature == nullptr)
    {
        OD_LOG_ERR("room=" + getName() + ", creature=" + creature.getName() + ", pos=" + Helper::toString(creature.getPosition()));
        return false;
    }

    Ogre::Real wantedX = static_cast<Ogre::Real>(tileSpot->getX());
    Ogre::Real wantedY = static_cast<Ogre::Real>(tileSpot->getY());
    wantedY += creaturePositionOffset;

    // We consider that the creature is in the good place if it near from where we want it to go
    if(Pathfinding::squaredDistance(creature.getPosition().x, wantedX, creature.getPosition().y, wantedY) > 0.4)
    {
        // We go there
        std::list<Tile*> pathToSpot = getGameMap()->path(&creature, tileSpot);
        std::vector<Ogre::Vector3> path;
        Creature::tileToVector3(pathToSpot, path, true, 0.0);
        // We add the last step to take account of the offset
        Ogre::Vector3 dest(wantedX, wantedY, 0.0);
        path.push_back(dest);
        creature.setWalkPath(EntityAnimation::walk_anim, EntityAnimation::idle_anim, true, true, path);
        return false;
    }

    // This creature is ready. If there is no opponent, we pick a random tile and make it wander
    if(opponentInfo->mCreature == nullptr)
    {
        std::vector<Tile*> tiles = getCoveredTiles();
        if(tiles.empty())
        {
            OD_LOG_ERR("room=" + getName() + ", creature=" + creature.getName());
            return false;
        }

        uint32_t index = Random::Uint(0, tiles.size() - 1);
        Tile* tile = tiles[index];
        creature.setDestination(tile);
        creatureInfo->mIsReady = false;
        return false;
    }

    // We are ready. We wait
    creatureInfo->mIsReady = true;
    return false;
}

void RoomCasino::setCreatureWinning(Creature& creature, const Ogre::Vector3& gamePosition)
{
    Ogre::Vector3 walkDirection(gamePosition.x - creature.getPosition().x, gamePosition.y - creature.getPosition().y, static_cast<Ogre::Real>(0));
    walkDirection.normalise();
    creature.setAnimationState(EntityAnimation::attack_anim, false, walkDirection);
}

void RoomCasino::setCreatureLoosing(Creature& creature, const Ogre::Vector3& gamePosition)
{
    Ogre::Vector3 walkDirection(gamePosition.x - creature.getPosition().x, gamePosition.y - creature.getPosition().y, static_cast<Ogre::Real>(0));
    walkDirection.normalise();
    creature.setAnimationState(EntityAnimation::idle_anim, false, walkDirection);
}
