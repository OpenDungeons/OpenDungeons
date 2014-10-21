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

#include "entities/GameEntity.h"

#include "entities/Creature.h"
#include "entities/RenderedMovableEntity.h"
#include "entities/Weapon.h"

#include "gamemap/GameMap.h"
#include "network/ODPacket.h"
#include "render/RenderManager.h"
#include "render/RenderRequest.h"
#include "rooms/Room.h"

void GameEntity::createMesh()
{
    if (meshExists)
        return;

    meshExists = true;
    createMeshLocal();
}

void GameEntity::destroyMesh()
{
    if(!meshExists)
        return;

    meshExists = false;

    destroyMeshLocal();
}

void GameEntity::deleteYourself()
{
    destroyMesh();
    if(mIsDeleteRequested)
        return;

    mIsDeleteRequested = true;

    deleteYourselfLocal();

    getGameMap()->queueEntityForDeletion(this);
}

std::string GameEntity::getNodeNameWithoutPostfix()
{
    return getOgreNamePrefix() + getName();
}

ODPacket& operator<<(ODPacket& os, const GameEntity::ObjectType& ot)
{
    os << static_cast<int32_t>(ot);
    return os;
}

ODPacket& operator>>(ODPacket& is, GameEntity::ObjectType& ot)
{
    int32_t tmp;
    is >> tmp;
    ot = static_cast<GameEntity::ObjectType>(tmp);
    return is;
}
