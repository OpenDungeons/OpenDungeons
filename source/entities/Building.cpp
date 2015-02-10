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

#include "entities/Building.h"

#include "entities/BuildingObject.h"
#include "entities/RenderedMovableEntity.h"
#include "entities/Tile.h"
#include "gamemap/GameMap.h"
#include "network/ODServer.h"

#include "utils/LogManager.h"

const double Building::DEFAULT_TILE_HP = 10.0;

const Ogre::Vector3 SCALE(RenderManager::BLENDER_UNITS_PER_OGRE_UNIT,
        RenderManager::BLENDER_UNITS_PER_OGRE_UNIT,
        RenderManager::BLENDER_UNITS_PER_OGRE_UNIT);

const Ogre::Vector3& Building::getScale() const
{
    return SCALE;
}

void Building::addBuildingObject(Tile* targetTile, RenderedMovableEntity* obj)
{
    if(obj == nullptr)
        return;

    // We assume the object position has been already set (most of the time in loadBuildingObject)
    mBuildingObjects[targetTile] = obj;
    obj->addToGameMap();
    obj->setPosition(obj->getPosition(), false);
}

void Building::removeBuildingObject(Tile* tile)
{
    if(mBuildingObjects.count(tile) == 0)
        return;

    RenderedMovableEntity* obj = mBuildingObjects[tile];
    obj->removeFromGameMap();
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
        obj->removeFromGameMap();
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
        obj->removeFromGameMap();
        obj->deleteYourself();
        ++itr;
    }
    mBuildingObjects.clear();
}

RenderedMovableEntity* Building::getBuildingObjectFromTile(Tile* tile)
{
    if(mBuildingObjects.count(tile) == 0)
        return nullptr;

    RenderedMovableEntity* obj = mBuildingObjects[tile];
    return obj;
}

RenderedMovableEntity* Building::loadBuildingObject(GameMap* gameMap, const std::string& meshName,
    Tile* targetTile, double rotationAngle, bool hideCoveredTile, float opacity)
{
    if (targetTile == nullptr)
        targetTile = getCentralTile();

    OD_ASSERT_TRUE(targetTile != nullptr);
    if(targetTile == nullptr)
        return nullptr;

    return loadBuildingObject(gameMap, meshName, targetTile, static_cast<double>(targetTile->getX()),
        static_cast<double>(targetTile->getY()), rotationAngle, hideCoveredTile, opacity);
}

RenderedMovableEntity* Building::loadBuildingObject(GameMap* gameMap, const std::string& meshName,
    Tile* targetTile, double x, double y, double rotationAngle, bool hideCoveredTile, float opacity)
{
    std::string baseName;
    if(targetTile == nullptr)
        baseName = getName();
    else
        baseName = getName() + "_" + Tile::displayAsString(targetTile);

    Ogre::Vector3 position(static_cast<Ogre::Real>(x), static_cast<Ogre::Real>(y), 0);
    BuildingObject* obj = new BuildingObject(gameMap, baseName, meshName,
        position, static_cast<Ogre::Real>(rotationAngle), hideCoveredTile, opacity);

    return obj;
}

Tile* Building::getCentralTile()
{
    if (mCoveredTiles.empty())
        return nullptr;

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

            return true;
        }
    }
    OD_ASSERT_TRUE_MSG(false, getGameMap()->serverStr() + "building=" + getName() + ", removing unknown covered tile=" + Tile::displayAsString(t));
    return false;
}

std::vector<Tile*> Building::getCoveredTiles()
{
    return mCoveredTiles;
}

Tile* Building::getCoveredTile(int index)
{
    OD_ASSERT_TRUE_MSG(index < static_cast<int>(mCoveredTiles.size()), "name=" + getName()
        + ", index=" + Ogre::StringConverter::toString(index));

    if(index >= static_cast<int>(mCoveredTiles.size()))
        return nullptr;

    return mCoveredTiles[index];
}

uint32_t Building::numCoveredTiles()
{
    return mCoveredTiles.size();
}

void Building::clearCoveredTiles()
{
    mCoveredTiles.clear();
}

double Building::getHP(Tile *tile) const
{
    if (tile != nullptr)
    {
        std::map<Tile*, double>::const_iterator tileSearched = mTileHP.find(tile);
        OD_ASSERT_TRUE(tileSearched != mTileHP.end());
        if(tileSearched == mTileHP.end())
            return 0.0;

        return tileSearched->second;
    }

    // If the tile given was nullptr, we add the total HP of all the tiles in the room and return that.
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
    if (seat == nullptr)
        return damageDone;

    Player* player = gameMap->getPlayerBySeatId(seat->getId());
    if (player == nullptr)
        return damageDone;

    // Tells the server game map the player is under attack.
    gameMap->playerIsFighting(player);

    return damageDone;
}

std::string Building::getNameTile(Tile* tile)
{
    return getMeshName() + "_tile_" + Ogre::StringConverter::toString(tile->getX())
        + "_" + Ogre::StringConverter::toString(tile->getY());
}

bool Building::isAttackable(Tile* tile, Seat* seat) const
{
    if(getHP(tile) <= 0.0)
        return false;

    return true;
}
