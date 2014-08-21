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

#include "MovableGameEntity.h"

#include "ODServer.h"
#include "ServerNotification.h"
#include "ODApplication.h"
#include "Tile.h"
#include "GameMap.h"
#include "RenderRequest.h"
#include "RenderManager.h"
#include "LogManager.h"

MovableGameEntity::MovableGameEntity(GameMap* gameMap) :
        GameEntity(gameMap),
        mWalkQueueFirstEntryAdded(false),
        mAnimationState(NULL),
        mDestinationAnimationState("Idle"),
        mSceneNode(NULL),
        mMoveSpeed(1.0),
        mPrevAnimationStateLoop(true)
{
    setAnimationSpeedFactor(1.0);
}

void MovableGameEntity::addDestination(Ogre::Real x, Ogre::Real y, Ogre::Real z)
{
    Ogre::Vector3 destination(x, y, z);

    // if there are currently no destinations in the walk queue
    if (mWalkQueue.empty())
    {
        // Add the destination and set the remaining distance counter
        mWalkQueue.push_back(destination);
        mShortestDistance = getPosition().distance(mWalkQueue.front());
        mWalkQueueFirstEntryAdded = true;
    }
    else
    {
        // Add the destination
        mWalkQueue.push_back(destination);
    }

    if (getGameMap()->isServerGameMap())
    {
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
            Ogre::LogManager::getSingleton().logMessage("ERROR: bad alloc in MovableGameEntity::clearDestinations", Ogre::LML_CRITICAL);
            exit(1);
        }
    }
}

void MovableGameEntity::stopWalking()
{
    mWalkDirection = Ogre::Vector3::ZERO;

    // Set the animation state of this object to the state that was set for it to enter into after it reaches it's destination.
    setAnimationState(mDestinationAnimationState);
}

void MovableGameEntity::faceToward(int x, int y)
{
    faceToward(static_cast<Ogre::Real>(x), static_cast<Ogre::Real>(y));
}

void MovableGameEntity::faceToward(Ogre::Real x, Ogre::Real y)
{
    // Rotate the object to face the direction of the destination
    Ogre::Vector3 tempPosition = getPosition();
    tempPosition = Ogre::Vector3(x, y, tempPosition.z) - tempPosition;
    tempPosition.normalise();
    setWalkDirection(tempPosition);
}

void MovableGameEntity::setWalkDirection(Ogre::Vector3& direction)
{
    mWalkDirection = direction;

    if(getGameMap()->isServerGameMap())
        return;

    RenderRequest* request = new RenderRequest;
    request->type = RenderRequest::orientSceneNodeToward;
    request->vec = mWalkDirection;
    request->p = static_cast<void*>(this);
    RenderManager::queueRenderRequest(request);
}

void MovableGameEntity::setAnimationState(const std::string& state, bool setWalkDirection, bool loop)
{
    // Ignore the command if the command is exactly the same as what we did last time,
    // this is not only faster it prevents non-looped actions
    // like 'die' from being inadvertantly repeated.
    if (state.compare(mPrevAnimationState) == 0 && loop == mPrevAnimationStateLoop)
        return;

    mPrevAnimationState = state;

    if (state.compare("Walk") == 0 || state.compare("Flee") == 0)
        setAnimationSpeedFactor(mMoveSpeed);
    else
        setAnimationSpeedFactor(1.0);

    if (getGameMap()->isServerGameMap())
    {
        try
        {
            ServerNotification* serverNotification = new ServerNotification(
                ServerNotification::setObjectAnimationState, NULL);
            std::string name = getName();
            serverNotification->mPacket << name << state << loop << setWalkDirection;
            if(setWalkDirection)
                serverNotification->mPacket << mWalkDirection;
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
    if (mAnimationState != NULL)
    {
        mAnimationState->addTime((Ogre::Real)(ODApplication::turnsPerSecond
                                 * timeSinceLastFrame
                                 * getAnimationSpeedFactor()));
    }

    if (mWalkQueue.empty())
        return;

    // Move the entity

    // If the previously empty walk queue has had a destination added to it we need to rotate the entity to face its initial walk direction.
    if (mWalkQueueFirstEntryAdded)
    {
        mWalkQueueFirstEntryAdded = false;
        faceToward((int)mWalkQueue.front().x, (int)mWalkQueue.front().y);
    }

    //FIXME: The moveDist should probably be tied to the scale of the entity as well
    //FIXME: When the client and the server are using different frame rates, the entities walk at different speeds
    double moveDist = ODApplication::turnsPerSecond
                      * getMoveSpeed()
                      * timeSinceLastFrame;
    mShortestDistance -= moveDist;

    // Check to see if we have walked to, or past, the first destination in the queue
    if (mShortestDistance <= 0.0)
    {
        // Compensate for any overshoot and place the creature at the intended destination
        setPosition(mWalkQueue.front());
        mWalkQueue.pop_front();

        // If there are no more places to walk to still left in the queue
        if (mWalkQueue.empty())
        {
            // Stop walking
            stopWalking();
        }
        else // There are still entries left in the queue
        {
            // Turn to face the next direction
            faceToward((int)mWalkQueue.front().x, (int)mWalkQueue.front().y);

            // Compute the distance to the next location in the queue and store it in the shortDistance datamember.
            Ogre::Vector3 tempVector = mWalkQueue.front() - getPosition();
            mShortestDistance = tempVector.normalise();
        }
    }
    else // We have not reached the destination at the front of the queue
    {
        // Move the object closer to its destination by the amount it should travel this frame.
        setPosition(getPosition() + mWalkDirection * (Ogre::Real)moveDist);
    }
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
