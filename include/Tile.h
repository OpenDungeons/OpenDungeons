#ifndef TILE_H
#define TILE_H

#include <iostream>
#include <Ogre.h>
#include <OgreIteratorWrappers.h>
using namespace std;
using namespace Ogre;

#include <semaphore.h>

//FIXME:  this extern is probably not needed once the rendering code is all in one thread.
extern SceneManager* mSceneMgr;

#include "RenderRequest.h"

class Creature;

class Tile
{
	public:
		enum TileType {dirt=0, gold=1, rock=2, water=3, lava=4, claimed=5, nullTileType};

		Tile();
		Tile(int nX, int nY, TileType nType, int nFullness);

		void setType(TileType t);
		TileType getType();

		void setFullness(int f);
		int getFullness();
		int getFullnessMeshNumber();

		static string tileTypeToString(TileType t);
		static TileType nextTileType(TileType t);
		static int nextTileFullness(int f);

		void refreshMesh();
		void createMesh();
		void destroyMesh();
		void deleteYourself();

		void setSelected(bool s);
		bool getSelected();

		void setMarkedForDigging(bool s);
		bool getMarkedForDigging();

		void addCreature(Creature *c);
		void removeCreature(Creature *c);
		int numCreaturesInCell();
		Creature* getCreature(int index);

		Vector3 location;
		int x, y;
		vector<Tile*> neighbors;
		double rotation;
		string name;

		friend ostream& operator<<(ostream& os, Tile *t);
		friend istream& operator>>(istream& is, Tile *t);


	private:
		TileType type;
		bool selected, markedForDigging;
		int fullness;
		int fullnessMeshNumber;
		vector<Creature*> creaturesInCell;
};


#endif

