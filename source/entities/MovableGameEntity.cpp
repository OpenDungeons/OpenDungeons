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

#include "entities/MovableGameEntity.h"

#include "entities/Tile.h"
#include "game/Player.h"
#include "game/Seat.h"
#include "gamemap/GameMap.h"
#include "network/ODServer.h"
#include "network/ServerNotification.h"
#include "render/RenderManager.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"
#include "utils/Random.h"
#include "ODApplication.h"

#include <OgreAnimationState.h>

MovableGameEntity::MovableGameEntity(GameMap* gameMap) :
    GameEntity(gameMap),
    mAnimationState(nullptr),
    mDestinationAnimationState(EntityAnimation::idle_anim),
    mDestinationAnimationLoop(false),
    mDestinationPlayIdleWhenAnimationEnds(false),
    mDestinationAnimationDirection(Ogre::Vector3::ZERO),
    mWalkDirection(Ogre::Vector3::ZERO),
    mAnimationTime(0.0)
{
}

bool MovableGameEntity::isMoving()
{
    return !mWalkQueue.empty();
}

void MovableGameEntity::tileToVector3(const std::list<Tile*>& tiles, std::vector<Ogre::Vector3>& path,
    bool skipFirst, Ogre::Real z)
{
    for(Tile* tile : tiles)
    {
        if(skipFirst)
        {
            skipFirst = false;
            continue;
        }

        Ogre::Vector3 dest(static_cast<Ogre::Real>(tile->getX()), static_cast<Ogre::Real>(tile->getY()), z);
        path.push_back(dest);
    }
}

void MovableGameEntity::setWalkPath(const std::string& walkAnim, const std::string& endAnim, bool loopEndAnim,
                                    bool playIdleWhenAnimationEnds, const std::vector<Ogre::Vector3>& path, bool walkDistortion)
{
    mWalkQueue.clear();
    // We set the animation after clearing mWalkQueue and before filling it to be
    // sure it is empty when we set it
    if(!path.empty())
        setAnimationState(walkAnim);

    for(const Ogre::Vector3& dest : path)
        mWalkQueue.push_back(dest);

    if(path.empty())
    {
        setAnimationState(endAnim, loopEndAnim, Ogre::Vector3::ZERO, playIdleWhenAnimationEnds);
    }
    else
    {
        // We save the wanted animation
        mDestinationAnimationState = endAnim;
        mDestinationAnimationLoop = loopEndAnim;
        mDestinationPlayIdleWhenAnimationEnds = playIdleWhenAnimationEnds;
    }

    if(!getIsOnServerMap())
        return;

    for(Seat* seat : mSeatsWithVisionNotified)
    {
        if(seat->getPlayer() == nullptr)
            continue;
        if(!seat->getPlayer()->getIsHuman())
            continue;

        const std::string& name = getName();
        uint32_t nbDest = mWalkQueue.size();
        ServerNotification *serverNotification = new ServerNotification(
            ServerNotificationType::animatedObjectSetWalkPath, seat->getPlayer());
        serverNotification->mPacket << walkDistortion << name << walkAnim << endAnim << loopEndAnim << playIdleWhenAnimationEnds << nbDest;
        for(const Ogre::Vector3& v : mWalkQueue)
            serverNotification->mPacket << v;

        ODServer::getSingleton().queueServerNotification(serverNotification);
    }
}

void MovableGameEntity::clearDestinations(const std::string& animation, bool loopAnim, bool playIdleWhenAnimationEnds)
{
    mWalkQueue.clear();
    stopWalking();

    for(Seat* seat : mSeatsWithVisionNotified)
    {
        if(seat->getPlayer() == nullptr)
            continue;
        if(!seat->getPlayer()->getIsHuman())
            continue;

        const std::string& name = getName();
        const std::string emptyString;
        uint32_t nbDest = 0;
        ServerNotification *serverNotification = new ServerNotification(
            ServerNotificationType::animatedObjectSetWalkPath, seat->getPlayer());
        serverNotification->mPacket << name << emptyString << animation
            << loopAnim << playIdleWhenAnimationEnds << nbDest;
        ODServer::getSingleton().queueServerNotification(serverNotification);
    }
}

void MovableGameEntity::stopWalking()
{
    // Set the animation state of this object to the state that was set for it to enter into after it reaches it's destination.
    if(mDestinationAnimationState.empty())
        return;

    if(mDestinationAnimationDirection == Ogre::Vector3::ZERO)
        mDestinationAnimationDirection = mWalkDirection;

    setAnimationState(mDestinationAnimationState, mDestinationAnimationLoop, mDestinationAnimationDirection, mDestinationPlayIdleWhenAnimationEnds);

    // We reset the destination state
    mDestinationAnimationState.clear();
    mDestinationAnimationLoop = false;
    mDestinationAnimationDirection = Ogre::Vector3::ZERO;
}

void MovableGameEntity::setWalkDirection(const Ogre::Vector3& direction)
{
    mWalkDirection = direction;
    if(getIsOnServerMap())
        return;

    RenderManager::getSingleton().rrOrientEntityToward(this, direction);
}

void MovableGameEntity::setAnimationState(const std::string& state, bool loop, const Ogre::Vector3& direction, bool playIdleWhenAnimationEnds)
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

    // On server side, we update the entity
    if(getIsOnServerMap())
    {
        mAnimationTime = 0;
        mPrevAnimationState = state;
        mPrevAnimationStateLoop = loop;

        if(direction != Ogre::Vector3::ZERO)
            setWalkDirection(direction);

        fireObjectAnimationState(state, loop, direction, playIdleWhenAnimationEnds);
        return;
    }

    mDestinationPlayIdleWhenAnimationEnds = playIdleWhenAnimationEnds;
    // On client side, if the entity has reached its destination, we change the animation. Otherwise, we save
    // the wanted animation that will be set when it has arrived by stopWalking
    if(!mWalkQueue.empty())
    {
        mDestinationAnimationState = state;
        mDestinationAnimationLoop = loop;
        mDestinationAnimationDirection = direction;
        return;
    }

    mAnimationTime = 0;
    mPrevAnimationState = state;
    mPrevAnimationStateLoop = loop;

    if(direction != Ogre::Vector3::ZERO)
        setWalkDirection(direction);

    RenderManager::getSingleton().rrSetObjectAnimationState(this, state, loop);
}

