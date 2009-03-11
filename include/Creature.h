#ifndef CREATURE_H
#define CREATURE_H

#include <Ogre.h>
#include <string>
using namespace std;

#include "Tile.h"

class Creature
{
	public:
		Creature();
		Creature(string nClassName, string nMeshName, Ogre::Vector3 nScale);
		friend ostream& operator<<(ostream& os, Creature *c);
		friend istream& operator>>(istream& is, Creature *c);
		void createMesh();
		void destroyMesh();
		void setPosition(double x, double y, double z);
		Ogre::Vector3 getPosition();
		virtual void doTurn();

		// Class properties
		string className;
		string meshName;
		Ogre::Vector3 scale;

		// Individual properties
		string name;			// The creature's unique name
		string meshID, nodeID;		// The unique names for the OGRE entities
		int color;			// The color of the player who controls this creature
		int hp, mana;			// Basic stats

		// Visual debugging routines
		void createVisualDebugEntities();
		void destroyVisualDebugEntities();

		// AI stuff
		enum Action {idle, walkTo, dig};
		Action currentTask;
		vector<Tile*> visibleTiles;
		void updateVisibleTiles();
		Tile *positionTile;

	private:
		Ogre::Vector3 position;
		int destinationX, destinationY;
};

#endif

