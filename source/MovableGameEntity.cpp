#include "Functions.h"
#include "ServerNotification.h"
#include "Tile.h"
#include "Socket.h"
#include "RenderRequest.h"
#include "RenderManager.h"

#include "MovableGameEntity.h"

MovableGameEntity::MovableGameEntity() :
        walkQueueFirstEntryAdded(false),
        animationState(NULL),
        destinationAnimationState("Idle"),
        sceneNode(NULL),
        moveSpeed(1.0),
        prevAnimationState(""),
        prevAnimationStateLoop(true)
{
    sem_init(&animationSpeedFactorLockSemaphore, 0, 1);
    sem_init(&walkQueueLockSemaphore, 0, 1);

    setAnimationSpeedFactor(1.0);
}

/*! \brief Adds a position in 3d space to an animated object's walk queue and, if necessary, starts it walking.
 *
 * This function also places a message in the serverNotificationQueue so that
 * relevant clients are informed about the change.
 */
void MovableGameEntity::addDestination(Ogre::Real x, Ogre::Real y, Ogre::Real z)
{
    Ogre::Vector3 destination(x, y, z);

    // if there are currently no destinations in the walk queue
    sem_wait(&walkQueueLockSemaphore);
    if (walkQueue.empty())
    {
        // Add the destination and set the remaining distance counter
        walkQueue.push_back(destination);
        shortDistance = getPosition().distance(walkQueue.front());
        walkQueueFirstEntryAdded = true;
    }
    else
    {
        // Add the destination
        walkQueue.push_back(destination);
    }
    sem_post(&walkQueueLockSemaphore);

    if (Socket::serverSocket != NULL)
    {
        // Place a message in the queue to inform the clients about the new destination
        ServerNotification *serverNotification = new ServerNotification;
        serverNotification->type
                = ServerNotification::animatedObjectAddDestination;
        serverNotification->str = getName();
        serverNotification->vec = destination;

        queueServerNotification(serverNotification);
    }
}

/*! \brief Replaces a object's current walk queue with a new path.
 *
 * This replacement is done if, and only if, the new path is at least minDestinations
 * long; if addFirstStop is false the new path will start with the second entry in path.
 */
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

    //setAnimationState("Idle");
    return false;
}

/*! \brief Clears all future destinations from the walk queue, stops the object where it is, and sets its animation state.
 *
 */
void MovableGameEntity::clearDestinations()
{
    sem_wait(&walkQueueLockSemaphore);
    walkQueue.clear();
    sem_post(&walkQueueLockSemaphore);
    stopWalking();

    if (Socket::serverSocket != NULL)
    {
        // Place a message in the queue to inform the clients about the clear
        ServerNotification *serverNotification = new ServerNotification;
        serverNotification->type
                = ServerNotification::animatedObjectClearDestinations;
        serverNotification->ani = this;

        queueServerNotification(serverNotification);
    }
}

/*! \brief Stops the object where it is, and sets its animation state.
 *
 */
void MovableGameEntity::stopWalking()
{
    walkDirection = Ogre::Vector3::ZERO;

    // Set the animation state of this object to the state that was set for it to enter into after it reaches it's destination.
    setAnimationState(destinationAnimationState);
}

/** Rotates the object so that it is facing toward the given x-y location.
 *
 */
void MovableGameEntity::faceToward(int x, int y)
{
    // Rotate the object to face the direction of the destination
    Ogre::Vector3 tempPosition = getPosition();
	walkDirection = Ogre::Vector3(
		static_cast<Ogre::Real>(x),
		static_cast<Ogre::Real>(y),
		tempPosition.z) - tempPosition;
    walkDirection.normalise();

    RenderRequest *request = new RenderRequest;
    request->type = RenderRequest::orientSceneNodeToward;
    request->vec = walkDirection;
    request->str = getName() + "_node";

    // Add the request to the queue of rendering operations to be performed before the next frame.
    RenderManager::queueRenderRequest(request);
}

double MovableGameEntity::getMoveSpeed()
{
    return moveSpeed;
}

void MovableGameEntity::setMoveSpeed(double s)
{
    moveSpeed = s;
}

void MovableGameEntity::setAnimationState(const std::string& s, bool loop)
{
    // Ignore the command if the command is exactly the same as what we did last time, this is not only faster it prevents non-looped actions like die from being inadvertantly repeated.
    if (s.compare(prevAnimationState) == 0 && loop == prevAnimationStateLoop)
        return;

    prevAnimationState = s;

    if (s.compare("Walk") == 0 || s.compare("Flee") == 0)
        setAnimationSpeedFactor(moveSpeed);
    else
        setAnimationSpeedFactor(1.0);

    RenderRequest *request = new RenderRequest;
    request->type = RenderRequest::setObjectAnimationState;
    request->p = static_cast<void*>(this);
    request->str = s;
    request->b = loop;

    if (Socket::serverSocket != NULL)
    {
        try
        {
            // Place a message in the queue to inform the clients about the new animation state
            ServerNotification *serverNotification = new ServerNotification;
            serverNotification->type
                    = ServerNotification::setObjectAnimationState;
            serverNotification->str = s;
            serverNotification->p = static_cast<void*>(this);
            serverNotification->b = loop;

            queueServerNotification(serverNotification);
        }
        catch (bad_alloc&)
        {
            cerr << "\n\nERROR:  bad alloc in Creature::setAnimationState\n\n";
            exit(1);
        }
    }

    // Add the request to the queue of rendering operations to be performed before the next frame.
    RenderManager::queueRenderRequest(request);
}

double MovableGameEntity::getAnimationSpeedFactor()
{
    sem_wait(&animationSpeedFactorLockSemaphore);
    double tempDouble = animationSpeedFactor;
    sem_post(&animationSpeedFactorLockSemaphore);

    return tempDouble;
}

void MovableGameEntity::setAnimationSpeedFactor(double f)
{
    sem_wait(&animationSpeedFactorLockSemaphore);
    animationSpeedFactor = f;
    sem_post(&animationSpeedFactorLockSemaphore);
}

