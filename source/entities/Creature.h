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

#include "entities/CreatureAction.h"
#include "entities/MovableGameEntity.h"

#include <OgreVector2.h>
#include <OgreVector3.h>
#include <CEGUI/EventArgs.h>

#include <string>
#include <deque>

class Creature;
class CreatureEffect;
class CreatureDefinition;
class CreatureOverlayStatus;
class GameMap;
class ODPacket;
class Room;
class Weapon;

enum class CreatureMoodLevel;
enum class ResearchType;
enum class CreatureSoundType;

namespace CEGUI
{
class Window;
}

namespace Ogre
{
class ParticleSystem;
}

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

    CreatureEffect* mEffect;
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
    //! \brief Constructor for creatures. It generates an unique name
    Creature(GameMap* gameMap, const CreatureDefinition* definition, Seat* seat, Ogre::Vector3 position = Ogre::Vector3(0.0f,0.0f,0.0f));
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

    inline double getHP(Tile *tile) const
    { return getHP(); }

    double getHP() const;

    //! \brief Gets the maximum HP the creature can have currently
    inline double getMaxHp()const
    { return mMaxHP; }

    //! \brief Gets the current dig rate
    inline double getDigRate() const
    { return mDigRate; }

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

    /*! \brief Changes the creature's position to a new position.
     *  This is an overloaded function which just calls MovableGameEntity::setPosition.
     */
    void setPosition(const Ogre::Vector3& v, bool isMove);

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

    void drop(const Ogre::Vector3& v);

    void setHP(double nHP);

    inline void setHomeTile(Tile* ht)
    { mHomeTile = ht; }

    //! \brief Set the level of the creature
    void setLevel(unsigned int level);

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
     * At the beginning of the game the 'idle' action is pushed onto each
     * creature's actionQueue, this action is never removed from the tail end of
     * the queue and acts as a "last resort" for when the creature completely runs
     * out of things to do.  Other actions such as 'walkToTile' or 'attackObject'
     * are then pushed onto the front of the queue and will determine the
     * creature's future behavior.  When actions are complete they are popped off
     * the front of the action queue, causing the creature to revert back into the
     * state it was in when the actions was placed onto the queue.  This allows
     * actions to be carried out recursively, i.e. if a creature is trying to dig a
     * tile and it is not nearby it can begin walking toward the tile as a new
     * action, and when it arrives at the tile it will revert to the 'digTile'
     * action.
     *
     * In the future there should also be a post-cognition phase to do any
     * additional checks after it tries to move, etc.
     */
    void doUpkeep();

    //! \brief Computes the visible tiles and tags them to know which are visible
    void computeVisibleTiles();

    virtual bool isAttackable(Tile* tile, Seat* seat) const;

    double getPhysicalDamage(double range);
    double getPhysicalDefense() const;
    double getMagicalDamage(double range);
    double getMagicalDefense() const;

    //! \brief Returns the currently best attack range of the creature.
    //! \note Depends also on its equipment.
    double getBestAttackRange() const;

    //! \brief Check whether a creature has earned one level.
    bool checkLevelUp();

    //! \brief Refreshes current creature with the giver ODPacket. This function works
    //! with fireCreatureRefreshIfNeeded. The data carried in the ODPacket is the needed data
    //! on client side
    void refreshCreature(ODPacket& packet);

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

    //! \brief Loops over the visibleTiles and updates any which are marked for digging, and are reachable.
    void updateVisibleMarkedTiles();

    //! \brief Loops over the visibleTiles and adds any which are claimable walls.
    std::vector<Tile*> getVisibleClaimableWallTiles();

    //! \brief Loops over the visibleTiles and returns any creatures in those tiles
    //! allied with the given seat (or if invert is true, does not allied)
    std::vector<GameEntity*> getVisibleForce(Seat* seat, bool invert);

    //! \brief Conform: GameEntity functions handling covered tiles
    std::vector<Tile*> getCoveredTiles();
    Tile* getCoveredTile(int index);
    uint32_t numCoveredTiles();

    //! \brief Conform: AttackableObject - Deducts a given amount of HP from this creature.
    double takeDamage(GameEntity* attacker, double physicalDamage, double magicalDamage, Tile* tileTakingDamage);

    //! \brief Conform: AttackableObject - Adds experience to this creature.
    void receiveExp(double experience);

    //! \brief Returns true if the given action is queued in the action list. False otherwise
    bool isActionInList(CreatureActionType action);

    inline const std::deque<CreatureAction>& getActionQueue()
    { return mActionQueue; }

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
    // NOTE: This function doesn't actually get a creature from stream,
    // it simply creates a creature object.
    static Creature* getCreatureFromStream(GameMap* gameMap, std::istream& is);
    //! \brief Get a creature from a packet
    // NOTE: This function doesn't actually get a creature the packet,
    // it simply creates a creature object.
    static Creature* getCreatureFromPacket(GameMap* gameMap, ODPacket& is);
    virtual void exportToPacket(ODPacket& os) const override;
    virtual void importFromPacket(ODPacket& is) override;
    virtual void exportToStream(std::ostream& os) const override;
    virtual void importFromStream(std::istream& is) override;

    //! \brief Checks if the creature can be picked up. If yes, this function does the needed
    //! to prepare for the pickup (removing creature from GameMap, changing states, ...).
    //! Returns true if the creature can be picked up
    bool tryPickup(Seat* seat);
    void pickup();
    bool tryDrop(Seat* seat, Tile* tile);

    inline void jobDone(double val)
    {
        mAwakeness -= val;
        if(mAwakeness < 0.0)
            mAwakeness = 0.0;
    }
    inline void setJobCooldown(int val) { mJobCooldown = val; }
    inline int getJobCooldown() { return mJobCooldown; }
    inline void foodEaten(double val)
    {
        mHunger -= val;
        if(mHunger < 0.0)
            mHunger = 0.0;
    }

    bool isJobRoom(Room* room);
    bool isEatRoom(Room* room);

    //! \brief Allows to change the room the creature is using (when room absorption for example). Beware, the room
    //! change logic has to be handled elsewhere
    inline void changeJobRoom(Room* newRoom)
    { mJobRoom = newRoom; }

    //! \brief Allows to change the room the creature is using (when room absorption for example). Beware, the room
    //! change logic has to be handled elsewhere
    inline void changeEatRoom(Room* newRoom)
    { mEatRoom = newRoom; }

    //! \brief Makes the creature stop its job (releases the job room)
    void stopJob();

    //! \brief Makes the creature stop eating (releases the hatchery)
    void stopEating();

    //! \brief Tells whether the creature can go through the given tile.
    bool canGoThroughTile(const Tile* tile) const;

    virtual EntityCarryType getEntityCarryType();
    virtual void notifyEntityCarryOn(Creature* carrier);
    virtual void notifyEntityCarryOff(const Ogre::Vector3& position);

    bool canSlap(Seat* seat);
    void slap();

    void fireCreatureSound(CreatureSoundType sound);

    void itsPayDay();

    inline const std::vector<Tile*>& getVisibleTiles() const
    { return mVisibleTiles; }

    inline double getAwakeness() const
    { return mAwakeness; }

    inline double getHunger() const
    { return mHunger; }

    inline int32_t getGoldFee() const
    { return mGoldFee; }

    inline uint32_t getOverlayHealthValue() const
    { return mOverlayHealthValue; }

    inline uint32_t getOverlayMoodValue() const
    { return mOverlayMoodValue; }

    inline int32_t getNbTurnsWithoutBattle() const
    { return mNbTurnsWithoutBattle; }

    void fireCreatureRefreshIfNeeded();

    //! \brief Load creature definition according to @mDefinitionString
    //! This should be called before createMesh. This was formerly in CreateMesh
    //! but is now split out since this is needed on the server, while the mesh isn't.
    //! This is normally called by the constructor, but creatures loaded from the map files
    //! use a different constructor, and this is then called by the gameMap when other details have been loaded.
    void setupDefinition(GameMap& gameMap, const CreatureDefinition& defaultWorkerCreatureDefinition);

    //! Called on server side to add an effect (spell, slap, ...) to this creature
    void addCreatureEffect(CreatureEffect* effect);

    void addDestination(Ogre::Real x, Ogre::Real y, Ogre::Real z = 0.0f) override;

    //! Called on client side and server side. true if the creature is hurt and false
    //! if at max HP or above
    bool isHurt() const;

    //! Checks if the creature current walk path is still valid. This will be called if tiles passability changes (for
    //! example if a door is closed)
    void checkWalkPathValid();

