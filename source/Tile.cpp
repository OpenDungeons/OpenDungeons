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

#include "Tile.h"

#include "ODServer.h"
#include "ServerNotification.h"
#include "Creature.h"
#include "GameMap.h"
#include "RenderRequest.h"
#include "MapLight.h"
#include "Seat.h"
#include "SoundEffectsHelper.h"
#include "RenderManager.h"
#include "Player.h"
#include "Helper.h"
#include "LogManager.h"

#include <cstddef>
#include <bitset>

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#define snprintf_is_banned_in_OD_code _snprintf
#endif

static Ogre::MeshManager *myOgreMeshManger = Ogre:: MeshManager::getSingletonPtr();

void Tile::createMeshLocal()
{
    GameEntity::createMeshLocal();

    if(getGameMap()->isServerGameMap())
        return;

    RenderRequest* request = new RenderRequest;
    request->type   = RenderRequest::createTile;
    request->p = static_cast<void*>(this);
    RenderManager::queueRenderRequest(request);
}

void Tile::destroyMeshLocal()
{
    GameEntity::destroyMeshLocal();

    if(getGameMap()->isServerGameMap())
        return;

    RenderRequest* request = new RenderRequest;
    request->type = RenderRequest::destroyTile;
    request->p = static_cast<void*>(this);
    RenderManager::queueRenderRequest(request);
}

void Tile::deleteYourselfLocal()
{
    GameEntity::deleteYourselfLocal();
    if(getGameMap()->isServerGameMap())
        return;

    RenderRequest* request = new RenderRequest;
    request->type = RenderRequest::deleteTile;
    request->p = static_cast<void*>(this);
    RenderManager::queueRenderRequest(request);
}

void Tile::setType(TileType t)
{
    // If the type has changed from its previous value we need to see if
    // the mesh should be updated
    if (t != type)
    {
        type = t;
        refreshMesh();
    }
}

