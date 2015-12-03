/*!
 * \file   Creature.h
 * \brief  Creature class
 *
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

#ifndef CREATURE_H
#define CREATURE_H

#include "entities/MovableGameEntity.h"

#include <OgreVector2.h>
#include <OgreVector3.h>
#include <CEGUI/EventArgs.h>

#include <memory>
#include <string>

class Building;
class Creature;
class CreatureAction;
class CreatureEffect;
class CreatureDefinition;
class CreatureOverlayStatus;
class CreatureSkill;
class GameMap;
class ODPacket;
class Room;
class Weapon;

enum class CreatureActionType;
enum class CreatureMoodLevel;
enum class SkillType;

namespace CEGUI
{
class Window;
}

namespace Ogre
{
class ParticleSystem;
}

enum class CreatureSound
{
    Pickup,
    Drop,
    Attack,
    Die,
    Slap,
    Dig
};

class CreatureSkillData
{
public:
    CreatureSkillData(const CreatureSkill* skill, uint32_t cooldown, uint32_t warmup) :
        mSkill(skill),
        mCooldown(cooldown),
        mWarmup(warmup)
    {
    }

    const CreatureSkill* mSkill;
    uint32_t mCooldown;
    uint32_t mWarmup;
};

//! Class used on server side to link creature effects (spells, slap, ...) with particle effects
class CreatureParticuleEffect : public EntityParticleEffect
{
public:
    CreatureParticuleEffect(const std::string& name, const std::string& script, uint32_t nbTurnsEffect,
            CreatureEffect* effect) :
        EntityParticleEffect(name, script, nbTurnsEffect),
        mEffect(effect)
    {}

    virtual ~CreatureParticuleEffect();

    virtual EntityParticleEffectType getEntityParticleEffectType() override
    { return EntityParticleEffectType::creature; }

    CreatureEffect* mEffect;
};

//! Class used on client side to display the particle effects on the creature
class CreatureParticuleEffectClient : public EntityParticleEffect
{
public:
    CreatureParticuleEffectClient(const std::string& name, const std::string& script, uint32_t nbTurnsEffect) :
        EntityParticleEffect(name, script, nbTurnsEffect)
    {}

    virtual EntityParticleEffectType getEntityParticleEffectType() override
    { return EntityParticleEffectType::creature; }
};

/*! \class Creature Creature.h
 *  \brief Position, status, and AI state for a single game creature.
 *
 *  The creature class is the place where an individual creature's state is
 *  stored and manipulated.  The creature class is also used to store creature
 *  class descriptions, since a class decription is really just a subset of the
 *  overall creature information.  This is not really an optimal design and
 *  will probably be refined later but it works fine for now and the code
 *  affected by this change is relatively limited.
 */
class Creature: public MovableGameEntity
{
    friend class ODClient;
public:
    static const int32_t NB_TURNS_BEFORE_CHECKING_TASK;

    //! \brief Constructor for creatures. It generates an unique name
    Creature(GameMap* gameMap, bool isOnServerMap, const CreatureDefinition* definition, Seat* seat, Ogre::Vector3 position = Ogre::Vector3(0.0f,0.0f,0.0f));
    virtual ~Creature();

    static const uint32_t NB_OVERLAY_HEALTH_VALUES;

    virtual GameEntityType getObjectType() const
    { return GameEntityType::creature; }

    virtual void addToGameMap();
    virtual void removeFromGameMap() override;

    bool canDisplayStatsWindow(Seat* seat) override
    { return true; }
    void createStatsWindow();
    void destroyStatsWindow();
    bool CloseStatsWindow(const CEGUI::EventArgs& /*e*/);
    void updateStatsWindow(const std::string& txt);
    std::string getStatsText();

    //! \brief Get the level of the object
    inline unsigned int getLevel() const
    { return mLevel; }

    inline double getHP(Tile *tile) const override
    { return mHp; }

    bool isAlive() const;

    //! \brief Gets the maximum HP the creature can have currently
    inline double getMaxHp() const
    { return mMaxHP; }

    //! \brief Gets the maximum HP the creature can have currently
    inline double getHP() const
    { return mHp; }

    //! \brief Gets the current dig rate
    inline double getDigRate() const
    { return mDigRate; }

    //! \brief Gets the current claim rate
    inline double getClaimRate() const
    { return mClaimRate; }

