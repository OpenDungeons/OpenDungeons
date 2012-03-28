#ifndef TILE_H
#define TILE_H

#include <string>
#include <vector>
#include <ostream>
#include <istream>

#include <semaphore.h>
#include <OgrePrerequisites.h>

#include "GameEntity.h"

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
class Tile : public GameEntity
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

        Tile( int         nX          = 0,
              int         nY          = 0,
              TileType    nType       = dirt,
              double      nFullness   = 100.0
            ) :
                x                   (nX),
                y                   (nY),
                colorDouble         (0.0),
                floodFillColor      (-1),
                rotation            (0.0),
                type                (nType),
                selected            (false),
                fullness            (nFullness),
                fullnessMeshNumber  (-1),
                coveringRoom        (0),
                coveringTrap        (false),
                claimLight          (0),
                gameMap             (0)
        {
            sem_init(&creaturesInCellLockSemaphore, 0, 1);
            sem_init(&fullnessLockSemaphore, 0, 1);
            sem_init(&coveringRoomLockSemaphore, 0, 1);
            sem_init(&neighborsLockSemaphore, 0, 1);
            sem_init(&claimLightLockSemaphore, 0, 1);

            setColor(0);
            setObjectType(GameEntity::tile);
        }

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

        void setSelected(bool s);
        bool getSelected() const;

        void setMarkedForDigging(bool s, Player *p);
        void setMarkedForDiggingForAllSeats(bool s);
        bool getMarkedForDigging(Player *p);
        bool isMarkedForDiggingByAnySeat();

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

        void setGameMap(GameMap* gameMap);

        int getX() const {return x;}
        int getY() const {return y;}

        int x, y;
        double colorDouble;
        int floodFillColor;
        Ogre::Real rotation;

        //TODO properly implement these
        bool doUpkeep(){ return true; }
        void recieveExp(double experience){}
        double getDefense() const  { return 0; }
        void takeDamage(double damage, Tile *tileTakingDamage) {}
        double getHP(Tile *tile) {return 0;}
        std::vector<Tile*> getCoveredTiles() { return std::vector<Tile*>() ;}

    private:
        void setFullnessValue(double f);
        // Private datamembers
        TileType type;
        bool selected;
        
        double fullness;
        int fullnessMeshNumber;
        
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
};

#endif