void Tile::setFullness(double f)
{
    int oldFullness = getFullness();
    int oldFullnessMeshNumber = fullnessMeshNumber;
    TileClearType oldTilePassability = getTilePassability();

    fullness = f;

    // If the tile was marked for digging and has been dug out, unmark it and set its fullness to 0.
    if (fullness < 1 && isMarkedForDiggingByAnySeat() == true)
    {
        setMarkedForDiggingForAllSeats(false);
        fullness = 0.0;

        //Play block destroy sound
        //TODO - play this if it's marked for any player.
        SoundEffectsHelper::getSingleton().playBlockDestroySound(x, y);
    }

    /*
     if(fullness < 1 && gameMap.)
     {
     SoundEffectsHelper::getSingleton().playBlockDestroySound(x, y);
     }
     */

    // If the passability has changed we may have opened up new paths on the gameMap.
    if (oldTilePassability != getTilePassability())
    {
        // Do a flood fill to update the contiguous region touching the tile.
        getGameMap()->doFloodFill(x, y);
    }

    // 		4 0 7		    180
    // 		2 8 3		270  .  90
    // 		7 1 5		     0
    //
    bool fillStatus[9];
    Tile *tempTile = getGameMap()->getTile(x, y + 1);
    fillStatus[0] = (tempTile != NULL) ? tempTile->getFullness() > 0.1 : false;
    tempTile = getGameMap()->getTile(x, y - 1);
    fillStatus[1] = (tempTile != NULL) ? tempTile->getFullness() > 0.1 : false;
    tempTile = getGameMap()->getTile(x - 1, y);
    fillStatus[2] = (tempTile != NULL) ? tempTile->getFullness() > 0.1 : false;
    tempTile = getGameMap()->getTile(x + 1, y);
    fillStatus[3] = (tempTile != NULL) ? tempTile->getFullness() > 0.1 : false;

    int fullNeighbors = 0;
    if (fillStatus[0])
        ++fullNeighbors;
    if (fillStatus[1])
        ++fullNeighbors;
    if (fillStatus[2])
        ++fullNeighbors;
    if (fillStatus[3])
        ++fullNeighbors;

    //FIXME:  This needs to be updated to reflect the allowable fill states for each tile type
    // This is also where the logic for checking neighboring fullness should go
    fullnessMeshNumber = 0;
    if (f > 0 && f <= 25)
        fullnessMeshNumber = 25;
    else if (f > 25 && f <= 50)
    {
        fullnessMeshNumber = 50;
        switch (fullNeighbors)
        {
            case 1:
                fullnessMeshNumber = 51;
                if (fillStatus[0])
                {
                    rotation = 270;
                    break;
                }//correct
                if (fillStatus[1])
                {
                    rotation = 90;
                    break;
                }//correct
                if (fillStatus[2])
                {
                    rotation = 0;
                    break;
                }//correct
                if (fillStatus[3])
                {
                    rotation = 180;
                    break;
                }//correct
                break;

            case 2:
                fullnessMeshNumber = 52;
                if (fillStatus[0] && fillStatus[2])
                {
                    rotation = 270;
                    break;
                }//correct
                if (fillStatus[0] && fillStatus[3])
                {
                    rotation = 180;
                    break;
                }//correct
                if (fillStatus[1] && fillStatus[2])
                {
                    rotation = 0;
                    break;
                }//correct
                if (fillStatus[1] && fillStatus[3])
                {
                    rotation = 90;
                    break;
                }//correct

                //TODO:  These next two options are for when the half full tile is in the middle of a wall, the need a separate mesh to be made.
                if (fillStatus[0] && fillStatus[1])
                {
                    fullnessMeshNumber = 51;
                    rotation = 0;
                    break;
                }//correct
                if (fillStatus[2] && fillStatus[3])
                {
                    fullnessMeshNumber = 51;
                    rotation = 90;
                    break;
                }//correct
                break;
        }
    }

    else if (f > 50 && f <= 75)
    {
        fullnessMeshNumber = 75;
        switch (fullNeighbors)
        {
            case 1:
                if (fillStatus[0])
                {
                    rotation = 270;
                    break;
                }//correct
                if (fillStatus[1])
                {
                    rotation = 90;
                    break;
                }//correct
                if (fillStatus[2])
                {
                    rotation = 0;
                    break;
                }//correct
                if (fillStatus[3])
                {
                    rotation = 180;
                    break;
                }//correct
                break;
        }
    }
    else if (f > 75)
    {
        switch (fullNeighbors)
        {
            //TODO:  Determine the rotation for each of these case statements
            int tempInt;
            case 0:
                fullnessMeshNumber = 104;
                rotation = 0;
                break;

            case 1:
                fullnessMeshNumber = 103;
                if (fillStatus[0])
                {
                    rotation = 180;
                    break;
                }//correct
                if (fillStatus[1])
                {
                    rotation = 0;
                    break;
                }//correct
                if (fillStatus[2])
                {
                    rotation = 270;
                    break;
                }//correct
                if (fillStatus[3])
                {
                    rotation = 90;
                    break;
                }//correct
                break;

            case 2:
                tempInt = 0;
                if (fillStatus[0])
                    tempInt += 1;
                if (fillStatus[1])
                    tempInt += 2;
                if (fillStatus[2])
                    tempInt += 4;
                if (fillStatus[3])
                    tempInt += 8;

                switch (tempInt)
                {
                    case 5:
                        fullnessMeshNumber = 52;
                        rotation = 270;
                        break;

                    case 6:
                        fullnessMeshNumber = 52;
                        rotation = 0;
                        break;

                    case 9:
                        fullnessMeshNumber = 52;
                        rotation = 180;
                        break;

                    case 10:
                        fullnessMeshNumber = 52;
                        rotation = 90;
                        break;

                    case 3:
                        fullnessMeshNumber = 102;
                        rotation = 0.0;
                        break;

                    case 12:
                        fullnessMeshNumber = 102;
                        rotation = 90.0;
                        break;

                    default:
                        std::cerr
                                << "\n\nERROR:  Unhandled case statement in Tile::setFullness(), exiting.  tempInt = "
                                << tempInt << "\n\n";
                        exit(1);
                        break;
                }
                break;

            case 3:
                fullnessMeshNumber = 101; //this is wrong for now it should be 101
                if (!fillStatus[0])
                {
                    rotation = 90;
                    break;
                }//correct
                if (!fillStatus[1])
                {
                    rotation = 270;
                    break;
                }
                if (!fillStatus[2])
                {
                    rotation = 180;
                    break;
                }
                if (!fillStatus[3])
                {
                    rotation = 0;
                    break;
                }
                break;

            case 4:
                fullnessMeshNumber = 100;
                rotation = 0;
                break;

            default:
                std::cerr
                        << "\n\nERROR:  fullNeighbors != 0 or 1 or 2 or 3 or 4.  This is impossible, exiting program.\n\n";
                exit(1);
                break;
        }

    }

    // If the mesh has changed it means that a new path may have opened up.
    if (oldFullnessMeshNumber != fullnessMeshNumber)
    {
        refreshMesh();
    }
}

