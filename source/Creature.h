#ifndef CREATURE_H
#define CREATURE_H

#include <string>
#include <deque>

#include <semaphore.h>
#include <Ogre.h>

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

#include "CreatureSound.h"
#include "Tile.h"
#include "CreatureDefinition.h"
#include "AttackableEntity.h"
#include "CreatureAction.h"
#include "MovableGameEntity.h"

/*! \brief Position, status, and AI state for a single game creature.
 *
 *  The creature class is the place where an individual creature's state is
 *  stored and manipulated.  The creature class is also used to store creature
 *  class descriptions, since a class decription is really just a subset of the
 *  overall creature information.  This is not really an optimal design and
 *  will probably be refined later but it works fine for now and the code
 *  affected by this change is relatively limited.
 */
class Creature: public MovableGameEntity, public AttackableEntity
{
    public:
        //TODO: write a good default constructor with default arguments
        Creature(GameMap* gameMap = 0, const std::string& name = "");
        //~Creature();

        //TODO: make these private
        // Individual properties
        Weapon *weaponL, *weaponR; // The weapons the creature is holding
        Tile *homeTile;

        // Object methods
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

        void setPosition(Ogre::Real x, Ogre::Real y, Ogre::Real z);
        void setPosition(const Ogre::Vector3& v);

        void setHP(double nHP);

        double getHP(Tile *tile) { return getHP(); };
        double getHP() const;

        double getMaxHp() const { return maxHP; }

        bool getIsOnMap() const;
        void setIsOnMap(bool nIsOnMap);

        void setMana(double nMana);
        double getMana() const;

        int     getDeathCounter() const     { return deathCounter; }
        void    setDeathCounter(int nCount) { deathCounter = nCount; }

        double  getMoveSpeed() const        { return moveSpeed; }

        double  getDigRate() const          { return digRate; }

        const CreatureDefinition* getDefinition() const { return definition; }

        // AI stuff
        virtual void doTurn();
        double getHitroll(double range);
        double getDefense() const;
        void doLevelUp();
        void updateVisibleTiles();
        std::vector<AttackableEntity*> getVisibleEnemyObjects();
        std::vector<AttackableEntity*> getReachableAttackableObjects(
                const std::vector<AttackableEntity*> &objectsToCheck,
                unsigned int *minRange, AttackableEntity **nearestObject);
        std::vector<AttackableEntity*> getEnemyObjectsInRange(
                const std::vector<AttackableEntity*> &enemyObjectsToCheck);
        std::vector<AttackableEntity*> getVisibleAlliedObjects();
        std::vector<Tile*> getVisibleMarkedTiles();
        std::vector<AttackableEntity*> getVisibleForce(int color, bool invert);
        Tile* positionTile();
        std::vector<Tile*> getCoveredTiles();
        bool isMobile() const;
        int getLevel() const;
        //int getColor() const;
        //void setColor(int nColor);
        void takeDamage(double damage, Tile *tileTakingDamage);
        void recieveExp(double experience);
        AttackableEntity::AttackableObjectType getAttackableObjectType() const;
        //const std::string& getName() const;
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
        Creature& operator=(CreatureDefinition c2);

        static const int maxGoldCarriedByWorkers = 1500;

    private:
        void pushAction(CreatureAction action);
        void popAction();
        CreatureAction peekAction();

        //! \brief Pointer to the struct holding the general type of the creature with its values
        const CreatureDefinition* definition;

        mutable sem_t   hpLockSemaphore;
        mutable sem_t   manaLockSemaphore;
        mutable sem_t   isOnMapLockSemaphore;
        sem_t           actionQueueLockSemaphore;
        sem_t           statsWindowLockSemaphore;

        //FIXME:  This is not set from file yet.  Also, it should be moved to CreatureClass.
        Tile::TileClearType tilePassability;

//        std::string     name;
        bool            isOnMap;
        bool            hasVisualDebuggingEntities;
        bool            meshesExist;
        double          awakeness;
        double          maxHP;
        double          maxMana;
        double          hp;
        double          mana;
        double          exp;
        double          digRate;
        double          danceRate;
        int             level;
        int             deathCounter;
        int             gold;
        int             battleFieldAgeCounter;
        int             trainWait;
        Tile*           previousPositionTile;
        Field*          battleField;
        RoomDojo*       trainingDojo;
        CEGUI::Window*  statsWindow;

        std::string     className;

        std::vector<Tile*>              visibleTiles;
        std::vector<AttackableEntity*>  visibleEnemyObjects;
        std::vector<AttackableEntity*>  reachableEnemyObjects;
        std::vector<AttackableEntity*>  enemyObjectsInRange;
        std::vector<AttackableEntity*>  livingEnemyObjectsInRange;
        std::vector<AttackableEntity*>  visibleAlliedObjects;
        std::vector<AttackableEntity*>  reachableAlliedObjects;
        std::deque<CreatureAction>      actionQueue;
        std::list<Tile*>                visualDebugEntityTiles;
        Ogre::SharedPtr<CreatureSound>  sound;
};

#endif
