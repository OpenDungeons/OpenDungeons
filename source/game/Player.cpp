/*
 *  Copyright (C) 2011-2016  OpenDungeons Team
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

#include "game/Player.h"

#include "creatureaction/CreatureAction.h"
#include "entities/Creature.h"
#include "entities/GameEntityType.h"
#include "entities/Tile.h"
#include "game/SkillManager.h"
#include "game/Seat.h"
#include "gamemap/GameMap.h"
#include "gamemap/Pathfinding.h"
#include "render/RenderManager.h"
#include "rooms/Room.h"
#include "rooms/RoomType.h"
#include "sound/SoundEffectsManager.h"
#include "spells/SpellManager.h"
#include "spells/SpellType.h"
#include "traps/Trap.h"
#include "network/ODPacket.h"
#include "network/ODServer.h"
#include "network/ServerNotification.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"
#include "utils/Random.h"
#include "ODApplication.h"

#include <cmath>

//! \brief The number of seconds the local player must stay out of danger to trigger the calm music again.
const float BATTLE_TIME_COUNT = 10.0f;
//! \brief Minimum distance to consider that a new event of the same type is in another place
//! Note that the distance is squared (100 would mean 10 tiles)
const int MIN_DIST_EVENT_SQUARED = 100;

//! \brief The number of seconds the local player will not be notified again if the skill queue is empty
const float NO_RESEARCH_TIME_COUNT = 60.0f;

//! \brief The number of seconds the local player will not be notified again if he has no worker
const float NO_WORKER_TIME_COUNT = 60.0f;

//! \brief The number of seconds the local player will not be notified again if no treasury is available
const float NO_TREASURY_TIME_COUNT = 30.0f;

//! \brief The number of seconds the local player will not be notified again if a creature cannot find place in a dormitory
const float CREATURE_CANNOT_FIND_BED_TIME_COUNT = 30.0f;

//! \brief The number of seconds the local player will not be notified again if a creature cannot find place in a dormitory
const float CREATURE_CANNOT_FIND_FOOD_TIME_COUNT = 30.0f;

Player::Player(GameMap* gameMap, int32_t id) :
    mId(id),
    mGameMap(gameMap),
    mSeat(nullptr),
    mIsHuman(false),
    mNoSkillInQueueTime(0.0f),
    mNoWorkerTime(0.0f),
    mNoTreasuryAvailableTime(0.0f),
    mCreatureCannotFindBed(0.0f),
    mCreatureCannotFindFood(0.0f),
    mHasLost(false),
    mSpellsCooldown(std::vector<PlayerSpellData>(static_cast<uint32_t>(SpellType::nbSpells), PlayerSpellData(0, 0.0f))),
    mWorkersActions(std::vector<uint32_t>(static_cast<uint32_t>(CreatureActionType::nb), 0))
{
}

unsigned int Player::numCreaturesInHand(const Seat* seat) const
{
    unsigned int cpt = 0;
    for(GameEntity* entity : mObjectsInHand)
    {
        if(entity->getObjectType() != GameEntityType::creature)
            continue;

        if(seat != nullptr && entity->getSeat() != seat)
            continue;

        ++cpt;
    }
    return cpt;
}

unsigned int Player::numObjectsInHand() const
{
    return mObjectsInHand.size();
}

void Player::addEntityToHand(GameEntity *entity)
{
    if (mObjectsInHand.empty())
    {
        mObjectsInHand.push_back(entity);
        return;
    }

    // creaturesInHand.push_front(c);
    // Since vectors have no push_front method,
    // we need to move all of the elements in the vector back one
    // and then add this one to the beginning.
    mObjectsInHand.push_back(nullptr);
    for (unsigned int j = mObjectsInHand.size() - 1; j > 0; --j)
        mObjectsInHand[j] = mObjectsInHand[j - 1];

    mObjectsInHand[0] = entity;
}

void Player::clearObjectsInHand()
{
    mObjectsInHand.clear();
}

ODPacket& operator<<(ODPacket& os, const PlayerEventType& type)
{
    os << static_cast<int32_t>(type);
    return os;
}

ODPacket& operator>>(ODPacket& is, PlayerEventType& type)
{
    int32_t tmp;
    OD_ASSERT_TRUE(is >> tmp);
    type = static_cast<PlayerEventType>(tmp);
    return is;
}

void Player::updateEvents(const std::vector<PlayerEvent*>& events)
{
    for(PlayerEvent* event : mEvents)
        delete event;

    mEvents = events;
}

const PlayerEvent* Player::getNextEvent(uint32_t& index) const
{
    if(mEvents.empty())
        return nullptr;

    index = (index + 1) % mEvents.size();

    return mEvents[index];
}

uint32_t Player::getSpellCooldownTurns(SpellType spellType) const
{
    uint32_t spellIndex = static_cast<uint32_t>(spellType);
    if(spellIndex >= mSpellsCooldown.size())
    {
        OD_LOG_ERR("seatId=" + Helper::toString(getId()) + ", spellType=" + SpellManager::getSpellNameFromSpellType(spellType));
        return 0;
    }

    return mSpellsCooldown.at(spellIndex).mCooldownNbTurnPending;
}

float Player::getSpellCooldownSmooth(SpellType spellType) const
{
    uint32_t spellIndex = static_cast<uint32_t>(spellType);
    if(spellIndex >= mSpellsCooldown.size())
    {
        OD_LOG_ERR("seatId=" + Helper::toString(getId()) + ", spellType=" + SpellManager::getSpellNameFromSpellType(spellType));
        return 0;
    }

    const PlayerSpellData& cooldown = mSpellsCooldown.at(spellIndex);
    uint32_t cooldownTurns = cooldown.mCooldownNbTurnPending;
    if(cooldownTurns <= 0)
        return 0.0f;

    uint32_t maxCooldownTurns = SpellManager::getSpellCooldown(spellType);
    if(maxCooldownTurns <= 0)
        return 0.0f;

    float cooldownTime = static_cast<float>(cooldownTurns - 1) / ODApplication::turnsPerSecond;
    cooldownTime += cooldown.mCooldownTimePendingTurn;
    float maxCooldownTime = static_cast<float>(maxCooldownTurns) / ODApplication::turnsPerSecond;

    return cooldownTime / maxCooldownTime;
}

float Player::getSpellCooldownSmoothTime(SpellType spellType) const
{
    uint32_t spellIndex = static_cast<uint32_t>(spellType);
    if(spellIndex >= mSpellsCooldown.size())
    {
        OD_LOG_ERR("seatId=" + Helper::toString(getId()) + ", spellType=" + SpellManager::getSpellNameFromSpellType(spellType));
        return 0;
    }

    const PlayerSpellData& cooldown = mSpellsCooldown.at(spellIndex);
    uint32_t cooldownTurns = cooldown.mCooldownNbTurnPending;
    if(cooldownTurns <= 0)
        return 0.0f;

    float cooldownTime = static_cast<float>(cooldownTurns - 1) / ODApplication::turnsPerSecond;
    cooldownTime += cooldown.mCooldownTimePendingTurn;

    return cooldownTime;
}

void Player::decreaseSpellCooldowns()
{
    for(PlayerSpellData& cooldown : mSpellsCooldown)
    {
        if(cooldown.mCooldownNbTurnPending <= 0)
            continue;

        --cooldown.mCooldownNbTurnPending;
        cooldown.mCooldownTimePendingTurn = 1.0f / ODApplication::turnsPerSecond;
    }
}

void Player::frameStarted(float timeSinceLastFrame)
{
    // Update the smooth spell cooldown
    for(PlayerSpellData& cooldown : mSpellsCooldown)
    {
        if(cooldown.mCooldownNbTurnPending <= 0)
            continue;
        if(timeSinceLastFrame > cooldown.mCooldownTimePendingTurn)
        {
            cooldown.mCooldownTimePendingTurn = 0.0f;
            continue;
        }

        cooldown.mCooldownTimePendingTurn -= timeSinceLastFrame;
    }
}

PlayerEvent* PlayerEvent::getPlayerEventFromPacket(GameMap* gameMap, ODPacket& is)
{
    PlayerEvent* event = new PlayerEvent;
    event->importFromPacket(gameMap, is);
    return event;
}

void PlayerEvent::exportPlayerEventToPacket(PlayerEvent* event, GameMap* gameMap, ODPacket& is)
{
    event->exportToPacket(gameMap, is);
}

void PlayerEvent::exportToPacket(GameMap* gameMap, ODPacket& os)
{
    os << mType;
    gameMap->tileToPacket(os, mTile);
}

void PlayerEvent::importFromPacket(GameMap* gameMap, ODPacket& is)
{
    OD_ASSERT_TRUE(is >> mType);
    mTile = gameMap->tileFromPacket(is);
}

void Player::pickUpEntity(GameEntity *entity)
{
    if(!entity->tryPickup(getSeat()))
    {
        OD_LOG_ERR("player seatId" + Helper::toString(getSeat()->getId()) + " couldn't pickup entity=" + entity->getName());
        return;
    }

    OD_LOG_INF("player seatId=" + Helper::toString(getSeat()->getId()) + " picked up " + entity->getName());
    entity->pickup();

    // Start tracking this creature as being in this player's hand
    addEntityToHand(entity);

    if (mGameMap->isServerGameMap())
    {
        entity->firePickupEntity(this);
        return;
    }

    if (this != mGameMap->getLocalPlayer())
    {
        OD_LOG_ERR("cannot pickup entity player seat=" + Helper::toString(getSeat()->getId()) + ", localPlayer seat id=" + Helper::toString(mGameMap->getLocalPlayer()->getSeat()->getId()) + ", entity=" + entity->getName());
        return;
    }
    RenderManager::getSingleton().rrPickUpEntity(entity, this);
}


bool Player::isDropHandPossible(Tile *t, unsigned int index)
{
    // if we have a creature to drop
    if (index >= mObjectsInHand.size())
    {
        OD_LOG_ERR("seatId=" + Helper::toString(getSeat()->getId()) + ", index=" + Helper::toString(index) + ", size=" + Helper::toString(mObjectsInHand.size()));
        return false;
    }

    GameEntity* entity = mObjectsInHand[index];
    return entity->tryDrop(getSeat(), t);
}

void Player::dropHand(Tile *t, unsigned int index)
{
    // Add the creature to the map
    if(index >= mObjectsInHand.size())
    {
        OD_LOG_ERR("seatId=" + Helper::toString(getSeat()->getId()) + ", index=" + Helper::toString(index) + ", size=" + Helper::toString(mObjectsInHand.size()));
        return;
    }

    GameEntity *entity = mObjectsInHand[index];
    mObjectsInHand.erase(mObjectsInHand.begin() + index);

    Ogre::Vector3 pos(static_cast<Ogre::Real>(t->getX()),
       static_cast<Ogre::Real>(t->getY()), entity->getPosition().z);
    if(mGameMap->isServerGameMap())
    {
        entity->drop(pos);
        entity->fireDropEntity(this, t);
        return;
    }

    entity->correctDropPosition(pos);
    OD_LOG_INF("player seatId=" + Helper::toString(getSeat()->getId()) + " drop " + entity->getName() + " on tile=" + Tile::displayAsString(t));
    entity->drop(pos);

    // If this is the result of another player dropping the creature it is currently not visible so we need to create a mesh for it
    //cout << "\nthis:  " << this << "\nme:  " << gameMap->getLocalPlayer() << endl;
    //cout.flush();
    if (this != mGameMap->getLocalPlayer())
    {
        OD_LOG_ERR("cannot pickup entity player seat=" + Helper::toString(getSeat()->getId()) + ", localPlayer seat id=" + Helper::toString(mGameMap->getLocalPlayer()->getSeat()->getId()) + ", entity=" + entity->getName());
        return;
    }
    // Send a render request to rearrange the creatures in the hand to move them all forward 1 place
    RenderManager::getSingleton().rrDropHand(entity, this);
}

void Player::rotateHand(Direction d)
{
    if(mObjectsInHand.size() > 1)
    {
        if(d == Direction::left)
        {
            std::rotate(mObjectsInHand.begin(), mObjectsInHand.begin() + 1, mObjectsInHand.end());
        }
        else
        {
            std::rotate(mObjectsInHand.begin(), mObjectsInHand.end() - 1, mObjectsInHand.end());
        }
        // Send a render request to move the entity into the "hand"
        RenderManager::getSingleton().rrRotateHand(this);
    }
}

void Player::notifyNoMoreDungeonTemple()
{
    if(mHasLost)
        return;

    mHasLost = true;
    OD_LOG_INF("Player seatId=" + Helper::toString(getSeat()->getId()) + " lost");

    // We check if there is still a player in the team with a dungeon temple. If yes, we notify the player he lost his dungeon
    // if no, we notify the team they lost
    std::vector<Room*> dungeonTemples = mGameMap->getRoomsByType(RoomType::dungeonTemple);
    bool hasTeamLost = true;
    for(Room* dungeonTemple : dungeonTemples)
    {
        if(getSeat()->isAlliedSeat(dungeonTemple->getSeat()))
        {
            hasTeamLost = false;
            break;
        }
    }

    if(hasTeamLost)
    {
        // This message will be sent in 1v1 or multiplayer so it should not talk about team. If we want to be
        // more precise, we shall handle the case
        std::vector<Seat*> seats;
        for(Seat* seat : mGameMap->getSeats())
        {
            if(seat->getPlayer() == nullptr)
                continue;
            if(!seat->getPlayer()->getIsHuman())
                continue;
            if(!getSeat()->isAlliedSeat(seat))
                continue;

            seats.push_back(seat);

            ServerNotification *serverNotification = new ServerNotification(
                ServerNotificationType::chatServer, seat->getPlayer());
            serverNotification->mPacket << "You lost the game" << EventShortNoticeType::majorGameEvent;
            ODServer::getSingleton().queueServerNotification(serverNotification);
        }
        mGameMap->fireRelativeSound(seats, SoundRelativeKeeperStatements::Lost);
    }
    else
    {
        std::vector<Seat*> seats;
        for(Seat* seat : mGameMap->getSeats())
        {
            if(seat->getPlayer() == nullptr)
                continue;
            if(!seat->getPlayer()->getIsHuman())
                continue;
            if(!getSeat()->isAlliedSeat(seat))
                continue;

            if(this == seat->getPlayer())
            {
                // For the current player, we send the defeat message
                ServerNotification *serverNotification = new ServerNotification(
                    ServerNotificationType::chatServer, seat->getPlayer());
                serverNotification->mPacket << "You lost" << EventShortNoticeType::majorGameEvent;
                ODServer::getSingleton().queueServerNotification(serverNotification);

                mGameMap->fireRelativeSound(seats, SoundRelativeKeeperStatements::Defeat);
                continue;
            }

            seats.push_back(seat);

            ServerNotification *serverNotification = new ServerNotification(
                ServerNotificationType::chatServer, seat->getPlayer());
            serverNotification->mPacket << "An ally has lost" << EventShortNoticeType::majorGameEvent;
            ODServer::getSingleton().queueServerNotification(serverNotification);
        }
        mGameMap->fireRelativeSound(seats, SoundRelativeKeeperStatements::AllyDefeated);
    }
}

void Player::notifyTeamFighting(Player* player, Tile* tile)
{
    // We check if there is a fight event currently near this tile. If yes, we update
    // the time event. If not, we create a new fight event
    bool isFirstFight = true;
    for(PlayerEvent* event : mEvents)
    {
        if(event->getType() != PlayerEventType::fight)
            continue;

        // There is already another fight event. We check the distance
        isFirstFight = false;
        if(Pathfinding::squaredDistanceTile(*tile, *event->getTile()) < MIN_DIST_EVENT_SQUARED)
        {
            // A similar event is already on nearly. We only update it
            event->setTimeRemain(BATTLE_TIME_COUNT);
            return;
        }
    }

    if(isFirstFight)
    {
        ServerNotification *serverNotification = new ServerNotification(
            ServerNotificationType::playerFighting, this);
        serverNotification->mPacket << player->getId();
        ODServer::getSingleton().queueServerNotification(serverNotification);

        std::vector<Seat*> seats;
        seats.push_back(getSeat());
        mGameMap->fireRelativeSound(seats, SoundRelativeKeeperStatements::WeAreUnderAttack);
    }

    // We add the fight event
    mEvents.push_back(new PlayerEvent(PlayerEventType::fight, tile, BATTLE_TIME_COUNT));
    fireEvents();
}

void Player::notifyNoSkillInQueue()
{
    if(mNoSkillInQueueTime == 0.0f)
    {
        mNoSkillInQueueTime = NO_RESEARCH_TIME_COUNT;

        std::string chatMsg = "Your skill queue is empty, while there are still skills that could be unlocked.";
        ServerNotification *serverNotification = new ServerNotification(
            ServerNotificationType::chatServer, this);
        serverNotification->mPacket << chatMsg << EventShortNoticeType::genericGameInfo;
        ODServer::getSingleton().queueServerNotification(serverNotification);
    }
}

void Player::notifyNoWorker()
{
    if(mNoWorkerTime > 0.0f)
        return;

    mNoWorkerTime = NO_WORKER_TIME_COUNT;

    std::string chatMsg = "You have no worker to fullfill your dark wishes.";
    ServerNotification *serverNotification = new ServerNotification(
        ServerNotificationType::chatServer, this);
    serverNotification->mPacket << chatMsg << EventShortNoticeType::genericGameInfo;
    ODServer::getSingleton().queueServerNotification(serverNotification);
}

void Player::notifyNoTreasuryAvailable()
{
    if(mNoTreasuryAvailableTime == 0.0f)
    {
        mNoTreasuryAvailableTime = NO_TREASURY_TIME_COUNT;

        std::string chatMsg = "No treasury available. You should build a bigger one.";
        ServerNotification *serverNotification = new ServerNotification(
            ServerNotificationType::chatServer, this);
        serverNotification->mPacket << chatMsg << EventShortNoticeType::genericGameInfo;
        ODServer::getSingleton().queueServerNotification(serverNotification);
    }
}

void Player::notifyCreatureCannotFindBed(Creature& creature)
{
    if(mCreatureCannotFindBed <= 0.0f)
    {
        mCreatureCannotFindBed = CREATURE_CANNOT_FIND_BED_TIME_COUNT;

        std::string chatMsg = creature.getName() + " cannot find room for a bed";
        ServerNotification *serverNotification = new ServerNotification(
            ServerNotificationType::chatServer, this);
        serverNotification->mPacket << chatMsg << EventShortNoticeType::genericGameInfo;
        ODServer::getSingleton().queueServerNotification(serverNotification);

        std::vector<Seat*> seats;
        seats.push_back(getSeat());
        mGameMap->fireRelativeSound(seats, SoundRelativeKeeperStatements::CreatureNoBed);
    }
}

void Player::notifyCreatureCannotFindFood(Creature& creature)
{
    if(mCreatureCannotFindFood <= 0.0f)
    {
        mCreatureCannotFindFood = CREATURE_CANNOT_FIND_FOOD_TIME_COUNT;

        std::string chatMsg = creature.getName() + " cannot find food";
        ServerNotification *serverNotification = new ServerNotification(
            ServerNotificationType::chatServer, this);
        serverNotification->mPacket << chatMsg << EventShortNoticeType::genericGameInfo;
        ODServer::getSingleton().queueServerNotification(serverNotification);

        std::vector<Seat*> seats;
        seats.push_back(getSeat());
        mGameMap->fireRelativeSound(seats, SoundRelativeKeeperStatements::CreatureNoFood);
    }
}

void Player::fireEvents()
{
    // On server side, we update the client
    if(!mGameMap->isServerGameMap())
        return;

    ServerNotification *serverNotification = new ServerNotification(
        ServerNotificationType::playerEvents, this);
    uint32_t nbItems = mEvents.size();
    serverNotification->mPacket << nbItems;
    for(PlayerEvent* event : mEvents)
    {
        PlayerEvent::exportPlayerEventToPacket(event, mGameMap,
            serverNotification->mPacket);
    }

    ODServer::getSingleton().queueServerNotification(serverNotification);
}


void Player::markTilesForDigging(bool marked, const std::vector<Tile*>& tiles, bool asyncMsg)
{
    if(tiles.empty())
        return;

    std::vector<Tile*> tilesMark;
    // If human player, we notify marked tiles
    if(getIsHuman())
    {
        for(Tile* tile : tiles)
        {
            if(!marked)
            {
                getSeat()->tileMarkedDiggingNotifiedToPlayer(tile, marked);
                tile->setMarkedForDigging(marked, this);
                tilesMark.push_back(tile);

                continue;
            }

            // If the tile is diggable for the client, we mark it for him
            if(getSeat()->isTileDiggableForClient(tile))
            {
                getSeat()->tileMarkedDiggingNotifiedToPlayer(tile, marked);
                tilesMark.push_back(tile);
            }

            // If the tile can be marked on server side, we mark it
            if(!tile->isDiggable(getSeat()))
                continue;

            tile->setMarkedForDigging(marked, this);
        }
    }
    else
    {
        for(Tile* tile : tiles)
        {
            if(!marked)
            {
                getSeat()->tileMarkedDiggingNotifiedToPlayer(tile, marked);
                tile->setMarkedForDigging(marked, this);
                continue;
            }

            // If the tile can be marked on server side, we mark it
            if(!tile->isDiggable(getSeat()))
                continue;

            tile->setMarkedForDigging(marked, this);
        }
        return;
    }

    // If no tile to mark, we do not send the message
    if(tilesMark.empty())
        return;

    // On client side, we ask to mark the tile
    if(!asyncMsg)
    {
        ServerNotification* serverNotification = new ServerNotification(
            ServerNotificationType::markTiles, this);
        uint32_t nbTiles = tilesMark.size();
        serverNotification->mPacket << marked << nbTiles;
        for(Tile* tile : tilesMark)
        {
            // On client side, we ask to mark the tile
            mGameMap->tileToPacket(serverNotification->mPacket, tile);
        }
        ODServer::getSingleton().queueServerNotification(serverNotification);
    }
    else
    {
        ServerNotification serverNotification(
            ServerNotificationType::markTiles, this);
        uint32_t nbTiles = tilesMark.size();
        serverNotification.mPacket << marked << nbTiles;
        for(Tile* tile : tilesMark)
            mGameMap->tileToPacket(serverNotification.mPacket, tile);

        ODServer::getSingleton().sendAsyncMsg(serverNotification);
    }
}

void Player::upkeepPlayer(double timeSinceLastUpkeep)
{
    decreaseSpellCooldowns();

    // Specific stuff for human players
    if(!getIsHuman())
        return;

    // Handle fighting time
    bool wasFightHappening = false;
    bool isFightHappening = false;
    bool isEventListUpdated = false;
    for(auto it = mEvents.begin(); it != mEvents.end();)
    {
        PlayerEvent* event = *it;
        if(event->getType() != PlayerEventType::fight)
        {
            ++it;
            continue;
        }

        wasFightHappening = true;

        float timeRemain = event->getTimeRemain();
        if(timeRemain > timeSinceLastUpkeep)
        {
            isFightHappening = true;
            timeRemain -= timeSinceLastUpkeep;
            event->setTimeRemain(timeRemain);
            ++it;
            continue;
        }

        // This event is outdated, we remove it
        isEventListUpdated = true;
        delete event;
        it = mEvents.erase(it);
    }

    if(wasFightHappening && !isFightHappening)
    {
        // Notify the player he is no longer under attack.
        ServerNotification *serverNotification = new ServerNotification(
            ServerNotificationType::playerNoMoreFighting, this);
        ODServer::getSingleton().queueServerNotification(serverNotification);
    }

    // Do not notify skill queue empty if no library
    if(getIsHuman() &&
       !getHasLost() &&
       (getSeat()->getNbRooms(RoomType::library) > 0))
    {
        if(mNoSkillInQueueTime > timeSinceLastUpkeep)
            mNoSkillInQueueTime -= timeSinceLastUpkeep;
        else
        {
            mNoSkillInQueueTime = 0.0f;

            // Reprint the warning if there is still no skill being done
            if(getSeat() != nullptr && !getSeat()->isSkilling() && !SkillManager::isAllSkillsDoneForSeat(getSeat()))
                notifyNoSkillInQueue();
        }
    }

    if(getIsHuman() &&
       !getHasLost())
    {
        if(mNoWorkerTime > timeSinceLastUpkeep)
            mNoWorkerTime -= timeSinceLastUpkeep;
        else
        {
            mNoWorkerTime = 0.0f;

            if(getSeat()->getNumCreaturesWorkers() <= 0)
                notifyNoWorker();
        }
    }

    if(mNoTreasuryAvailableTime > 0.0f)
    {
        if(mNoTreasuryAvailableTime > timeSinceLastUpkeep)
            mNoTreasuryAvailableTime -= timeSinceLastUpkeep;
        else
            mNoTreasuryAvailableTime = 0.0f;
    }

    if(mCreatureCannotFindBed > 0.0f)
    {
        if(mCreatureCannotFindBed > timeSinceLastUpkeep)
            mCreatureCannotFindBed -= timeSinceLastUpkeep;
        else
            mCreatureCannotFindBed = 0.0f;
    }

    if(mCreatureCannotFindFood > 0.0f)
    {
        if(mCreatureCannotFindFood > timeSinceLastUpkeep)
            mCreatureCannotFindFood -= timeSinceLastUpkeep;
        else
            mCreatureCannotFindFood = 0.0f;
    }

    if(isEventListUpdated)
        fireEvents();
}

void Player::setSpellCooldownTurns(SpellType spellType, uint32_t cooldown)
{
    uint32_t spellIndex = static_cast<uint32_t>(spellType);
    if(spellIndex >= mSpellsCooldown.size())
    {
        OD_LOG_ERR("seatId=" + Helper::toString(getId()) + ", spellType=" + SpellManager::getSpellNameFromSpellType(spellType));
        return;
    }

    mSpellsCooldown[spellIndex] = PlayerSpellData(cooldown, 1.0f / ODApplication::turnsPerSecond);

    if(mGameMap->isServerGameMap() && getIsHuman())
    {
        ServerNotification *serverNotification = new ServerNotification(
            ServerNotificationType::setSpellCooldown, this);
        serverNotification->mPacket << spellType << cooldown;
        ODServer::getSingleton().queueServerNotification(serverNotification);
    }
}

void Player::notifyWorkerAction(Creature& worker, CreatureActionType actionType)
{
    uint32_t index = static_cast<uint32_t>(actionType);
    if(index >= mWorkersActions.size())
    {
        OD_LOG_ERR("Invalid index seatId=" + Helper::toString(getId()) + ", value=" + Helper::toString(index) + ", size=" + Helper::toString(mWorkersActions.size()));
        return;
    }

    mWorkersActions[index]++;
}

void Player::notifyWorkerStopsAction(Creature& worker, CreatureActionType actionType)
{
    uint32_t index = static_cast<uint32_t>(actionType);
    if(index >= mWorkersActions.size())
    {
        OD_LOG_ERR("Invalid index seatId=" + Helper::toString(getId()) + ", value=" + Helper::toString(index) + ", size=" + Helper::toString(mWorkersActions.size()));
        return;
    }

    // Sanity check
    if(mWorkersActions[index] <= 0)
    {
        OD_LOG_ERR("No worker doing action seatId=" + Helper::toString(getId()) + ", action=" + CreatureAction::toString(actionType));
        return;
    }


    mWorkersActions[index]--;
}

uint32_t Player::getNbWorkersDoing(CreatureActionType actionType) const
{
    uint32_t index = static_cast<uint32_t>(actionType);
    if(index >= mWorkersActions.size())
    {
        OD_LOG_ERR("Invalid index seatId=" + Helper::toString(getId()) + ", value=" + Helper::toString(index) + ", size=" + Helper::toString(mWorkersActions.size()));
        return 0;
    }

    return mWorkersActions.at(index);
}

std::vector<CreatureActionType> Player::getWorkerPreferredActions(Creature& worker) const
{
    std::vector<CreatureActionType> ret;
    // We want to have more or less 40% workers digging, 40% claiming ground tiles and 20% claiming wall tiles
    // Concerning carrying stuff, most workers should try unless more than 20% are already carrying.
    uint32_t nbWorkersDigging = getNbWorkersDoing(CreatureActionType::searchTileToDig);
    uint32_t nbWorkersClaimingGround = getNbWorkersDoing(CreatureActionType::searchGroundTileToClaim);
    uint32_t nbWorkersClaimingWall = getNbWorkersDoing(CreatureActionType::searchWallTileToClaim);
    uint32_t nbWorkersCarrying = getNbWorkersDoing(CreatureActionType::searchEntityToCarry);
    // For the total number of workers, we consider only those doing something in the wanted list (and not
    // the ones fighting or having nothing to do) + the one we are considering
    uint32_t nbWorkersTotal = nbWorkersDigging + nbWorkersClaimingGround
            + nbWorkersClaimingWall + nbWorkersCarrying + 1;

    double percent;
    bool isCarryAdded = false;
    percent = static_cast<double>(nbWorkersCarrying) / static_cast<double>(nbWorkersTotal);
    if(percent <= 0.2)
    {
        isCarryAdded = true;
        ret.push_back(CreatureActionType::searchEntityToCarry);
    }

    bool isClaimWallAdded = false;
    percent = static_cast<double>(nbWorkersDigging + nbWorkersClaimingGround) / static_cast<double>(nbWorkersTotal);
    if(percent > 0.8)
    {
        isClaimWallAdded = true;
        ret.push_back(CreatureActionType::searchWallTileToClaim);
    }

    bool digTileFirst = false;
    if(nbWorkersDigging < nbWorkersClaimingGround)
        digTileFirst = true;
    else if(nbWorkersDigging == nbWorkersClaimingGround)
        digTileFirst = (Random::Uint(0,1) == 0);

    if(digTileFirst)
    {
        ret.push_back(CreatureActionType::searchTileToDig);
        ret.push_back(CreatureActionType::searchGroundTileToClaim);
    }
    else
    {
        ret.push_back(CreatureActionType::searchGroundTileToClaim);
        ret.push_back(CreatureActionType::searchTileToDig);
    }

    if(!isClaimWallAdded)
        ret.push_back(CreatureActionType::searchWallTileToClaim);
    if(!isCarryAdded)
        ret.push_back(CreatureActionType::searchEntityToCarry);

    return ret;
}