void Tile::setFullnessValue(double f)
{
    fullness = f;
}

double Tile::getFullness() const
{
    return fullness;
}

int Tile::getFullnessMeshNumber() const
{
    return fullnessMeshNumber;
}

Tile::TileClearType Tile::getTilePassability() const
{
    // Check to see if the tile is filled in.
    if (getFullness() >= 1)
        return impassableTile;

    //Check to see if there is a room with objects covering this tile preventing creatures from walking through it.
    //FIXME: The second portion of this if statement throws a segfault.  Something is incorrectly setting the coveringRoom.
    //NOTE: If this code is turned back on the coveringRoom variable is protected by a LockSemaphore.
    //if(coveringRoom != NULL)// && !coveringRoom->tileIsPassable(this))
    //return impassableTile;

    switch (type)
    {
        case nullTileType:
            return impassableTile;
            break;

        case dirt:
        case gold:
        case rock:
        case claimed:
            return walkableTile;
            break;

        case water:
        case lava:
            return flyableTile;
            break;

        default:
            break;
    }

    return impassableTile;
}

bool Tile::permitsVision() const
{
    TileClearType clearType = getTilePassability();
    return (clearType == walkableTile || clearType == flyableTile);
}

bool Tile::isBuildableUpon() const
{
    if(type != claimed || getFullness() > 0.01 || coveringTrap == true)
        return false;

    return (coveringRoom == NULL);
}

bool Tile::getCoveringTrap() const
{
    return coveringTrap;
}

void Tile::setCoveringTrap(bool t)
{
    coveringTrap = t;
}

Room* Tile::getCoveringRoom()
{
    return coveringRoom;
}

void Tile::setCoveringRoom(Room *r)
{
    coveringRoom = r;

    // Set the tile as claimed and of the team color of the room
    if (coveringRoom == NULL)
    {
        setColor(0);
        colorDouble = 0.0;
        setType(dirt);
        return;
    }

    setColor(coveringRoom->getColor());
    colorDouble = 1.0;
    setType(claimed);
}

bool Tile::isDiggable(int team_color_id) const
{
    if (getFullness() < 1)
        return false;

    // Return true for common types.
    if (type == dirt || type == gold)
        return true;

    // Return false for undiggable types.
    if (type == lava || type == water || type == rock)
        return false;

    if (type != claimed)
        return false;

    // type == claimed

    // For claimed walls, we check whether the walls is either claimed by the given player,
    if (getColor() == team_color_id && colorDouble > 0.99)
        return true;

    // or whether it isn't belonging to a specific team.
    if (colorDouble <= 0.0)
        return true;

    return false;
}

bool Tile::isGroundClaimable() const
{
    return ((type == dirt || type == gold || type == claimed) && getFullness() < 1);
}

bool Tile::isWallClaimable(int team_color_id)
{
    if (getFullness() < 1)
        return false;

    if (type == lava || type == water || type == rock || type == gold)
        return false;

    // Check whether at least one neighbor is a claimed ground tile of the given color
    // which is a condition to permit claiming the given wall tile.
    bool foundClaimedGroundTile = false;
    for (unsigned int j = 0; j < neighbors.size(); ++j)
    {
        if (neighbors[j]->getFullness() > 1)
            continue;

        if (neighbors[j]->getType() == claimed
                && neighbors[j]->colorDouble >= 1.0
                && neighbors[j]->getColor() == team_color_id)
        {
            foundClaimedGroundTile = true;
            break;
        }
    }

    if (foundClaimedGroundTile == false)
        return false;

    if (type == dirt)
        return true;

    if (type != claimed)
        return false;

    // type == claimed

    // For claimed walls, we check whether it isn't belonging completely to a specific team.
    if (colorDouble <= 0.99)
        return true;

    // Or whether the wall tile is either claimed by the given player entirely already,
    if (getColor() == team_color_id) // NOTE: colorDouble >= 1.0 here
        return false; // Already claimed.

    // Or whether the enemy player that claimed the wall tile has got any ground tiles permitting to keep claiming that wall tile.
    foundClaimedGroundTile = false;
    int enemy_color_id = getColor(); // NOTE: != team_color_id here.
    for (unsigned int j = 0; j < neighbors.size(); ++j)
    {
        if (neighbors[j]->getFullness() > 1)
            continue;

        if (neighbors[j]->getType() == claimed
                && neighbors[j]->colorDouble >= 1.0
                && neighbors[j]->getColor() == enemy_color_id)
        {
            foundClaimedGroundTile = true;
            break;
        }
    }

    if (!foundClaimedGroundTile)
        return true;

    return false;
}

