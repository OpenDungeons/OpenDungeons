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

#include "entities/Tile.h"

#include <OgreVector3.h>

#include <string>
#include <iostream>

class ODPacket;

//! \brief The maximum level of a creature
static const uint32_t MAX_LEVEL = 30;

class CreatureDefinition
{
public:
    enum CreatureJob
    {
        Worker = 1, // Dig, claim tile and deposit gold. Only fights workers
        Fighter,    // Sleep, eat, train and fight any enemy thing.
    };

    CreatureDefinition(
            const std::string&      className   = std::string(),
            CreatureJob             job         = Fighter,
            const std::string&      meshName    = std::string(),
            const std::string&      bedMeshName = std::string(),
            int                     bedDim1     = 1,
            int                     bedDim2     = 1,
            const Ogre::Vector3&    scale       = Ogre::Vector3(1, 1, 1),
            double                  sightRadius = 15.0,

            double                  digRate     = 10.0,
            double                  digRatePerLevel = 2.1,
            double                  claimRate   = 0.35,
            double                  claimRatePerLevel = 0.06,

            double                  minHP       = 1.0,
            double                  hpPerLevel  = 5.0,
            double                  hpHealPerTurn = 0.1,

            double                  awakenessLostPerTurn = 0.1,
            double                  hungerGrowthPerTurn = 0.1,

            double                  moveSpeedGround     = 1.0,
            double                  moveSpeedWater      = 0.0,
            double                  moveSpeedLava       = 0.0,
            double                  groundSpeedPerLevel = 0.02,
            double                  waterSpeedPerLevel  = 0.0,
            double                  lavaSpeedPerLevel   = 0.0,

            double                  physicalAttack      = 1.0,
            double                  physicalAtkPerLevel = 0.6,
            double                  magicalAttack       = 0.8,
            double                  magicalAtkPerLevel  = 0.4,
            double                  physicalDefense     = 3.0,
            double                  physicalDefPerLevel = 0.2,
            double                  magicalDefense      = 1.5,
            double                  magicalDefPerLevel  = 0.1,
            double                  attackRange         = 1.0,
            double                  atkRangePerLevel    = 0.0,
            double                  attackWarmupTime    = 1.0) :
        mCreatureJob (job),
        mClassName   (className),
        mMeshName    (meshName),
        mBedMeshName (bedMeshName),
        mBedDim1     (bedDim1),
        mBedDim2     (bedDim2),
        mScale       (scale),
        mSightRadius (sightRadius),
        mDigRate     (digRate),
        mDigRatePerLevel (digRatePerLevel),
        mClaimRate   (claimRate),
        mClaimRatePerLevel(claimRatePerLevel),
        mMinHP       (minHP),
        mHpPerLevel  (hpPerLevel),
        mHpHealPerTurn      (hpHealPerTurn),
        mAwakenessLostPerTurn(awakenessLostPerTurn),
        mHungerGrowthPerTurn(hungerGrowthPerTurn),
        mMoveSpeedGround    (moveSpeedGround),
        mMoveSpeedWater     (moveSpeedWater),
        mMoveSpeedLava      (moveSpeedLava),
        mGroundSpeedPerLevel(groundSpeedPerLevel),
        mWaterSpeedPerLevel (waterSpeedPerLevel),
        mLavaSpeedPerLevel  (lavaSpeedPerLevel),
        mPhysicalAttack     (physicalAttack),
        mPhysicalAtkPerLevel(physicalAtkPerLevel),
        mMagicalAttack      (magicalAttack),
        mMagicalAtkPerLevel (magicalAtkPerLevel),
        mPhysicalDefense    (physicalDefense),
        mPhysicalDefPerLevel(physicalDefPerLevel),
        mMagicalDefense     (magicalDefense),
        mMagicalDefPerLevel (magicalDefPerLevel),
        mAttackRange        (attackRange),
        mAtkRangePerLevel   (atkRangePerLevel),
        mAttackWarmupTime   (attackWarmupTime),
        mWeaponSpawnL       ("none"),
        mWeaponSpawnR       ("none")
    {
        mXPTable.assign(MAX_LEVEL - 1, 100.0);
    }

    static CreatureJob creatureJobFromString(const std::string& s);
    static std::string creatureJobToString(CreatureJob c);
    //! \brief Writes the differences between def1 and def2 in the given file. Note that def1 can be null. In
    //! this case, every parameters in def2 will be written. def2 cannot be null.
    static void writeCreatureDefinitionDiff(const CreatureDefinition* def1, const CreatureDefinition* def2, std::ofstream& file);

    inline bool isWorker() const
    { return (mCreatureJob == Worker); }

    friend ODPacket& operator <<(ODPacket& os, CreatureDefinition *c);
    friend ODPacket& operator >>(ODPacket& is, CreatureDefinition *c);

    //! \brief Loads a definition from the creature definition file sub [Creature][/Creature] part
    //! \returns A creature definition if valid, nullptr otherwise.
    static CreatureDefinition* load(std::stringstream& defFile);
    static bool update(CreatureDefinition* creatureDef, std::stringstream& defFile);

