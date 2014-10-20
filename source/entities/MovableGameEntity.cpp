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

#include "entities/MovableGameEntity.h"

#include "entities/Tile.h"

#include "gamemap/GameMap.h"
#include "network/ODServer.h"
#include "network/ServerNotification.h"
#include "render/RenderRequest.h"
#include "render/RenderManager.h"
#include "utils/LogManager.h"
#include "ODApplication.h"

MovableGameEntity::MovableGameEntity(GameMap* gameMap) :
    GameEntity(gameMap),
    mAnimationState(NULL),
    mSceneNode(NULL),
    mMoveSpeed(1.0),
    mPrevAnimationStateLoop(true),
    mAnimationSpeedFactor(1.0),
    mDestinationAnimationState("Idle")
{
}

void MovableGameEntity::setMoveSpeed(double s)
{
    mMoveSpeed = s;

    if (getGameMap()->isServerGameMap())
    {
        try
        {
            ServerNotification* serverNotification = new ServerNotification(
                ServerNotification::setMoveSpeed, NULL);
            std::string name = getName();
            serverNotification->mPacket << name << s;
            ODServer::getSingleton().queueServerNotification(serverNotification);
        }
        catch (std::bad_alloc&)
        {
            OD_ASSERT_TRUE(false);
            exit(1);
        }
    }
}

void MovableGameEntity::addDestination(Ogre::Real x, Ogre::Real y, Ogre::Real z)
{
    Ogre::Vector3 destination(x, y, z);

    mWalkQueue.push_back(destination);

    if (!getGameMap()->isServerGameMap())
        return;

    try
    {
        ServerNotification* serverNotification = new ServerNotification(
            ServerNotification::animatedObjectAddDestination, NULL);
        std::string name = getName();
        serverNotification->mPacket << name << destination;
        ODServer::getSingleton().queueServerNotification(serverNotification);
    }
    catch (std::bad_alloc&)
    {
        Ogre::LogManager::getSingleton().logMessage("ERROR: bad alloc in MovableGameEntity::addDestination", Ogre::LML_CRITICAL);
        exit(1);
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
            addDestination((*itr)->x, (*itr)->y);
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

    if (getGameMap()->isServerGameMap())
    {
        try
        {
            ServerNotification* serverNotification = new ServerNotification(
                ServerNotification::animatedObjectClearDestinations, NULL);
            std::string name = getName();
            serverNotification->mPacket << name;
            ODServer::getSingleton().queueServerNotification(serverNotification);
        }
        catch (std::bad_alloc&)
        {
            OD_ASSERT_TRUE(false);
            exit(1);
        }
    }
}

void MovableGameEntity::stopWalking()
{
    // Set the animation state of this object to the state that was set for it to enter into after it reaches it's destination.
    setAnimationState(mDestinationAnimationState);
}

void MovableGameEntity::setWalkDirection(Ogre::Vector3& direction)
{
    if(getGameMap()->isServerGameMap())
        return;

    RenderRequest* request = new RenderRequest;
    request->type = RenderRequest::orientSceneNodeToward;
    request->vec = direction;
    request->p = static_cast<void*>(this);
    RenderManager::queueRenderRequest(request);
}

void MovableGameEntity::setAnimationState(const std::string& state, bool loop, Ogre::Vector3* direction)
{
    // Ignore the command if the command is exactly the same as what we did last time,
    // this is not only faster it prevents non-looped actions
    // like 'die' from being inadvertantly repeated.
    if (state.compare(mPrevAnimationState) == 0 && loop == mPrevAnimationStateLoop)
        return;

    mPrevAnimationState = state;

    // NOTE : if we add support to increase speed like a spell or slapping, it
    // would be nice to increase speed factor
    setAnimationSpeedFactor(1.0);

    if (getGameMap()->isServerGameMap())
    {
        try
        {
            ServerNotification* serverNotification = new ServerNotification(
                ServerNotification::setObjectAnimationState, NULL);
            std::string name = getName();
            serverNotification->mPacket << name << state << loop;
            if(direction != NULL)
                serverNotification->mPacket << true << *direction;
            else
                serverNotification->mPacket << false;
            ODServer::getSingleton().queueServerNotification(serverNotification);
        }
        catch (std::bad_alloc&)
        {
            LogManager::getSingleton().logMessage("ERROR: Bad memory allocation in Creature::setAnimationState()");
        }
        return;
    }

    // Add the request to the queue of rendering operations to be performed before the next frame.
    RenderRequest* request = new RenderRequest;
    request->type = RenderRequest::setObjectAnimationState;
    request->p = static_cast<void*>(this);
    request->str = state;
    request->b = loop;
    RenderManager::queueRenderRequest(request);
}

double MovableGameEntity::getAnimationSpeedFactor()
{
    double tempDouble = mAnimationSpeedFactor;
    return tempDouble;
}

void MovableGameEntity::setAnimationSpeedFactor(double f)
{
    mAnimationSpeedFactor = f;
}

void MovableGameEntity::update(Ogre::Real timeSinceLastFrame)
{
    // Advance the animation
    if (!getGameMap()->isServerGameMap() && mAnimationState != NULL)
    {
        mAnimationState->addTime((Ogre::Real)(ODApplication::turnsPerSecond
                                 * timeSinceLastFrame
                                 * getAnimationSpeedFactor()));
    }

    if (mWalkQueue.empty())
        return;

    // Move the entity

    //FIXME: The moveDist should probably be tied to the scale of the entity as well
    //FIXME: When the client and the server are using different frame rates, the entities walk at different speeds
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
            newPosition = newPosition + walkDirection * (Ogre::Real)moveDist;
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
    GameEntity::setPosition(v);
    if(getGameMap()->isServerGameMap())
        return;

    // Create a RenderRequest to notify the render queue that the scene node for this creature needs to be moved.
    RenderRequest *request = new RenderRequest;
    request->type = RenderRequest::moveSceneNode;
    request->str = getOgreNamePrefix() + getName() + "_node";
    request->vec = v;
    RenderManager::queueRenderRequest(request);

}

Tile* MovableGameEntity::getPositionTile() const
{
    Ogre::Vector3 tempPosition = getPosition();

    return getGameMap()->getTile(static_cast<int>(std::round(tempPosition.x)),
                                 static_cast<int>(std::round(tempPosition.y)));
}
