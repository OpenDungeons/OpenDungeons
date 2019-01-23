/*
 *  Copyright (C) 2011-2016  OpenDungeons Team
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

#ifndef TILESET_H
#define TILESET_H

#include <OgreVector3.h>

#include <cstdint>
#include <string>

class Tile;

enum class TileType;
enum class TileVisual;

class TileSetValue
{
public:
    TileSetValue() :
    mRotationX(0.0),
    mRotationY(0.0),
    mRotationZ(0.0)
    {}

    TileSetValue(const std::string& meshName, const std::string& materialName,
                 Ogre::Real rotationX, Ogre::Real rotationY,Ogre::Real rotationZ) :
        mMeshName(meshName),
        mMaterialName(materialName),
        mRotationX(rotationX),
        mRotationY(rotationY),
        mRotationZ(rotationZ)
    {}

    inline const std::string& getMeshName() const
    { return mMeshName; }

    inline const std::string& getMaterialName() const
    { return mMaterialName; }

    inline Ogre::Real getRotationX() const
    { return mRotationX; }

    inline Ogre::Real getRotationY() const
    { return mRotationY; }

    inline Ogre::Real getRotationZ() const
    { return mRotationZ; }

private:
    std::string mMeshName;
    std::string mMaterialName;
    Ogre::Real mRotationX;
    Ogre::Real mRotationY;
    Ogre::Real mRotationZ;
};

class TileSet
{
public:
    TileSet();

    std::vector<std::vector<TileSetValue>>& configureTileValues(TileVisual tileVisual);

    const std::vector<std::vector<TileSetValue>>& getTileValues(TileVisual tileVisual) const;

    //! Returns true if the 2 tiles are linked and false otherwise.
    //! Used on client side only
    bool areLinked(const Tile* tile1, const Tile* tile2) const;

    void addTileLink(TileVisual tileVisual1, TileVisual tileVisual2);

private:
    std::vector<std::vector<std::vector<TileSetValue>>> mTileValues;
    //! Represents the links between tiles. The uint is used as a bit array.
    //! The index in the vector corresponds to the TileVisual
    std::vector<uint32_t> mTileLinks;
};

#endif