    //! \brief Gets pointer to the Weapon in left hand
    inline const Weapon* getWeaponL() const
    { return mWeaponL; }

    //! \brief Gets pointer to the Weapon in right hand
    inline const Weapon* getWeaponR() const
    { return mWeaponR; }

    //! \brief Pointer to the creatures home tile, where its bed is located
    inline Tile* getHomeTile() const
    { return mHomeTile; }

    //! \brief Pointer to the creature type specification
    inline const CreatureDefinition* getDefinition() const
    { return mDefinition; }

    inline CreatureMoodLevel getMoodValue() const
    { return mMoodValue; }

    inline int32_t getNbTurnFurious() const
    { return mNbTurnFurious; }

    void setPosition(const Ogre::Vector3& v) override;

    //! \brief Gets the move speed on the current tile.
    virtual double getMoveSpeed() const;

    //! \brief Gets the move speed on the current tile.
    double getMoveSpeed(Tile* tile) const;

    //! \brief Gets the creature depending the terrain type.
    inline double getMoveSpeedGround() const
    { return mGroundSpeed; }
    inline double getMoveSpeedWater() const
    { return mWaterSpeed; }
    inline double getMoveSpeedLava() const
    { return mLavaSpeed; }

    //! \brief Updates the entity path, movement, and direction, and creature attack time
    //! \param timeSinceLastFrame the elapsed time since last displayed frame in seconds.
    virtual void update(Ogre::Real timeSinceLastFrame);

    bool setDestination(Tile* tile);

    //! \brief Picks a destination far away in the visible tiles and goes there
    //! Returns true if a valid Tile was found. The creature will go there
    //! Returns false if no reachable Tile was found
    bool wanderRandomly(const std::string& animationState);

    void setHP(double nHP);

    void heal(double hp);

    inline void setHomeTile(Tile* ht)
    { mHomeTile = ht; }

    //! \brief Set the level of the creature
    void setLevel(unsigned int level);

    //! \brief Called when the creature dies or is KO to death. triggers death animation, stops
    //! the creature job and drops what it is carrying
    void dropCarriedEquipment();

    /*! \brief The main AI routine which decides what the creature will do and carries out that action.
     *
     * The doUpkeep routine is the heart of the Creature AI subsystem.  The other,
     * higher level, functions such as GameMap::doUpkeep() ultimately just call this
     * function to make the creatures act.
     *
     * The function begins in a pre-cognition phase which prepares the creature's
     * brain state for decision making.  This involves generating lists of known
     * about creatures, either through sight, hearing, keeper knowledge, etc, as
     * well as some other bookkeeping stuff.
     *
     * Next the function enters the cognition phase where the creature's current
     * state is examined and a decision is made about what to do.  The state of the
     * creature is in the form of a queue, which is really used more like a stack.
     * If the queue is empty,  the 'idle' action is used. It acts as a "last resort"
     * for when the creature completely runs out of things to do. Other actions such
     * as 'walkToTile' or 'job' can be pushed to  determine the what the creature
     * will do. Once the action is finished (because the creature is tired or there is
     * nothing related to do), the action will be popped and the previous one will be
     * used (if any - idle otherwise). This allows actions to be carried out recursively,
     * i.e. if a creature is trying to dig a tile and it is not nearby it can begin
     * walking toward the tile as a new action, and when it arrives at the tile it will
     * revert to the 'digTile' action.
     */
    void doUpkeep();

    //! \brief Computes the visible tiles and tags them to know which are visible
    void computeVisibleTiles();

    virtual bool isAttackable(Tile* tile, Seat* seat) const;

    double getPhysicalDefense() const;
    double getMagicalDefense() const;
    double getElementDefense() const;

    //! \brief Check whether a creature has earned one level. If yes, handle leveling it up
    void checkLevelUp();

    //! \brief Updates the lists of tiles within sight radius.
    //! And the tiles the creature can "see" (removing the ones behind walls).
    void updateTilesInSight();

    //! \brief Loops over the visibleTiles and adds all enemy creatures in each tile to a list which it returns.
    std::vector<GameEntity*> getVisibleEnemyObjects();

    //! \brief Loops over objectsToCheck and returns a vector containing all the ones which can be reached via a valid path.
    std::vector<GameEntity*> getReachableAttackableObjects(const std::vector<GameEntity*> &objectsToCheck);

