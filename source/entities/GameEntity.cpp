/*!
 * \file   GameEntity.cpp
 * \date:  28 March 2012
 * \author StefanP.MUC
 * \brief  Provides the GameEntity class, the base class for all ingame objects
 *
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

#include "entities/GameEntity.h"

#include "entities/Tile.h"

#include "game/Player.h"
#include "game/Seat.h"

#include "gamemap/GameMap.h"
#include "network/ODPacket.h"
#include "network/ODServer.h"
#include "network/ServerNotification.h"

#include "render/RenderManager.h"

#include "utils/Helper.h"
#include "utils/LogManager.h"

void GameEntity::deleteYourself()
{
    destroyMesh();
    if(mIsDeleteRequested)
        return;

    mIsDeleteRequested = true;

    getGameMap()->queueEntityForDeletion(this);
}

Tile* GameEntity::getPositionTile() const
{
    const Ogre::Vector3& tempPosition = getPosition();

    return getGameMap()->getTile(Helper::round(tempPosition.x),
                                 Helper::round(tempPosition.y));
}

bool GameEntity::addEntityToTile(Tile* tile)
{
    return tile->addEntity(this);
}

bool GameEntity::removeEntityFromTile(Tile* tile)
{
    return tile->removeEntity(this);
}

void GameEntity::firePickupEntity(Player* playerPicking)
{
    int seatId = playerPicking->getSeat()->getId();
    GameEntityType entityType = getObjectType();
    const std::string& entityName = getName();
    for(std::vector<Seat*>::iterator it = mSeatsWithVisionNotified.begin(); it != mSeatsWithVisionNotified.end();)
    {
        Seat* seat = *it;
        if(seat->getPlayer() == nullptr)
        {
            ++it;
            continue;
        }
        if(!seat->getPlayer()->getIsHuman())
        {
            ++it;
            continue;
        }

        // For other players than the one picking up the entity, we send a remove message
        if(seat->getPlayer() != playerPicking)
        {
            fireRemoveEntity(seat);
            it = mSeatsWithVisionNotified.erase(it);
            continue;
        }

        ++it;

        // If the creature was picked up by a human, we send an async message
        if(playerPicking->getIsHuman())
        {
            ServerNotification serverNotification(
                ServerNotificationType::entityPickedUp, seat->getPlayer());
            serverNotification.mPacket << seatId << entityType << entityName;
            ODServer::getSingleton().sendAsyncMsg(serverNotification);
        }
        else
        {
            ServerNotification* serverNotification = new ServerNotification(
                ServerNotificationType::entityPickedUp, seat->getPlayer());
            serverNotification->mPacket << seatId << entityType << entityName;
            ODServer::getSingleton().queueServerNotification(serverNotification);
        }
    }
}

void GameEntity::fireDropEntity(Player* playerPicking, Tile* tile)
{
    // If the player is a human, we send an asynchronous message to be as reactive as
    // possible. If it is an AI, we queue the message because it might have been created
    // during this turn (and, thus, not exist on client side)
    int seatId = playerPicking->getSeat()->getId();
    for(Seat* seat : getGameMap()->getSeats())
    {
        if(seat->getPlayer() == nullptr)
            continue;
        if(!seat->getPlayer()->getIsHuman())
            continue;
        if(!seat->hasVisionOnTile(tile))
            continue;

        // For players with vision on the tile where the entity is dropped, we send an add message
        if(seat->getPlayer() != playerPicking)
        {
            // Because the entity is dropped, it is not on the map for the other players so no need
            // to check
            mSeatsWithVisionNotified.push_back(seat);
            fireAddEntity(seat, false);
            continue;
        }

        // If the creature was dropped by a human, we send an async message
        if(playerPicking->getIsHuman())
        {
            ServerNotification serverNotification(
                ServerNotificationType::entityDropped, seat->getPlayer());
            serverNotification.mPacket << seatId;
            getGameMap()->tileToPacket(serverNotification.mPacket, tile);
            ODServer::getSingleton().sendAsyncMsg(serverNotification);
        }
        else
        {
            ServerNotification* serverNotification = new ServerNotification(
                ServerNotificationType::entityDropped, seat->getPlayer());
            serverNotification->mPacket << seatId;
            getGameMap()->tileToPacket(serverNotification->mPacket, tile);
            ODServer::getSingleton().queueServerNotification(serverNotification);
        }
    }
}

void GameEntity::notifySeatsWithVision(const std::vector<Seat*>& seats)
{
    // We notify seats that lost vision
    for(std::vector<Seat*>::iterator it = mSeatsWithVisionNotified.begin(); it != mSeatsWithVisionNotified.end();)
    {
        Seat* seat = *it;
        // If the seat is still in the list, nothing to do
        if(std::find(seats.begin(), seats.end(), seat) != seats.end())
        {
            ++it;
            continue;
        }

        it = mSeatsWithVisionNotified.erase(it);

        if(seat->getPlayer() == nullptr)
            continue;
        if(!seat->getPlayer()->getIsHuman())
            continue;

        fireRemoveEntity(seat);
    }

    // We notify seats that gain vision
    for(Seat* seat : seats)
    {
        // If the seat was already in the list, nothing to do
        if(std::find(mSeatsWithVisionNotified.begin(), mSeatsWithVisionNotified.end(), seat) != mSeatsWithVisionNotified.end())
            continue;

        mSeatsWithVisionNotified.push_back(seat);

        if(seat->getPlayer() == nullptr)
            continue;
        if(!seat->getPlayer()->getIsHuman())
            continue;

        fireAddEntity(seat, false);
    }
}

void GameEntity::addSeatWithVision(Seat* seat, bool async)
{
    if(std::find(mSeatsWithVisionNotified.begin(), mSeatsWithVisionNotified.end(), seat) != mSeatsWithVisionNotified.end())
        return;

    mSeatsWithVisionNotified.push_back(seat);
    fireAddEntity(seat, async);
}

void GameEntity::removeSeatWithVision(Seat* seat)
{
    std::vector<Seat*>::iterator it = std::find(mSeatsWithVisionNotified.begin(), mSeatsWithVisionNotified.end(), seat);
    if(it == mSeatsWithVisionNotified.end())
        return;

    mSeatsWithVisionNotified.erase(it);
    fireRemoveEntity(seat);
}

void GameEntity::fireRemoveEntityToSeatsWithVision()
{
    for(Seat* seat : mSeatsWithVisionNotified)
    {
        if(seat->getPlayer() == nullptr)
            continue;
        if(!seat->getPlayer()->getIsHuman())
            continue;

        fireRemoveEntity(seat);
    }

    mSeatsWithVisionNotified.clear();
}

std::string GameEntity::getGameEntityStreamFormat()
{
    return "SeatId\tName\tMeshName\tPosX\tPosY\tPosZ";
}


void GameEntity::exportHeadersToStream(std::ostream& os) const
{
    // GameEntity are saved in the level file per type. For this reason, there is no
    // need to save the type
}

void GameEntity::exportHeadersToPacket(ODPacket& os) const
{
    os << getObjectType();
}

void GameEntity::exportToPacket(ODPacket& os) const
{
    int seatId = -1;
    if(mSeat != nullptr)
        seatId = mSeat->getId();

    os << seatId;
    os << mName;
    os << mMeshName;
    os << mPosition;
}

void GameEntity::importFromPacket(ODPacket& is)
{
    int seatId;
    OD_ASSERT_TRUE(is >> seatId);
    if(seatId != -1)
        mSeat = mGameMap->getSeatById(seatId);

    OD_ASSERT_TRUE(is >> mName);
    OD_ASSERT_TRUE(is >> mMeshName);
    OD_ASSERT_TRUE(is >> mPosition);
}

void GameEntity::exportToStream(std::ostream& os) const
{
    int seatId = -1;
    if(mSeat != nullptr)
        seatId = mSeat->getId();

    os << seatId << "\t";
    os << mName << "\t";
    os << mMeshName << "\t";
    os << mPosition.x << "\t" << mPosition.y << "\t" << mPosition.z << "\t";
}

void GameEntity::importFromStream(std::istream& is)
{
    int seatId;
    OD_ASSERT_TRUE(is >> seatId);
    if(seatId != -1)
        mSeat = mGameMap->getSeatById(seatId);

    OD_ASSERT_TRUE(is >> mName);
    OD_ASSERT_TRUE(is >> mMeshName);
    OD_ASSERT_TRUE(is >> mPosition.x >> mPosition.y >> mPosition.z);
}

void GameEntity::destroyMeshLocal()
{
    EntityBase::destroyMeshLocal();
    if(!getGameMap()->isServerGameMap())
    {
        for(EntityParticleEffect* effect : mEntityParticleEffects)
            RenderManager::getSingleton().rrEntityRemoveParticleEffect(this, effect->mParticleSystem);
    }

    for(EntityParticleEffect* effect : mEntityParticleEffects)
        delete effect;

    mEntityParticleEffects.clear();
}

std::string GameEntity::nextParticleSystemsName()
{
    return getName() + "_Particle" + Helper::toString(mParticleSystemsNumber);
}

void GameEntity::clientUpkeep()
{
    for(auto it = mEntityParticleEffects.begin(); it != mEntityParticleEffects.end();)
    {
        EntityParticleEffect* effect = *it;
        // We check if it is a permanent effect
        if(effect->mNbTurnsEffect < 0)
            continue;

        // We check if the effect is still active
        if(effect->mNbTurnsEffect > 0)
        {
            --effect->mNbTurnsEffect;
            ++it;
            continue;
        }

        // Remove the effect
        RenderManager::getSingleton().rrEntityRemoveParticleEffect(this, effect->mParticleSystem);
        it = mEntityParticleEffects.erase(it);
        delete effect;
    }
}

void GameEntity::restoreEntityState()
{
    for(EntityParticleEffect* effect : mEntityParticleEffects)
    {
        effect->mParticleSystem = RenderManager::getSingleton().rrEntityAddParticleEffect(this,
            effect->mName, effect->mScript);
    }
}
