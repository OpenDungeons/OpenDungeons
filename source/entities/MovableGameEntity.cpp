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

#include "entities/MovableGameEntity.h"

#include "entities/Tile.h"

#include "gamemap/GameMap.h"
#include "network/ODServer.h"
#include "network/ServerNotification.h"
#include "render/RenderManager.h"

#include "utils/Helper.h"
#include "utils/LogManager.h"

#include "ODApplication.h"

MovableGameEntity::MovableGameEntity(GameMap* gameMap, const std::string& initialAnimationState,
        bool initialAnimationLoop) :
    GameEntity(gameMap),
    mAnimationState(nullptr),
    mMoveSpeed(1.0),
    mPrevAnimationState(initialAnimationState),
    mPrevAnimationStateLoop(initialAnimationLoop),
    mAnimationSpeedFactor(1.0),
    mDestinationAnimationState("Idle"),
    mWalkDirection(Ogre::Vector3::ZERO),
    mAnimationTime(0.0)
{
}

void MovableGameEntity::setMoveSpeed(double s)
{
    mMoveSpeed = s;

    if (!getGameMap()->isServerGameMap())
        return;

    for(Seat* seat : mSeatsWithVisionNotified)
    {
        if(seat->getPlayer() == nullptr)
            continue;
        if(!seat->getPlayer()->getIsHuman())
            continue;

        const std::string& name = getName();
        ServerNotification *serverNotification = new ServerNotification(
            ServerNotification::setMoveSpeed, seat->getPlayer());
        serverNotification->mPacket << name << s;
        ODServer::getSingleton().queueServerNotification(serverNotification);
    }
}

void MovableGameEntity::addDestination(Ogre::Real x, Ogre::Real y, Ogre::Real z)
{
    Ogre::Vector3 destination(x, y, z);

    mWalkQueue.push_back(destination);

    if (!getGameMap()->isServerGameMap())
        return;

    for(Seat* seat : mSeatsWithVisionNotified)
    {
        if(seat->getPlayer() == nullptr)
            continue;
        if(!seat->getPlayer()->getIsHuman())
            continue;

        const std::string& name = getName();
        ServerNotification *serverNotification = new ServerNotification(
            ServerNotification::animatedObjectAddDestination, seat->getPlayer());
        serverNotification->mPacket << name << destination;
        ODServer::getSingleton().queueServerNotification(serverNotification);
    }
}

bool MovableGameEntity::isMoving()
{
    return !mWalkQueue.empty();
}

bool MovableGameEntity::setWalkPath(std::list<Tile*> path,
                                    unsigned int minDestinations, bool addFirstStop)
{
    // Remove any existing stops from the walk queue.
    clearDestinations();

    // Verify that the given path is long enough to be considered valid.
    if (path.size() >= minDestinations)
    {
        std::list<Tile*>::iterator itr = path.begin();

        // If we are not supposed to add the first tile in the path to the destination queue, then we skip over it.
        if (!addFirstStop)
            ++itr;

        // Loop over the path adding each tile as a destination in the walkQueue.
        while (itr != path.end())
        {
            addDestination((*itr)->getX(), (*itr)->getY());
            ++itr;
        }

        return true;
    }

    return false;
}

void MovableGameEntity::clearDestinations()
{
    mWalkQueue.clear();
    stopWalking();

    if (!getGameMap()->isServerGameMap())
        return;

    for(Seat* seat : mSeatsWithVisionNotified)
    {
        if(seat->getPlayer() == nullptr)
            continue;
        if(!seat->getPlayer()->getIsHuman())
            continue;

        const std::string& name = getName();
        ServerNotification *serverNotification = new ServerNotification(
            ServerNotification::animatedObjectClearDestinations, seat->getPlayer());
        serverNotification->mPacket << name;
        ODServer::getSingleton().queueServerNotification(serverNotification);
    }
}

void MovableGameEntity::stopWalking()
{
    // Set the animation state of this object to the state that was set for it to enter into after it reaches it's destination.
    setAnimationState(mDestinationAnimationState);
}

void MovableGameEntity::setWalkDirection(const Ogre::Vector3& direction)
{
    mWalkDirection = direction;
    if(getGameMap()->isServerGameMap())
        return;

    RenderManager::getSingleton().rrOrientEntityToward(this, direction);
}

