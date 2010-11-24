#ifndef TILE_H
#define TILE_H

#include <iostream>
#include <Ogre.h>
#include <OgreIteratorWrappers.h>
using namespace Ogre;

#include <semaphore.h>

//FIXME:  this extern is probably not needed once the rendering code is all in one thread.
extern SceneManager* mSceneMgr;
class Tile;
class Creature;
class Player;
class Room;

#include "RenderRequest.h"
#include "Room.h"
#include "SoundEffectsHelper.h"
#include "MapLight.h"

/*! \brief The tile class contains information about tile type and contents and is the basic level bulding block.
 *
 * A Tile is the basic building block for the GameMap.  It consists of a tile
 * type (rock, dirt, gold, etc.) as well as a fullness which indicates how much
 * the tile has been dug out.  Additionally the tile contains lists of the
 * entities located within it to aid in AI calculations.
 */
class Tile
{
	public:
		//TODO:  These should be renumbered to put 0 as the nullTileType, however this will mean converting all the dirt tiles in the level files.
		enum TileType {dirt=0, gold=1, rock=2, water=3, lava=4, claimed=5, nullTileType};
		// Changes to this enum must be reflected in Tile::getTilePassability() as well as in GameMap::path()
		enum TileClearType { impassableTile=0, walkableTile=1, flyableTile=2 };

		// Public functions
		Tile();
		Tile(int nX, int nY, TileType nType, int nFullness);

		void setType(TileType t);
		TileType getType();

		void setFullness(int f);
		int getFullness();
		int getFullnessMeshNumber();
		TileClearType getTilePassability();
		bool permitsVision();

		static string tileTypeToString(TileType t);
		static TileType nextTileType(TileType t);
		static int nextTileFullness(int f);

		void refreshMesh();
		void createMesh();
		void destroyMesh();
		void deleteYourself();

		void setSelected(bool s);
		bool getSelected();

		void setMarkedForDigging(bool s, Player *p);
		bool getMarkedForDigging(Player *p);

		void addCreature(Creature *c);
		void removeCreature(Creature *c);
		unsigned int numCreaturesInCell();
		Creature* getCreature(int index);

		void addPlayerMarkingTile(Player *p);
		void removePlayerMarkingTile(Player *p);
		unsigned int numPlayersMarkingTile();
		Player* getPlayerMarkingTile(int index);

		void addNeighbor(Tile *n);
		Tile* getNeighbor(unsigned int index);
		std::vector<Tile*> getAllNeighbors();

		double claimForColor(int nColor, double nDanceRate);
		double digOut(double digRate);

		Room* getCoveringRoom();
		void setCoveringRoom(Room *r);

		static string getFormat();
		friend ostream& operator<<(ostream& os, Tile *t);
		friend istream& operator>>(istream& is, Tile *t);

		// Public datamembers
		//Vector3 location;
		int x, y;
		int color;
		double colorDouble;
		int floodFillColor;
		double rotation;
		string name;

	private:
		// Private datamembers
		TileType type;
		bool selected, markedForDigging;
		int fullness;
		int fullnessMeshNumber;
		std::vector<Tile*> neighbors;
		std::vector<Creature*> creaturesInCell;
		sem_t creaturesInCellLockSemaphore;
		std::vector<Player*> playersMarkingTile;
		Room *coveringRoom;
		MapLight *claimLight;
};


#endif

