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
		string name;
		string meshID, nodeID;
		int color;
		int hp, mana;

		// AI stuff
		enum Action {idle, walkTo};
		Action currentTask;
		vector<Tile*> visibleTiles;
		void updateVisibleTiles();

	private:
		Ogre::Vector3 position;
		int destinationX, destinationY;
};

#endif