    //! \brief Loops over objectsToCheck and returns a vector containing all the creatures in the list.
    std::vector<GameEntity*> getCreaturesFromList(const std::vector<GameEntity*> &objectsToCheck, bool workersOnly);

    //! \brief Loops over the visibleTiles and adds all allied creatures in each tile to a list which it returns.
    std::vector<GameEntity*> getVisibleAlliedObjects();

    //! \brief Loops over the visibleTiles and returns any creatures in those tiles
    //! allied with the given seat (or if invert is true, does not allied)
    std::vector<GameEntity*> getVisibleForce(Seat* seat, bool invert);

    //! \brief Conform: GameEntity functions handling covered tiles
    std::vector<Tile*> getCoveredTiles();
    Tile* getCoveredTile(int index);
    uint32_t numCoveredTiles() const override;

    //! \brief Conform: AttackableObject - Deducts a given amount of HP from this creature.
    double takeDamage(GameEntity* attacker, double absoluteDamage, double physicalDamage, double magicalDamage, double elementDamage,
        Tile *tileTakingDamage, bool ko) override;

    //! \brief Conform: AttackableObject - Adds experience to this creature.
    void receiveExp(double experience);

    //! \brief performs the given attack on the given target
    void useAttack(CreatureSkillData& skillData, GameEntity& entityAttack,
        Tile& tileAttack, bool ko);

    //! \brief Returns true if the given action is queued in the action list. False otherwise
    bool isActionInList(CreatureActionType action) const;

    //! \brief Clears the action queue, except for the Idle action at the end.
    void clearActionQueue();

    //! \brief Computes the tiles visible for the creature and sends a message to the clients to mark those tiles. This function
    //! should be called on server side only
    void computeVisualDebugEntities();

    //! \brief Displays a mesh on all of the tiles in the list. This function should be called on client side only
    void refreshVisualDebugEntities(const std::vector<Tile*>& tiles);

    //! \brief Sends a message to the clients to stop displaying the tiles this creature sees
    void stopComputeVisualDebugEntities();

    //! \brief Destroy the meshes created by createVisualDebuggingEntities(). This function should be called on client side only
    void destroyVisualDebugEntities();

    //! \brief An accessor to return whether or not the creature has OGRE entities for its visual debugging entities.
    inline bool getHasVisualDebuggingEntities() const
    { return mHasVisualDebuggingEntities; }

    virtual const Ogre::Vector3& getScale() const
    { return mScale; }

    inline CreatureOverlayStatus* getOverlayStatus() const
    { return mOverlayStatus; }

    inline void setOverlayStatus(CreatureOverlayStatus* overlayStatus)
    { mOverlayStatus = overlayStatus; }

    //! \brief Get the text format of creatures in level files (already spawned at startup).
    //! \returns A string describing the IO format the creatures need to have in file.
    static std::string getCreatureStreamFormat();

    //! \brief Get a creature from a stream
    static Creature* getCreatureFromStream(GameMap* gameMap, std::istream& is);
    //! \brief Get a creature from a packet
    static Creature* getCreatureFromPacket(GameMap* gameMap, ODPacket& is);
    //! \brief Checks if the creature can be picked up. If yes, this function does the needed
    //! to prepare for the pickup (removing creature from GameMap, changing states, ...).
    //! Returns true if the creature can be picked up
    bool tryPickup(Seat* seat) override;
    void pickup() override;
    bool tryDrop(Seat* seat, Tile* tile) override;
    void drop(const Ogre::Vector3& v) override;

    //! \brief sets the speed modifier (coef)
    void setMoveSpeedModifier(double modifier);
    void clearMoveSpeedModifier();

    //! \brief sets the defense modifier
    void setDefenseModifier(double phy, double mag, double ele);
    void clearDefenseModifier();

    //! \brief sets the strength modifier (coef)
    void setStrengthModifier(double modifier);
    void clearStrengthModifier();

    virtual double getAnimationSpeedFactor() const override
    { return mSpeedModifier; }

    inline void jobDone(double val)
    {
        mWakefulness -= val;
        if(mWakefulness < 0.0)
            mWakefulness = 0.0;
    }
    inline bool decreaseJobCooldown()
    {
        if(mJobCooldown <= 0)
            return true;

        --mJobCooldown;
        return false;
    }
    void setJobCooldown(int val);

    inline void foodEaten(double val)
    {
        mHunger -= val;
        if(mHunger < 0.0)
            mHunger = 0.0;
    }

