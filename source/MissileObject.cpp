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

#include "MissileObject.h"

#include "RenderRequest.h"
#include "RenderManager.h"
#include "GameMap.h"

#include <iostream>
#include <sstream>

sem_t MissileObject::mMissileObjectUniqueNumberLockSemaphore;

MissileObject::MissileObject(const std::string& nMeshName, const Ogre::Vector3& nPosition, GameMap* gameMap)
{
    setGameMap(gameMap);
    static int uniqueNumber = 0;

    setObjectType(GameEntity::missileobject);

    sem_init(&mPositionLockSemaphore, 0, 1);

    std::stringstream tempSS;
    sem_wait(&mMissileObjectUniqueNumberLockSemaphore);
    tempSS << "Missile_Object_" << ++uniqueNumber;
    sem_post(&mMissileObjectUniqueNumberLockSemaphore);
    setName(tempSS.str());

    setMeshName(nMeshName);
    setMeshExisting(false);
    setPosition(nPosition);
}

bool MissileObject::doUpkeep()
{
    // TODO: check if we collide with a creature, if yes, do some damage and delete ourselves
    return true;
}

void MissileObject::stopWalking()
{
    MovableGameEntity::stopWalking();
    getGameMap()->removeMissileObject(this);
    deleteYourself();
}

void MissileObject::setPosition(const Ogre::Vector3& v)
{
    MovableGameEntity::setPosition(v);

    // Create a RenderRequest to notify the render queue that the scene node for this creature needs to be moved.
    RenderRequest* request = new RenderRequest;
    request->type = RenderRequest::moveSceneNode;
    request->str = getName() + "_node";
    request->vec = v;

    // Add the request to the queue of rendering operations to be performed before the next frame.
    RenderManager::queueRenderRequest(request);
}
