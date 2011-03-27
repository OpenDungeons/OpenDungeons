#ifndef TILE_H
#define TILE_H

#include <string>
#include <vector>
#include <ostream>
#include <istream>
#include <OgreSceneManager.h>
#include <semaphore.h>



class Tile;
class Creature;
class Player;
class Room;
class MapLight;

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
        TileType getType();

        void setFullness(double f);
        double getFullness();
        int getFullnessMeshNumber();
        TileClearType getTilePassability();
        bool permitsVision();

        static const char* tileTypeToString(TileType t);
        static TileType nextTileType(TileType t);
        static int nextTileFullness(int f);
        static std::string meshNameFromFullness(TileType t, float fullnessMeshNumber);

        void refreshMesh();
        void createMesh();
        void destroyMesh();
        void deleteYourself();

        void setSelected(bool s);
        bool getSelected();

        void setMarkedForDigging(bool s, Player *p);
        void setMarkedForDiggingForAllSeats(bool s);
        bool getMarkedForDigging(Player *p);

        void addCreature(Creature *c);
        void removeCreature(Creature *c);
        unsigned numCreaturesInCell();
        Creature* getCreature(int index);

        void addPlayerMarkingTile(Player *p);
        void removePlayerMarkingTile(Player *p);
        unsigned numPlayersMarkingTile();
        Player* getPlayerMarkingTile(int index);

        void addNeighbor(Tile *n);Tile* getNeighbor(unsigned index);
        std::vector<Tile*> getAllNeighbors();

        double claimForColor(int nColor, double nDanceRate);
        double digOut(double digRate, bool doScaleDigRate = false);
        double scaleDigRate(double digRate);

        Room* getCoveringRoom();
        void setCoveringRoom(Room *r);

        bool isDiggable();
        bool isClaimable();

        static std::string getFormat();
        friend std::ostream& operator<<(std::ostream& os, Tile *t);
        friend std::istream& operator>>(std::istream& is, Tile *t);

        int getColor();
        void setColor(int nColor);

        // Public datamembers
        //Vector3 location;
        int x, y;
        double colorDouble;
        int floodFillColor;
        Ogre::Real rotation;
        std::string name;

    private:
        // Private datamembers
        TileType type;
        bool selected, markedForDigging;
        double fullness;
        sem_t fullnessLockSemaphore;
        int fullnessMeshNumber;
        std::vector<Tile*> neighbors;
        sem_t neighborsLockSemaphore;
        std::vector<Creature*> creaturesInCell;
        sem_t creaturesInCellLockSemaphore;
        std::vector<Player*> playersMarkingTile;
        Room *coveringRoom;
        sem_t coveringRoomLockSemaphore;
        MapLight *claimLight;
        sem_t claimLightLockSemaphore;
        bool meshesExist;

        int color;
};

#endif

