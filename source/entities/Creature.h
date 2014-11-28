/*!
 * \file   Creature.h
 * \brief  Creature class
 *
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

#ifndef CREATURE_H
#define CREATURE_H

#include "entities/CreatureSound.h"
#include "entities/CreatureDefinition.h"
#include "entities/CreatureAction.h"
#include "entities/MovableGameEntity.h"

#include <OgreVector2.h>
#include <OgreVector3.h>
#include <CEGUI/EventArgs.h>

#include <string>
#include <deque>

class GameMap;
class Creature;
class Weapon;
class CullingQuad;

namespace CEGUI
{
class Window;
}

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
    friend class CullingQuad;
    friend class ODClient;
public:
    //! \brief Constructor for creatures. It generates an unique name
    Creature(GameMap* gameMap, const CreatureDefinition* definition);
    virtual ~Creature();

    static const std::string CREATURE_PREFIX;

    //! \brief Conform: AttackableEntity - Returns the prefix used in the OGRE identifier for this object.
    std::string getOgreNamePrefix() const
    { return CREATURE_PREFIX; }

    bool canDisplayStatsWindow(Seat* seat)
    { return true; }
    void createStatsWindow();
    void destroyStatsWindow();
    bool CloseStatsWindow(const CEGUI::EventArgs& /*e*/);
    void updateStatsWindow(const std::string& txt);
    std::string getStatsText();

    Ogre::Vector2 get2dPosition()
    {
        Ogre::Vector3 tmp = getPosition();
        return Ogre::Vector2(tmp.x,tmp.y);
    }

    //! \brief Get the level of the object
    inline unsigned int getLevel() const
    { return mLevel; }

    double getHP(Tile *tile) const
    { return getHP(); }

    double getHP() const;

    //! \brief Gets the maximum HP the creature can have currently
    double getMaxHp()const
    { return mMaxHP; }

    //! \brief Gets the current dig rate
    double getDigRate() const
    { return mDigRate; }

    //! \brief Gets pointer to the Weapon in left hand
    const Weapon* getWeaponL() const
    { return mWeaponL; }

    //! \brief Gets pointer to the Weapon in right hand
    const Weapon* getWeaponR() const
    { return mWeaponR; }

    //! \brief Pointer to the creatures home tile, where its bed is located
    Tile* getHomeTile() const
    { return mHomeTile; }

    //! \brief Pointer to the creature type specification
    const CreatureDefinition* getDefinition() const
    { return mDefinition; }

    /*! \brief Changes the creature's position to a new position.
     *  This is an overloaded function which just calls Creature::setPosition(double x, double y, double z).
     */
    void setPosition(const Ogre::Vector3& v);

    //! \brief Gets the move speed on the current tile.
    virtual double getMoveSpeed() const;

    //! \brief Gets the move speed on the current tile.
    double getMoveSpeed(Tile* tile) const;

    //! \brief Gets the creature depending the terrain type.
    double getMoveSpeedGround() const
    { return mGroundSpeed; }
    double getMoveSpeedWater() const
    { return mWaterSpeed; }
    double getMoveSpeedLava() const
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
    inline void setLevel(unsigned int nL)
    { mLevel = nL; }

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

    virtual bool isAttackable() const;

    double getPhysicalDamage(double range);
    double getPhysicalDefense() const;
    double getMagicalDamage(double range);
    double getMagicalDefense() const;

    //! \brief Returns the currently best attack range of the creature.
    //! \note Depends also on its equipment.
    double getBestAttackRange() const;

    //! \brief Check whether a creature has earned one level.
    bool checkLevelUp();
    //! \brief Refreshes current creature with creatureNewState (hp, scale, level, ...)
    void refreshFromCreature(Creature *creatureNewState);

    //! \brief Updates the lists of tiles within sight radius.
    //! And the tiles the creature can "see" (removing the ones behind walls).
    void updateTilesInSight();

    //! \brief Loops over the visibleTiles and adds all enemy creatures in each tile to a list which it returns.
    std::vector<GameEntity*> getVisibleEnemyObjects();

    //! \brief Loops over objectsToCheck and returns a vector containing all the ones which can be reached via a valid path.
    std::vector<GameEntity*> getReachableAttackableObjects(const std::vector<GameEntity*> &objectsToCheck);

    //! \brief Loops over objectsToCheck and returns a vector containing all the creatures in the list.
    std::vector<GameEntity*> getCreaturesFromList(const std::vector<GameEntity*> &objectsToCheck, bool koboldsOnly);

    //! \brief Loops over the visibleTiles and adds all allied creatures in each tile to a list which it returns.
    std::vector<GameEntity*> getVisibleAlliedObjects();

    //! \brief Loops over the visibleTiles and updates any which are marked for digging, and are reachable.
    void updateVisibleMarkedTiles();

    //! \brief Loops over the visibleTiles and adds any which are claimable walls.
    std::vector<Tile*> getVisibleClaimableWallTiles();

    //! \brief Loops over the visibleTiles and returns any creatures in those tiles
    //! allied with the given seat (or if invert is true, does not allied)
    std::vector<GameEntity*> getVisibleForce(Seat* seat, bool invert);

    //! \brief Conform: AttackableObject - Returns a vector containing the tile the creature is in.
    std::vector<Tile*> getCoveredTiles();

    //! \brief Conform: AttackableObject - Deducts a given amount of HP from this creature.
    double takeDamage(GameEntity* attacker, double physicalDamage, double magicalDamage, Tile* tileTakingDamage);

    //! \brief Conform: AttackableObject - Adds experience to this creature.
    void receiveExp(double experience);

    //! \brief Returns true if the given action is queued in the action list. False otherwise
    bool isActionInList(CreatureAction::ActionType action);

    const std::deque<CreatureAction>& getActionQueue()
    { return mActionQueue; }

    //! \brief Clears the action queue, except for the Idle action at the end.
    void clearActionQueue();

    //! \brief Displays a mesh on all of the tiles visible to the creature.
    void createVisualDebugEntities();

    //! \brief Destroy the meshes created by createVisualDebuggingEntities().
    void destroyVisualDebugEntities();

    //! \brief An accessor to return whether or not the creature has OGRE entities for its visual debugging entities.
    bool getHasVisualDebuggingEntities();

    //! \FIXME Those functions are lacking parameters to be actually functional
    //! \brief Get the text format of creatures in level files (already spawned at startup).
    //! \returns A string describing the IO format the creatures need to have in file.
    static std::string getFormat();

    static Creature* getCreatureFromStream(GameMap* gameMap, std::istream& is);
    static Creature* getCreatureFromPacket(GameMap* gameMap, ODPacket& is);
    virtual void exportToPacket(ODPacket& os);
    virtual void importFromPacket(ODPacket& is);
    virtual void exportToStream(std::ostream& os);
    virtual void importFromStream(std::istream& is);
    inline void setQuad(CullingQuad* cq)
    { mTracingCullingQuad = cq; }

    inline CullingQuad* getQuad()
    { return mTracingCullingQuad; }

    //! \brief Checks if the creature can be picked up. If yes, this function does the needed
    //! to prepare for the pickup (removing creature from GameMap, changing states, ...).
    //! Returns true if the creature can be picked up
    bool tryPickup(Seat* seat, bool isEditorMode);
    void pickup();
    bool tryDrop(Seat* seat, Tile* tile, bool isEditorMode);

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

    //! \brief Allows to change the room the creature is using (when room absorbtion for example)
    void changeJobRoom(Room* newRoom);

    //! \brief Allows to change the room the creature is using (when room absorbtion for example)
    void changeEatRoom(Room* newRoom);

    //! \brief Play a spatial sound at the creature position of the corresponding type.
    void playSound(CreatureSound::SoundType soundType);

    //! \brief Tells whether the creature can go through the given tile.
    bool canGoThroughTile(const Tile* tile) const;

    virtual void notifyEntityCarried(bool isCarried);

    bool canSlap(Seat* seat, bool isEditorMode);
    void slap(bool isEditorMode);

