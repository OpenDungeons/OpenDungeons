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

#include "CreatureSound.h"
#include "CreatureDefinition.h"
#include "CreatureAction.h"
#include "MovableGameEntity.h"

#include <semaphore.h>
#include <OgreVector2.h>
#include <OgreVector3.h>
#include <OgreSharedPtr.h>

#include <string>
#include <deque>

class GameMap;
class Creature;
class RoomDojo;
class Weapon;
class Player;
class Field;
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
public:
    Creature(GameMap* gameMap = NULL, const std::string& name = std::string());
    virtual ~Creature();
    std::string getUniqueCreatureName();

    //! \brief Conform: AttackableEntity - Returns the prefix used in the OGRE identifier for this object.
    std::string getOgreNamePrefix()
    { return "Creature_"; }

    void createStatsWindow();
    void destroyStatsWindow();
    void updateStatsWindow();
    std::string getStatsText();

    void setCreatureDefinition(const CreatureDefinition* def);

    Ogre::Vector2 get2dPosition() {
        Ogre::Vector3 tmp = getPosition();
        return Ogre::Vector2(tmp.x,tmp.y);
    }

    //! \brief Get the level of the object
    inline unsigned int getLevel() const
    { return mLevel; }

    double getHP(Tile *tile)
    { return getHP(); }

    double getHP() const;

    //! \brief Gets the maximum HP the creature can have currently
    double getMaxHp()const
    { return mMaxHP; }

    //! \brief True if the creature is on the map, false if not (e.g. when in hand)
    bool getIsOnMap() const;

    //! \brief Gets the current mana
    double getMana() const;

    //! \brief Gets the current dig rate
    double getDigRate() const
    { return mDigRate; }

    //! \brief Gets the death counter
    int getDeathCounter() const
    { return mDeathCounter; }

    //! \brief Gets pointer to the Weapon in left hand
    Weapon* getWeaponL() const
    { return mWeaponL; }

    //! \brief Gets pointer to the Weapon in right hand
    Weapon* getWeaponR() const
    { return mWeaponR; }

    //! \brief Pointer to the creatures home tile, where its bed is located
    Tile* getHomeTile() const
    { return mHomeTile; }

    //! \brief Pointer to the creature type specification
    const CreatureDefinition* getDefinition() const
    { return mDefinition; }

    void setPosition(const Ogre::Vector3& v);
    void setHP(double nHP);
    void setIsOnMap(bool nIsOnMap);
    void setMana(double nMana);

    inline void setDeathCounter(unsigned int nC)
    { mDeathCounter = nC; }

    inline void setWeaponL(Weapon* wL)
    { mWeaponL = wL; }

    inline void setWeaponR(Weapon* wR)
    { mWeaponR = wR; }

    inline void setHomeTile(Tile* ht)
    { mHomeTile = ht; }

    //! \brief Set the level of the creature
    inline void setLevel(unsigned int nL)
    { mLevel = nL; }

    // AI stuff
    void doTurn();
    //TODO: convert doTurn to doUpkeep();
    bool doUpkeep()
    { return true; }

    double getHitroll(double range);
    double getDefense() const;
    void doLevelUp();
    void updateVisibleTiles();

    std::vector<GameEntity*> getVisibleEnemyObjects();
    std::vector<GameEntity*> getReachableAttackableObjects(const std::vector<GameEntity*> &objectsToCheck,
                                                           unsigned int *minRange, GameEntity **nearestObject);
    std::vector<GameEntity*> getEnemyObjectsInRange(const std::vector<GameEntity*> &enemyObjectsToCheck);
    std::vector<GameEntity*> getVisibleAlliedObjects();
    std::vector<Tile*> getVisibleMarkedTiles();
    std::vector<GameEntity*> getVisibleForce(int color, bool invert);
    Tile* positionTile();
    std::vector<Tile*> getCoveredTiles();
    void takeDamage(double damage, Tile* tileTakingDamage);
    void recieveExp(double experience);
    void clearActionQueue();

    Player* getControllingPlayer();
    void computeBattlefield();

    // Visual debugging routines
    void createVisualDebugEntities();
    void destroyVisualDebugEntities();
    bool getHasVisualDebuggingEntities();

    void attach();
    void detach();

    static std::string getFormat();
    friend std::ostream& operator<<(std::ostream& os, Creature *c);
    friend std::istream& operator>>(std::istream& is, Creature *c);
    Creature& operator=(const CreatureDefinition* c2);

    inline void setQuad(CullingQuad* cq)
    { mTracingCullingQuad = cq; }

    inline CullingQuad* getQuad()
    { return mTracingCullingQuad; }

