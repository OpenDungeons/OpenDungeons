/*
 *  Copyright (C) 2011-2015  OpenDungeons Team
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

#include "EntityBase.h"

#include "utils/Helper.h"

EntityBase::EntityBase(std::string name, std::string meshName, Seat* seat)
    :
    mPosition          (Ogre::Vector3::ZERO),
    mName              (name),
    mMeshName          (meshName),
    mMeshExists        (false),
    mSeat              (seat),
    mIsDeleteRequested (false),
    mParentSceneNode   (nullptr),
    mEntityNode        (nullptr)
{

}

std::string EntityBase::getOgreNamePrefix() const
{
    return Helper::toString(static_cast<int32_t>(getObjectType())) + "-";
}

void EntityBase::createMesh()
{
    if (mMeshExists)
        return;

    mMeshExists = true;
    createMeshLocal();
}

void EntityBase::destroyMesh()
{
    if(!mMeshExists)
        return;

    mMeshExists = false;

    destroyMeshLocal();
}