bool Tile::isWallClaimedForColor(int team_color_id)
{
    if (getFullness() < 1)
        return false;

    if (type != claimed)
        return false;

    if (colorDouble <= 0.99)
        return false;

    if (getColor() != team_color_id)
        return false;

    return true;
}

const char* Tile::getFormat()
{
    return "posX\tposY\ttype\tfullness";
}

ODPacket& operator<<(ODPacket& os, Tile *t)
{
    int intType =static_cast<Tile::TileType>(t->getType());
    os << t->getColor() << t->x << t->y << intType
       << t->getFullness();

    return os;
}

ODPacket& operator>>(ODPacket& is, Tile *t)
{
    int tempInt, xLocation, yLocation;
    double tempDouble;
    std::stringstream ss;

    is >> tempInt;
    t->setColor(tempInt);

    is >> xLocation >> yLocation;

    ss.str(std::string());
    ss << "Level";
    ss << "_";
    ss << xLocation;
    ss << "_";
    ss << yLocation;

    t->setName(ss.str());
    t->x = xLocation;
    t->y = yLocation;

    is >> tempInt;
    t->setType(static_cast<Tile::TileType>(tempInt));

    is >> tempDouble;
    t->setFullnessValue(tempDouble);

    return is;
}

void Tile::loadFromLine(const std::string& line, Tile *t)
{
    std::vector<std::string> elems = Helper::split(line, '\t');

    int xLocation = Helper::toInt(elems[0]);
    int yLocation = Helper::toInt(elems[1]);

    std::stringstream tileName("");
    tileName << "Level";
    tileName << "_";
    tileName << xLocation;
    tileName << "_";
    tileName << yLocation;

    t->setName(tileName.str());
    t->x = xLocation;
    t->y = yLocation;

    t->setType((Tile::TileType) Helper::toInt(elems[2]));

    t->setFullnessValue(Helper::toDouble(elems[3]));
    //std::cout << "Tile: " << xLocation << ", " << yLocation << ", " << t->getName()
    //<< ", " << t->getFullness() << std::endl;
}

std::string Tile::tilePassabilityToString(TileClearType t)
{
    switch(t)
    {
        default:
        case impassableTile:
            return "impassableTile";
        case walkableTile:
            return "walkableTile";
        case flyableTile:
            return "flyableTile";
    }
}

Tile::TileClearType Tile::tilePassabilityFromString(const::string& t)
{
    if (t == "flyableTile")
        return flyableTile;
    else if (t == "walkableTile")
        return walkableTile;

    return impassableTile;
}

std::string Tile::tileTypeToString(TileType t)
{
    switch (t)
    {
        default:
        case dirt:
            return "Dirt";

        case rock:
            return "Rock";

        case gold:
            return "Gold";

        case water:
            return "Water";

        case lava:
            return "Lava";

        case claimed:
            return "Claimed";
    }
}

Tile::TileType Tile::nextTileType(TileType t)
{
    return static_cast<TileType>((static_cast<int>(t) + 1)
            % static_cast<int>(claimed));
}

int Tile::nextTileFullness(int f)
{

    // Cycle the tile's fullness through the possible values
    switch (f)
    {
        case 0:
            return 25;

        case 25:
            return 50;

        case 50:
            return 75;

        case 75:
            return 100;

        case 100:
            return 0;

        default:
            return 0;
    }
}