private:
    CullingQuad* mTracingCullingQuad;

    //! \brief The weapon the creature is holding in its left hand
    Weapon* mWeaponL;

    //! \brief The weapon the creature is holding in its right hand
    Weapon* mWeaponR;

    //! \brief The creatures home tile (where its bed is located)
    Tile *mHomeTile;

    //! \brief Pointer to the struct holding the general type of the creature with its values
    const CreatureDefinition* mDefinition;

    mutable sem_t   mHpLockSemaphore;
    mutable sem_t   mManaLockSemaphore;
    mutable sem_t   mIsOnMapLockSemaphore;
    sem_t           mActionQueueLockSemaphore;
    sem_t           mStatsWindowLockSemaphore;

    bool            mIsOnMap;
    bool            mHasVisualDebuggingEntities;
    double          mAwakeness;
    double          mMaxHP;
    double          mMaxMana;

    //! \brief The level of the creature
    unsigned int    mLevel;

    double          mHp;
    double          mMana;
    double          mExp;
    double          mDigRate;
    double          mDanceRate;
    unsigned int    mDeathCounter;
    int             mGold;
    int             mBattleFieldAgeCounter;
    int             mTrainWait;

    Tile*           mPreviousPositionTile;
    Field*          mBattleField;
    RoomDojo*       mTrainingDojo;
    CEGUI::Window*  mStatsWindow;

    std::vector<Tile*>              mVisibleTiles;
    std::vector<GameEntity*>        mVisibleEnemyObjects;
    std::vector<GameEntity*>        mReachableEnemyObjects;
    std::vector<GameEntity*>        mEnemyObjectsInRange;
    std::vector<GameEntity*>        mLivingEnemyObjectsInRange;
    std::vector<GameEntity*>        mVisibleAlliedObjects;
    std::vector<GameEntity*>        mReachableAlliedObjects;
    std::deque<CreatureAction>      mActionQueue;
    std::list<Tile*>                mVisualDebugEntityTiles;
    Ogre::SharedPtr<CreatureSound>  mSound;

    void pushAction(CreatureAction action);
    void popAction();
    CreatureAction peekAction();

    //! \brief A sub-function called by doTurn()
    //! This one setup the next action push on the creature action stack.
    void decideNextAction();

    //! \brief A sub-function called by doTurn()
    //! This functions will hanlde the creature walking action logic.
    //! \return true when another action should handled after that one.
    bool handleWalkToTileAction();

    //! \brief A sub-function called by doTurn()
    //! This functions will hanlde the creature deposit gold action logic.
    //! \return true when another action should handled after that one.
    bool handleDepositGoldAction();

    //! \brief A sub-function called by doTurn()
    //! This functions will hanlde the creature finding home action logic.
    //! \return true when another action should handled after that one.
    bool handleFindHomeAction();

    //! \brief A sub-function called by doTurn()
    //! This functions will hanlde the creature training action logic.
    //! \return true when another action should handled after that one.
    bool handleTrainingAction();

    //! \brief Makes the creature stop using the Dojo (Training room)
    void stopUsingDojo();

    //! \brief A sub-function called by doTurn()
    //! This functions will hanlde the creature attack action logic.
    //! \return true when another action should handled after that one.
    bool handleAttackAction();

    //! \brief A sub-function called by doTurn()
    //! This functions will hanlde the creature maneuver action logic.
    //! \return true when another action should handled after that one.
    bool handleManeuverAction();

    //! \brief A sub-function called by doTurn()
    //! This functions will hanlde the creature sleeping action logic.
    //! \return true when another action should handled after that one.
    bool handleSleepAction();
};

#endif // CREATURE_H
