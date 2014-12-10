/*
 *  Copyright (C) 2011-2014  OpenDungeons Team
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

#include "game/Seat.h"

#include "entities/Creature.h"
#include "entities/RenderedMovableEntity.h"

#include "gamemap/GameMap.h"

#include "network/ODServer.h"
#include "network/ServerNotification.h"
#include "network/ODClient.h"

#include "render/RenderManager.h"

#include "utils/LogManager.h"

Player::Player(GameMap* gameMap, int32_t id) :
    mId(id),
    mNewRoomType(Room::nullRoomType),
    mNewTrapType(Trap::nullTrapType),
    mCurrentAction(SelectedAction::none),
    mGameMap(gameMap),
    mSeat(nullptr),
    mIsHuman(false),
    mFightingTime(0.0f),
    mIsPlayerLostSent(false)
{
}

unsigned int Player::numCreaturesInHand(const Seat* seat) const
{
    unsigned int cpt = 0;
    for(std::vector<GameEntity*>::const_iterator it = mObjectsInHand.begin(); it != mObjectsInHand.end(); ++it)
    {
        const GameEntity* entity = *it;
        if(entity->getObjectType() != GameEntity::ObjectType::creature)
            continue;

        if(seat != NULL && entity->getSeat() != seat)
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
    mObjectsInHand.push_back(NULL);
    for (unsigned int j = mObjectsInHand.size() - 1; j > 0; --j)
        mObjectsInHand[j] = mObjectsInHand[j - 1];

    mObjectsInHand[0] = entity;
}

void Player::pickUpEntity(GameEntity *entity, bool isEditorMode)
{
    if (!ODServer::getSingleton().isConnected() && !ODClient::getSingleton().isConnected())
        return;

    if(entity->getObjectType() == GameEntity::ObjectType::creature)
    {
        Creature* creature = static_cast<Creature*>(entity);
        if(!creature->tryPickup(getSeat(), isEditorMode))
           return;

       creature->pickup();
    }
    else if(entity->getObjectType() == GameEntity::ObjectType::renderedMovableEntity)
    {
        RenderedMovableEntity* obj = static_cast<RenderedMovableEntity*>(entity);
        if(!obj->tryPickup(getSeat(), isEditorMode))
           return;

        obj->pickup();
    }

    // Start tracking this creature as being in this player's hand
    addEntityToHand(entity);

    if (mGameMap->isServerGameMap())
    {
        // If the player is a human, we send an asynchronous message to be as reactive as
        // possible. If it is an AI, we queue the message because it might have been created
        // during this turn (and, thus, not exist on client side)
        int seatId = getSeat()->getId();
        GameEntity::ObjectType entityType = entity->getObjectType();
        const std::string& entityName = entity->getName();
        if(getIsHuman())
        {
            ServerNotification serverNotification(ServerNotification::entityPickedUp, this);
            serverNotification.mPacket << isEditorMode << seatId << entityType << entityName;
            ODServer::getSingleton().sendAsyncMsgToAllClients(serverNotification);
            return;
        }
        else
        {
            ServerNotification* serverNotification = new ServerNotification(ServerNotification::entityPickedUp,
                this);
            serverNotification->mPacket << isEditorMode << seatId << entityType << entityName;
            ODServer::getSingleton().queueServerNotification(serverNotification);
            return;
        }
    }

    // If it is actually the user picking up a creature we move the scene node.
    // Otherwise we just hide the creature from the map.
    if (this == mGameMap->getLocalPlayer())
    {
        // Send a render request to move the crature into the "hand"
        RenderManager::getSingleton().rrPickUpEntity(entity, this);
    }
    else // it is just a message indicating another player has picked up a creature
    {
        // Hide the creature
        RenderManager::getSingleton().rrDetachEntity(entity);
    }
}

void Player::clearObjectsInHand()
{
    mObjectsInHand.clear();
}

bool Player::isDropHandPossible(Tile *t, unsigned int index, bool isEditorMode)
{
    // if we have a creature to drop
    if (mObjectsInHand.empty())
        return false;

    GameEntity* entity = mObjectsInHand[index];
    if(entity != NULL && entity->getObjectType() == GameEntity::ObjectType::creature)
    {
        Creature* creature = static_cast<Creature*>(entity);
        if(creature->tryDrop(getSeat(), t, isEditorMode))
            return true;
    }
    else if(entity != NULL && entity->getObjectType() == GameEntity::ObjectType::renderedMovableEntity)
    {
        RenderedMovableEntity* obj = static_cast<RenderedMovableEntity*>(entity);
        if(obj->tryDrop(getSeat(), t, isEditorMode))
            return true;
    }

    return false;
}

GameEntity* Player::dropHand(Tile *t, unsigned int index)
{
    // Add the creature to the map
    OD_ASSERT_TRUE(index < mObjectsInHand.size());
    if(index >= mObjectsInHand.size())
        return NULL;

    GameEntity *entity = mObjectsInHand[index];
    mObjectsInHand.erase(mObjectsInHand.begin() + index);
    if(entity->getObjectType() == GameEntity::ObjectType::creature)
    {
        Creature* creature = static_cast<Creature*>(entity);
        creature->drop(Ogre::Vector3(static_cast<Ogre::Real>(t->x),
            static_cast<Ogre::Real>(t->y), 0.0));

        if (!mGameMap->isServerGameMap() && (this == mGameMap->getLocalPlayer()))
        {
            creature->playSound(CreatureSound::DROP);
        }
    }
    else if(entity->getObjectType() == GameEntity::ObjectType::renderedMovableEntity)
    {
        RenderedMovableEntity* obj = static_cast<RenderedMovableEntity*>(entity);
        obj->drop(Ogre::Vector3(static_cast<Ogre::Real>(t->x),
            static_cast<Ogre::Real>(t->y), 0.0));
    }

    if(mGameMap->isServerGameMap())
    {
        // If the player is a human, we send an asynchronous message to be as reactive as
        // possible. If it is an AI, we queue the message because it might have been created
        // during this turn (and, thus, not exist on client side)
        int seatId = getSeat()->getId();
        if(getIsHuman())
        {
            ServerNotification serverNotification(ServerNotification::entityDropped, this);
            serverNotification.mPacket << seatId << t;
            ODServer::getSingleton().sendAsyncMsgToAllClients(serverNotification);
            return entity;
        }
        else
        {
            ServerNotification* serverNotification = new ServerNotification(ServerNotification::entityDropped,
                this);
            serverNotification->mPacket << seatId << t;
            ODServer::getSingleton().queueServerNotification(serverNotification);
            return entity;
        }
    }

    // If this is the result of another player dropping the creature it is currently not visible so we need to create a mesh for it
    //cout << "\nthis:  " << this << "\nme:  " << gameMap->getLocalPlayer() << endl;
    //cout.flush();
    if (this != mGameMap->getLocalPlayer())
    {
        RenderManager::getSingleton().rrAttachEntity(entity);
    }
    else // This is the result of the player on the local computer dropping the creature
    {
        // Send a render request to rearrange the creatures in the hand to move them all forward 1 place
        RenderManager::getSingleton().rrDropHand(entity, this);
    }

    return entity;
}

void Player::rotateHand(int n)
{
    // If there are no creatures or only one creature in our hand, rotation doesn't change the order.
    if (mObjectsInHand.size() <= 1)
        return;

    for (unsigned int i = 0; i < (unsigned int) fabs((double) n); ++i)
    {
        if (n > 0)
        {
            GameEntity* entity = mObjectsInHand.back();

            // Since vectors have no push_front method
            // we need to move all of the elements in the vector back one and then add this one to the beginning.
            for (unsigned int j = mObjectsInHand.size() - 1; j > 0; --j)
                mObjectsInHand[j] = mObjectsInHand[j - 1];

            mObjectsInHand[0] = entity;

        }
        else
        {
            GameEntity* entity = mObjectsInHand.front();
            mObjectsInHand.erase(mObjectsInHand.begin());
            mObjectsInHand.push_back(entity);
        }
    }

    // Send a render request to move the entity into the "hand"
    RenderManager::getSingleton().rrRotateHand(this);
}

void Player::notifyNoMoreDungeonTemple()
{
    if(mIsPlayerLostSent)
        return;

    mIsPlayerLostSent = true;
    ServerNotification *serverNotification = new ServerNotification(
        ServerNotification::playerLost, this);
    ODServer::getSingleton().queueServerNotification(serverNotification);

}
