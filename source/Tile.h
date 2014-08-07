/*
 *  Copyright (C) 2011-2014  OpenDungeons Team
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TILE_H
#define TILE_H

#include "GameEntity.h"
#include "ODPacket.h"

#include <OgrePrerequisites.h>
#include <OgreSceneNode.h>
#include <OgreMeshManager.h>

#include <string>
#include <vector>
#include <ostream>
#include <istream>

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

friend class TileContainersModificator;

public:
    // Changes to this enum must be reflected in Tile::getTilePassability()
    enum TileType
    {
        nullTileType = 0,
        dirt = 1,
        gold = 2,
        rock = 3,
        water = 4,
        lava = 5,
        //if the last type is changed it must also substitute the type
        //  referenced in Tile::nextTileType()
        claimed = 6
    };

    // Changes to this enum must be reflected in Tile::getTilePassability() as well as in GameMap::path()
    enum TileClearType
    {
        impassableTile = 0,
        walkableTile = 1,
        flyableTile = 2,
        diggableTile = 3
    };

    Tile(GameMap* gameMap, int nX = 0, int nY = 0, TileType nType = dirt, double nFullness = 100.0) :
        GameEntity          (gameMap),
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
        claimLight          (0)
    {
        setColor(0);
        setObjectType(GameEntity::tile);
    }

    /*! \brief A mutator to set the type (rock, claimed, etc.) of the tile.
     *
     * In addition to setting the tile type this function also reloads the new mesh
     * for the tile.
     */
    void setType(TileType t);

    //! \brief An accessor which returns the tile type (rock, claimed, etc.).
    TileType getType() const
    {
        return type;
    }

    /*! \brief A mutator to change how "filled in" the tile is.
     *
     * Additionally this function reloads the proper mesh to display to the user
     * how full the tile is.  It also determines the orientation of the
     * tile to make corners display correctly.  Both of these tasks are
     * accomplished by setting the fullnessMeshNumber variable which is
     * concatenated to the tile's type to determine the mesh to load, e.g.
     * Rock104.mesh for a rocky tile which has all 4 sides shown because it is an
     * "island" with all four sides visible.  Claimed102.mesh would be a fully
     * filled in tile but only two sides are drawn because it borders full tiles on
     * 2 sides.
     */
    void setFullness(double f);

    //! \brief An accessor which returns the tile's fullness which should range from 0 to 100.
    double getFullness() const;

    /*! \brief An accessor which returns the tile's fullness mesh number.
     *
     * The fullness mesh number is concatenated to the tile's type to determine the
     * mesh to load to display a given tile type.
     */
    int getFullnessMeshNumber() const;

    /*! \brief Returns the 'passability' state of a tile (impassableTile, walkableTile, etc.).
     *
     * The passability of a tile indicates what type of creatures may move into the
     * given tile.  As an example, no creatures may move into an 'impassableTile'
     * like Dirt100 and only flying creatures may move into a 'flyableTile' like
     * Lava0.
     */
    TileClearType getTilePassability() const;

    //! \brief Tells whether a creature can see through a tile
    bool permitsVision() const;

    /*! \brief This is a helper function to scroll through the list of available tile types.
     *
     * This function is used in the map editor when the user presses the button to
     * select the next tile type to be active in the user interface.  The active
     * tile type is the one which is placed when the user clicks the mouse button.
     */
    static TileType nextTileType(TileType t);

    /*! \brief This is a helper function to scroll through the list of available fullness levels.
     *
     * This function is used in the map editor when the user presses the button to
     * select the next tile fullness level to be active in the user interface.  The
     * active fullness level is the one which is placed when the user clicks the
     * mouse button.
     */
    static int nextTileFullness(int f);

    //! \brief This is a helper function that generates a mesh filename from a tile type and a fullness mesh number.
    static std::string meshNameFromFullness(TileType t, int fullnessMeshNumber);

    //! \brief This is a helper function that generates a mesh filename from a tile type and a fullness mesh number.
    //! \TODO Define what is a postfix.
    static std::string meshNameFromNeighbors(TileType myType, int fullnessMeshNumber, const TileType* neighbors,
                                             const bool* neighborsFullness, int &rt);

    static void meshNameAux(std::stringstream &ss, int &postfixInt, int& fMN, TileType myType);

    //! \brief This function puts a message in the renderQueue to change the mesh for this tile.
    void refreshMesh();

    //! \brief This function marks the tile as being selected through a mouse click or drag.
    void setSelected(bool ss, Player *pp);

    //! \brief This accessor function returns whether or not the tile has been selected.
    bool getSelected() const
    {
        return selected;
    }

    //! \brief Set the tile digging mark for the given player.
    void setMarkedForDigging(bool s, Player *p);

    //! \brief This accessor function returns whether or not the tile has been marked to be dug out by a given Player p.
    bool getMarkedForDigging(Player *p);

    //! \brief This is a simple helper function which just calls setMarkedForDigging() for everyone in the game (including me).
    void setMarkedForDiggingForAllSeats(bool s);

    //! \brief Tells whether the tile is selected for digging by any player/AI.
    bool isMarkedForDiggingByAnySeat();

    //! \brief Add a player to the vector of players who have marked this tile for digging.
    void addPlayerMarkingTile(Player *p);

    void removePlayerMarkingTile(Player *p);
    unsigned numPlayersMarkingTile() const;
    Player* getPlayerMarkingTile(int index);

    //! \brief This function adds a creature to the list of creatures in this tile.
    void addCreature(Creature *c);

    //! \brief This function removes a creature to the list of creatures in this tile.
    void removeCreature(Creature *c);

    //! \brief This function returns the count of the number of creatures in the tile.
    unsigned numCreaturesInCell() const;

    //! \brief This function returns the i'th creature in the tile.
    Creature* getCreature(unsigned int index);

    void addNeighbor(Tile *n);Tile* getNeighbor(unsigned index);
    std::vector<Tile*> getAllNeighbors();

    void claimForColor(int nColor, double nDanceRate);
    void claimTile(int nColor);
    double digOut(double digRate, bool doScaleDigRate = false);
    double scaleDigRate(double digRate);

    Room* getCoveringRoom();
    void setCoveringRoom(Room *r);
    bool getCoveringTrap() const;
    void setCoveringTrap(bool t);

    //! \brief Tells whether the tile is diggable by dig-capable creatures.
    //! \brief The player team color index.
    //! The function will check whether a tile is not already a reinforced wall owned by another team.
    bool isDiggable(int team_color_id) const;

    //! \brief Tells whether the tile fullness is empty (ground tile) and can be claimed.
    bool isGroundClaimable() const;

    //! \brief Tells whether the tile is a wall (fullness > 1) and can be claimed for the given team color.
    //! Reinforced walls by another team and hard rocks can't be claimed.
    bool isWallClaimable(int team_color_id);

    //! \brief Tells whether the given tile is a claimed wall of the given color.
    //! Used to discover active spots for rooms.
    bool isWallClaimedForColor(int team_color_id);

    //! \brief Tells whether a room can be built upon this tile.
    bool isBuildableUpon() const;

    static const char* getFormat();

    //! \brief Loads the tile data from a level line.
    static void loadFromLine(const std::string& line, Tile *t);

    /*! \brief The << operator is used for saving tiles to a file and sending them over the net.
     *
     * This operator is used in conjunction with the >> operator to standardize
     * tile format in the level files, as well as sending tiles over the network.
     */
    friend ODPacket& operator<<(ODPacket& os, Tile *t);

    /*! \brief The >> operator is used for loading tiles from a file and for receiving them over the net.
     *
     * This operator is used in conjunction with the << operator to standardize
     * tile format in the level files, as well as sending tiles over the network.
     */
    friend ODPacket& operator>>(ODPacket& is, Tile *t);

    /*! \brief This is a helper function which just converts the tile type enum into a string.
     *
     * This function is used primarily in forming the mesh names to load from disk
     * for the various tile types.  The name returned by this function is
     * concatenated with a fullnessMeshNumber to form the filename, e.g.
     * Dirt104.mesh is a 4 sided dirt mesh with 100% fullness.
     */
    static std::string tileTypeToString(TileType t);

    //! \brief Gives the tile passability string
    static std::string tilePassabilityToString(TileClearType t);

    //! \brief Gives the tile passability from the given string
    static TileClearType tilePassabilityFromString(const::string& t);

    int getX() const
    { return x; }

    int getY() const
    { return y; }

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

protected:
    virtual void createMeshLocal();
    virtual void destroyMeshLocal();
    virtual void deleteYourselfLocal();
private:
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

    /*! \brief Set the fullness value for the tile.
     *  This only sets the fullness variable. This function is here to change the value
     *  before a map object has been set. setFullness is called once a map is assigned.
     */
    void setFullnessValue(double f);
};

#endif // TILE_H
