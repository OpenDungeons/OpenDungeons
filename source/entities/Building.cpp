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

#include "entities/Building.h"

#include "entities/RenderedMovableEntity.h"
#include "entities/Tile.h"
#include "gamemap/GameMap.h"
#include "network/ServerNotification.h"
#include "network/ODServer.h"

#include "utils/LogManager.h"

const double Building::DEFAULT_TILE_HP = 10.0;

void Building::createMeshLocal()
{
    GameEntity::createMeshLocal();

    if(getGameMap()->isServerGameMap())
        return;

    std::vector<Tile*> coveredTiles = getCoveredTiles();
    for (Tile* tile : coveredTiles)
    {
        RenderRequest* request = new RenderRequestCreateBuilding(this, tile);
        RenderManager::queueRenderRequest(request);
    }
}

void Building::destroyMeshLocal()
{
    GameEntity::destroyMeshLocal();

    if(getGameMap()->isServerGameMap())
        return;

    std::vector<Tile*> coveredTiles = getCoveredTiles();
    for (Tile* tile : coveredTiles)
    {
        RenderRequest* request = new RenderRequestDestroyBuilding(this, tile);
        RenderManager::queueRenderRequest(request);
    }
}

void Building::addBuildingObject(Tile* targetTile, RenderedMovableEntity* obj)
{
    if(obj == NULL)
        return;

    // We assume the object position has been already set (most of the time in loadBuildingObject)
    mBuildingObjects[targetTile] = obj;
    getGameMap()->addRenderedMovableEntity(obj);
}

void Building::removeBuildingObject(Tile* tile)
{
    if(mBuildingObjects.count(tile) == 0)
        return;

    RenderedMovableEntity* obj = mBuildingObjects[tile];
    getGameMap()->removeRenderedMovableEntity(obj);
    obj->deleteYourself();
    mBuildingObjects.erase(tile);
}

void Building::removeBuildingObject(RenderedMovableEntity* obj)
{
    std::map<Tile*, RenderedMovableEntity*>::iterator it;

    for (it = mBuildingObjects.begin(); it != mBuildingObjects.end(); ++it)
    {
        if(it->second == obj)
            break;
    }

    if(it != mBuildingObjects.end())
    {
        getGameMap()->removeRenderedMovableEntity(obj);
        obj->deleteYourself();
        mBuildingObjects.erase(it);
    }
}

void Building::removeAllBuildingObjects()
{
    if(mBuildingObjects.empty())
        return;

    std::map<Tile*, RenderedMovableEntity*>::iterator itr = mBuildingObjects.begin();
    while (itr != mBuildingObjects.end())
    {
        RenderedMovableEntity* obj = itr->second;
        getGameMap()->removeRenderedMovableEntity(obj);
        obj->deleteYourself();
        ++itr;
    }
    mBuildingObjects.clear();
}

RenderedMovableEntity* Building::getBuildingObjectFromTile(Tile* tile)
{
    if(mBuildingObjects.count(tile) == 0)
        return NULL;

    RenderedMovableEntity* obj = mBuildingObjects[tile];
    return obj;
}

RenderedMovableEntity* Building::loadBuildingObject(GameMap* gameMap, const std::string& meshName,
    Tile* targetTile, double rotationAngle, bool hideCoveredTile, float opacity)
{
    if (targetTile == NULL)
        targetTile = getCentralTile();

    OD_ASSERT_TRUE(targetTile != NULL);
    if(targetTile == NULL)
        return NULL;

    return loadBuildingObject(gameMap, meshName, targetTile, static_cast<double>(targetTile->x),
        static_cast<double>(targetTile->y), rotationAngle, hideCoveredTile, opacity);
}

RenderedMovableEntity* Building::loadBuildingObject(GameMap* gameMap, const std::string& meshName,
    Tile* targetTile, double x, double y, double rotationAngle, bool hideCoveredTile, float opacity)
{
    std::string baseName;
    if(targetTile == nullptr)
        baseName = getName();
    else
        baseName = getName() + "_" + Tile::displayAsString(targetTile);

    RenderedMovableEntity* obj = new RenderedMovableEntity(gameMap, baseName, meshName,
        static_cast<Ogre::Real>(rotationAngle), hideCoveredTile, opacity);
    obj->setPosition(Ogre::Vector3((Ogre::Real)x, (Ogre::Real)y, 0.0f));

    return obj;
}

