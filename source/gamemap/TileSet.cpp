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

#include "gamemap/TileSet.h"

#include "entities/Tile.h"
#include "utils/LogManager.h"

TileSet::TileSet(const Ogre::Vector3& scale) :
    mTileValues(static_cast<uint32_t>(TileVisual::countTileVisual), std::vector<TileSetValue>(16)),
    mScale(scale),
    mTileLinks(std::vector<uint32_t>(static_cast<uint32_t>(TileVisual::countTileVisual), 0))
{
}

std::vector<TileSetValue>& TileSet::configureTileValues(TileVisual tileVisual)
{
    uint32_t tileTypeNumber = static_cast<uint32_t>(tileVisual);
    if(tileTypeNumber >= mTileValues.size())
    {
        OD_ASSERT_TRUE_MSG(false, "Trying to get unknow tileType=" + Tile::tileVisualToString(tileVisual));
        return mTileValues.at(0);
    }

    return mTileValues[tileTypeNumber];
}

const std::vector<TileSetValue>& TileSet::getTileValues(TileVisual tileVisual) const
{
    uint32_t tileTypeNumber = static_cast<uint32_t>(tileVisual);
    if(tileTypeNumber >= mTileValues.size())
    {
        OD_ASSERT_TRUE_MSG(false, "Trying to get unknow tileType=" + Tile::tileVisualToString(tileVisual));
        return mTileValues.at(0);
    }

    return mTileValues[tileTypeNumber];
}

void TileSet::addTileLink(TileVisual tileVisual1, TileVisual tileVisual2)
{
    uint32_t intTile1Visual = static_cast<uint32_t>(tileVisual1);
    uint32_t intTile2Visual = static_cast<uint32_t>(tileVisual2);
    if(intTile1Visual >= mTileLinks.size())
    {
        OD_ASSERT_TRUE_MSG(false, "TileVisual=" + Tile::tileVisualToString(tileVisual1));
        return;
    }
    if(intTile2Visual >= mTileLinks.size())
    {
        OD_ASSERT_TRUE_MSG(false, "TileVisual=" + Tile::tileVisualToString(tileVisual2));
        return;
    }

    uint32_t tileLink;
    tileLink = 1 << intTile2Visual;
    mTileLinks[intTile1Visual] |= tileLink;

    // We want a commutative relationship
    tileLink = 1 << intTile1Visual;
    mTileLinks[intTile2Visual] |= tileLink;
}

bool TileSet::areLinked(const Tile* tile1, const Tile* tile2) const
{
    // Check if the tile visual is linkable
    uint32_t intTile1Visual = static_cast<uint32_t>(tile1->getTileVisual());
    uint32_t intTile2Visual = static_cast<uint32_t>(tile2->getTileVisual());
    uint32_t linkValue = 1 << intTile2Visual;
    linkValue &= mTileLinks[intTile1Visual];

    if(linkValue == 0)
        return false;

    return true;
}
