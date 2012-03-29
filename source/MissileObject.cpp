#include <iostream>
#include <sstream>

#include "MissileObject.h"
#include "RenderRequest.h"
#include "RenderManager.h"
#include "GameMap.h"

sem_t MissileObject::missileObjectUniqueNumberLockSemaphore;

MissileObject::MissileObject(const std::string& nMeshName, const Ogre::Vector3& nPosition, GameMap* gameMap)
{
    setGameMap(gameMap);
    static int uniqueNumber = 0;

    setObjectType(GameEntity::missileobject);

    sem_init(&positionLockSemaphore, 0, 1);

    std::stringstream tempSS;
    sem_wait(&missileObjectUniqueNumberLockSemaphore);
    tempSS << "Missile_Object_" << ++uniqueNumber;
    sem_post(&missileObjectUniqueNumberLockSemaphore);
    setName(tempSS.str());

    setMeshName(nMeshName);
    setMeshExisting(false);
    setPosition(nPosition);
}

bool MissileObject::doUpkeep()
{
	// check if we collide with a creature, if yes, do some damage and delete ourselves
    return true;
}


/*! \brief The missile reach the end of the travel, it's destroyed
 *
 */
void MissileObject::stopWalking()
{
	MovableGameEntity::stopWalking();
	getGameMap()->removeMissileObject(this);
	deleteYourself();
}

/*! \brief Changes the missile's position to a new position.
 *  Moves the creature to a new location in 3d space.  This function is
 *  responsible for informing OGRE anything it needs to know, as well as
 *  maintaining the list of creatures in the individual tiles.
 */
void MissileObject::setPosition(const Ogre::Vector3& v)
{
    MovableGameEntity::setPosition(v);

    // Create a RenderRequest to notify the render queue that the scene node for this creature needs to be moved.
    RenderRequest *request = new RenderRequest;
    request->type = RenderRequest::moveSceneNode;
    request->str = getName() + "_node";
    request->vec = v;

    // Add the request to the queue of rendering operations to be performed before the next frame.
    RenderManager::queueRenderRequest(request);
}
