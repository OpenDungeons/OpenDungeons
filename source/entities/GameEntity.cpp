/*!
 * \file   GameEntity.cpp
 * \date:  28 March 2012
 * \author StefanP.MUC
 * \brief  Provides the GameEntity class, the base class for all ingame objects
 *
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

#include "entities/GameEntity.h"

#include "entities/GameEntityType.h"
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

#include <cassert>

void EntityParticleEffect::exportParticleEffectToPacket(const EntityParticleEffect& effect, ODPacket& os)
{
    os << effect.mName;
    os << effect.mScript;
    os << effect.mNbTurnsEffect;
}

EntityParticleEffect* EntityParticleEffect::importParticleEffectFromPacket(ODPacket& is)
{
    static const std::vector<EntityParticleEffect*> effects;
    return importParticleEffectFromPacketIfNotInList(effects, is);
}

EntityParticleEffect* EntityParticleEffect::importParticleEffectFromPacketIfNotInList(const std::vector<EntityParticleEffect*>& effects, ODPacket& is)
{
    std::string name;
    std::string script;
    int32_t nbTurnsEffect;
    if(!(is >> name))
    {
        OD_LOG_ERR("error");
        return nullptr;
    }
    if(!(is >> script))
    {
        OD_LOG_ERR("error");
        return nullptr;
    }
    if(!(is >> nbTurnsEffect))
    {
        OD_LOG_ERR("error");
        return nullptr;
    }

    for(EntityParticleEffect* effect : effects)
    {
        if(effect->mName == name)
            return nullptr;
    }

    EntityParticleEffect* effect = new EntityParticleEffect(name, script, nbTurnsEffect);
    return effect;
}

GameEntity::GameEntity(
          GameMap*        gameMap,
          std::string     name,
          std::string     meshName,
          Seat*           seat
          ) :
    mPosition          (Ogre::Vector3::ZERO),
    mName              (name),
    mMeshName          (meshName),
    mMeshExists        (false),
    mSeat              (seat),
    mIsDeleteRequested (false),
    mParentSceneNode   (nullptr),
    mEntityNode        (nullptr),
    mGameMap           (gameMap),
    mIsOnMap           (false),
    mParticleSystemsNumber   (0),
    mCarryLock         (false),
    mEntityParentNodeAttach     (EntityParentNodeAttach::ATTACHED)
{
    assert(mGameMap != nullptr);
}

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

void GameEntity::addEntityToPositionTile()
{
    if(getIsOnMap())
        return;

    setIsOnMap(true);
    Tile* tile = getPositionTile();
    if(tile == nullptr)
    {
        OD_LOG_ERR(getGameMap()->serverStr() + "entityName=" + getName() + ", pos=" + Helper::toString(getPosition()));
        return;
    }
    OD_ASSERT_TRUE_MSG(tile->addEntity(this), getGameMap()->serverStr() + "entity=" + getName() + ", pos=" + Helper::toString(getPosition()) + ", tile=" + Tile::displayAsString(tile));
}

void GameEntity::removeEntityFromPositionTile()
{
    if(!getIsOnMap())
        return;

    setIsOnMap(false);
    Tile* tile = getPositionTile();
    if(tile == nullptr)
    {
        OD_LOG_ERR("entityName=" + getName() + ", pos=" + Helper::toString(getPosition()));
        return;
    }

    tile->removeEntity(this);
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

    for(auto it = mGameEntityListeners.begin(); it != mGameEntityListeners.end();)
    {
        GameEntityListener* listener = *it;
        if(listener->notifyPickedUp(this))
        {
            ++it;
            continue;
        }

        it = mGameEntityListeners.erase(it);
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

    for(auto it = mGameEntityListeners.begin(); it != mGameEntityListeners.end();)
    {
        GameEntityListener* listener = *it;
        if(listener->notifyDropped(this))
        {
            ++it;
            continue;
        }

        it = mGameEntityListeners.erase(it);
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

void GameEntity::exportToPacket(ODPacket& os, const Seat* seat) const
{
    int seatId = -1;
    if(mSeat != nullptr)
        seatId = mSeat->getId();

    os << seatId;
    os << mName;
    os << mMeshName;
    os << mPosition;

    uint32_t nbEffects = mEntityParticleEffects.size();
    os << nbEffects;
    for(EntityParticleEffect* effect : mEntityParticleEffects)
        EntityParticleEffect::exportParticleEffectToPacket(*effect, os);
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

    uint32_t nbEffects;
    OD_ASSERT_TRUE(is >> nbEffects);
    while(nbEffects > 0)
    {
        --nbEffects;
        EntityParticleEffect* effect = EntityParticleEffect::importParticleEffectFromPacket(is);
        if(effect == nullptr)
        {
            OD_LOG_ERR("entity=" + getName() + ", nbEffects=" + Helper::toString(nbEffects));
            break;
        }
        mEntityParticleEffects.push_back(effect);
    }
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

    // We do not export to stream the particle effects. It is the entity work to know if
    // they should be exported or not
}

bool GameEntity::importFromStream(std::istream& is)
{
    int seatId;
    if(!(is >> seatId))
        return false;

    if(seatId != -1)
        mSeat = mGameMap->getSeatById(seatId);

    if(!(is >> mName))
        return false;
    if(!(is >> mMeshName))
        return false;
    if(!(is >> mPosition.x >> mPosition.y >> mPosition.z))
        return false;

    // We do not import from stream the particle effects. It is the entity work to know if
    // they should be imported or not

    return true;
}

void GameEntity::destroyMeshLocal()
{
    if(!getIsOnServerMap())
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
    return getName() + "_Particle" + Helper::toString(++mParticleSystemsNumber);
}

void GameEntity::clientUpkeep()
{
    for(auto it = mEntityParticleEffects.begin(); it != mEntityParticleEffects.end();)
    {
        EntityParticleEffect* effect = *it;
        // We check if it is a permanent effect
        if(effect->mNbTurnsEffect < 0)
        {
            ++it;
            continue;
        }

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

void GameEntity::addGameEntityListener(GameEntityListener* listener)
{
    if(std::find(mGameEntityListeners.begin(), mGameEntityListeners.end(), listener) != mGameEntityListeners.end())
    {
        OD_LOG_ERR("Entity=" + listener->getListenerName() + " already listening entity=" + getName());
        return;
    }
    mGameEntityListeners.push_back(listener);
}

void GameEntity::removeGameEntityListener(GameEntityListener* listener)
{
    auto it = std::find(mGameEntityListeners.begin(), mGameEntityListeners.end(), listener);
    if(it == mGameEntityListeners.end())
    {
        OD_LOG_ERR("Entity=" + listener->getListenerName() + " not listening entity=" + getName() + " but wants to stop listening");
        return;
    }

    mGameEntityListeners.erase(it);
}

void GameEntity::fireEntityDead()
{
    for(auto it = mGameEntityListeners.begin(); it != mGameEntityListeners.end();)
    {
        GameEntityListener* listener = *it;
        if(listener->notifyDead(this))
        {
            ++it;
            continue;
        }

        it = mGameEntityListeners.erase(it);
    }
}

void GameEntity::fireEntityRemoveFromGameMap()
{
    for(auto it = mGameEntityListeners.begin(); it != mGameEntityListeners.end();)
    {
        GameEntityListener* listener = *it;
        if(listener->notifyRemovedFromGameMap(this))
        {
            ++it;
            continue;
        }

        it = mGameEntityListeners.erase(it);
    }
}

bool GameEntity::getIsOnServerMap() const
{
    return mGameMap->isServerGameMap();
}

void GameEntity::exportToStream(GameEntity* entity, std::ostream& os)
{
    entity->exportHeadersToStream(os);
    entity->exportToStream(os);
}

std::string GameEntity::getOgreNamePrefix() const
{
    return Helper::toString(static_cast<int32_t>(getObjectType())) + "-";
}

void GameEntity::createMesh()
{
    if (mMeshExists)
        return;

    mMeshExists = true;
    createMeshLocal();
}

void GameEntity::destroyMesh()
{
    if(!mMeshExists)
        return;

    mMeshExists = false;

    destroyMeshLocal();
}

void GameEntity::exportToPacketForUpdate(ODPacket& os, const Seat* seat) const
{
    uint32_t nbCreatureEffect = mEntityParticleEffects.size();
    os << nbCreatureEffect;
    for(EntityParticleEffect* effect : mEntityParticleEffects)
        EntityParticleEffect::exportParticleEffectToPacket(*effect, os);
}

void GameEntity::updateFromPacket(ODPacket& is)
{
    uint32_t nbEffects;
    OD_ASSERT_TRUE(is >> nbEffects);
    // We copy the list of effects currently on this entity. That will allow to
    // check if the effect is already on it and only display the effect if it is not
    std::vector<EntityParticleEffect*> currentEffects = mEntityParticleEffects;
    while(nbEffects > 0)
    {
        --nbEffects;

        EntityParticleEffect* effect = EntityParticleEffect::importParticleEffectFromPacketIfNotInList(currentEffects, is);
        if(effect == nullptr)
            continue;

        effect->mParticleSystem = RenderManager::getSingleton().rrEntityAddParticleEffect(this,
            effect->mName, effect->mScript);
        mEntityParticleEffects.push_back(effect);
    }
}

void GameEntity::notifyFightPlayer(Tile* tile)
{
    getGameMap()->playerIsFighting(getSeat()->getPlayer(), tile);
}

void GameEntity::setParentNodeDetachFlags(uint32_t mask, bool value)
{
    // We save the current attach  state
    bool oldState = (mEntityParentNodeAttach == EntityParentNodeAttach::ATTACHED);
    // We compute the new attach state
    mEntityParentNodeAttach = (value ? mEntityParentNodeAttach | mask : mEntityParentNodeAttach & ~mask);

    // If the attach state changed, we do what is needed
    bool newState = (mEntityParentNodeAttach == EntityParentNodeAttach::ATTACHED);
    if(oldState == newState)
        return;

    if(newState)
        RenderManager::getSingleton().rrAttachEntity(this);
    else
        RenderManager::getSingleton().rrDetachEntity(this);
}