std::string Tile::meshNameFromNeighbors(TileType myType, int fullnessMeshNumber,
                                        const TileType* neighbors, const bool* neighborsFullness, int& rt)
{
    std::stringstream ss;
    //FIXME - define postfix somewhere
    int postfixInt = 0;
    unsigned char shiftedAroundBits;

    // get the integer from neighbors[], using it as a number coded in binary base
    for(int ii = 8; ii >=1; ii--)
    {
        postfixInt *=2;
        postfixInt += ((neighbors[(ii)%8] == myType &&  (!(myType!=water && myType!=lava)  || neighborsFullness[(ii)%8]  )) // || (!(myType!=water && myType!=lava) ||  fullnessMeshNumber > 0   )
                      );
    }

    int storedInt = postfixInt;
    // current implementation does not allow on separate corner tiles
    // leave only those corner tiles  ( the one in  the even position in PostfixInt binary base ) who have at least one ver or hor neighbor

    // shift the 8ht position bit to the 1st position, shifting the rest 1 position to the left .
    shiftedAroundBits = postfixInt &  0x80;
    postfixInt <<= 1;
    shiftedAroundBits >>= 7;
    shiftedAroundBits &= 0x01;
    postfixInt &= 0xFF;
    postfixInt += shiftedAroundBits;

    // check for the clockwise rotation hor or ver neighbor for diagonal tile
    int foobar = postfixInt;

    shiftedAroundBits = postfixInt &  0x03;
    postfixInt >>= 2;
    postfixInt &= 0x3F;
    shiftedAroundBits <<= 6;

    postfixInt += shiftedAroundBits;

    // check for the anti - clockwise rotation hor or ver neighbor of a diagonal tile
    int foobar2 = postfixInt;

    // 85  == 01010101b
    // 170 == 10101010b
    postfixInt = (foobar & foobar2 & 85) | (storedInt & 170);

    // Naros	rather than simply using getByName() and testing if MeshPtr.isNull() is true
    // 	Naros	if its null, then load it.
    // Naros	you want to use Ogre::ResourceGroupManager::getSingletonPtr()->resourceExists("MyResourceGroupName", "MyMeshFileName");
    meshNameAux(ss, postfixInt, fullnessMeshNumber, myType);

    // rotate the postfix number, as long , as we won't find Exisitng mesh

    // cerr <<  ss.str() << endl ;
    for(rt = 0; !Ogre::ResourceGroupManager::getSingletonPtr()->resourceExists("Graphics", ss.str()) && rt < 4; rt++)
    {
        shiftedAroundBits = postfixInt &  0xC0;
        postfixInt <<= 2;
        postfixInt &= 0xFF;
        shiftedAroundBits >>= 6;
        shiftedAroundBits &= 0x03;
        postfixInt += shiftedAroundBits;

        ss.str("");
        ss.clear();

        meshNameAux(ss, postfixInt, fullnessMeshNumber, myType);

        // cerr <<  ss.str()<< endl ;
    }

    //DEBUG find the name of the missing mesh
    // Bad hack to workaround a bug with the file Dirt_10001111.mesh
    // Since the corresponding file used must be turned by 180°, this ugly hack handles the rotation
    // manually.
    if (neighbors[0] == myType && neighbors[2] == myType && neighbors[4] == myType
        && (neighbors[6] != myType || !neighborsFullness[6])
        && neighborsFullness[0] && neighborsFullness[2] && neighborsFullness[4])
    {
        if (myType == Tile::dirt || myType == Tile::gold || myType == Tile::rock)
            rt = 2;
    }

    return ss.str();
}

std::string Tile::meshNameFromFullness(TileType t, int fullnessMeshNumber)
{
    std::stringstream ss;
    //FIXME - define postfix somewhere
    ss << tileTypeToString(t) << "_" << (fullnessMeshNumber > 0 ?  "11111111" : "00000000" ) << ".mesh";
    return ss.str();
}

void Tile::meshNameAux(std::stringstream &ss, int &postfixInt, int& fMN, TileType myType)
{
    ss << tileTypeToString( (myType == rock || myType == gold ) ? dirt : (myType == lava) ? water : myType  ) << "_"
       << (fMN > 0 ?  std::bitset<8>( postfixInt ).to_string() : ( (myType == water || myType == lava) ? std::bitset<8>( postfixInt ).to_string() : "0"  ))  << ".mesh";
}

void Tile::refreshMesh()
{
    if (!isMeshExisting())
        return;

    if(getGameMap()->isServerGameMap())
        return;

    RenderRequest *request = new RenderRequest;
    request->type = RenderRequest::refreshTile;
    request->p = static_cast<void*>(this);
    RenderManager::queueRenderRequest(request);
}

void Tile::setMarkedForDigging(bool ss, Player *pp)
{
    /* If we are trying to mark a tile that is not dirt or gold
     * or is already dug out, ignore the request.
     */
    if (ss && !isDiggable(pp->getSeat()->getColor()))
        return;

    // If the tile was already in the given state, we can return
    if (getMarkedForDigging(pp) == ss)
        return;

    if (ss)
        addPlayerMarkingTile(pp);
    else
        removePlayerMarkingTile(pp);

    refreshMesh();
}