protected:
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
    Creature(GameMap* gameMap);

    //! \brief Natural physical and magical attack and defense (without equipment)
    double mPhysicalAttack;
    double mMagicalAttack;
    double mPhysicalDefense;
    double mMagicalDefense;
    double mWeaponlessAtkRange;

    //! \brief The time left before being to draw an attack in seconds
    double mAttackWarmupTime;

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
    double          mAwakeness;
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
    int             mEatCooldown;

    //! \brief At pay day, mGoldFee will be set to the creature fee and decreased when the creature gets gold
    int32_t         mGoldFee;
    //! \brief Gold carried by the creature that will be dropped if it gets killed
    int32_t         mGoldCarried;

    //! Research type that will be dropped when the creature dies
    ResearchType    mResearchTypeDropDeath;

    //! Weapon that will be dropped when the creature dies
    std::string     mWeaponDropDeath;

    Room*           mJobRoom;
    Room*           mEatRoom;
    CEGUI::Window*  mStatsWindow;
    int32_t         mNbTurnsWithoutBattle;

    //! \brief Every tiles within the creature sight radius, used for common actions.
    std::vector<Tile*>              mTilesWithinSightRadius;

    //! \brief Only visible tiles, not hidden for other tiles,
    //! used for actions linked to enemies.
    std::vector<Tile*>              mVisibleTiles;

    //! \brief Visible, reachable, marked and diggable tiles.
    std::vector<Tile*>              mVisibleMarkedTiles;

    std::vector<GameEntity*>        mVisibleEnemyObjects;
    std::vector<GameEntity*>        mReachableEnemyObjects;
    std::vector<GameEntity*>        mReachableEnemyCreatures;
    std::vector<GameEntity*>        mVisibleAlliedObjects;
    std::vector<GameEntity*>        mReachableAlliedObjects;
    std::deque<CreatureAction>      mActionQueue;
    std::vector<Tile*>              mVisualDebugEntityTiles;

    //! \brief Contains the actions that have already been tested to avoid trying several times same action
    std::vector<CreatureActionType> mActionTry;

    ForceAction                     mForceAction;

    GameEntity*                     mCarriedEntity;
    GameEntityType                  mCarriedEntityDestType;
    std::string                     mCarriedEntityDestName;

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

    //! \brief Reminds the first turn the creature gets furious. If is stays like this for too long,
    //! it will become rogue
    int64_t                         mFirstTurnFurious;

    //! \brief Represents the life value displayed on client side. We do not notify each HP change
    //! to avoid too many communication. But when mOverlayHealthValue changes, we will
    uint32_t                        mOverlayHealthValue;

    //! \brief Represents the mood of the creature. It is a bit array
    uint32_t                        mOverlayMoodValue;

    //! Used by the renderer to save this entity's overlay. It is its responsability
    //! to allocate/delete this pointer
    CreatureOverlayStatus*          mOverlayStatus;

    //! Used on server side to indicate if a change that needs to be notified to the clients happened (like changing
    //! level or HP)
    bool                            mNeedFireRefresh;

    //! \brief The logic in the idle function is basically to roll a dice and, if the value allows, push an action to test if
    //! it is possible. To avoid testing several times the same action, we check in mActionTry if the action as already been
    //! tried. If yes and forcePush is false, the action won't be pushed and pushAction will return false. If the action has
    //! not been tested or if forcePush is true, the action will be pushed and pushAction will return true
    bool pushAction(CreatureAction action, bool forcePush = false);
    void popAction();

    //! \brief Picks a destination far away in the visible tiles and goes there
    //! Returns true if a valid Tile was found. The creature will go there
    //! Returns false if no reachable Tile was found
    bool wanderRandomly(const std::string& animationState);

    //! \brief Search within listObjects the closest one and handle attacks (by moving or attacking)
    //! If a foe can be attacked (in this case, a fight action can be pushed), attackedEntity is set to the attackable entity
    //! and attackedTile to the attacked tile
    //! returns true if a fitting object is found and false otherwise
    bool fightClosestObjectInList(const std::vector<GameEntity*>& listObjects, GameEntity*& attackedEntity, Tile*& attackedTile);

    //! \brief Search within listObjects if an object is reachable and handle attacks (by moving or attacking)
    //! canAttackObject is set to true is a foe is in the good range to hit
    //! If a foe can be attacked (in this case, a fight action can be pushed), attackedEntity is set to the attackable entity
    //! and attackedTile to the attacked tile
    //! returns true if a fitting object is found and false otherwise
    bool fightInRangeObjectInList(const std::vector<GameEntity*>& listObjects, GameEntity*& attackedEntity, Tile*& attackedTile);

    //! \brief A sub-function called by doTurn()
    //! This one checks if there is something prioritary to do (like fighting). If it is the case,
    //! it should empty the action list before adding what to do.
    void decidePrioritaryAction();

    //! \brief A sub-function called by doTurn()
    //! This functions will handle the creature idle action logic.
    //! \return true when another action should handled after that one.
    bool handleIdleAction(const CreatureAction& actionItem);

    //! \brief A sub-function called by doTurn()
    //! This functions will handle the creature walking action logic.
    //! \return true when another action should handled after that one.
    bool handleWalkToTileAction(const CreatureAction& actionItem);

    //! \brief A sub-function called by doTurn()
    //! This functions will handle the creature claim tile action logic.
    //! \return true when another action should handled after that one.
    bool handleClaimTileAction(const CreatureAction& actionItem);

    //! \brief A sub-function called by doTurn()
    //! This functions will handle the creature claim wall tile action logic.
    //! \return true when another action should handled after that one.
    bool handleClaimWallTileAction(const CreatureAction& actionItem);

    //! \brief A sub-function called by doTurn()
    //! This functions will handle the creature dig tile action logic.
    //! \return true when another action should handled after that one.
    bool handleDigTileAction(const CreatureAction& actionItem);

    //! \brief A sub-function called by doTurn()
    //! This functions will handle the creature finding home action logic.
    //! \return true when another action should handled after that one.
    bool handleFindHomeAction(const CreatureAction& actionItem);

    //! \brief A sub-function called by doTurn()
    //! This functions will handle the creature job action logic.
    //! \return true when another action should handled after that one.
    bool handleJobAction(const CreatureAction& actionItem);

    //! \brief A sub-function called by doTurn()
    //! This functions will handle the creature eating action logic.
    //! \return true when another action should handled after that one.
    bool handleEatingAction(const CreatureAction& actionItem);

    //! \brief A sub-function called by doTurn()
    //! This functions will handle the creature attack action logic.
    //! \return true when another action should handled after that one.
    bool handleAttackAction(const CreatureAction& actionItem);

    //! \brief A sub-function called by doTurn()
    //! This functions will handle the creature fighting action logic.
    //! \return true when another action should handled after that one.
    bool handleFightAction(const CreatureAction& actionItem);

    //! \brief A sub-function called by doTurn()
    //! This functions will handle the creature sleeping action logic.
    //! \return true when another action should handled after that one.
    bool handleSleepAction(const CreatureAction& actionItem);

    //! \brief A sub-function called by doTurn()
    //! This functions will handle the creature fleeing action logic.
    //! \return true when another action should handled after that one.
    bool handleFleeAction(const CreatureAction& actionItem);

    //! \brief A sub-function called by doTurn()
    //! This functions will handle the creature action logic about finding a carryable entity.
    //! And trying to carry it to a suitable building
    //! \return true when another action should handled after that one.
    bool handleCarryableEntities(const CreatureAction& actionItem);

    //! \brief A sub-function called by doTurn()
    //! This functions will handle the creature action logic about getting the creature fee.
    //! \return true when another action should handled after that one.
    bool handleGetFee(const CreatureAction& actionItem);

    //! \brief A sub-function called by doTurn()
    //! This functions will handle the creature action logic about trying to leave the dungeon.
    //! \return true when another action should handled after that one.
    bool handleLeaveDungeon(const CreatureAction& actionItem);

    //! \brief A sub-function called by doTurn()
    //! This functions will handle the creature fighting action logic when
    //! fighting an allied natural enemy (when in bad mood)
    //! \return true when another action should handled after that one.
    bool handleFightAlliedNaturalEnemyAction(const CreatureAction& actionItem);

    //! \brief Restores the creature's stats according to its current level
    void buildStats();

    void carryEntity(GameEntity* carriedEntity);

    void releaseCarriedEntity();

    void increaseHunger(double value);

    void decreaseAwakeness(double value);

    void computeMood();

    //! \brief Called when an angry creature wants to attack a natural enemy
    void engageAlliedNaturalEnemy(Creature* attacker);

    void computeCreatureOverlayHealthValue();

    void computeCreatureOverlayMoodValue();
};

#endif // CREATURE_H