    //! \brief Tells whether the creature can go through the given tile.
    bool canGoThroughTile(Tile* tile) const;

    virtual EntityCarryType getEntityCarryType(Creature* carrier);
    virtual void notifyEntityCarryOn(Creature* carrier);
    virtual void notifyEntityCarryOff(const Ogre::Vector3& position);
    bool canBeCarriedToBuilding(const Building* building) const;

    bool canSlap(Seat* seat);
    void slap();

    void fireCreatureSound(CreatureSound sound);

    void itsPayDay();

    inline const std::vector<Tile*>& getVisibleTiles() const
    { return mVisibleTiles; }

    inline const std::vector<Tile*>& getTilesWithinSightRadius() const
    { return mTilesWithinSightRadius; }

    inline const std::vector<GameEntity*>& getVisibleEnemyObjects() const
    { return mVisibleEnemyObjects; }

    inline const std::vector<GameEntity*>& getVisibleAlliedObjects() const
    { return mVisibleAlliedObjects; }

    inline const std::vector<GameEntity*>& getReachableAlliedObjects() const
    { return mReachableAlliedObjects; }

    inline const std::vector<std::unique_ptr<CreatureAction>>& getActions() const
    { return mActions; }

    inline double getWakefulness() const
    { return mWakefulness; }

    inline void increaseWakefulness(double value)
    {
        mWakefulness += value;
        if(mWakefulness > 100.0)
            mWakefulness = 100.0;
    }

    void decreaseWakefulness(double value);

    inline double getHunger() const
    { return mHunger; }

    inline int32_t getGoldFee() const
    { return mGoldFee; }

    void decreaseGoldFee(int32_t value)
    {
        mGoldFee -= value;
        if(mGoldFee < 0)
            mGoldFee = 0;
    }

    inline int32_t getGoldCarried() const
    { return mGoldCarried; }

    inline void resetGoldCarried()
    { mGoldCarried = 0; }

    inline void addGoldCarried(int32_t gold)
    { mGoldCarried += gold; }

    inline uint32_t getOverlayHealthValue() const
    { return mOverlayHealthValue; }

    inline uint32_t getOverlayMoodValue() const
    { return mOverlayMoodValue; }

    inline int32_t getNbTurnsWithoutBattle() const
    { return mNbTurnsWithoutBattle; }

    inline void setNbTurnsWithoutBattle(int32_t nbTurnsWithoutBattle)
    { mNbTurnsWithoutBattle = nbTurnsWithoutBattle; }

    inline GameEntity* getCarriedEntity() const
    { return mCarriedEntity; }

    void carryEntity(GameEntity* carriedEntity);

    void releaseCarriedEntity();

    bool hasActionBeenTried(CreatureActionType actionType) const;

    void pushAction(std::unique_ptr<CreatureAction>&& action);
    void popAction();

    void fireCreatureRefreshIfNeeded();

    //! \brief Load creature definition according to @mDefinitionString
    //! This should be called before createMesh. This was formerly in CreateMesh
    //! but is now split out since this is needed on the server, while the mesh isn't.
    //! This is normally called by the constructor, but creatures loaded from the map files
    //! use a different constructor, and this is then called by the gameMap when other details have been loaded.
    void setupDefinition(GameMap& gameMap, const CreatureDefinition& defaultWorkerCreatureDefinition);

    //! Called on server side to add an effect (spell, slap, ...) to this creature
    void addCreatureEffect(CreatureEffect* effect);

    //!\brief Returns true if the creature is forced to work for some reason (like an effect)
    bool isForcedToWork() const;

    virtual void correctEntityMovePosition(Ogre::Vector3& position) override;

    //! \brief Called on client side and server side. true if the creature is hurt and false
    //! if at max HP or above
    bool isHurt() const;

    //! \brief Called on client side and server side. true if the creature is ko and false if not
    bool isKo() const;
    bool isKoDeath() const;
    bool isKoTemp() const;

    //! \brief Called on client side and server side. true if the creature is in prison and false if not
    bool isInPrison() const;

    //! Checks if the creature current walk path is still valid. This will be called if tiles passability changes (for
    //! example if a door is closed)
    void checkWalkPathValid();

    bool isTired() const;

    bool isHungry() const;

    void releasedInBed();

    void setSeatPrison(Seat* seat);

    inline Seat* getSeatPrison() const
    { return mSeatPrison; }