void Tile::setSelected(bool ss, Player* pp)
{
    if (selected != ss)
    {
        selected = ss;

        RenderRequest *request = new RenderRequest;

        request->type = RenderRequest::temporalMarkTile;
        request->p = static_cast<void*>(this);
        request->p2 = static_cast<void*>(pp);

        // Add the request to the queue of rendering operations to be performed before the next frame.
        RenderManager::queueRenderRequest(request);
    }
}

void Tile::setMarkedForDiggingForAllSeats(bool s)
{
    setMarkedForDigging(s, getGameMap()->getLocalPlayer());

    for (unsigned int i = 0, num = getGameMap()->numPlayers(); i < num; ++i)
        setMarkedForDigging(s, getGameMap()->getPlayer(i));
}

bool Tile::getMarkedForDigging(Player *p)
{
    // Loop over any players who have marked this tile and see if 'p' is one of them
    for (unsigned int i = 0, size = playersMarkingTile.size(); i < size; ++i)
    {
        if (playersMarkingTile[i] == p)
        {
            return true;
        }
    }

    return false;
}

bool Tile::isMarkedForDiggingByAnySeat()
{
    return !playersMarkingTile.empty();
}

void Tile::addCreature(Creature *c)
{
    creaturesInCell.push_back(c);
}

void Tile::removeCreature(Creature *c)
{
    // Check to see if the given crature is actually in this tile
    std::vector<Creature*>::iterator itr;
    for (itr = creaturesInCell.begin(); itr != creaturesInCell.end(); ++itr)
    {
        if ((*itr) == c)
        {
            // Remove the creature from the list
            creaturesInCell.erase(itr);
            break;
        }
    }
}

unsigned int Tile::numCreaturesInCell() const
{
    return creaturesInCell.size();
}

Creature* Tile::getCreature(unsigned int index)
{
    Creature* creature = NULL;
    if (index < creaturesInCell.size())
        creature = creaturesInCell[index];

    return creature;
}

void Tile::addPlayerMarkingTile(Player *p)
{
    playersMarkingTile.push_back(p);
}

void Tile::removePlayerMarkingTile(Player *p)
{
    for (unsigned int i = 0; i < playersMarkingTile.size(); ++i)
    {
        if (p == playersMarkingTile[i])
        {
            playersMarkingTile.erase(playersMarkingTile.begin() + i);
            return;
        }
    }
}

unsigned int Tile::numPlayersMarkingTile() const
{
    return playersMarkingTile.size();
}

Player* Tile::getPlayerMarkingTile(int index)
{
    return playersMarkingTile[index];
}

void Tile::addNeighbor(Tile *n)
{
    neighbors.push_back(n);
}

