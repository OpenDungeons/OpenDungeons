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
#include "SoundEffectsManager.h"
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

    fullness = f;

    // If the tile was marked for digging and has been dug out, unmark it and set its fullness to 0.
    if (fullness == 0.0 && isMarkedForDiggingByAnySeat())
    {
        setMarkedForDiggingForAllSeats(false);
    }

    if ((oldFullness > 0.0) && (fullness == 0.0))
    {
        // Do a flood fill to update the contiguous region touching the tile.
        getGameMap()->refreshFloodFill(this);
    }

    // 		4 0 7		    180
    // 		2 8 3		270  .  90
    // 		7 1 5		     0
    //
    bool fillStatus[9];
    Tile *tempTile = getGameMap()->getTile(x, y + 1);
    fillStatus[0] = (tempTile != NULL) ? tempTile->getFullness() > 0.0 : false;
    tempTile = getGameMap()->getTile(x, y - 1);
    fillStatus[1] = (tempTile != NULL) ? tempTile->getFullness() > 0.0 : false;
    tempTile = getGameMap()->getTile(x - 1, y);
    fillStatus[2] = (tempTile != NULL) ? tempTile->getFullness() > 0.0 : false;
    tempTile = getGameMap()->getTile(x + 1, y);
    fillStatus[3] = (tempTile != NULL) ? tempTile->getFullness() > 0.0 : false;

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

bool Tile::permitsVision() const
{
    return (fullness == 0.0);
}

bool Tile::isBuildableUpon() const
{
    if(type != claimed || getFullness() > 0.0 || coveringTrap != NULL)
        return false;

    return (coveringRoom == NULL);
}

Trap* Tile::getCoveringTrap() const
{
    return coveringTrap;
}

void Tile::setCoveringTrap(Trap* t)
{
    coveringTrap = t;
    mClaimedPercentage = 1.0;
    setType(claimed);
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
        setSeat(NULL);
        mClaimedPercentage = 0.0;
        setType(dirt);
        return;
    }

    setSeat(coveringRoom->getSeat());
    mClaimedPercentage = 1.0;
    setType(claimed);
}

bool Tile::isDiggable(Seat* seat) const
{
    if (getFullness() == 0.0)
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
    if (isClaimedForSeat(seat) && mClaimedPercentage >= 1.0)
        return true;

    // or whether it isn't belonging to a specific team.
    if (mClaimedPercentage <= 0.0)
        return true;

    return false;
}

bool Tile::isGroundClaimable() const
{
    return ((type == dirt || type == gold || type == claimed) && getFullness() == 0.0);
}