void MovableGameEntity::setAnimationState(const std::string& state, bool loop, const Ogre::Vector3& direction)
{
    // Ignore the command if the command is exactly the same and looped. Otherwise, we accept
    // the command because it may be a trap/building object that is triggered several times
    if (state.compare(mPrevAnimationState) == 0 &&
        loop &&
        mPrevAnimationStateLoop &&
        (direction == Ogre::Vector3::ZERO || direction == mWalkDirection))
    {
        return;
    }

    mAnimationTime = 0;
    mPrevAnimationState = state;
    mPrevAnimationStateLoop = loop;

    if(direction != Ogre::Vector3::ZERO)
        setWalkDirection(direction);

    // NOTE : if we add support to increase speed like a spell or slapping, it
    // would be nice to increase speed factor
    setAnimationSpeedFactor(1.0);

    if (getGameMap()->isServerGameMap())
    {
        fireObjectAnimationState(state, loop, direction);
        return;
    }

    RenderManager::getSingleton().rrSetObjectAnimationState(this, state, loop);
}

double MovableGameEntity::getAnimationSpeedFactor()
{
    return mAnimationSpeedFactor;
}

void MovableGameEntity::setAnimationSpeedFactor(double f)
{
    mAnimationSpeedFactor = f;
}

void MovableGameEntity::update(Ogre::Real timeSinceLastFrame)
{
    // Advance the animation
    double addedTime = static_cast<Ogre::Real>(ODApplication::turnsPerSecond
         * static_cast<double>(timeSinceLastFrame)
         * getAnimationSpeedFactor());
    mAnimationTime += addedTime;
    if (!getGameMap()->isServerGameMap() && getAnimationState() != nullptr)
        getAnimationState()->addTime(static_cast<Ogre::Real>(addedTime));

    if (mWalkQueue.empty())
        return;

    // Move the entity

    // Note: When the client and the server are using different frame rates, the entities walk at different speeds
    // If this happens to become a problem, resyncing mechanisms will be needed.
    double moveDist = ODApplication::turnsPerSecond
                      * getMoveSpeed()
                      * timeSinceLastFrame;
    Ogre::Vector3 newPosition = getPosition();
    Ogre::Vector3 nextDest = mWalkQueue.front();
    Ogre::Vector3 walkDirection = nextDest - newPosition;
    walkDirection.normalise();

    while(moveDist > 0.0)
    {
        Ogre::Real distToNextDest = newPosition.distance(nextDest);
        if(distToNextDest > moveDist)
        {
            newPosition = newPosition + walkDirection * static_cast<Ogre::Real>(moveDist);
            break;
        }
        else
        {
            // We have reached the destination. We go to the next if available
            newPosition = nextDest;
            moveDist -= distToNextDest;
            mWalkQueue.pop_front();
            if(mWalkQueue.empty())
            {
                // Stop walking
                stopWalking();
                break;
            }

            nextDest = mWalkQueue.front();
            walkDirection = nextDest - newPosition;
            walkDirection.normalise();
        }
    }

    setWalkDirection(walkDirection);
    setPosition(newPosition, true);
}

void MovableGameEntity::setPosition(const Ogre::Vector3& v, bool isMove)
{
    Tile* oldTile = getPositionTile();
    GameEntity::setPosition(v, isMove);
    if(!getIsOnMap())
        return;

    if(!getGameMap()->isServerGameMap())
    {
        RenderManager::getSingleton().rrMoveEntity(this, v);
        return;
    }

    Tile* tile = getPositionTile();
    OD_ASSERT_TRUE_MSG(tile != nullptr, "entityName=" + getName());
    if(tile == nullptr)
        return;
    if(isMove && (tile == oldTile))
        return;

    if((oldTile != nullptr) && isMove)
    {
        OD_ASSERT_TRUE_MSG(removeEntityFromTile(oldTile), "name=" + getName());
    }

    OD_ASSERT_TRUE_MSG(addEntityToTile(tile), "name=" + getName());
}

bool MovableGameEntity::addEntityToTile(Tile* tile)
{
    return tile->addEntity(this);
}

bool MovableGameEntity::removeEntityFromTile(Tile* tile)
{
    return tile->removeEntity(this);
}

void MovableGameEntity::notifySeatsWithVision(const std::vector<Seat*>& seats)
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

void MovableGameEntity::addSeatWithVision(Seat* seat, bool async)
{
    if(std::find(mSeatsWithVisionNotified.begin(), mSeatsWithVisionNotified.end(), seat) != mSeatsWithVisionNotified.end())
        return;

    mSeatsWithVisionNotified.push_back(seat);
    fireAddEntity(seat, async);
}

void MovableGameEntity::removeSeatWithVision(Seat* seat)
{
    std::vector<Seat*>::iterator it = std::find(mSeatsWithVisionNotified.begin(), mSeatsWithVisionNotified.end(), seat);
    if(it == mSeatsWithVisionNotified.end())
        return;

    mSeatsWithVisionNotified.erase(it);
    fireRemoveEntity(seat);
}

void MovableGameEntity::fireRemoveEntityToSeatsWithVision()
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