void Tile::claimForColor(int nColor, double nDanceRate)
{
    double amountClaimed;

    //std::cout << "Claiming for color" << std::endl;
    // If the color is the same as ours we add to it, if it is an enemy color we subtract from it.
    if (nColor == getColor())
    {
        amountClaimed = std::min(nDanceRate, 1.0 - colorDouble);
        //std::cout << "Claiming: Same color" << std::endl;
        colorDouble += nDanceRate;
        if (colorDouble >= 1.0)
        {
            claimTile(nColor);

            // We inform the clients that the tile has been claimed
            if(getGameMap()->isServerGameMap())
            {
                try
                {
                    // Inform the clients that the fullness has changed.
                    ServerNotification *serverNotification = new ServerNotification(
                        ServerNotification::tileClaimed, getGameMap()->getPlayerByColor(nColor));
                    serverNotification->packet << this;

                    ODServer::getSingleton().queueServerNotification(serverNotification);
                }
                catch (std::bad_alloc&)
                {
                    std::cerr << "\n\nERROR:  bad alloc in Tile::setFullness\n\n";
                    exit(1);
                }
            }

            //std::cout << "Claiming: color complete" << std::endl;
        }
    }
    else
    {
        //std::cout << "Claiming: different color" << std::endl;
        amountClaimed = std::min(nDanceRate, 1.0 + colorDouble);
        colorDouble -= nDanceRate;
        if (colorDouble <= 0.0)
        {
            // The tile is not yet claimed, but it is now our color.
            colorDouble *= -1.0;
            setColor(nColor);

            if (colorDouble >= 1.0)
            {
                colorDouble = 1.0;
                refreshMesh();

                // Force all the neighbors to recheck their meshes as we have updated this tile.
                for (unsigned int j = 0; j < neighbors.size(); ++j)
                {
                    neighbors[j]->refreshMesh();
                    // Update potential active spots.
                    Room* room = neighbors[j]->getCoveringRoom();
                    if (room != NULL)
                    {
                        room->updateActiveSpots();
                        room->createMesh();
                    }
                }
            }
        }
    }

    /*
    // TODO: This should rather add lights along claimed walls, and not on every walls. Maybe each 5 ones?
    // If this is the first time this tile has been claimed, emit a flash of light indicating that the tile was claimed.
    if (amountClaimed > 0.0 && claimLight == NULL)
    {
        //Disabled claim lights for now, as they make things look rather ugly
        //and hamper performance.
        Ogre::ColourValue tempColour =
                gameMap->getSeatByColor(nColor)->colourValue;

        claimLight = new TemporaryMapLight(Ogre::Vector3(x, y, 0.5),
                tempColour.r, tempColour.g, tempColour.b, 1.0, 0.1, 0.5, 0.5);
        gameMap->addMapLight(claimLight);
        claimLight->createOgreEntity();
        SoundEffectsHelper::getSingleton().playInterfaceSound(
                SoundEffectsHelper::CLAIM, this->x, this->y);
    }
    */
}

void Tile::claimTile(int nColor)
{
    // Claim the tile.
    // We need this because if we are a client, the tile may be
    // from a different color
    setColor(nColor);
    colorDouble = 1.0;
    setType(Tile::claimed);

    refreshMesh();

    // Force all the neighbors to recheck their meshes as we have updated this tile.
    for (unsigned int j = 0; j < neighbors.size(); ++j)
    {
        neighbors[j]->refreshMesh();
        // Update potential active spots.
        Room* room = neighbors[j]->getCoveringRoom();
        if (room != NULL)
        {
            room->updateActiveSpots();
            room->createMesh();
        }
    }
}

double Tile::digOut(double digRate, bool doScaleDigRate)
{
    if (doScaleDigRate)
        digRate = scaleDigRate(digRate);

    double amountDug = 0.0;

    if (getFullness() < 1 || type == lava || type == water || type == rock)
        return 0.0;

    if (digRate >= fullness)
    {
        amountDug = fullness;
        setFullness(0.0);

        // If we are a sever, the clients need to be told about the change to the tile's fullness.
        if (getGameMap()->isServerGameMap())
        {
            try
            {
                // Inform the clients that the fullness has changed.
                ServerNotification *serverNotification = new ServerNotification(
                    ServerNotification::tileFullnessChange, NULL);
                serverNotification->packet << this;
                ODServer::getSingleton().queueServerNotification(serverNotification);
            }
            catch (std::bad_alloc&)
            {
                std::cerr << "\n\nERROR:  bad alloc in Tile::setFullness\n\n";
                exit(1);
            }
        }

        for (unsigned int j = 0; j < neighbors.size(); ++j)
        {
            // Update potential active spots.
            Room* room = neighbors[j]->getCoveringRoom();
            if (room != NULL)
            {
                room->updateActiveSpots();
                room->createMesh();
            }
        }
    }
    else
    {
        amountDug = digRate;
        setFullness(fullness - digRate);
    }

    if (amountDug > 0.0 && amountDug < digRate)
    {
        double amountToDig = digRate - amountDug;
        if (amountToDig < 0.05)
            return amountDug;

        amountToDig /= (double) neighbors.size();
        for (unsigned int j = 0; j < neighbors.size(); ++j)
        {
            if (neighbors[j]->getType() == dirt)
            {
                Tile* tempTile = neighbors[j];
                // Release and relock the semaphore since the digOut() routine will eventually need to lock it.
                amountDug += tempTile->digOut(amountToDig);
            }
        }
    }

    return amountDug;
}

double Tile::scaleDigRate(double digRate)
{
    switch (type)
    {
        case claimed:
            return 0.2 * digRate;

        default:
            return digRate;
    }
}

Tile* Tile::getNeighbor(unsigned int index)
{
    return neighbors[index];
}

std::vector<Tile*> Tile::getAllNeighbors()
{
    return neighbors;
}