    virtual bool isDangerous(const Creature* creature, int distance) const override;

    virtual void clientUpkeep() override;

    virtual void exportToPacketForUpdate(ODPacket& os, const Seat* seat) const override;
    virtual void updateFromPacket(ODPacket& is) override;

    //! \brief Called when an angry creature wants to attack a natural enemy
    void engageAlliedNaturalEnemy(Creature& attacker);

    inline double getModifierStrength() const
    { return mModifierStrength; }

    void fightInArena(Creature& opponent);

    void fight();

    void fightCreature(Creature& creature);

    void flee();

    void sleep();

    void leaveDungeon();

    bool isWarmup() const;

    void computeCreatureOverlayHealthValue();

    //! \brief Search within listObjects the closest attackable one.
    //! If a target is found and can be attacked, returns true and
    //! attackedEntity, attackedTile will be set to the target closest tile and positionTile will
    //! be set to the best spot.
    //! If the creature should flee (ranged units attacked by melee), true is returned, positionTile is
    //! set to the tile where it should flee and attackedEntity = nullptr and attackedTile = nullptr
    //! If no suitable target is found, returns false
    // TODO: check if we can move it in the creature action
    bool searchBestTargetInList(const std::vector<GameEntity*>& listObjects, GameEntity*& attackedEntity, Tile*& attackedTile, Tile*& positionTile, CreatureSkillData*& creatureSkillData);

    //! \brief returns true if the creature needs to eat. forced should be true if
    //! the creature is forced to eat (ie it has been dropped on a hatchery) and
    //! false otherwise
    bool needsToEat(bool forced) const;

protected:
    virtual void exportToPacket(ODPacket& os, const Seat* seat) const override;
    virtual void importFromPacket(ODPacket& is) override;
    virtual void exportToStream(std::ostream& os) const override;
    virtual bool importFromStream(std::istream& is) override;

    virtual void createMeshLocal();
    virtual void destroyMeshLocal();
    virtual void fireAddEntity(Seat* seat, bool async);
    virtual void fireRemoveEntity(Seat* seat);
private:
    enum ForceAction
    {
        forcedActionNone,
        forcedActionSearchAction,
        forcedActionDigTile,
        forcedActionClaimTile,
        forcedActionClaimWallTile
    };

    void createMeshWeapons();
    void destroyMeshWeapons();

    //! \brief Constructor for sending creatures through network. It should not be used in game.
    Creature(GameMap* gameMap, bool isOnServerMap);

    //! \brief Natural physical and magical attack and defense (without equipment)
    double mPhysicalDefense;
    double mMagicalDefense;
    double mElementDefense;

    //! \brief Strength modifiers (can be changed by effects like spells)
    double mModifierStrength;

    //! \brief The weapon the creature is holding in its left hand or nullptr if none. It will be set by a pointer
    //! managed by the game map and thus, should not be deleted by the creature class
    const Weapon* mWeaponL;

    //! \brief The weapon the creature is holding in its right hand or nullptr if none. It will be set by a pointer
    //! managed by the game map and thus, should not be deleted by the creature class
    const Weapon* mWeaponR;

    //! \brief The creatures home tile (where its bed is located)
    Tile *mHomeTile;

    //! Class name of the creature. The CreatureDefinition will be set from this name
    //! when the creature will be initialized
    std::string     mDefinitionString;
    //! \brief Pointer to the struct holding the general type of the creature with its values
    const CreatureDefinition* mDefinition;

    bool            mHasVisualDebuggingEntities;
    double          mWakefulness;
    double          mHunger;

    //! \brief The level of the creature
    unsigned int    mLevel;

    //! \brief The creature stats
    std::string     mHpString;
    double          mHp;
    double          mMaxHP;
    double          mExp;
    double          mGroundSpeed;
    double          mWaterSpeed;
    double          mLavaSpeed;

    //! \brief Workers only
    double          mDigRate;
    double          mClaimRate;

    //! \brief Counter to let the creature stay some turns after its death
    unsigned int    mDeathCounter;
    int             mJobCooldown;

    //! \brief At pay day, mGoldFee will be set to the creature fee and decreased when the creature gets gold
    int32_t         mGoldFee;
    //! \brief Gold carried by the creature that will be dropped if it gets killed
    int32_t         mGoldCarried;

    //! Skill type that will be dropped when the creature dies
    SkillType       mSkillTypeDropDeath;

