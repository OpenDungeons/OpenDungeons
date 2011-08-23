#ifndef TILE_H
#define TILE_H

#include <string>
#include <vector>
#include <ostream>
#include <istream>
#include <semaphore.h>
#include <OgrePrerequisites.h>


class Tile;
class Creature;
class Player;
class Room;
class MapLight;
class GameMap;

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
        enum TileType
        {
            dirt = 0,
            gold = 1,
            rock = 2,
            water = 3,
            lava = 4,
            claimed = 5,
            nullTileType
        };
        // Changes to this enum must be reflected in Tile::getTilePassability() as well as in GameMap::path()
        enum TileClearType
        {
            impassableTile = 0, walkableTile = 1, flyableTile = 2
        };

        // Public functions
        Tile();
        Tile(int nX, int nY, TileType nType, double nFullness);
        void initialize();

        void setType(TileType t);
        TileType getType() const;

        void setFullness(double f);
        double getFullness() const;
        int getFullnessMeshNumber() const;
        TileClearType getTilePassability() const;
        bool permitsVision() const;

        static const char* tileTypeToString(TileType t);
        static TileType nextTileType(TileType t);
        static int nextTileFullness(int f);
        static std::string meshNameFromFullness(TileType t, int fullnessMeshNumber);

        void refreshMesh();
        void createMesh();
        void destroyMesh();
        void deleteYourself();

        void setSelected(bool s);
        bool getSelected() const;

        void setMarkedForDigging(bool s, Player *p);
        void setMarkedForDiggingForAllSeats(bool s);
        bool getMarkedForDigging(Player *p);

        void addCreature(Creature *c);
        void removeCreature(Creature *c);
        unsigned numCreaturesInCell() const;
        Creature* getCreature(int index);

        void addPlayerMarkingTile(Player *p);
        void removePlayerMarkingTile(Player *p);
        unsigned numPlayersMarkingTile() const;
        Player* getPlayerMarkingTile(int index);

        void addNeighbor(Tile *n);Tile* getNeighbor(unsigned index);
        std::vector<Tile*> getAllNeighbors();

        double claimForColor(int nColor, double nDanceRate);
        double digOut(double digRate, bool doScaleDigRate = false);
        double scaleDigRate(double digRate);

        Room* getCoveringRoom();
        void setCoveringRoom(Room *r);
		bool getCoveringTrap() const;
		void setCoveringTrap(bool t);

        bool isDiggable() const;
        bool isClaimable() const;
        bool isBuildableUpon() const;

        static const char* getFormat();
        friend std::ostream& operator<<(std::ostream& os, Tile *t);
        friend std::istream& operator>>(std::istream& is, Tile *t);

        int getColor() const;
        void setColor(int nColor);

        void setGameMap(GameMap* gameMap);

        // Public datamembers
        //Vector3 location;
        int x, y;
        double colorDouble;
        int floodFillColor;
        Ogre::Real rotation;
        std::string name;

    private:
        void setFullnessValue(double f);
        // Private datamembers
        TileType type;
        bool selected, markedForDigging;
        
        double fullness;
        int fullnessMeshNumber;
        bool meshesExist;
        
        std::vector<Tile*> neighbors;
        std::vector<Creature*> creaturesInCell;
        std::vector<Player*> playersMarkingTile;
        Room *coveringRoom;
        bool coveringTrap;
        MapLight *claimLight;
        
        mutable sem_t fullnessLockSemaphore;
        mutable sem_t creaturesInCellLockSemaphore;
        mutable sem_t coveringRoomLockSemaphore;
        sem_t claimLightLockSemaphore;
        sem_t neighborsLockSemaphore;
        
        GameMap* gameMap;

        int color;
};

#endif

