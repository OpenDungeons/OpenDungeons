#ifndef CREATURE_H
#define CREATURE_H

#include <Ogre.h>
#include <string>
#include <deque>

#include <semaphore.h>
//#include <OgreOggSoundManager.h>
//#include <OgreOggISound.h>

class Creature;
namespace CEGUI {
class Window;
}

#include "Tile.h"
#include "CreatureAction.h"
#include "Weapon.h"
#include "Field.h"
#include "CreatureClass.h"
#include "AttackableObject.h"
#include "AnimatedObject.h"
#include "CreatureSound.h"

/*! \brief Position, status, and AI state for a single game creature.
 *
 *  The creature class is the place where an individual creature's state is
 *  stored and manipulated.  The creature class is also used to store creature
 *  class descriptions, since a class decription is really just a subset of the
 *  overall creature information.  This is not really an optimal design and
 *  will probably be refined later but it works fine for now and the code
 *  affected by this change is relatively limited.
 */
class Creature : public CreatureClass, public AttackableObject
{
	public:
		Creature();
		//~Creature();

		// Individual properties
		Weapon *weaponL, *weaponR;	// The weapons the creature is holding
		string meshID, nodeID;		// The unique names for the OGRE entities
		int color;			// The color of the player who controls this creature
		int level;
		double exp;
		Tile::TileClearType tilePassability;	//FIXME:  This is not set from file yet.  Also, it should be moved to CreatureClass.
		Tile *homeTile;
		RoomDojo *trainingDojo;
		int trainWait;

		// Object methods
		void createMesh();
		void destroyMesh();
		void deleteYourself();
		string getUniqueCreatureName();

		void createStatsWindow();
		void destroyStatsWindow();
		void updateStatsWindow();
		std::string getStatsText();

		void setPosition(double x, double y, double z);
		void setPosition(const Ogre::Vector3& v);

		void setHP(double nHP);
		double getHP(Tile *tile);

		void setMana(double nMana);
		double getMana();

		double getMoveSpeed();

		// AI stuff
		virtual void doTurn();
		double getHitroll(double range);
		double getDefense();
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
		std::vector<AttackableObject*> getReachableAttackableObjects(const std::vector<AttackableObject*> &objectsToCheck, unsigned int *minRange, AttackableObject **nearestObject);
		std::vector<AttackableObject*> getEnemyObjectsInRange(const std::vector<AttackableObject*> &enemyObjectsToCheck);
		std::vector<AttackableObject*> getVisibleAlliedObjects();
		std::vector<Tile*> getVisibleMarkedTiles();
		std::vector<AttackableObject*> getVisibleForce(int color, bool invert);
		Tile* positionTile();
		std::vector<Tile*> getCoveredTiles();
		bool isMobile();
		int getLevel();
		int getColor();
		void setColor(int nColor);
		void takeDamage(double damage, Tile *tileTakingDamage);
		void recieveExp(double experience);
		AttackableObject::AttackableObjectType getAttackableObjectType();
		string getName();
		void clearActionQueue();

		Player* getControllingPlayer();
		void computeBattlefield();

		// Sound stuff
		//void setSounds(OgreOggSound::OgreOggISound* attackSound);

		// Visual debugging routines
		void createVisualDebugEntities();
		void destroyVisualDebugEntities();
		bool getHasVisualDebuggingEntities();

		static string getFormat();
		friend ostream& operator<<(ostream& os, Creature *c);
		friend istream& operator>>(istream& is, Creature *c);
		Creature operator=(CreatureClass c2);

		// Public data members
		static const int maxGoldCarriedByWorkers = 1500;
		bool isOnMap;
		sem_t isOnMapLockSemaphore;
		CEGUI::Window *statsWindow;
		int deathCounter;

	private:
		// Private functions
		void pushAction(CreatureAction action);
		void popAction();

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

		//sf::Sound attackSound;

		//Ogre::SharedPtr<CreatureSound> sound;
};

#endif