void MovableGameEntity::fireObjectAnimationState(const std::string& state, bool loop, const Ogre::Vector3& direction)
{
    for(Seat* seat : mSeatsWithVisionNotified)
    {
        if(seat->getPlayer() == nullptr)
            continue;
        if(!seat->getPlayer()->getIsHuman())
            continue;

        ServerNotification* serverNotification = new ServerNotification(
            ServerNotification::setObjectAnimationState, seat->getPlayer());
        const std::string& name = getName();
        serverNotification->mPacket << name << state << loop;
        if(direction != Ogre::Vector3::ZERO)
            serverNotification->mPacket << true << direction;
        else if(mWalkDirection != Ogre::Vector3::ZERO)
            serverNotification->mPacket << true << mWalkDirection;
        else
            serverNotification->mPacket << false;
        ODServer::getSingleton().queueServerNotification(serverNotification);
    }
}

void MovableGameEntity::firePickupEntity(Player* playerPicking, bool isEditorMode)
{
    int seatId = playerPicking->getSeat()->getId();
    GameEntity::ObjectType entityType = getObjectType();
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
                ServerNotification::entityPickedUp, seat->getPlayer());
            serverNotification.mPacket << isEditorMode << seatId << entityType << entityName;
            ODServer::getSingleton().sendAsyncMsg(serverNotification);
        }
        else
        {
            ServerNotification* serverNotification = new ServerNotification(
                ServerNotification::entityPickedUp, seat->getPlayer());
            serverNotification->mPacket << isEditorMode << seatId << entityType << entityName;
            ODServer::getSingleton().queueServerNotification(serverNotification);
        }
    }
}

void MovableGameEntity::fireDropEntity(Player* playerPicking, Tile* tile)
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
                ServerNotification::entityDropped, seat->getPlayer());
            serverNotification.mPacket << seatId;
            getGameMap()->tileToPacket(serverNotification.mPacket, tile);
            ODServer::getSingleton().sendAsyncMsg(serverNotification);
        }
        else
        {
            ServerNotification* serverNotification = new ServerNotification(
                ServerNotification::entityDropped, seat->getPlayer());
            serverNotification->mPacket << seatId;
            getGameMap()->tileToPacket(serverNotification->mPacket, tile);
            ODServer::getSingleton().queueServerNotification(serverNotification);
        }
    }
}

void MovableGameEntity::exportToStream(std::ostream& os) const
{
    // Note : When we will implement game save, we should consider saving informations
    // about animation (like exportToPacket does)
}

void MovableGameEntity::importFromStream(std::istream& is)
{
    // Note : When we will implement game save, we should consider saving informations
    // about animation (like importFromPacket does)
}

void MovableGameEntity::exportToPacket(ODPacket& os) const
{
    os << mMoveSpeed;
    os << mPrevAnimationState;
    os << mPrevAnimationStateLoop;
    os << mWalkDirection;
    os << mAnimationSpeedFactor;
    os << mAnimationTime;

    int32_t nbDestinations = mWalkQueue.size();
    os << nbDestinations;
    for(const Ogre::Vector3& dest : mWalkQueue)
    {
        os << dest;
    }
}

void MovableGameEntity::importFromPacket(ODPacket& is)
{
    OD_ASSERT_TRUE(is >> mMoveSpeed);
    OD_ASSERT_TRUE(is >> mPrevAnimationState);
    OD_ASSERT_TRUE(is >> mPrevAnimationStateLoop);
    OD_ASSERT_TRUE(is >> mWalkDirection);
    OD_ASSERT_TRUE(is >> mAnimationSpeedFactor);
    OD_ASSERT_TRUE(is >> mAnimationTime);

    int32_t nbDestinations;
    OD_ASSERT_TRUE(is >> nbDestinations);
    for(int32_t i = 0; i < nbDestinations; ++i)
    {
        Ogre::Vector3 dest;
        OD_ASSERT_TRUE(is >> dest);
        addDestination(dest.x, dest.y, dest.z);
    }
}

void MovableGameEntity::restoreEntityState()
{
    if(!mPrevAnimationState.empty())
    {
        setAnimationSpeedFactor(mAnimationSpeedFactor);
        RenderManager::getSingleton().rrSetObjectAnimationState(this, mPrevAnimationState, mPrevAnimationStateLoop);

        if(mWalkDirection != Ogre::Vector3::ZERO)
            RenderManager::getSingleton().rrOrientEntityToward(this, mWalkDirection);

        // If the mesh has no skeleton, getAnimationState() could return null
        if(getAnimationState() != nullptr)
            getAnimationState()->addTime(mAnimationTime);
    }
}

void MovableGameEntity::notifyRemovedFromGamemap()
{
    if(!getGameMap()->isServerGameMap())
        return;

    fireRemoveEntityToSeatsWithVision();
}
