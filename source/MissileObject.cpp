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

const std::string MissileObject::MISSILEOBJECT_NAME_PREFIX = "Missile_Object_";

MissileObject::MissileObject(GameMap* gameMap, const std::string& nMeshName, const Ogre::Vector3& nPosition) :
    MovableGameEntity(gameMap)
{
    setObjectType(GameEntity::missileobject);

    setName(gameMap->nextUniqueNameMissileObj());

    setMeshName(nMeshName);
    setMeshExisting(false);
    setPosition(nPosition);
}

MissileObject::MissileObject(GameMap* gameMap) :
    MovableGameEntity(gameMap)
{
    setObjectType(GameEntity::missileobject);
    setMeshExisting(false);
}

void MissileObject::createMeshLocal()
{
    MovableGameEntity::createMeshLocal();

    if(getGameMap()->isServerGameMap())
        return;

    RenderRequest* request = new RenderRequest;
    request->type = RenderRequest::createMissileObject;
    request->p = static_cast<void*>(this);
    RenderManager::queueRenderRequest(request);
}

void MissileObject::destroyMeshLocal()
{
    MovableGameEntity::destroyMeshLocal();

    if(getGameMap()->isServerGameMap())
        return;

    RenderRequest* request = new RenderRequest;
    request->type = RenderRequest::destroyMissileObject;
    request->p = static_cast<void*>(this);
    RenderManager::queueRenderRequest(request);
}

void MissileObject::deleteYourselfLocal()
{
    MovableGameEntity::deleteYourselfLocal();
    if(getGameMap()->isServerGameMap())
        return;

    RenderRequest* request = new RenderRequest;
    request->type = RenderRequest::deleteMissileObject;
    request->p = static_cast<void*>(this);
    RenderManager::queueRenderRequest(request);
}

void MissileObject::doUpkeep()
{
    // TODO: check if we collide with a creature, if yes, do some damage and delete ourselves
    if(!isMoving())
    {
         getGameMap()->removeMissileObject(this);
         deleteYourself();
         return;
    }
}

void MissileObject::setPosition(const Ogre::Vector3& v)
{
    MovableGameEntity::setPosition(v);
    if(getGameMap()->isServerGameMap())
        return;

    RenderRequest* request = new RenderRequest;
    request->type = RenderRequest::moveSceneNode;
    request->str = getOgreNamePrefix() + "_node";
    request->vec = v;
    RenderManager::queueRenderRequest(request);
}

ODPacket& operator<<(ODPacket& os, MissileObject *mo)
{
    std::string name =  mo->getName();
    std::string meshName = mo->getMeshName();
    double moveSpeed = mo->getMoveSpeed();
    Ogre::Vector3 position = mo->getPosition();
    os << meshName << position << name << moveSpeed;

    return os;
}

ODPacket& operator>>(ODPacket& is, MissileObject *mo)
{
    std::string name;
    std::string meshName;
    Ogre::Vector3 position;
    double moveSpeed;
    is >> meshName;
    mo->setMeshName(meshName);
    is >> position;
    mo->setPosition(position);
    is >> name;
    mo->setName(name);
    is >> moveSpeed;
    mo->setMoveSpeed(moveSpeed);
    return is;
}
