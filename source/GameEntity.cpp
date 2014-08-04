/*!
 * \file   GameEntity.cpp
 * \date:  28 March 2012
 * \author StefanP.MUC
 * \brief  Provides the GameEntity class, the base class for all ingame objects
 *
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

#include "GameEntity.h"

#include "Creature.h"
#include "GameMap.h"
#include "RenderManager.h"
#include "RenderRequest.h"
#include "Room.h"
#include "RoomObject.h"
#include "Weapon.h"

void GameEntity::createMesh()
{
    if (meshExists)
        return;

    meshExists = true;

    if(getGameMap()->isServerGameMap())
        return;

    RenderRequest* request = NULL;

    switch(objectType)
    {
        case creature:
            request = new RenderRequest;
            request->type   = RenderRequest::createCreature;
            request->str    = static_cast<Creature*>(this)->getDefinition()->getMeshName();
            request->vec    = static_cast<Creature*>(this)->getDefinition()->getScale();
            break;

        case room:
        {
            std::vector<Tile*> coveredTiles = getCoveredTiles();
            for (unsigned int i = 0, size = coveredTiles.size(); i < size; ++i)
            {
                request = new RenderRequest;
                request->type = RenderRequest::createRoom;
                request->p    = static_cast<void*>(this);
                request->p2   = coveredTiles[i];
                RenderManager::queueRenderRequest(request);
            }

            // return because rooms create every tile separately
            return;
        }

        case roomobject:
        {
            Room* tempRoom = static_cast<RoomObject*>(this)->getParentRoom();

            request = new RenderRequest;
            request->type   = RenderRequest::createRoomObject;
            request->p2     = tempRoom;
            request->str    = getName();
            request->str2   = getMeshName();
            break;
        }

        case missileobject:
            request = new RenderRequest;
            request->type = RenderRequest::createMissileObject;
            break;

        case trap:
        {
            for (unsigned int i = 0; i < getCoveredTiles().size(); ++i)
            {
                RenderRequest* r = new RenderRequest;

                r->type = RenderRequest::createTrap;
                r->p    = static_cast<void*>(this);
                r->p2   = getCoveredTiles()[i];

                RenderManager::queueRenderRequest(r);
            }

            // return because traps create every tile separately
            return;
        }

        case tile:
            request = new RenderRequest;
            request->type = RenderRequest::createTile;
            break;

        case weapon:
        {
            if (getName().compare("none") == 0)
                return;

            Weapon* tempWeapon = static_cast<Weapon*>(this);
            request = new RenderRequest;
            request->type   = RenderRequest::createWeapon;
            request->p      = static_cast<void*>(this);
            request->p2     = tempWeapon->getParentCreature();
            request->p3     = new std::string(tempWeapon->getHandString());

            break;
        }

        default:
            request = new RenderRequest;
            request->type = RenderRequest::noRequest;
            break;
    }
    if(request != NULL)
    {
        request->p = static_cast<void*>(this);
        RenderManager::queueRenderRequest(request);
    }
}

void GameEntity::destroyMesh()
{
    if(!meshExists)
    {
        return;
    }

    meshExists = false;

    if(getGameMap()->isServerGameMap())
        return;

    RenderRequest* request = new RenderRequest;

    switch(objectType)
    {
        case creature:
        {
            Creature* tempCreature = static_cast<Creature*>(this);
            tempCreature->destroyStatsWindow();
            tempCreature->getWeaponL()->destroyMesh();
            tempCreature->getWeaponR()->destroyMesh();
            request->type = RenderRequest::destroyCreature;
            break;
        }

        case room:
        {
            Room* tempRoom = static_cast<Room*>(this);
            tempRoom->destroyRoomObjectMeshes();

            for (unsigned int i = 0; i < getCoveredTiles().size(); ++i)
            {
                RenderRequest* r = new RenderRequest;
                r->type = RenderRequest::destroyRoom;
                r->p    = static_cast<void*>(this);
                r->p2   = getCoveredTiles()[i];

                RenderManager::queueRenderRequest(r);
            }

            //delete original request and return because rooms delete evert tile separately
            delete request;
            return;
        }

        case roomobject:
        {
            Room* parent = static_cast<RoomObject*>(this)->getParentRoom();

            request->type   = RenderRequest::destroyRoomObject;
            request->p2     = parent;
            break;
        }

        case missileobject:
            request->type = RenderRequest::destroyMissileObject;
            break;

        case trap:
        {
            for (unsigned int i = 0; i < getCoveredTiles().size(); ++i)
            {
                RenderRequest *r = new RenderRequest;
                r->type = RenderRequest::destroyTrap;
                r->p    = static_cast<void*>(this);
                r->p2   = getCoveredTiles()[i];

                // Add the request to the queue of rendering operations to be performed before the next frame.
                RenderManager::queueRenderRequest(r);
            }

            //delete original request and return because traps delete every tile separately
            delete request;
            return;
        }

        case tile:
            request->type = RenderRequest::destroyTile;
            break;

        case weapon:
            request->type   = RenderRequest::destroyWeapon;
            request->p2     = static_cast<Weapon*>(this)->getParentCreature();
            break;

        default:
            request->type = RenderRequest::noRequest;
            break;
    }

    request->p = static_cast<void*>(this);

    RenderManager::queueRenderRequest(request);
}

void GameEntity::deleteYourself()
{
    if(meshExists)
    {
        destroyMesh();
    }

    if(getGameMap()->isServerGameMap())
    {
        getGameMap()->queueEntityForDeletion(this);
        return;
    }

    RenderRequest* request = NULL;
    switch(objectType)
    {
        case creature:
        {
            Creature* tempCreature = static_cast<Creature*>(this);
            tempCreature->getWeaponL()->deleteYourself();
            tempCreature->getWeaponR()->deleteYourself();

            request = new RenderRequest;
            request->type = RenderRequest::deleteCreature;
            break;
        }

        case room:
            request = new RenderRequest;
            request->type = RenderRequest::deleteRoom;
            break;

        case roomobject:
            request = new RenderRequest;
            request->type = RenderRequest::deleteRoomObject;
            break;

        case missileobject:
            request = new RenderRequest;
            request->type = RenderRequest::deleteMissileObject;
            break;

        case trap:
            request = new RenderRequest;
            request->type = RenderRequest::deleteTrap;
            break;

        case tile:
            request = new RenderRequest;
            request->type = RenderRequest::deleteTile;
            break;

        case weapon:
            request = new RenderRequest;
            request->type = RenderRequest::deleteWeapon;
            break;

        default:
            break;
    }

    if(request != NULL)
    {
        request->p = static_cast<void*>(this);
        RenderManager::queueRenderRequest(request);
    }
}