    //! Weapon that will be dropped when the creature dies
    std::string     mWeaponDropDeath;

    CEGUI::Window*  mStatsWindow;
    int32_t         mNbTurnsWithoutBattle;

    //! \brief Every tiles within the creature sight radius, used for common actions.
    std::vector<Tile*>              mTilesWithinSightRadius;

    //! \brief Only visible tiles, not hidden for other tiles,
    //! used for actions linked to enemies.
    std::vector<Tile*>              mVisibleTiles;

    std::vector<GameEntity*>        mVisibleEnemyObjects;
    std::vector<GameEntity*>        mVisibleAlliedObjects;
    std::vector<GameEntity*>        mReachableAlliedObjects;
    std::vector<std::unique_ptr<CreatureAction>>    mActions;
    std::vector<Tile*>              mVisualDebugEntityTiles;

    //! \brief Contains the actions that have already been tested to avoid trying several times same action
    std::vector<CreatureActionType> mActionTry;

    GameEntity*                     mCarriedEntity;

    Ogre::Vector3                   mScale;

    //! \brief The mood do not have to be computed at every turn. This cooldown will
    //! count how many turns the creature should wait before computing it
    int32_t                         mMoodCooldownTurns;

    //! \brief Mood value. Depending on this value, the creature will be in bad mood and
    //! might attack allied creatures or refuse to work or to go to combat
    CreatureMoodLevel               mMoodValue;
    //! \brief Mood points. Computed by the creature MoodModifiers. It is promoted to class variable for debug purposes and
    //! should not be used to check mood. If the mood is to be tested, mMoodValue should be used
    int32_t                         mMoodPoints;

    //! \brief Counts turns the creature is furious. If it stays like this for too long, it will become rogue
    int32_t                         mNbTurnFurious;

    //! \brief Represents the life value displayed on client side. We do not notify each HP change
    //! to avoid too many communication. But when mOverlayHealthValue changes, we will
    uint32_t                        mOverlayHealthValue;

    //! \brief Represents the mood of the creature. It is a bit array
    uint32_t                        mOverlayMoodValue;

    //! Used by the renderer to save this entity's overlay. It is its responsibility
    //! to allocate/delete this pointer
    CreatureOverlayStatus*          mOverlayStatus;

    //! Used on server side to indicate if a change that needs to be notified to the clients happened (like changing
    //! level or HP)
    bool                            mNeedFireRefresh;

    //! \brief Used on client side. When a creature is dropped, this cooldown will be set to a value > 0
    //! and decreased at each turn. Until it is > 0, the creature cannot be slapped. That's to avoid
    //! slapping creatures to death when dropping many.
    //! Note that this is done on client side and not checked on server side because it is just to be
    //! player friendly
    uint32_t                        mDropCooldown;

    //! \brief Speed modifier that will apply to both animation speed and move speed. If
    //! 1.0, it will be default speed
    double                          mSpeedModifier;

    //! \brief Counter when the creature is KO. If = 0, the creature is not KO.
    //! If > 0, the creature is temporary KO (after being drooped for example). Each
    //! turn, the counter will decrease and the creature will wake up when the counter
    //! reaches 0.
    //! If < 0, the creature is KO to death. The counter will increase each turn and
    //! if it reaches 0, the creature will die.
    //! While KO to death, if a kobold carries the creature to its bed, the counter will
    //! stop during the travel (and reset to 0 when the creature is dropped in its bed).
    int32_t                         mKoTurnCounter;

    //! \brief If nullptr, the creature is not in prison. If not, it is in the prison of
    //! the given seat
    Seat*                           mSeatPrison;

    //! \brief Skills the creature can use
    std::vector<CreatureSkillData> mSkillData;

    //! \brief A sub-function called by doTurn()
    //! This one checks if there is something prioritary to do (like fighting). If it is the case,
    //! it should empty the action list before adding what to do.
    void decidePrioritaryAction();

    //! \brief A sub-function called by doTurn()
    //! This functions will handle the creature idle action logic.
    //! \return true when another action should handled after that one.
    bool handleIdleAction();

    //! \brief Restores the creature's stats according to its current level
    void buildStats();

    void updateScale();

    void increaseHunger(double value);

    void computeMood();

    void computeCreatureOverlayMoodValue();

    std::vector<Tile*> getAccessibleVisibleTiles(Tile* center, int radius) const;
};

#endif // CREATURE_H