Tile* Building::getCentralTile()
{
    if (mCoveredTiles.empty())
        return NULL;

    int minX, maxX, minY, maxY;
    minX = maxX = mCoveredTiles[0]->getX();
    minY = maxY = mCoveredTiles[0]->getY();

    for(unsigned int i = 0, size = mCoveredTiles.size(); i < size; ++i)
    {
        int tempX = mCoveredTiles[i]->getX();
        int tempY = mCoveredTiles[i]->getY();

        if (tempX < minX)
            minX = tempX;
        if (tempY < minY)
            minY = tempY;
        if (tempX > maxX)
            maxX = tempX;
        if (tempY > maxY)
            maxY = tempY;
    }

    return getGameMap()->getTile((minX + maxX) / 2, (minY + maxY) / 2);
}

void Building::addCoveredTile(Tile* t, double nHP)
{
    mCoveredTiles.push_back(t);
    mTileHP[t] = nHP;
    t->setCoveringBuilding(this);
}

bool Building::removeCoveredTile(Tile* t)
{
    LogManager::getSingleton().logMessage(getGameMap()->serverStr() + "building=" + getName() + ", removing covered tile=" + Tile::displayAsString(t));
    for (std::vector<Tile*>::iterator it = mCoveredTiles.begin(); it != mCoveredTiles.end(); ++it)
    {
        if (t == *it)
        {
            mCoveredTiles.erase(it);
            mTileHP.erase(t);
            t->setCoveringBuilding(nullptr);

            if(getGameMap()->isServerGameMap())
                return true;

            // Destroy the mesh for this tile.
            RenderRequest *request = new RenderRequestDestroyBuilding(this, t);
            RenderManager::queueRenderRequest(request);
            return true;
        }
    }
    OD_ASSERT_TRUE_MSG(false, getGameMap()->serverStr() + "building=" + getName() + ", removing unknown covered tile=" + Tile::displayAsString(t));
    return false;
}

Tile* Building::getCoveredTile(int index)
{
    return mCoveredTiles[index];
}

std::vector<Tile*> Building::getCoveredTiles()
{
    return mCoveredTiles;
}

unsigned int Building::numCoveredTiles()
{
    return mCoveredTiles.size();
}

void Building::clearCoveredTiles()
{
    mCoveredTiles.clear();
}

double Building::getHP(Tile *tile) const
{
    if (tile != NULL)
    {
        std::map<Tile*, double>::const_iterator tileSearched = mTileHP.find(tile);
        OD_ASSERT_TRUE(tileSearched != mTileHP.end());
        if(tileSearched == mTileHP.end())
            return 0.0;

        return tileSearched->second;
    }

    // If the tile given was NULL, we add the total HP of all the tiles in the room and return that.
    double total = 0.0;

    for(const std::pair<Tile* const, double>& p : mTileHP)
    {
        total += p.second;
    }

    return total;
}

double Building::takeDamage(GameEntity* attacker, double physicalDamage, double magicalDamage, Tile *tileTakingDamage)
{
    double damageDone = std::min(mTileHP[tileTakingDamage], physicalDamage + magicalDamage);
    mTileHP[tileTakingDamage] -= damageDone;

    GameMap* gameMap = getGameMap();
    if(!gameMap->isServerGameMap())
        return damageDone;

    Seat* seat = getSeat();
    if (seat == NULL)
        return damageDone;

    Player* player = gameMap->getPlayerBySeatId(seat->getId());
    if (player == NULL)
        return damageDone;

    // Tells the server game map the player is under attack.
    gameMap->playerIsFighting(player);

    return damageDone;
}

std::string Building::getNameTile(Tile* tile)
{
    return getMeshName() + "_tile_" + Ogre::StringConverter::toString(tile->x)
        + "_" + Ogre::StringConverter::toString(tile->y);
}

bool Building::isAttackable() const
{
    if(getHP(NULL) <= 0.0)
        return false;

    return true;
}
