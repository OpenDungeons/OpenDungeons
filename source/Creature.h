/*!
 * \file   Creature.h
 * \date
 * \author
 * \brief  Creature class
 */

/*TODO list:
 * - write a good default constructor with default arguments
 * - make hp, mana and exp unsigned int
 * - replace hardcoded calculations by scripts and/or read the numbers from XML defintion files
 * - the doTurn() functions needs a major rewrite, splitup and script support
 */

#ifndef CREATURE_H
#define CREATURE_H

#include <string>
#include <deque>

#include <semaphore.h>
#include <Ogre.h>

#include "CreatureSound.h"
#include "CreatureDefinition.h"
//#include "AttackableEntity.h"
#include "CreatureAction.h"
#include "MovableGameEntity.h"

class GameMap;
class Creature;
class RoomDojo;
class Weapon;
class Player;
class Field;
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
    public:
        Creature(GameMap* gameMap = 0, const std::string& name = "");

        void createMesh();
        void destroyMesh();
        void deleteYourself();
        std::string getUniqueCreatureName();

        //! \brief Conform: AttackableEntity - Returns the prefix used in the OGRE identifier for this object.
        std::string getOgreNamePrefix() { return "Creature_"; }

        void createStatsWindow();
        void destroyStatsWindow();
        void updateStatsWindow();
        std::string getStatsText();
        
        void setCreatureDefinition(const CreatureDefinition* def); 

        // ----- GETTERS -----
        double  getHP           (Tile *tile)            { return getHP(); }
        double  getHP           ()              const;

        //! \brief Gets the maximum HP the creature can have currently
        double  getMaxHp        ()              const   { return maxHP; }

        //! \brief True if the creature is on the map, false if not (e.g. when in hand)
        bool    getIsOnMap      ()              const;

        //! \brief Gets the current mana
        double  getMana         ()              const;

        //! \brief Gets the current move speed
        double  getMoveSpeed    ()              const   { return moveSpeed; }

        //! \brief Gets the current dig rate
        double  getDigRate      ()              const   { return digRate; }

        //! \brief Gets the death counter
        int     getDeathCounter ()              const   { return deathCounter; }

        //! \brief Gets pointer to the Weapon in left hand
        Weapon* getWeaponL      ()              const   { return weaponL; }

        //! \brief Gets pointer to the Weapon in right hand
        Weapon* getWeaponR      ()              const   { return weaponR; }

        //! \brief Pointer to the creatures home tile, where its bed is located
        Tile*   getHomeTile     ()              const   { return homeTile; }

        //! \brief Pointer to the creature type specification
        const CreatureDefinition* getDefinition() const { return definition; }

        // ----- SETTERS -----
        void setPosition        (Ogre::Real x, Ogre::Real y, Ogre::Real z);
        void setPosition        (const Ogre::Vector3& v);
        void setHP              (double nHP);
        void setIsOnMap         (bool nIsOnMap);
        void setMana            (double nMana);
        void setDeathCounter    (int nCount)        { deathCounter = nCount; }
        void setWeaponL         (Weapon* wL)        { weaponL = wL; }
        void setWeaponR         (Weapon* wR)        { weaponR = wR; }
        void setHomeTile        (Tile* ht)          { homeTile = ht; }

        // AI stuff
        void doTurn();
        //TODO: convert doTurn to doUpkeep();
        bool doUpkeep(){return true;}
        double getHitroll(double range);
        double getDefense() const;
        void doLevelUp();
        void updateVisibleTiles();
        std::vector<GameEntity*> getVisibleEnemyObjects();
        std::vector<GameEntity*> getReachableAttackableObjects(
                const std::vector<GameEntity*> &objectsToCheck,
                unsigned int *minRange, GameEntity **nearestObject);
        std::vector<GameEntity*> getEnemyObjectsInRange(
                const std::vector<GameEntity*> &enemyObjectsToCheck);
        std::vector<GameEntity*> getVisibleAlliedObjects();
        std::vector<Tile*> getVisibleMarkedTiles();
        std::vector<GameEntity*> getVisibleForce(int color, bool invert);
        Tile* positionTile();
        std::vector<Tile*> getCoveredTiles();
        void takeDamage(double damage, Tile *tileTakingDamage);
        void recieveExp(double experience);
        void clearActionQueue();

        Player* getControllingPlayer();
        void computeBattlefield();

        // Visual debugging routines
        void createVisualDebugEntities();
        void destroyVisualDebugEntities();
        bool getHasVisualDebuggingEntities();

        static std::string getFormat();
        friend std::ostream& operator<<(std::ostream& os, Creature *c);
        friend std::istream& operator>>(std::istream& is, Creature *c);
        Creature& operator=(const CreatureDefinition* c2);

        //TODO: make this read from definition file
        static const int maxGoldCarriedByWorkers = 1500;

    private:
        void pushAction(CreatureAction action);
        void popAction();
        CreatureAction peekAction();

        //! \brief The weapon the creature is holding in its left hand
        Weapon* weaponL;

        //! \brief The weapon the creature is holding in its right hand
        Weapon* weaponR;

        //! \brief The creatures home tile (where its bed is located)
        Tile *homeTile;

        //! \brief Pointer to the struct holding the general type of the creature with its values
        const CreatureDefinition* definition;

        mutable sem_t   hpLockSemaphore;
        mutable sem_t   manaLockSemaphore;
        mutable sem_t   isOnMapLockSemaphore;
        sem_t           actionQueueLockSemaphore;
        sem_t           statsWindowLockSemaphore;

        bool            isOnMap;
        bool            hasVisualDebuggingEntities;
        double          awakeness;
        double          maxHP;
        double          maxMana;
        double          hp;
        double          mana;
        double          exp;
        double          digRate;
        double          danceRate;
        int             deathCounter;
        int             gold;
        int             battleFieldAgeCounter;
        int             trainWait;
        Tile*           previousPositionTile;
        Field*          battleField;
        RoomDojo*       trainingDojo;
        CEGUI::Window*  statsWindow;

        //TODO? isn't this something for CreatureDefintion?
        std::string                     className;

        std::vector<Tile*>              visibleTiles;
        std::vector<GameEntity*>        visibleEnemyObjects;
        std::vector<GameEntity*>        reachableEnemyObjects;
        std::vector<GameEntity*>        enemyObjectsInRange;
        std::vector<GameEntity*>        livingEnemyObjectsInRange;
        std::vector<GameEntity*>        visibleAlliedObjects;
        std::vector<GameEntity*>        reachableAlliedObjects;
        std::deque<CreatureAction>      actionQueue;
        std::list<Tile*>                visualDebugEntityTiles;
        Ogre::SharedPtr<CreatureSound>  sound;
};

#endif
