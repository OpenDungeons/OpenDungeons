#ifndef CREATURE_H
#define CREATURE_H

#include <Ogre.h>
#include <string>
#include <deque>
using namespace std;

#include <semaphore.h>

class Creature;

#include "Tile.h"
#include "CreatureAction.h"
#include "Weapon.h"
#include "Field.h"
#include "CreatureClass.h"
#include "AttackableObject.h"

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
		string name;			// The creature's unique name
		Weapon *weaponL, *weaponR;	// The weapons the creature is holding
		string meshID, nodeID;		// The unique names for the OGRE entities
		int color;			// The color of the player who controls this creature
		unsigned int level;
		double exp;
		Tile::TileClearType tilePassability;	//FIXME:  This is not set from file yet.  Also, it should be moved to CreatureClass.
		Tile *homeTile;

		// Object methods
		void createMesh();
		void destroyMesh();
		void deleteYourself();

		void setPosition(double x, double y, double z);
		void setPosition(Ogre::Vector3 v);
		Ogre::Vector3 getPosition();

		void setHP(double nHP);
		double getHP(Tile *tile);

		void setMana(double nMana);
		double getMana();

		// AI stuff
		virtual void doTurn();
		double getHitroll(double range);
		double getDefense();
		void doLevelUp();
		vector<Tile*> visibleTiles;
		vector<AttackableObject*> visibleEnemyObjects;
		vector<AttackableObject*> reachableEnemyObjects;
		vector<AttackableObject*> enemyObjectsInRange;
		vector<AttackableObject*> visibleAlliedObjects;
		vector<AttackableObject*> reachableAlliedObjects;
		void updateVisibleTiles();
		vector<AttackableObject*> getVisibleEnemyObjects();
		vector<AttackableObject*> getReachableAttackableObjects(const vector<AttackableObject*> &objectsToCheck, unsigned int *minRange, AttackableObject **nearestObject);
		vector<AttackableObject*> getEnemyObjectsInRange(const vector<AttackableObject*> &enemyObjectsToCheck);
		vector<AttackableObject*> getVisibleAlliedObjects();
		vector<Tile*> getVisibleMarkedTiles();
		vector<AttackableObject*> getVisibleForce(int color, bool invert);
		Tile* positionTile();
		vector<Tile*> getCoveredTiles();
		void setAnimationState(string s);
		AnimationState* getAnimationState();
		bool isMobile();
		int getLevel();
		int getColor();
		void takeDamage(double damage, Tile *tileTakingDamage);
		void recieveExp(double experience);
		AttackableObject::AttackableObjectType getAttackableObjectType();
		string getName();

		void addDestination(int x, int y);
		bool setWalkPath(list<Tile*> path, unsigned int minDestinations, bool addFirstStop);
		void clearDestinations();
		void clearActionQueue();
		void stopWalking();
		void faceToward(int x, int y);

		Player* getControllingPlayer();
		void computeBattlefield();

		// Visual debugging routines
		void createVisualDebugEntities();
		void destroyVisualDebugEntities();
		bool getHasVisualDebuggingEntities();

		static string getFormat();
		friend ostream& operator<<(ostream& os, Creature *c);
		friend istream& operator>>(istream& is, Creature *c);
		Creature operator=(CreatureClass c2);

		// Public data members
		AnimationState *animationState;
		string destinationAnimationState;
		double shortDistance;
		deque<Ogre::Vector3> walkQueue;
		sem_t walkQueueLockSemaphore;
		bool walkQueueFirstEntryAdded;
		Ogre::Vector3 walkDirection;
		SceneNode *sceneNode;
		static const int maxGoldCarriedByWorkers = 1500;

	private:
		double hp;
		sem_t hpLockSemaphore;
		double mana;
		sem_t manaLockSemaphore;
		int gold;
		deque<CreatureAction> actionQueue;
		Ogre::Vector3 position;
		sem_t positionLockSemaphore;
		int destinationX, destinationY;
		bool hasVisualDebuggingEntities;
		Tile *previousPositionTile;
		list<Tile*> visualDebugEntityTiles;
		Field *battleField;
		bool meshesExist;
};

#endif