bool Tile::isWallClaimable(Seat* seat)
{
    if (getFullness() == 0.0)
        return false;

    if (type == lava || type == water || type == rock || type == gold)
        return false;

    // Check whether at least one neighbor is a claimed ground tile of the given seat
    // which is a condition to permit claiming the given wall tile.
    bool foundClaimedGroundTile = false;
    for (unsigned int j = 0; j < neighbors.size(); ++j)
    {
        if (neighbors[j]->getFullness() > 0.0)
            continue;

        if (neighbors[j]->getType() == claimed
                && neighbors[j]->getClaimedPercentage() >= 1.0
                && neighbors[j]->isClaimedForSeat(seat))
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
    if (mClaimedPercentage < 1.0)
        return true;

    Seat* tileSeat = getSeat();
    if (tileSeat == NULL)
        return true;

    // Or whether the wall tile is either claimed by the given player entirely already,
    // NOTE: mClaimedPercentage >= 1.0 from here
    if (isClaimedForSeat(seat))
        return false; // Already claimed.

    // The wall is claimed by another team.
    // Or whether the enemy player that claimed the wall tile has got any ground tiles permitting to keep claiming that wall tile.
    foundClaimedGroundTile = false;
    for (unsigned int j = 0; j < neighbors.size(); ++j)
    {
        if (neighbors[j]->getFullness() > 0.0)
            continue;

        if (neighbors[j]->getType() == claimed
                && neighbors[j]->mClaimedPercentage >= 1.0
                && neighbors[j]->isClaimedForSeat(tileSeat))
        {
            foundClaimedGroundTile = true;
            break;
        }
    }

    if (!foundClaimedGroundTile)
        return true;

    return false;
}

bool Tile::isWallClaimedForSeat(Seat* seat)
{
    if (getFullness() == 0.0)
        return false;

    if (type != claimed)
        return false;

    if (mClaimedPercentage <= 0.99)
        return false;

    Seat* tileSeat = getSeat();
    if(tileSeat == NULL)
        return false;

    if (tileSeat->canOwnedTileBeClaimedBy(seat))
        return false;

    return true;
}

const char* Tile::getFormat()
{
    return "posX\tposY\ttype\tfullness";
}

std::ostream& operator<<(std::ostream& os, Tile *t)
{
    os << t->x << "\t" << t->y << "\t";
    os << t->getType() << "\t" << t->getFullness();
    Seat* seat = t->getSeat();
    if(t->getType() != Tile::TileType::claimed || seat == NULL)
        return os;

    os << "\t" << seat->getId();

    return os;
}

ODPacket& operator<<(ODPacket& os, Tile *t)
{
    Seat* seat = t->getSeat();
    int seatId = 0;
    // We only pass the seat to the client if the tile is fully claimed
    if((seat != NULL) && (t->mClaimedPercentage >= 1.0))
        seatId = seat->getId();
    int intType =static_cast<Tile::TileType>(t->getType());
    double fullness = t->getFullness();
    os << seatId << t->x << t->y << intType << fullness;

    return os;
}

ODPacket& operator>>(ODPacket& is, Tile *t)
{
    int seatId, intTileType, xLocation, yLocation;
    double fullness;
    std::stringstream ss;

    // We set the seat if there is one
    is >> seatId;

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

    is >> intTileType;
    Tile::TileType tileType = static_cast<Tile::TileType>(intTileType);
    t->setType(tileType);

    is >> fullness;
    t->setFullnessValue(fullness);

    if((tileType != Tile::TileType::claimed) || (seatId == 0))
        return is;
    Seat* seat = t->getGameMap()->getSeatById(seatId);
    if(seat == NULL)
        return is;
    t->setSeat(seat);
    t->mClaimedPercentage = 1.0;
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

    Tile::TileType tileType = static_cast<Tile::TileType>(Helper::toInt(elems[2]));
    t->setType(tileType);

    t->setFullnessValue(Helper::toDouble(elems[3]));

    // If the tile is claimed, there can be an optional parameter with the seat id
    if(tileType != Tile::TileType::claimed || elems.size() < 5)
        return;

    int seatId = Helper::toInt(elems[4]);
    Seat* seat = t->getGameMap()->getSeatById(seatId);
    if(seat == NULL)
        return;
    t->setSeat(seat);
    t->mClaimedPercentage = 1.0;
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
    if (ss && !isDiggable(pp->getSeat()))
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

void Tile::claimForSeat(Seat* seat, double nDanceRate)
{
    double amountClaimed;

    // If the seat is allied, we add to it. If it is an enemy seat, we subtract from it.
    Seat* tileSeat = getSeat();
    if (tileSeat != NULL && tileSeat->isAlliedSeat(seat))
    {
        amountClaimed = std::min(nDanceRate, 1.0 - mClaimedPercentage);
        mClaimedPercentage += nDanceRate;
        if (mClaimedPercentage >= 1.0)
        {
            claimTile(seat);

            // We inform the clients that the tile has been claimed
            if(getGameMap()->isServerGameMap())
            {
                try
                {
                    // Inform the clients that the fullness has changed.
                    ServerNotification *serverNotification = new ServerNotification(
                        ServerNotification::tileClaimed, getGameMap()->getPlayerBySeatId(seat->getId()));
                    serverNotification->mPacket << this;

                    ODServer::getSingleton().queueServerNotification(serverNotification);
                }
                catch (std::bad_alloc&)
                {
                    std::cerr << "\n\nERROR:  bad alloc in Tile::setFullness\n\n";
                    exit(1);
                }
            }

        }
    }
    else
    {
        amountClaimed = std::min(nDanceRate, 1.0 + mClaimedPercentage);
        mClaimedPercentage -= nDanceRate;
        if (mClaimedPercentage <= 0.0)
        {
            // The tile is not yet claimed, but it is now an allied seat.
            mClaimedPercentage *= -1.0;
            setSeat(seat);

            if (mClaimedPercentage >= 1.0)
            {
                mClaimedPercentage = 1.0;
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
    }
    */
}

void Tile::claimTile(Seat* seat)
{
    // Claim the tile.
    // We need this because if we are a client, the tile may be from a non allied seat
    setSeat(seat);
    mClaimedPercentage = 1.0;
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

    if (getFullness() == 0.0 || type == lava || type == water || type == rock)
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
                serverNotification->mPacket << this;
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

std::string Tile::buildName(int x, int y)
{
    std::stringstream ss;
    ss << "Level_";
    ss << x;
    ss << "_";
    ss << y;
    return ss.str();
}

bool Tile::checkTileName(const std::string& tileName, int& x, int& y)
{
    if (tileName.find("Tile_Level_") == std::string::npos)
        return false;

    if(sscanf(tileName.c_str(), "Tile_Level_%i_%i", &x, &y) != 2)
        return false;

    return true;
}

bool Tile::isFloodFillFilled()
{
    if(getFullness() > 0.0)
        return true;

    switch(getType())
    {
        case Tile::dirt:
        case Tile::gold:
        case Tile::claimed:
        {
            if((mFloodFillColor[Tile::FloodFillTypeGround] != -1) &&
               (mFloodFillColor[Tile::FloodFillTypeGroundWater] != -1) &&
               (mFloodFillColor[Tile::FloodFillTypeGroundLava] != -1) &&
               (mFloodFillColor[Tile::FloodFillTypeGroundWaterLava] != -1))
            {
                return true;
            }
            break;
        }
        case Tile::water:
        {
            if((mFloodFillColor[Tile::FloodFillTypeGroundWater] != -1) &&
               (mFloodFillColor[Tile::FloodFillTypeGroundWaterLava] != -1))
            {
                return true;
            }
            break;
        }
        case Tile::lava:
        {
            if((mFloodFillColor[Tile::FloodFillTypeGroundLava] != -1) &&
               (mFloodFillColor[Tile::FloodFillTypeGroundWaterLava] != -1))
            {
                return true;
            }
            break;
        }
        default:
            return true;
    }

    return false;
}

bool Tile::canCreatureGoThroughTile(const CreatureDefinition* creatureDef)
{
    if(getFullness() > 0)
        return false;

    switch(getType())
    {
        case Tile::dirt:
        case Tile::gold:
        case Tile::claimed:
        {
            if(creatureDef->getMoveSpeedGround() > 0.0)
                return true;

            break;
        }
        case Tile::water:
        {
            if(creatureDef->getMoveSpeedWater() > 0.0)
                return true;

            break;
        }
        case Tile::lava:
        {
            if(creatureDef->getMoveSpeedLava() > 0.0)
                return true;

            break;
        }
        default:
            return false;
    }
    return false;
}

double Tile::getCreatureSpeedOnTile(const CreatureDefinition* creatureDef)
{
    switch(getType())
    {
        case Tile::dirt:
        case Tile::gold:
        case Tile::claimed:
            return creatureDef->getMoveSpeedGround();
        case Tile::water:
            return creatureDef->getMoveSpeedWater();
        case Tile::lava:
            return creatureDef->getMoveSpeedLava();
        default:
            break;
    }
    return creatureDef->getMoveSpeedGround();
}

void Tile::refreshFromTile(const Tile& tile)
{
    type = tile.type;
    setFullness(tile.fullness);
    Seat* seat = tile.getSeat();
    setSeat(seat);
    mClaimedPercentage = tile.mClaimedPercentage;
    // Note : There will be no visual change until the tile mesh is refreshed
}

bool Tile::isClaimedForSeat(Seat* seat) const
{
    Seat* tileSeat = getSeat();
    if(tileSeat == NULL)
        return false;

    if(tileSeat->canOwnedTileBeClaimedBy(seat))
        return false;

    return true;
}

int Tile::getFloodFill(FloodFillType type)
{
    OD_ASSERT_TRUE(type < FloodFillTypeMax);
    return mFloodFillColor[type];
}

// TODO : use this function where tiles are displayed in logs (mostly in ODClient and ODServer)
std::string Tile::displayAsString(Tile* tile)
{
    return "[" + Ogre::StringConverter::toString(tile->x) + ","
         + Ogre::StringConverter::toString(tile->y)+ "]";
}
