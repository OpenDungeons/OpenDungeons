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

#ifndef CREATUREDEFINITION_H
#define CREATUREDEFINITION_H

#include "Tile.h"

#include <OgreVector3.h>

#include <string>
#include <iostream>

class ODPacket;

class CreatureDefinition
{
public:
    enum CreatureJob
    {
        Worker = 1, // Dig, claim tile and deposit gold. Only fights workers
        Fighter,    // Sleep, eat, train and fight any enemy thing.
    };

    CreatureDefinition(
            CreatureJob             job         = Fighter,
            const std::string&      className   = std::string(),
            const std::string&      meshName    = std::string(),
            const std::string&      bedMeshName = std::string(),
            int                     bedDim1     = 1,
            int                     bedDim2     = 1,
            const Ogre::Vector3&    scale       = Ogre::Vector3(1, 1, 1),
            double                  sightRadius = 15.0,
            double                  digRate     = 10.0,
            double                  danceRate   = 0.35,
            double                  minHP       = 1.0,
            double                  hpPerLevel  = 5.0,
            double                  hpHealPerTurn = 0.1,
            double                  awakenessLostPerTurn = 0.1,
            double                  hungerGrowthPerTurn = 0.1,
            double                  moveSpeedGround     = 1.0,
            double                  moveSpeedWater      = 0.0,
            double                  moveSpeedLava       = 0.0) :
        mCreatureJob (job),
        mClassName   (className),
        mMeshName    (meshName),
        mBedMeshName (bedMeshName),
        mBedDim1     (bedDim1),
        mBedDim2     (bedDim2),
        mScale       (scale),
        mSightRadius (sightRadius),
        mDigRate     (digRate),
        mClaimRate   (danceRate),
        mMinHP       (minHP),
        mHpPerLevel  (hpPerLevel),
        mHpHealPerTurn      (hpHealPerTurn),
        mAwakenessLostPerTurn(awakenessLostPerTurn),
        mHungerGrowthPerTurn(hungerGrowthPerTurn),
        mMoveSpeedGround    (moveSpeedGround),
        mMoveSpeedWater     (moveSpeedWater),
        mMoveSpeedLava      (moveSpeedLava)
    {}

    static CreatureJob creatureJobFromString(const std::string& s);
    static std::string creatureJobToString(CreatureJob c);

    inline bool isWorker() const
    { return (mCreatureJob == Worker); }

    friend ODPacket& operator <<(ODPacket& os, CreatureDefinition *c);
    friend ODPacket& operator >>(ODPacket& is, CreatureDefinition *c);

    //! \brief Loads a definition from the creature definition file sub [Creature][/Creature] part
    //! \returns A creature definition if valid, nulptr otherwise.
    static CreatureDefinition* load(std::stringstream& defFile);

    inline CreatureJob          getCreatureJob  () const    { return mCreatureJob; }
    inline int                  getBedDim1      () const    { return mBedDim1; }
    inline int                  getBedDim2      () const    { return mBedDim2; }
    inline const std::string&   getBedMeshName  () const    { return mBedMeshName; }
    inline const std::string&   getClassName    () const    { return mClassName; }
    inline double               getClaimRate    () const    { return mClaimRate; }
    inline double               getDigRate      () const    { return mDigRate; }
    inline double               getMinHp        () const    { return mMinHP; }
    inline double               getHpPerLevel   () const    { return mHpPerLevel; }
    inline double               getHpHealPerTurn() const    { return mHpHealPerTurn; }
    inline double               getAwakenessLostPerTurn() const{ return mAwakenessLostPerTurn; }
    inline double               getHungerGrowthPerTurn() const { return mHungerGrowthPerTurn; }
    inline const std::string&   getMeshName     () const    { return mMeshName; }
    inline double               getMoveSpeedGround  () const    { return mMoveSpeedGround; }
    inline double               getMoveSpeedWater   () const    { return mMoveSpeedWater; }
    inline double               getMoveSpeedLava    () const    { return mMoveSpeedLava; }
    inline const Ogre::Vector3& getScale        () const    { return mScale; }
    inline double               getSightRadius  () const    { return mSightRadius; }

private:
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
    int mSightRadius;

    //! \brief Fullness removed per turn of digging
    double mDigRate;

    //! \brief How quick a worker can claim a ground or wall tile.
    double mClaimRate;

    //! \brief The minimum HP the creature can ever have
    double mMinHP;

    //! \brief How much HP the creature gets per level up
    double mHpPerLevel;

    //! \brief How much HP are healed per turn.
    double mHpHealPerTurn;

    //! \brief How much awakeness is lost per turn.
    double mAwakenessLostPerTurn;

    //! \brief How fast hunger grows per turn.
    double mHungerGrowthPerTurn;

    //! \brief How fast the creature moves (0 -> The creature cannot go through corresponding tile)
    double mMoveSpeedGround;
    double mMoveSpeedWater;
    double mMoveSpeedLava;
};

#endif // CREATUREDEFINITION_H