void MovableGameEntity::update(Ogre::Real timeSinceLastFrame)
{
    // Advance the animation
    double addedTime = static_cast<Ogre::Real>(ODApplication::turnsPerSecond
         * static_cast<double>(timeSinceLastFrame)
         * getAnimationSpeedFactor());
    mAnimationTime += addedTime;
    if (!getIsOnServerMap() && getAnimationState() != nullptr)
    {
        // If the animation has stopped we set it to idle if we have to
        if(mDestinationPlayIdleWhenAnimationEnds && getAnimationState()->hasEnded())
            RenderManager::getSingleton().rrSetObjectAnimationState(this, EntityAnimation::idle_anim, true);
        else
            getAnimationState()->addTime(static_cast<Ogre::Real>(addedTime));
    }

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
    setPosition(newPosition);
}

void MovableGameEntity::setPosition(const Ogre::Vector3& v)
{
    Tile* oldTile = nullptr;
    if(getIsOnMap())
    {
        oldTile = getPositionTile();
        OD_ASSERT_TRUE_MSG(oldTile != nullptr, "entityName=" + getName() + ", oldPos=" + Helper::toString(getPosition()));
    }

    int newX = Helper::round(v.x);
    int newY = Helper::round(v.y);
    Tile* newTile = getGameMap()->getTile(newX, newY);
    if(newTile == nullptr)
    {
        OD_LOG_ERR("entityName=" + getName() + ", newPos=" + Helper::toString(v));
        return;
    }

    if((oldTile != newTile) && (oldTile != nullptr))
        removeEntityFromPositionTile();

    mPosition = v;

    if(!getIsOnServerMap())
        RenderManager::getSingleton().rrMoveEntity(this, v);

    if(oldTile != newTile)
        addEntityToPositionTile();
}

void MovableGameEntity::fireObjectAnimationState(const std::string& state, bool loop, const Ogre::Vector3& direction, bool playIdleWhenAnimationEnds)
{
    for(Seat* seat : mSeatsWithVisionNotified)
    {
        if(seat->getPlayer() == nullptr)
            continue;
        if(!seat->getPlayer()->getIsHuman())
            continue;

        ServerNotification* serverNotification = new ServerNotification(
            ServerNotificationType::setObjectAnimationState, seat->getPlayer());
        const std::string& name = getName();
        serverNotification->mPacket << name << state << loop << playIdleWhenAnimationEnds;
        if(direction != Ogre::Vector3::ZERO)
            serverNotification->mPacket << true << direction;
        else if(mWalkDirection != Ogre::Vector3::ZERO)
            serverNotification->mPacket << true << mWalkDirection;
        else
            serverNotification->mPacket << false;
        ODServer::getSingleton().queueServerNotification(serverNotification);
    }
}

void MovableGameEntity::exportToStream(std::ostream& os) const
{
    GameEntity::exportToStream(os);
    // Note : When we will implement game save, we should consider saving informations
    // about animation (like exportToPacket does)
}

bool MovableGameEntity::importFromStream(std::istream& is)
{
    return GameEntity::importFromStream(is);
    // Note : When we will implement game save, we should consider saving informations
    // about animation (like importFromPacket does)
}

std::string MovableGameEntity::getMovableGameEntityStreamFormat()
{
    return GameEntity::getGameEntityStreamFormat();
}

void MovableGameEntity::exportToPacket(ODPacket& os, const Seat* seat) const
{
    GameEntity::exportToPacket(os, seat);
    os << mPrevAnimationState;
    os << mPrevAnimationStateLoop;
    os << mWalkDirection;
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
    GameEntity::importFromPacket(is);
    OD_ASSERT_TRUE(is >> mPrevAnimationState);
    OD_ASSERT_TRUE(is >> mPrevAnimationStateLoop);
    OD_ASSERT_TRUE(is >> mWalkDirection);
    OD_ASSERT_TRUE(is >> mAnimationTime);

    int32_t nbDestinations;
    OD_ASSERT_TRUE(is >> nbDestinations);
    mWalkQueue.clear();
    for(int32_t i = 0; i < nbDestinations; ++i)
    {
        Ogre::Vector3 dest;
        OD_ASSERT_TRUE(is >> dest);
        mWalkQueue.push_back(dest);
    }
}

void MovableGameEntity::restoreEntityState()
{
    GameEntity::restoreEntityState();
    if(!mPrevAnimationState.empty())
    {
        RenderManager::getSingleton().rrSetObjectAnimationState(this, mPrevAnimationState, mPrevAnimationStateLoop);

        if(mWalkDirection != Ogre::Vector3::ZERO)
            RenderManager::getSingleton().rrOrientEntityToward(this, mWalkDirection);

        // If the mesh has no skeleton, getAnimationState() could return null
        if(getAnimationState() != nullptr)
            getAnimationState()->addTime(mAnimationTime);
    }
}

void MovableGameEntity::correctDropPosition(Ogre::Vector3& position)
{
    const double offset = 0.3;
    if(position.x > 0)
        position.x += Random::Double(-offset, offset);

    if(position.y > 0)
        position.y += Random::Double(-offset, offset);

    if(position.z > 0)
        position.z += Random::Double(-offset, offset);
}
