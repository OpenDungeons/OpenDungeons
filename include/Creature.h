#ifndef CREATURE_H
#define CREATURE_H

#include <Ogre.h>
#include <string>
#include <deque>

#include <semaphore.h>

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
#include "CreatureClass.h"
#include "AttackableObject.h"
#include "CreatureAction.h"


/*! \brief Position, status, and AI state for a single game creature.
 *
 *  The creature class is the place where an individual creature's state is
 *  stored and manipulated.  The creature class is also used to store creature
 *  class descriptions, since a class decription is really just a subset of the
 *  overall creature information.  This is not really an optimal design and
 *  will probably be refined later but it works fine for now and the code
 *  affected by this change is relatively limited.
 */
class Creature: public CreatureClass, public AttackableObject
{
    public:
        Creature();
        //~Creature();

        // Individual properties
        Weapon *weaponL, *weaponR; // The weapons the creature is holding
        int color; // The color of the player who controls this creature
        int level;
        double exp;
        Tile::TileClearType tilePassability; //FIXME:  This is not set from file yet.  Also, it should be moved to CreatureClass.
        Tile *homeTile;
        RoomDojo *trainingDojo;
        int trainWait;

        // Object methods
        void createMesh();
        void destroyMesh();
        void deleteYourself();
        std::string getUniqueCreatureName();

        void createStatsWindow();
        void destroyStatsWindow();
        void updateStatsWindow();
        std::string getStatsText();

        void setPosition(Ogre::Real x, Ogre::Real y, Ogre::Real z);
        void setPosition(const Ogre::Vector3& v);

        void setHP(double nHP);
        double getHP(Tile *tile);

        void setMana(double nMana);
        double getMana();

        double getMoveSpeed();

        // AI stuff
        virtual void doTurn();
        double getHitroll(double range);
        double getDefense() const;
        void doLevelUp();
        std::vector<Tile*> visibleTiles;
        std::vector<AttackableObject*> visibleEnemyObjects;
        std::vector<AttackableObject*> reachableEnemyObjects;
        std::vector<AttackableObject*> enemyObjectsInRange;
        std::vector<AttackableObject*> livingEnemyObjectsInRange;
        std::vector<AttackableObject*> visibleAlliedObjects;
        std::vector<AttackableObject*> reachableAlliedObjects;
        void updateVisibleTiles();
        std::vector<AttackableObject*> getVisibleEnemyObjects();
        std::vector<AttackableObject*> getReachableAttackableObjects(
                const std::vector<AttackableObject*> &objectsToCheck,
                unsigned int *minRange, AttackableObject **nearestObject);
        std::vector<AttackableObject*> getEnemyObjectsInRange(
                const std::vector<AttackableObject*> &enemyObjectsToCheck);
        std::vector<AttackableObject*> getVisibleAlliedObjects();
        std::vector<Tile*> getVisibleMarkedTiles();
        std::vector<AttackableObject*> getVisibleForce(int color, bool invert);
        Tile* positionTile();
        std::vector<Tile*> getCoveredTiles();
        bool isMobile() const;
        int getLevel() const;
        int getColor() const;
        void setColor(int nColor);
        void takeDamage(double damage, Tile *tileTakingDamage);
        void recieveExp(double experience);
        AttackableObject::AttackableObjectType getAttackableObjectType() const;
        const std::string& getName() const;
        void clearActionQueue();

        Player* getControllingPlayer();
        void computeBattlefield();

        // Sound stuff
        //void setSounds(OgreOggSound::OgreOggISound* attackSound);

        // Visual debugging routines
        void createVisualDebugEntities();
        void destroyVisualDebugEntities();
        bool getHasVisualDebuggingEntities();

        static std::string getFormat();
        friend std::ostream& operator<<(std::ostream& os, Creature *c);
        friend std::istream& operator>>(std::istream& is, Creature *c);
        Creature operator=(CreatureClass c2);

        // Public data members
        static const int maxGoldCarriedByWorkers = 1500;
        bool isOnMap;
        sem_t isOnMapLockSemaphore;
        int deathCounter;

    private:
        // Private functions
        void pushAction(CreatureAction action);
        void popAction();
        CreatureAction peekAction();

        // Private data members
        double hp;
        sem_t hpLockSemaphore;
        double mana;
        sem_t manaLockSemaphore;
        int gold;
        std::deque<CreatureAction> actionQueue;
        sem_t actionQueueLockSemaphore;
        int destinationX, destinationY;
        bool hasVisualDebuggingEntities;
        Tile *previousPositionTile;
        std::list<Tile*> visualDebugEntityTiles;
        Field *battleField;
        int battleFieldAgeCounter;
        bool meshesExist;
        double awakeness;
        CEGUI::Window *statsWindow;
        sem_t statsWindowLockSemaphore;

        //sf::Sound attackSound;

        Ogre::SharedPtr<CreatureSound> sound;
};

#endif

