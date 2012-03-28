/*!
 * \file   GameEntity.cpp
 * \date:  28 March 2012
 * \author StefanP.MUC
 * \brief  Provides the GameEntity class, the base class for all ingame objects
 */

#include "Creature.h"
#include "RenderManager.h"
#include "RenderRequest.h"
#include "Weapon.h"

#include "GameEntity.h"

void GameEntity::deleteYourself()
{
    RenderRequest* request = new RenderRequest;

    if(meshExists)
    {
        destroyMesh();
    }

    switch(objectType)
    {
        case creature:
        {
            Creature* tempCreature = static_cast<Creature*>(this);
            tempCreature->getWeaponL()->deleteYourself();
            tempCreature->getWeaponR()->deleteYourself();

            // If standing on a valid tile, notify that tile we are no longer there.
            if (tempCreature->positionTile() != 0)
                tempCreature->positionTile()->removeCreature(tempCreature);

            request->type = RenderRequest::deleteCreature;

            break;
        }

        case room:
            request->type = RenderRequest::deleteRoom;
            break;

        case roomobject:
            request->type = RenderRequest::deleteRoomObject;
            break;

        case missileobject:
            request->type = RenderRequest::deleteMissileObject;
            break;

        case trap:
            request->type = RenderRequest::deleteTrap;
            break;

        case tile:
        {
            request->type = RenderRequest::destroyTile;

            RenderRequest* request2 = new RenderRequest;
            request2->type = RenderRequest::deleteTile;
            request2->p = static_cast<void*>(this);
            RenderManager::queueRenderRequest(request2);

            break;
        }

        case weapon:
            request->type = RenderRequest::deleteWeapon;
            break;

        default:
            request->type = RenderRequest::noRequest;
            break;
    }

    request->p = static_cast<void*>(this);

    RenderManager::queueRenderRequest(request);
}
