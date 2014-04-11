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
#include "Tile.h"
#include "Socket.h"
#include "RenderRequest.h"
#include "RenderManager.h"
#include "LogManager.h"

MovableGameEntity::MovableGameEntity() :
        mWalkQueueFirstEntryAdded(false),
        mAnimationState(NULL),
        mDestinationAnimationState("Idle"),
        mSceneNode(NULL),
        mMoveSpeed(1.0),
        mPrevAnimationStateLoop(true)
{
    sem_init(&mAnimationSpeedFactorLockSemaphore, 0, 1);
    sem_init(&mWalkQueueLockSemaphore, 0, 1);

    setAnimationSpeedFactor(1.0);
}

void MovableGameEntity::addDestination(Ogre::Real x, Ogre::Real y, Ogre::Real z)
{
    Ogre::Vector3 destination(x, y, z);

    // if there are currently no destinations in the walk queue
    sem_wait(&mWalkQueueLockSemaphore);
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
    sem_post(&mWalkQueueLockSemaphore);

    if (Socket::serverSocket != NULL)
    {
        // Place a message in the queue to inform the clients about the new destination
        ServerNotification* serverNotification = new ServerNotification;
        serverNotification->type = ServerNotification::animatedObjectAddDestination;
        serverNotification->str = getName();
        serverNotification->vec = destination;

        ODServer::queueServerNotification(serverNotification);
    }
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
    sem_wait(&mWalkQueueLockSemaphore);
    mWalkQueue.clear();
    sem_post(&mWalkQueueLockSemaphore);
    stopWalking();

    if (Socket::serverSocket != NULL)
    {
        // Place a message in the queue to inform the clients about the clear
        ServerNotification* serverNotification = new ServerNotification;
        serverNotification->type = ServerNotification::animatedObjectClearDestinations;
        serverNotification->ani = this;

       ODServer::queueServerNotification(serverNotification);
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
    // Rotate the object to face the direction of the destination
    Ogre::Vector3 tempPosition = getPosition();
    mWalkDirection = Ogre::Vector3(
        static_cast<Ogre::Real>(x),
        static_cast<Ogre::Real>(y),
        tempPosition.z) - tempPosition;
    mWalkDirection.normalise();

    RenderRequest* request = new RenderRequest;
    request->type = RenderRequest::orientSceneNodeToward;
    request->vec = mWalkDirection;
    request->str = getName() + "_node";

    // Add the request to the queue of rendering operations to be performed before the next frame.
    RenderManager::queueRenderRequest(request);
}

void MovableGameEntity::setAnimationState(const std::string& s, bool loop)
{
    // Ignore the command if the command is exactly the same as what we did last time,
    // this is not only faster it prevents non-looped actions
    // like 'die' from being inadvertantly repeated.
    if (s.compare(mPrevAnimationState) == 0 && loop == mPrevAnimationStateLoop)
        return;

    mPrevAnimationState = s;

    if (s.compare("Walk") == 0 || s.compare("Flee") == 0)
        setAnimationSpeedFactor(mMoveSpeed);
    else
        setAnimationSpeedFactor(1.0);

    RenderRequest* request = new RenderRequest;
    request->type = RenderRequest::setObjectAnimationState;
    request->p = static_cast<void*>(this);
    request->str = s;
    request->b = loop;

    if (Socket::serverSocket != NULL)
    {
        try
        {
            // Place a message in the queue to inform the clients about the new animation state
            ServerNotification* serverNotification = new ServerNotification;
            serverNotification->type = ServerNotification::setObjectAnimationState;
            serverNotification->str = s;
            serverNotification->p = static_cast<void*>(this);
            serverNotification->b = loop;

            ODServer::queueServerNotification(serverNotification);
        }
        catch (bad_alloc&)
        {
            LogManager::getSingleton().logMessage("\n\nERROR: Bad memory allocation in Creature::setAnimationState()\n\n");
        }
    }

    // Add the request to the queue of rendering operations to be performed before the next frame.
    RenderManager::queueRenderRequest(request);
}

double MovableGameEntity::getAnimationSpeedFactor()
{
    sem_wait(&mAnimationSpeedFactorLockSemaphore);
    double tempDouble = mAnimationSpeedFactor;
    sem_post(&mAnimationSpeedFactorLockSemaphore);

    return tempDouble;
}

void MovableGameEntity::setAnimationSpeedFactor(double f)
{
    sem_wait(&mAnimationSpeedFactorLockSemaphore);
    mAnimationSpeedFactor = f;
    sem_post(&mAnimationSpeedFactorLockSemaphore);
}