protected:
    virtual void createMeshLocal();
    virtual void destroyMeshLocal();
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

    CullingQuad* mTracingCullingQuad;

    //! \brief Natural physical and magical attack and defense (without equipment)
    double mPhysicalAttack;
    double mMagicalAttack;
    double mPhysicalDefense;
    double mMagicalDefense;
    double mWeaponlessAtkRange;

    //! \brief The time left before being to draw an attack in seconds
    double mAttackWarmupTime;

    //! \brief The weapon the creature is holding in its left hand or NULL if none. It will be set by a pointer
    //! managed by the game map and thus, should not be deleted by the creature class
    const Weapon* mWeaponL;

    //! \brief The weapon the creature is holding in its right hand or NULL if none. It will be set by a pointer
    //! managed by the game map and thus, should not be deleted by the creature class
    const Weapon* mWeaponR;

    //! \brief The creatures home tile (where its bed is located)
    Tile *mHomeTile;

    //! \brief Pointer to the struct holding the general type of the creature with its values
    const CreatureDefinition* mDefinition;

    bool            mHasVisualDebuggingEntities;
    double          mAwakeness;
    double          mHunger;

    //! \brief The level of the creature
    unsigned int    mLevel;

    //! \brief The creature stats
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
    int             mGold;
    int             mJobCooldown;
    int             mEatCooldown;
    //! \brief The number of turns we are doing the same action. If the action is popped or pushed, mNbTurnAction is set to 0
    int             mNbTurnAction;

    Tile*           mPreviousPositionTile;
    Room*           mJobRoom;
    Room*           mEatRoom;
    CEGUI::Window*  mStatsWindow;

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
    std::list<Tile*>                mVisualDebugEntityTiles;


    //! \brief Allows to keep track of the entity we are currently attacking
    Tile*                           mAttackedTile;
    GameEntity*                     mAttackedObject;

    //! \brief The creature sounds library reference.
    //! \warning Do not delete it. The pointer in handled in SoundEffectsManager.
    CreatureSound*                  mSound;

    ForceAction                     mForceAction;

    bool                            mIsCarryActionTested;
    GameEntity*                     mCarriedEntity;
    Building*                       mCarriedEntityDest;

    void pushAction(CreatureAction action);
    void popAction();
    CreatureAction peekAction();

    //! \brief Picks a destination far away in the visible tiles and goes there
    //! Returns true if a valid Tile was found. The creature will go there
    //! Returns false if no reachable Tile was found
    bool wanderRandomly(const std::string& animationState);

    //! \brief Search within listObjects the closest one and handle attacks (by moving or attacking)
    //! canAttackObject is set to true is a foe is in the good range to hit (in this case, a fight action can be pushed)
    //! If a foe can be attacked, mAttackedObject is set to the attackable entity and mAttackedTile to the attacked tile
    //! returns true if a fitting object is found and false otherwise
    bool fightClosestObjectInList(const std::vector<GameEntity*>& listObjects, bool& canAttackObject);

    //! \brief Search within listObjects if an object is reachable and handle attacks (by moving or attacking)
    //! canAttackObject is set to true is a foe is in the good range to hit (in this case, a fight action can be pushed)
    //! If a foe can be attacked, mAttackedObject is set to the attackable entity and mAttackedTile to the attacked tile
    //! returns true if a fitting object is found and false otherwise
    bool fightInRangeObjectInList(const std::vector<GameEntity*>& listObjects, bool& canAttackObject);

    //! \brief A sub-function called by doTurn()
    //! This one checks if there is something prioritary to do (like fighting). If it is the case,
    //! it should empty the action list before adding what to do.
    void decidePrioritaryAction();

    //! \brief A sub-function called by doTurn()
    //! This functions will hanlde the creature idle action logic.
    //! \return true when another action should handled after that one.
    bool handleIdleAction();

    //! \brief A sub-function called by doTurn()
    //! This functions will hanlde the creature walking action logic.
    //! \return true when another action should handled after that one.
    bool handleWalkToTileAction();

    //! \brief A sub-function called by doTurn()
    //! This functions will hanlde the creature claim tile action logic.
    //! \return true when another action should handled after that one.
    bool handleClaimTileAction();

    //! \brief A sub-function called by doTurn()
    //! This functions will hanlde the creature claim wall tile action logic.
    //! \return true when another action should handled after that one.
    bool handleClaimWallTileAction();

    //! \brief A sub-function called by doTurn()
    //! This functions will hanlde the creature dig tile action logic.
    //! \return true when another action should handled after that one.
    bool handleDigTileAction();

    //! \brief A sub-function called by doTurn()
    //! This functions will hanlde the creature deposit gold action logic.
    //! \return true when another action should handled after that one.
    bool handleDepositGoldAction();

    //! \brief A sub-function called by doTurn()
    //! This functions will hanlde the creature finding home action logic.
    //! \return true when another action should handled after that one.
    bool handleFindHomeAction(bool isForced);

    //! \brief A sub-function called by doTurn()
    //! This functions will hanlde the creature job action logic.
    //! \return true when another action should handled after that one.
    bool handleJobAction(bool isForced);

    //! \brief A sub-function called by doTurn()
    //! This functions will hanlde the creature eating action logic.
    //! \return true when another action should handled after that one.
    bool handleEatingAction(bool isForced);

    //! \brief Makes the creature stop its job (releases the job room)
    void stopJob();

    //! \brief Makes the creature stop eating (releases the hatchery)
    void stopEating();

    //! \brief A sub-function called by doTurn()
    //! This functions will hanlde the creature attack action logic.
    //! \return true when another action should handled after that one.
    bool handleAttackAction();

    //! \brief A sub-function called by doTurn()
    //! This functions will hanlde the creature fighting action logic.
    //! \return true when another action should handled after that one.
    bool handleFightAction();

    //! \brief A sub-function called by doTurn()
    //! This functions will hanlde the creature sleeping action logic.
    //! \return true when another action should handled after that one.
    bool handleSleepAction();

    //! \brief A sub-function called by doTurn()
    //! This functions will hanlde the creature fleeing action logic.
    //! \return true when another action should handled after that one.
    bool handleFleeAction();

    //! \brief A sub-function called by doTurn()
    //! This functions will hanlde the creature action logic about finding a carryable entity.
    //! And trying to carry it to a suitable building
    //! \return true when another action should handled after that one.
    bool handleCarryableEntities();

    //! \brief Returns true if creature is in bad mood. False otherwise. A creature in bad mood will more likely
    //! flee or attack allied units
    bool isInBadMood();

    //! \brief Restores the creature's stats according to the given level
    void buildStats(unsigned int level);

    void carryEntity(GameEntity* carriedEntity);

    void releaseCarriedEntity();
};

#endif // CREATURE_H
