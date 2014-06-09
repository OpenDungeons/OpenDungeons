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

#ifndef CREATURECLASS_H
#define CREATURECLASS_H

#include "Tile.h"

#include <OgreVector3.h>

#include <string>
#include <iostream>

class CreatureDefinition
{
public:
    enum CreatureJob
    {
        nullCreatureJob = 0,
        basicWorker = 1,
        advancedWorker,
        scout,
        weakFighter,
        weakSpellcaster,
        weakBuilder,
        strongFighter,
        strongSpellcaster,
        strongBuilder,
        guard,
        specialCreature,
        summon,
        superCreature
    };

    CreatureDefinition(
            CreatureJob             job         = nullCreatureJob,
            const std::string&      className   = std::string(),
            const std::string&      meshName    = std::string(),
            const std::string&      bedMeshName = std::string(),
            int                     bedDim1     = 1,
            int                     bedDim2     = 1,
            const Ogre::Vector3&    scale       = Ogre::Vector3(1, 1, 1),
            double                  sightRadius = 15.0,
            double                  digRate     = 10.0,
            double                  danceRate   = 0.35,
            double                  hpPerLevel  = 5.0,
            double                  manaPerLevel= 5.0,
            double                  maxHP       = 100.0,
            double                  maxMana     = 100.0,
            double                  moveSpeed   = 1.0,

            Tile::TileClearType     tilePassability = Tile::walkableTile) :
        mCreatureJob (job),
        mClassName   (className),
        mMeshName    (meshName),
        mBedMeshName (bedMeshName),
        mBedDim1     (bedDim1),
        mBedDim2     (bedDim2),
        mScale       (scale),
        mSightRadius (sightRadius),
        mDigRate     (digRate),
        mDanceRate   (danceRate),
        mHpPerLevel  (hpPerLevel),
        mManaPerLevel(manaPerLevel),
        mMaxHP       (maxHP),
        mMaxMana     (maxMana),
        mMoveSpeed   (moveSpeed),

        mTilePassability         (tilePassability)
    {}

    static CreatureJob creatureJobFromString(const std::string& s);
    static std::string creatureJobToString(CreatureJob c);

    inline bool isWorker() const
    { return (mCreatureJob == basicWorker || mCreatureJob == advancedWorker); }

    inline static std::string getFormat()
    {
        return "# className\tcreatureJob\tmeshName\tbedMeshName\tbedDim1\tbedDim2\tscaleX\tscaleY\tscaleZ\t"
               "hp/level\tmana/level\tmaxHP\tmaxMana\tsightRadius\tdigRate\tdanceRate\tmoveSpeed\ttilePassability\n";
    }

    friend std::ostream & operator <<(std::ostream & os, CreatureDefinition *c);
    friend std::istream & operator >>(std::istream & is, CreatureDefinition *c);

    //! \brief Loads a definition from a line of the creature definition file.
    static void loadFromLine(const std::string& line, CreatureDefinition* c);

    inline CreatureJob          getCreatureJob  () const    { return mCreatureJob; }
    inline int                  getBedDim1      () const    { return mBedDim1; }
    inline int                  getBedDim2      () const    { return mBedDim2; }
    inline const std::string&   getBedMeshName  () const    { return mBedMeshName; }
    inline const std::string&   getClassName    () const    { return mClassName; }
    inline double               getDanceRate    () const    { return mDanceRate; }
    inline double               getDigRate      () const    { return mDigRate; }
    inline double               getHpPerLevel   () const    { return mHpPerLevel; }
    inline double               getManaPerLevel () const    { return mManaPerLevel; }
    inline double               getMaxHp        () const    { return mMaxHP; }
    inline double               getMaxMana      () const    { return mMaxMana; }
    inline const std::string&   getMeshName     () const    { return mMeshName; }
    inline double               getMoveSpeed    () const    { return mMoveSpeed; }
    inline const Ogre::Vector3& getScale        () const    { return mScale; }
    inline double               getSightRadius  () const    { return mSightRadius; }

    inline Tile::TileClearType  getTilePassability () const { return mTilePassability; }

private:
    //NOTE: Anything added to this class must be included in the '=' operator for the Creature class.

    //! \brief The job of the creature (e.g. worker, fighter, ...)
    CreatureJob mCreatureJob;

    //! \brief The name of the creatures class
    std::string mClassName;

    //! \brief The name of the model file
    std::string mMeshName;

    //! \brief The name of the bed model file
    std::string mBedMeshName;

    //! \brief size of the bed (x)
    int mBedDim1;

    //! \brief size of the bed (y)
    int mBedDim2;

    //! \brief The scale the mesh is displayed (bigger = stronger)
    Ogre::Vector3 mScale;

    //! \brief The inner radius where the creature sees everything
    double mSightRadius;

    //! \brief Fullness removed per turn of digging
    double mDigRate;

    //! \brief How much the danced upon tile's color changes per turn of dancing
    double mDanceRate;

    //! \brief How much HP the creature gets per level up
    double mHpPerLevel;

    //! \brief How much mana the creature gets per level up
    double mManaPerLevel;

    //! \brief The maximum HP the creature can ever have
    double mMaxHP;

    //! \brief The maximum mana the creature can ever have
    double mMaxMana;

    //! \brief How fast the creature moves
    double mMoveSpeed;

    //! \brief Tile passability (defines mostly if the creature is walking, flying, or etherreal)
    Tile::TileClearType mTilePassability;
};

#endif // CREATURECLASS_H
