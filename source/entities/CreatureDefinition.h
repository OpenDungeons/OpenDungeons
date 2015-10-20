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

#ifndef CREATUREDEFINITION_H
#define CREATUREDEFINITION_H

#include <OgreVector3.h>

#include <string>
#include <iosfwd>
#include <cstdint>

class CreatureBehaviour;
class CreatureSkill;
class ODPacket;

enum class RoomType;

//! \brief The maximum level of a creature
static const uint32_t MAX_LEVEL = 30;

class CreatureRoomAffinity
{
public:
    CreatureRoomAffinity(RoomType roomType, int32_t likeness, double efficiency):
        mRoomType(roomType),
        mLikeness(likeness),
        mEfficiency(efficiency)
    {
    }

    inline RoomType getRoomType() const
    { return mRoomType; }

    inline int32_t getLikeness() const
    { return mLikeness; }

    inline double getEfficiency() const
    { return mEfficiency; }

    bool operator==(const CreatureRoomAffinity& creatureRoomAffinity) const
    {
        return mRoomType == creatureRoomAffinity.mRoomType &&
            mLikeness == creatureRoomAffinity.mLikeness &&
            mEfficiency == creatureRoomAffinity.mEfficiency;
    }

private:
    RoomType mRoomType;
    int32_t mLikeness;
    double mEfficiency;
};

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
            const Ogre::Vector3&    scale       = Ogre::Vector3(1, 1, 1),
            const std::string&      bedMeshName = std::string(),
            int                     bedDim1     = 1,
            int                     bedDim2     = 1,
            int                     bedPosX     = 1,
            int                     bedPosY     = 1,
            double                  bedOrientX  = 0,
            double                  bedOrientY  = -1,
            double                  sightRadius = 15.0,

            int                     maxGoldCarryable = 0,
            double                  digRate     = 10.0,
            double                  digRatePerLevel = 2.1,
            double                  claimRate   = 0.35,
            double                  claimRatePerLevel = 0.06,

            double                  minHP       = 1.0,
            double                  hpPerLevel  = 5.0,
            double                  hpHealPerTurn = 0.1,

            double                  wakefulnessLostPerTurn = 0.1,
            double                  hungerGrowthPerTurn = 0.1,

            double                  moveSpeedGround     = 1.0,
            double                  moveSpeedWater      = 0.0,
            double                  moveSpeedLava       = 0.0,
            double                  groundSpeedPerLevel = 0.02,
            double                  waterSpeedPerLevel  = 0.0,
            double                  lavaSpeedPerLevel   = 0.0,

            double                  phyAtkMel           = 1.0,
            double                  phyAtkMelPerLvl     = 0.6,
            double                  magAtkMel           = 0.8,
            double                  magAtkMelPerLvl     = 0.4,
            double                  physicalDefense     = 3.0,
            double                  physicalDefPerLevel = 0.2,
            double                  magicalDefense      = 1.5,
            double                  magicalDefPerLevel  = 0.1,
            double                  elementDefense      = 1.5,
            double                  elementDefPerLevel  = 0.1,
            int32_t                 fightIdleDist       = 1,
            int32_t                 feeBase             = 0,
            int32_t                 feePerLevel         = 0,
            int32_t                 sleepHeal           = 1.0,
            int32_t                 turnsStunDropped    = 0);

    CreatureDefinition(const CreatureDefinition& def);

    virtual ~CreatureDefinition();

    static CreatureJob creatureJobFromString(const std::string& s);
    static std::string creatureJobToString(CreatureJob c);
    //! \brief Writes the differences between def1 and def2 in the given file. Note that def1 can be null. In
    //! this case, every parameters in def2 will be written. def2 cannot be null.
    static void writeCreatureDefinitionDiff(
        const CreatureDefinition* def1, const CreatureDefinition* def2, std::ostream& file, const std::map<std::string, CreatureDefinition*>& defMap);

    inline bool isWorker() const
    { return (mCreatureJob == Worker); }

    friend ODPacket& operator <<(ODPacket& os, const CreatureDefinition *c);
    friend ODPacket& operator >>(ODPacket& is, CreatureDefinition *c);

    //! \brief Loads a definition from the creature definition file sub [Creature][/Creature] part
    //! \returns A creature definition if valid, nullptr otherwise.
    static CreatureDefinition* load(std::stringstream& defFile, const std::map<std::string, CreatureDefinition*>& defMap);
    static bool update(CreatureDefinition* creatureDef, std::stringstream& defFile, const std::map<std::string, CreatureDefinition*>& defMap);

    inline CreatureJob          getCreatureJob  () const    { return mCreatureJob; }
    inline const std::string&   getClassName    () const    { return mClassName; }

    inline const std::string&   getMeshName     () const    { return mMeshName; }
    inline const Ogre::Vector3& getScale        () const    { return mScale; }

    inline const std::string&   getBedMeshName  () const    { return mBedMeshName; }
    inline int                  getBedDim1      () const    { return mBedDim1; }
    inline int                  getBedDim2      () const    { return mBedDim2; }
    inline int                  getBedPosX      () const    { return mBedPosX; }
    inline int                  getBedPosY      () const    { return mBedPosY; }
    inline double               getBedOrientX   () const    { return mBedOrientX; }
    inline double               getBedOrientY   () const    { return mBedOrientY; }

    inline int                  getSightRadius  () const    { return mSightRadius; }

    inline double               getClaimRate    () const    { return mClaimRate; }
    inline double               getClaimRatePerLevel() const{ return mClaimRatePerLevel; }
    inline int                  getMaxGoldCarryable () const    { return mMaxGoldCarryable; }
    inline double               getDigRate      () const    { return mDigRate; }
    inline double               getDigRatePerLevel() const  { return mDigRatePerLevel; }

    inline double               getMinHp        () const    { return mMinHP; }
    inline double               getHpPerLevel   () const    { return mHpPerLevel; }
    inline double               getHpHealPerTurn() const    { return mHpHealPerTurn; }

    inline double               getWakefulnessLostPerTurn() const{ return mWakefulnessLostPerTurn; }
    inline double               getHungerGrowthPerTurn() const { return mHungerGrowthPerTurn; }

    inline double               getMoveSpeedGround  () const    { return mMoveSpeedGround; }
    inline double               getMoveSpeedWater   () const    { return mMoveSpeedWater; }
    inline double               getMoveSpeedLava    () const    { return mMoveSpeedLava; }

    inline double               getGroundSpeedPerLevel() const  { return mGroundSpeedPerLevel; }
    inline double               getWaterSpeedPerLevel () const  { return mWaterSpeedPerLevel; }
    inline double               getLavaSpeedPerLevel  () const  { return mLavaSpeedPerLevel; }

    inline double               getPhysicalDefense() const      { return mPhysicalDefense; }
    inline double               getPhysicalDefPerLevel () const { return mPhysicalDefPerLevel; }
    inline double               getMagicalDefense  () const     { return mMagicalDefense; }
    inline double               getMagicalDefPerLevel () const  { return mMagicalDefPerLevel; }
    inline double               getElementDefense  () const     { return mElementDefense; }
    inline double               getElementDefPerLevel () const  { return mElementDefPerLevel; }

    inline int32_t              getFightIdleDist() const        { return mFightIdleDist; }

    inline const std::string&   getWeaponSpawnL () const        { return mWeaponSpawnL; }
    inline const std::string&   getWeaponSpawnR () const        { return mWeaponSpawnR; }

    int32_t                     getFee (unsigned int level) const;

    inline double               getSleepHeal () const           { return mSleepHeal; }

    inline int32_t              getTurnsStunDropped () const    { return mTurnsStunDropped; }

    double                      getXPNeededWhenLevel(unsigned int level) const;

    //! \brief Returns the creature affinity. The returned vector is assumed to be
    //! sorted so that highest likeness is at first
    inline const std::vector<CreatureRoomAffinity>& getRoomAffinity() const
    { return mRoomAffinity; }

    inline const std::vector<const CreatureSkill*>& getCreatureSkills() const
    { return mCreatureSkills; }

    inline const std::vector<const CreatureBehaviour*>& getCreatureBehaviours() const
    { return mCreatureBehaviours; }

    const CreatureRoomAffinity& getRoomAffinity(RoomType roomType) const;

    inline const std::string& getMoodModifierName() const
    { return mMoodModifierName; }

    inline const std::string& getSoundFamilyPickup() const
    { return mSoundFamilyPickup; }
    inline const std::string& getSoundFamilyDrop() const
    { return mSoundFamilyDrop; }
    inline const std::string& getSoundFamilyAttack() const
    { return mSoundFamilyAttack; }
    inline const std::string& getSoundFamilyDie() const
    { return mSoundFamilyDie; }
    inline const std::string& getSoundFamilySlap() const
    { return mSoundFamilySlap; }

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

    //! \brief Position and orientation of the creature when it goes to sleep
    int mBedPosX;
    int mBedPosY;
    double mBedOrientX;
    double mBedOrientY;

    //! \brief The scale the mesh is displayed (bigger = stronger)
    Ogre::Vector3 mScale;

    //! \brief The inner radius where the creature sees everything
    int mSightRadius;

    //! \brief Maximum gold amount a worker can carry before having to deposit it into a treasury
    int mMaxGoldCarryable;

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

    //! \brief How much wakefulness is lost per turn.
    double mWakefulnessLostPerTurn;

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

    //! \brief Defense stats (without equipment)
    double mPhysicalDefense;
    double mPhysicalDefPerLevel;
    double mMagicalDefense;
    double mMagicalDefPerLevel;
    double mElementDefense;
    double mElementDefPerLevel;

    //! \brief Distance from the nearest enemy creature that the creature will try to let when no skill can be used
    int32_t mFightIdleDist;

    //! \brief The base fee for this creature.
    int32_t mFeeBase;
    int32_t mFeePerLevel;

    //! \brief How many HP the creature will get per turn while sleeping
    double mSleepHeal;

    //! \brief Number of turns the creature will stay stunned after being dropped
    int32_t mTurnsStunDropped;

    //! \brief Weapons a creature should spawn with ("none" if no weapon)
    std::string mWeaponSpawnL;
    std::string mWeaponSpawnR;

    //! \brief The XP table, used to know how XP is needed to reach the next level.
    //! \note The creature starting at level 1, it can only change its level MAX_LEVEL - 1 times.
    std::vector<double> mXPTable;

    //! \brief Skills the creature can use
    std::vector<const CreatureSkill*> mCreatureSkills;

    //! \brief Creature specific behaviours
    std::vector<const CreatureBehaviour*> mCreatureBehaviours;

    //! \brief The rooms the creature should choose according to availability
    std::vector<CreatureRoomAffinity> mRoomAffinity;

    //! \brief The rooms the creature mood modifier that should be used to compute
    //! creature mood
    std::string mMoodModifierName;

    //! \brief Sound family used by the creature
    std::string mSoundFamilyPickup;
    std::string mSoundFamilyDrop;
    std::string mSoundFamilyAttack;
    std::string mSoundFamilyDie;
    std::string mSoundFamilySlap;

    //! \brief Loads the creature XP values for the given definition.
    static void loadXPTable(std::stringstream& defFile, CreatureDefinition* creatureDef);

    //! \brief Loads the creature skills for the given definition.
    static void loadCreatureSkills(std::stringstream& defFile, CreatureDefinition* creatureDef);

    //! \brief Loads the creature specific behaviours for the given definition.
    static void loadCreatureBehaviours(std::stringstream& defFile, CreatureDefinition* creatureDef);

    //! \brief Loads the creature room affinity for the given definition.
    static void loadRoomAffinity(std::stringstream& defFile, CreatureDefinition* creatureDef);
};

#endif // CREATUREDEFINITION_H
