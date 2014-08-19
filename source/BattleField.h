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

#ifndef BATTLE_FIELD_H
#define BATTLE_FIELD_H

#include <vector>
#include <string>

class GameMap;

//! \brief A structure storing the security level of the creature visible tiles
struct SecurityTile {
public:
    SecurityTile(int posX, int posY, double securityLevel):
        mPosX(posX),
        mPosY(posY),
        mSecurityLevel(securityLevel)
    {}

    void setSecurityLevel(double securityLevel)
    { mSecurityLevel = securityLevel; }

    double getSecurityLevel() const
    { return mSecurityLevel; }

    bool isPosition(int x, int y)
    { return mPosX == x && mPosY == y; }

    int getPosX() const
    { return mPosX; }

    int getPosY() const
    { return mPosY; }

private:
    //! \brief The tile position
    int mPosX;
    int mPosY;

    //! \brief The security level
    //! This value is set up according to the presence of allies and enemies around that tile.
    //! Allies will make the value raise, and enemies will make it lower.
    //! A creature will thus attack if it's own tile position has got a security level that is > 0
    //! meaning there are enough allies around of not enough enemies to scare it.
    double mSecurityLevel;
};

typedef std::vector<SecurityTile> FieldType;

//! \brief Class used to store the hostility ratio of visible tiles for a creature.
//! This way, the creature will know where to go to fight opponents.
class BattleField
{
public:
    BattleField(GameMap* gameMap);

    ~BattleField()
    {}

    virtual std::string getOgreNamePrefix() { return "Field_"; }

    const std::string& getName() const
    { return mName; }

    //! \brief Returns the stored value at a position (or 0.0 if not found)
    double getTileSecurityLevel(int x, int y);

    FieldType::iterator begin();
    FieldType::iterator end();
    void clear();

    //! \brief Sets the field security value at location (x,y) to the value f.
    void setTileSecurityLevel(int x, int y, double f);

    //! \brief Get the location with min/max security level
    SecurityTile getMinSecurityLevel();
    SecurityTile getMaxSecurityLevel();

    //! \brief Visual debugging functions showing the hostility field on map.
    void refreshMeshes(double offset);
    void createMeshes(double offset);
    //! FIXME: Not handled in the render manager.
    void destroyMeshes();

private:
    //! \brief The Battle Field name
    std::string mName;

    //! \brief The battle field hostility map
    FieldType mField;

    //! \brief Whether visual debugging meshes were created for it on the game map.
    bool mHasMeshes;
};

#endif // BATTLE_FIELD_H

