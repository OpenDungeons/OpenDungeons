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

#include "BattleField.h"

#include "GameMap.h"
#include "RenderRequest.h"
#include "RenderManager.h"

#include <OgreStringConverter.h>

#include <iostream>

BattleField::BattleField(GameMap* gameMap) :
    mHasMeshes(false)
{
    mName = "field_" + Ogre::StringConverter::toString(
        gameMap->nextUniqueNumberBattlefield());
}

double BattleField::getTileSecurityLevel(int x, int y)
{
    for (unsigned int i = 0; i < mField.size(); ++i)
    {
        if (mField[i].isPosition(x, y))
            return mField[i].getSecurityLevel();
    }

    return 0.0;
}

FieldType::iterator BattleField::begin()
{
    return mField.begin();
}

FieldType::iterator BattleField::end()
{
    return mField.end();
}

void BattleField::setTileSecurityLevel(int x, int y, double f)
{
    for (unsigned int i = 0; i < mField.size(); ++i)
    {
        if (mField[i].isPosition(x, y))
        {
            mField[i].setSecurityLevel(f);
            return;
        }
    }

    mField.push_back(SecurityTile(x, y, f));
}

void BattleField::clear()
{
    mField.clear();
}

SecurityTile BattleField::getMinSecurityLevel()
{
    if (mField.empty())
        return SecurityTile(-1, -1, 0.0);

    FieldType::iterator itr = mField.begin();
    FieldType::iterator minimum = mField.begin();
    while (itr != mField.end())
    {
        if (itr->getSecurityLevel() < minimum->getSecurityLevel())
            minimum = itr;

        ++itr;
    }

    return *minimum;
}

SecurityTile BattleField::getMaxSecurityLevel()
{
    if (mField.empty())
        return SecurityTile(-1, -1, 0.0);

    FieldType::iterator itr = mField.begin();
    FieldType::iterator maximum = mField.begin();
    while (itr != mField.end())
    {
        if (itr->getSecurityLevel() > maximum->getSecurityLevel())
            maximum = itr;

        ++itr;
    }

    return *maximum;
}

void BattleField::refreshMeshes(double offset = 0.0)
{
    if (mField.empty() && !mHasMeshes)
        return;

    if (!mHasMeshes)
    {
        createMeshes(offset);
    }
    else
    {
        mHasMeshes = true;

        RenderRequest *request = new RenderRequest;
        request->type = RenderRequest::refreshField;
        request->p = static_cast<void*>(this);
        request->p2 = new double(offset); // The new double() is later deleted by the render manager.

        // Add the request to the queue of rendering operations to be performed before the next frame.
        RenderManager::queueRenderRequest(request);
    }
}

void BattleField::createMeshes(double offset = 0.0)
{
    if (mField.empty() || mHasMeshes)
        return;

    mHasMeshes = true;

    RenderRequest* request = new RenderRequest;
    request->type = RenderRequest::createField;
    request->p = static_cast<void*>(this);
    request->p2 = new double(offset); // The new double() is later deleted by the render manager.

    // Add the request to the queue of rendering operations to be performed before the next frame.
    RenderManager::queueRenderRequest(request);
}

void BattleField::destroyMeshes()
{
    if (!mHasMeshes)
        return;

    mHasMeshes = false;

    RenderRequest *request = new RenderRequest;
    request->type = RenderRequest::destroyField;
    request->p = static_cast<void*>(this);

    // Add the request to the queue of rendering operations to be performed before the next frame.
    RenderManager::queueRenderRequest(request);
}