    inline CreatureJob          getCreatureJob  () const    { return mCreatureJob; }
    inline const std::string&   getClassName    () const    { return mClassName; }

    inline const std::string&   getMeshName     () const    { return mMeshName; }
    inline const Ogre::Vector3& getScale        () const    { return mScale; }

    inline const std::string&   getBedMeshName  () const    { return mBedMeshName; }
    inline int                  getBedDim1      () const    { return mBedDim1; }
    inline int                  getBedDim2      () const    { return mBedDim2; }

    inline double               getSightRadius  () const    { return mSightRadius; }

    inline double               getClaimRate    () const    { return mClaimRate; }
    inline double               getClaimRatePerLevel() const{ return mClaimRatePerLevel; }
    inline double               getDigRate      () const    { return mDigRate; }
    inline double               getDigRatePerLevel() const  { return mDigRatePerLevel; }

    inline double               getMinHp        () const    { return mMinHP; }
    inline double               getHpPerLevel   () const    { return mHpPerLevel; }
    inline double               getHpHealPerTurn() const    { return mHpHealPerTurn; }

    inline double               getAwakenessLostPerTurn() const{ return mAwakenessLostPerTurn; }
    inline double               getHungerGrowthPerTurn() const { return mHungerGrowthPerTurn; }

    inline double               getMoveSpeedGround  () const    { return mMoveSpeedGround; }
    inline double               getMoveSpeedWater   () const    { return mMoveSpeedWater; }
    inline double               getMoveSpeedLava    () const    { return mMoveSpeedLava; }

    inline double               getGroundSpeedPerLevel() const  { return mGroundSpeedPerLevel; }
    inline double               getWaterSpeedPerLevel () const  { return mWaterSpeedPerLevel; }
    inline double               getLavaSpeedPerLevel  () const  { return mLavaSpeedPerLevel; }


    inline double               getPhysicalAttack() const       { return mPhysicalAttack; }
    inline double               getPhysicalAtkPerLevel () const { return mPhysicalAtkPerLevel; }
    inline double               getMagicalAttack  () const      { return mMagicalAttack; }
    inline double               getMagicalAtkPerLevel () const  { return mMagicalAtkPerLevel; }
    inline double               getPhysicalDefense() const      { return mPhysicalDefense; }
    inline double               getPhysicalDefPerLevel () const { return mPhysicalDefPerLevel; }
    inline double               getMagicalDefense  () const     { return mMagicalDefense; }
    inline double               getMagicalDefPerLevel () const  { return mMagicalDefPerLevel; }

    inline double               getAttackRange      () const    { return mAttackRange; }
    inline double               getAtkRangePerLevel () const    { return mAtkRangePerLevel; }

    inline double               getAttackWarmupTime () const    { return mAttackWarmupTime; }

    const std::string&          getWeaponSpawnL () const        { return mWeaponSpawnL; }
    const std::string&          getWeaponSpawnR () const        { return mWeaponSpawnR; }

    double                      getXPNeededWhenLevel(unsigned int level) const;

private:
    //! \brief The job of the creature (e.g. worker, fighter, ...)
    CreatureJob mCreatureJob;

    //! \brief The name of the creatures class
    std::string mClassName;

    //! \brief The name of the creature definition this one is based on (can be empty if no base class)
    std::string mBaseDefinition;

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

    //! \brief How much dig rate is earned at each level up.
    double mDigRatePerLevel;

    //! \brief How quick a worker can claim a ground or wall tile.
    double mClaimRate;

    //! \brief How much claim rate is earned at each level up.
    double mClaimRatePerLevel;

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

    //! \brief how much speed is earn per level on each tile type.
    double mGroundSpeedPerLevel;
    double mWaterSpeedPerLevel;
    double mLavaSpeedPerLevel;

    //! \brief Physical and magical attack/defense stats (without equipment)
    double mPhysicalAttack;
    double mPhysicalAtkPerLevel;
    double mMagicalAttack;
    double mMagicalAtkPerLevel;
    double mPhysicalDefense;
    double mPhysicalDefPerLevel;
    double mMagicalDefense;
    double mMagicalDefPerLevel;

    //! \brief Weapon-less attack range and growth
    double mAttackRange;
    double mAtkRangePerLevel;

    //! \brief The time to wait before dealing a blow, in seconds.
    double mAttackWarmupTime;

    //! \brief Weapons a creature should spawn with ("none" if no weapon)
    std::string mWeaponSpawnL;
    std::string mWeaponSpawnR;

    //! \brief The XP table, used to know how XP is needed to reach the next level.
    //! \note The creature starting at level 1, it can only change its level MAX_LEVEL - 1 times.
    std::vector<double> mXPTable;

    //! \brief Loads the creature XP values for the current definition.
    static void loadXPTable(std::stringstream& defFile, CreatureDefinition* creatureDef);
};

#endif // CREATUREDEFINITION_H
