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

#include "game/Player.h"

#include "entities/Tile.h"

#include "game/Seat.h"

#include "gamemap/GameMap.h"

#include "network/ODServer.h"
#include "network/ServerNotification.h"
#include "network/ODClient.h"

#include "render/RenderManager.h"

#include "rooms/Room.h"
#include "rooms/RoomType.h"

#include "spell/Spell.h"

#include "traps/Trap.h"

#include "utils/Helper.h"
#include "utils/LogManager.h"

#include <cmath>

//! \brief The number of seconds the local player must stay out of danger to trigger the calm music again.
const float BATTLE_TIME_COUNT = 10.0f;
//! \brief Minimum distance to consider that a new event of the same type is in another place
//! Note that the distance is squared (100 would mean 10 tiles)
const float MIN_DIST_EVENT_SQUARED = 100.0f;

//! \brief The number of seconds the local player will not be notified again if no treasury is available
const float NO_TREASURY_TIME_COUNT = 30.0f;

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

Player::Player(GameMap* gameMap, int32_t id) :
    mId(id),
    mNewRoomType(RoomType::nullRoomType),
    mNewTrapType(TrapType::nullTrapType),
    mCurrentAction(SelectedAction::none),
    mGameMap(gameMap),
    mSeat(nullptr),
    mIsHuman(false),
    mNoTreasuryAvailableTime(0.0f),
    mIsPlayerLostSent(false)
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

void Player::pickUpEntity(GameEntity *entity)
{
    if (!ODServer::getSingleton().isConnected() && !ODClient::getSingleton().isConnected())
        return;

    if(!entity->tryPickup(getSeat()))
       return;

    entity->pickup();

    // Start tracking this creature as being in this player's hand
    addEntityToHand(entity);

    if (mGameMap->isServerGameMap())
    {
        entity->firePickupEntity(this);
        return;
    }

    OD_ASSERT_TRUE(this == mGameMap->getLocalPlayer());
    if (this == mGameMap->getLocalPlayer())
    {
        // Send a render request to move the crature into the "hand"
        RenderManager::getSingleton().rrPickUpEntity(entity, this);
    }
}

void Player::clearObjectsInHand()
{
    mObjectsInHand.clear();
}

bool Player::isDropHandPossible(Tile *t, unsigned int index)
{
    // if we have a creature to drop
    if (mObjectsInHand.empty())
        return false;

    GameEntity* entity = mObjectsInHand[index];
    return entity->tryDrop(getSeat(), t);
}

GameEntity* Player::dropHand(Tile *t, unsigned int index)
{
    // Add the creature to the map
    OD_ASSERT_TRUE_MSG(index < mObjectsInHand.size(), "playerNick=" + getNick() + ", index=" + Ogre::StringConverter::toString(index));
    if(index >= mObjectsInHand.size())
        return nullptr;

    GameEntity *entity = mObjectsInHand[index];
    mObjectsInHand.erase(mObjectsInHand.begin() + index);

    entity->drop(Ogre::Vector3(static_cast<Ogre::Real>(t->getX()),
            static_cast<Ogre::Real>(t->getY()), entity->getPosition().z));

    if(mGameMap->isServerGameMap())
    {
        entity->fireDropEntity(this, t);
        return entity;
    }

    // If this is the result of another player dropping the creature it is currently not visible so we need to create a mesh for it
    //cout << "\nthis:  " << this << "\nme:  " << gameMap->getLocalPlayer() << endl;
    //cout.flush();
    OD_ASSERT_TRUE(this == mGameMap->getLocalPlayer());
    if(this == mGameMap->getLocalPlayer())
    {
        // Send a render request to rearrange the creatures in the hand to move them all forward 1 place
        RenderManager::getSingleton().rrDropHand(entity, this);
    }

    return entity;
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
    if(mIsPlayerLostSent)
        return;

    mIsPlayerLostSent = true;
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
        for(Seat* seat : mGameMap->getSeats())
        {
            if(seat->getPlayer() == nullptr)
                continue;
            if(!seat->getPlayer()->getIsHuman())
                continue;
            if(!getSeat()->isAlliedSeat(seat))
                continue;

            ServerNotification *serverNotification = new ServerNotification(
            ServerNotificationType::chatServer, seat->getPlayer());
            serverNotification->mPacket << "You lost the game";
            ODServer::getSingleton().queueServerNotification(serverNotification);
        }
    }
    else
    {
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
                ServerNotification *serverNotification = new ServerNotification(
                ServerNotificationType::chatServer, seat->getPlayer());
                serverNotification->mPacket << "You lost";
                ODServer::getSingleton().queueServerNotification(serverNotification);
                continue;
            }

            ServerNotification *serverNotification = new ServerNotification(
            ServerNotificationType::chatServer, seat->getPlayer());
            serverNotification->mPacket << "An ally has lost";
            ODServer::getSingleton().queueServerNotification(serverNotification);
        }
    }
}

void Player::updateTime(Ogre::Real timeSinceLastUpdate)
{
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
        if(timeRemain > timeSinceLastUpdate)
        {
            isFightHappening = true;
            timeRemain -= timeSinceLastUpdate;
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

    if(mNoTreasuryAvailableTime > 0.0f)
    {
        if(mNoTreasuryAvailableTime > timeSinceLastUpdate)
            mNoTreasuryAvailableTime -= timeSinceLastUpdate;
        else
            mNoTreasuryAvailableTime = 0.0f;
    }

    if(isEventListUpdated)
        fireEvents();
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
        if(mGameMap->squaredCrowDistance(tile, event->getTile()) < MIN_DIST_EVENT_SQUARED)
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
    }

    // We add the fight event
    mEvents.push_back(new PlayerEvent(PlayerEventType::fight, tile, BATTLE_TIME_COUNT));
    fireEvents();
}

void Player::notifyNoTreasuryAvailable()
{
    if(mNoTreasuryAvailableTime == 0.0f)
    {
        mNoTreasuryAvailableTime = NO_TREASURY_TIME_COUNT;

        std::string chatMsg = "No treasury available. You should build a bigger one";
        ServerNotification *serverNotification = new ServerNotification(
            ServerNotificationType::chatServer, this);
        serverNotification->mPacket << chatMsg;
        ODServer::getSingleton().queueServerNotification(serverNotification);
    }
}

void Player::setCurrentAction(SelectedAction action)
{
    mCurrentAction = action;
    mNewTrapType = TrapType::nullTrapType;
    mNewRoomType = RoomType::nullRoomType;
    mNewSpellType = SpellType::nullSpellType;
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
        event->exportToPacket(mGameMap, serverNotification->mPacket);

    ODServer::getSingleton().queueServerNotification(serverNotification);
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

void Player::markTilesForDigging(bool marked, const std::vector<Tile*>& tiles, bool asyncMsg)
{
    if(tiles.empty())
        return;

    std::vector<Tile*> tilesMark;
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

    // If human player, we notify marked tiles
    if(!getIsHuman())
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
