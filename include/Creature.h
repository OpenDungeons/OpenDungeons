#ifndef CREATURE_H
#define CREATURE_H

#include <Ogre.h>
#include <string>
#include <deque>
using namespace std;
class Creature;

#include "Tile.h"
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
class Creature
{
	public:
		// Constructors and operators
		Creature();
		Creature(string nClassName, string nMeshName, Ogre::Vector3 nScale, int nHP, int nMana, double nSightRadius, double nDigRate, double nMoveSpeed);
		friend ostream& operator<<(ostream& os, Creature *c);
		friend istream& operator>>(istream& is, Creature *c);

		// Class properties
		string className;
		string meshName;
		Ogre::Vector3 scale;
		double sightRadius;		// The inner radius where the creature sees everything
		double digRate;			// Fullness removed per turn of digging

		// Individual properties
		string name;			// The creature's unique name
		string weaponL, weaponR;	// The names of the weapons the creature is holding
		string meshID, nodeID;		// The unique names for the OGRE entities
		int color;			// The color of the player who controls this creature
		int hp, mana;			// Basic stats
		double moveSpeed;		// How fast the creature moves and animates
		Tile::TileClearType tilePassability;	//FIXME:  This is not set from file yet.
		sem_t meshCreationFinishedSemaphore;

		// Object methods
		void createMesh();
		void destroyMesh();
		void deleteYourself();
		void setPosition(double x, double y, double z);
		void setPosition(Ogre::Vector3 v);
		Ogre::Vector3 getPosition();
		virtual void doTurn();

		// AI stuff
		vector<Tile*> visibleTiles;
		void updateVisibleTiles();
		vector<Creature*> getVisibleEnemies();
		Tile* positionTile();
		AnimationState *animationState;
		void setAnimationState(string s);
		AnimationState* getAnimationState();
		double shortDistance;
		deque<Ogre::Vector3> walkQueue;
		Ogre::Vector3 walkDirection;
		void addDestination(int x, int y);
		void clearDestinations();
		void stopWalking();
		Player* getControllingPlayer();

		// Visual debugging routines
		void createVisualDebugEntities();
		void destroyVisualDebugEntities();
		bool getHasVisualDebuggingEntities();

	private:
		deque<CreatureAction> actionQueue;
		Ogre::Vector3 position;
		int destinationX, destinationY;
		bool hasVisualDebuggingEntities;
		Tile *previousPositionTile;
		list<Tile*> visualDebugEntityTiles;
};

#endif

