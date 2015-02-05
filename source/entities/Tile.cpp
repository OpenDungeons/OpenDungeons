/*
 *  Copyright (C) 2011-2015  OpenDungeons Team
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

#include "entities/Tile.h"

#include "entities/BuildingObject.h"
#include "entities/Creature.h"
#include "entities/MapLight.h"
#include "entities/TreasuryObject.h"
#include "entities/ChickenEntity.h"
#include "entities/CraftedTrap.h"
#include "entities/PersistentObject.h"

#include "game/Player.h"
#include "game/Seat.h"

#include "gamemap/GameMap.h"

#include "network/ODServer.h"
#include "network/ODPacket.h"

#include "render/RenderManager.h"

#include "sound/SoundEffectsManager.h"

#include "utils/Helper.h"
#include "utils/LogManager.h"

#include <cstddef>
#include <bitset>

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#define snprintf_is_banned_in_OD_code _snprintf
#endif

const Ogre::Vector3 DEFAULT_TILE_SCALE(static_cast<Ogre::Real>(4.0 / RenderManager::BLENDER_UNITS_PER_OGRE_UNIT),
        static_cast<Ogre::Real>(4.0 / RenderManager::BLENDER_UNITS_PER_OGRE_UNIT),
        static_cast<Ogre::Real>(5.0 / RenderManager::BLENDER_UNITS_PER_OGRE_UNIT));


Tile::Tile(GameMap* gameMap, int x, int y, TileType type, double fullness) :
    GameEntity          (gameMap),
    mX                  (x),
    mY                  (y),
    mRotation           (0.0),
    mType               (type),
    mSelected           (false),
    mFullness           (fullness),
    mFullnessMeshNumber (-1),
    mCoveringBuilding   (nullptr),
    mClaimedPercentage  (0.0),
    mScale              (Ogre::Vector3::ZERO),
    mIsBuilding         (false),
    mLocalPlayerHasVision   (false),
    mLocalPlayerCanMarkTile (true)
{
    for(int i = 0; i < Tile::FloodFillTypeMax; i++)
    {
        mFloodFillColor[i] = -1;
    }
    setSeat(nullptr);
    mScale = DEFAULT_TILE_SCALE;
}

void Tile::createMeshLocal()
{
    GameEntity::createMeshLocal();

    if(getGameMap()->isServerGameMap())
        return;

    RenderManager::getSingleton().rrCreateTile(this, getGameMap()->getLocalPlayer());
}

void Tile::destroyMeshLocal()
{
    GameEntity::destroyMeshLocal();

    if(getGameMap()->isServerGameMap())
        return;

    RenderManager::getSingleton().rrDestroyTile(this);
}

void Tile::setType(TileType t)
{
    // If the type has changed from its previous value we need to see if
    // the mesh should be updated
    if (t != mType)
    {
        mType = t;
    }
}

void Tile::addToGameMap()
{
    getGameMap()->addTile(this);
}

void Tile::setFullness(double f)
{
    double oldFullness = getFullness();

    mFullness = f;

    // If the tile was marked for digging and has been dug out, unmark it and set its fullness to 0.
    if (mFullness == 0.0 && isMarkedForDiggingByAnySeat())
    {
        setMarkedForDiggingForAllPlayersExcept(false, nullptr);
    }

    if ((oldFullness > 0.0) && (mFullness == 0.0))
    {
        // Do a flood fill to update the contiguous region touching the tile.
        getGameMap()->refreshFloodFill(this);
    }

    // 		4 0 7		    180
    // 		2 8 3		270  .  90
    // 		7 1 5		     0
    //
    bool fillStatus[9];
    Tile *tempTile = getGameMap()->getTile(mX, mY + 1);
    fillStatus[0] = (tempTile != nullptr) ? tempTile->getFullness() > 0.0 : false;
    tempTile = getGameMap()->getTile(mX, mY - 1);
    fillStatus[1] = (tempTile != nullptr) ? tempTile->getFullness() > 0.0 : false;
    tempTile = getGameMap()->getTile(mX - 1, mY);
    fillStatus[2] = (tempTile != nullptr) ? tempTile->getFullness() > 0.0 : false;
    tempTile = getGameMap()->getTile(mX + 1, mY);
    fillStatus[3] = (tempTile != nullptr) ? tempTile->getFullness() > 0.0 : false;

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
    mFullnessMeshNumber = 0;
    if (f > 0 && f <= 25)
        mFullnessMeshNumber = 25;
    else if (f > 25 && f <= 50)
    {
        mFullnessMeshNumber = 50;
        switch (fullNeighbors)
        {
            case 1:
                mFullnessMeshNumber = 51;
                if (fillStatus[0])
                {
                    mRotation = 270;
                    break;
                }//correct
                if (fillStatus[1])
                {
                    mRotation = 90;
                    break;
                }//correct
                if (fillStatus[2])
                {
                    mRotation = 0;
                    break;
                }//correct
                if (fillStatus[3])
                {
                    mRotation = 180;
                    break;
                }//correct
                break;

            case 2:
                mFullnessMeshNumber = 52;
                if (fillStatus[0] && fillStatus[2])
                {
                    mRotation = 270;
                    break;
                }//correct
                if (fillStatus[0] && fillStatus[3])
                {
                    mRotation = 180;
                    break;
                }//correct
                if (fillStatus[1] && fillStatus[2])
                {
                    mRotation = 0;
                    break;
                }//correct
                if (fillStatus[1] && fillStatus[3])
                {
                    mRotation = 90;
                    break;
                }//correct

                //TODO:  These next two options are for when the half full tile is in the middle of a wall, the need a separate mesh to be made.
                if (fillStatus[0] && fillStatus[1])
                {
                    mFullnessMeshNumber = 51;
                    mRotation = 0;
                    break;
                }//correct
                if (fillStatus[2] && fillStatus[3])
                {
                    mFullnessMeshNumber = 51;
                    mRotation = 90;
                    break;
                }//correct
                break;
        }
    }

    else if (f > 50 && f <= 75)
    {
        mFullnessMeshNumber = 75;
        switch (fullNeighbors)
        {
            case 1:
                if (fillStatus[0])
                {
                    mRotation = 270;
                    break;
                }//correct
                if (fillStatus[1])
                {
                    mRotation = 90;
                    break;
                }//correct
                if (fillStatus[2])
                {
                    mRotation = 0;
                    break;
                }//correct
                if (fillStatus[3])
                {
                    mRotation = 180;
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
                mFullnessMeshNumber = 104;
                mRotation = 0;
                break;

            case 1:
                mFullnessMeshNumber = 103;
                if (fillStatus[0])
                {
                    mRotation = 180;
                    break;
                }//correct
                if (fillStatus[1])
                {
                    mRotation = 0;
                    break;
                }//correct
                if (fillStatus[2])
                {
                    mRotation = 270;
                    break;
                }//correct
                if (fillStatus[3])
                {
                    mRotation = 90;
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
                        mFullnessMeshNumber = 52;
                        mRotation = 270;
                        break;

                    case 6:
                        mFullnessMeshNumber = 52;
                        mRotation = 0;
                        break;

                    case 9:
                        mFullnessMeshNumber = 52;
                        mRotation = 180;
                        break;

                    case 10:
                        mFullnessMeshNumber = 52;
                        mRotation = 90;
                        break;

                    case 3:
                        mFullnessMeshNumber = 102;
                        mRotation = 0.0;
                        break;

                    case 12:
                        mFullnessMeshNumber = 102;
                        mRotation = 90.0;
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
                mFullnessMeshNumber = 101; //this is wrong for now it should be 101
                if (!fillStatus[0])
                {
                    mRotation = 90;
                    break;
                }//correct
                if (!fillStatus[1])
                {
                    mRotation = 270;
                    break;
                }
                if (!fillStatus[2])
                {
                    mRotation = 180;
                    break;
                }
                if (!fillStatus[3])
                {
                    mRotation = 0;
                    break;
                }
                break;

            case 4:
                mFullnessMeshNumber = 100;
                mRotation = 0;
                break;

            default:
                std::cerr
                        << "\n\nERROR:  fullNeighbors != 0 or 1 or 2 or 3 or 4.  This is impossible, exiting program.\n\n";
                exit(1);
                break;
        }

    }
}

void Tile::setFullnessValue(double f)
{
    mFullness = f;
}

double Tile::getFullness() const
{
    return mFullness;
}

int Tile::getFullnessMeshNumber() const
{
    return mFullnessMeshNumber;
}

bool Tile::permitsVision() const
{
    return (mFullness == 0.0);
}

bool Tile::isBuildableUpon() const
{
    if(mType != claimed)
        return false;
    if(getFullness() > 0.0)
        return false;
    if(mIsBuilding)
        return false;

    return true;
}

void Tile::setCoveringBuilding(Building *building)
{
    mCoveringBuilding = building;

    if (mCoveringBuilding == nullptr)
    {
        setDirtyForAllSeats();
        mIsBuilding = false;
        return;
    }

    for(std::pair<Seat*, bool>& seatChanged : mTileChangedForSeats)
    {
        if(!building->shouldSetCoveringTileDirty(seatChanged.first, this))
            continue;

        seatChanged.second = true;
    }

    mIsBuilding = true;
    // Set the tile as claimed and of the team color of the building
    setSeat(mCoveringBuilding->getSeat());
    mClaimedPercentage = 1.0;
    setType(claimed);
}

bool Tile::isDiggable(Seat* seat) const
{
    if (getFullness() == 0.0)
        return false;

    // Return true for common types.
    if (mType == dirt || mType == gold)
        return true;

    // Return false for undiggable types.
    if (mType == lava || mType == water || mType == rock)
        return false;

    if (mType != claimed)
        return false;

    // type == claimed
    if(!getGameMap()->isServerGameMap())
        return mLocalPlayerCanMarkTile;

    // For claimed walls, we check whether the walls is either claimed by the given player,
    if (isClaimedForSeat(seat))
        return true;

    // or whether it isn't belonging to a specific team.
    if (mClaimedPercentage <= 0.0)
        return true;

    return false;
}

bool Tile::isGroundClaimable() const
{
    return ((mType == dirt || mType == gold || mType == claimed) && getFullness() == 0.0
        && getCoveringRoom() == nullptr);
}

bool Tile::isWallClaimable(Seat* seat)
{
    if (getFullness() == 0.0)
        return false;

    if (mType == lava || mType == water || mType == rock || mType == gold)
        return false;

    // Check whether at least one neighbor is a claimed ground tile of the given seat
    // which is a condition to permit claiming the given wall tile.
    bool foundClaimedGroundTile = false;
    for (Tile* tile : mNeighbors)
    {
        if (tile->getFullness() > 0.0)
            continue;

        if (tile->getType() == claimed
                && tile->getClaimedPercentage() >= 1.0
                && tile->isClaimedForSeat(seat))
        {
            foundClaimedGroundTile = true;
            break;
        }
    }

    if (foundClaimedGroundTile == false)
        return false;

    if (mType == dirt)
        return true;

    if (mType != claimed)
        return false;

    // type == claimed

    // For claimed walls, we check whether it isn't belonging completely to a specific team.
    if (mClaimedPercentage < 1.0)
        return true;

    Seat* tileSeat = getSeat();
    if (tileSeat == nullptr)
        return true;

    // Or whether the wall tile is either claimed by the given player entirely already,
    // NOTE: mClaimedPercentage >= 1.0 from here
    if (isClaimedForSeat(seat))
        return false; // Already claimed.

    // The wall is claimed by another team.
    // Or whether the enemy player that claimed the wall tile has got any ground tiles permitting to keep claiming that wall tile.
    foundClaimedGroundTile = false;
    for (Tile* tile : mNeighbors)
    {
        if (tile->getFullness() > 0.0)
            continue;

        if (tile->getType() == claimed
                && tile->mClaimedPercentage >= 1.0
                && tile->isClaimedForSeat(tileSeat))
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

    if (mType != claimed)
        return false;

    if (mClaimedPercentage < 1.0)
        return false;

    Seat* tileSeat = getSeat();
    if(tileSeat == nullptr)
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
    os << t->getX() << "\t" << t->getY() << "\t";
    os << t->getType() << "\t" << t->getFullness();
    Seat* seat = t->getSeat();
    if(t->getType() != Tile::TileType::claimed || seat == nullptr)
        return os;

    os << "\t" << seat->getId();

    return os;
}

void Tile::exportToPacket(ODPacket& os, Seat* seat)
{
    Seat* tileSeat = getSeat();
    int seatId = 0;
    // We only pass the tile seat to the client if the tile is fully claimed
    if((tileSeat != nullptr) && (mClaimedPercentage >= 1.0))
        seatId = tileSeat->getId();

    std::string meshName;

    if((getCoveringBuilding() != nullptr) && (getCoveringBuilding()->shouldDisplayBuildingTile()))
    {
        meshName = getCoveringBuilding()->getMeshName() + ".mesh";
        mScale = getCoveringBuilding()->getScale();
        // Buildings are not colored with seat color
    }
    else
    {
        // We set an empty mesh so that the client can compute the tile itself
        meshName.clear();
        mScale = DEFAULT_TILE_SCALE;
    }

    os << mIsBuilding;

    bool localPlayerCanMarkTile = true;
    if((tileSeat != nullptr) &&
       (!isClaimedForSeat(seat)))
    {
        localPlayerCanMarkTile = false;
    }
    os << localPlayerCanMarkTile;

    os << seatId;
    os << meshName;
    os << mScale;
    os << getType() << mFullness;

    // We export the list of all the persistent objects on this tile. We do that because a persistent object might have
    // been removed on server side when the client did not had vision. Thus, it would still be on client side. Thanks to
    // this list, the clients will be able to remove them.
    uint32_t nbPersistentObject = 0;
    for(PersistentObject* obj : mPersistentObjectRegistered)
    {
        // We only set in the list objects that are visible (for example, we don't send traps that have not been triggered)
        if(!obj->isVisibleForSeat(seat))
            continue;

        ++nbPersistentObject;
    }

    os << nbPersistentObject;
    uint32_t nbPersistentObjectTmp = 0;
    for(PersistentObject* obj : mPersistentObjectRegistered)
    {
        // We only set in the list objects that are visible (for example, we don't send traps that have not been triggered)
        if(!obj->isVisibleForSeat(seat))
            continue;

        os << obj->getName();
        ++nbPersistentObjectTmp;
    }

    OD_ASSERT_TRUE_MSG(nbPersistentObjectTmp == nbPersistentObject, "nbPersistentObject=" + Ogre::StringConverter::toString(nbPersistentObject)
        + ", nbPersistentObjectTmp=" + Ogre::StringConverter::toString(nbPersistentObjectTmp)
        + ", tile=" + displayAsString(this));
}

void Tile::updateFromPacket(ODPacket& is)
{
    int seatId;
    std::string meshName;
    std::stringstream ss;
    double fullness;

    // We set the seat if there is one
    OD_ASSERT_TRUE(is >> mIsBuilding);
    OD_ASSERT_TRUE(is >> mLocalPlayerCanMarkTile);
    if(!mLocalPlayerCanMarkTile &&
       getMarkedForDigging(getGameMap()->getLocalPlayer()))
    {
        removePlayerMarkingTile(getGameMap()->getLocalPlayer());
    }

    OD_ASSERT_TRUE(is >> seatId);

    OD_ASSERT_TRUE(is >> meshName);
    setMeshName(meshName);

    OD_ASSERT_TRUE(is >> mScale);

    ss.str(std::string());
    ss << "Level";
    ss << "_";
    ss << getX();
    ss << "_";
    ss << getY();

    setName(ss.str());

    Tile::TileType tileType;
    OD_ASSERT_TRUE(is >> tileType);
    setType(tileType);

    OD_ASSERT_TRUE(is >> fullness);
    setFullness(fullness);

    uint32_t nbPersistentObject;
    OD_ASSERT_TRUE(is >> nbPersistentObject);
    mPersistentObjectNamesOnTile.clear();
    while(nbPersistentObject > 0)
    {
        --nbPersistentObject;

        std::string name;
        OD_ASSERT_TRUE(is >> name);
        mPersistentObjectNamesOnTile.push_back(name);
    }

    for(std::vector<PersistentObject*>::iterator it = mPersistentObjectRegistered.begin(); it != mPersistentObjectRegistered.end();)
    {
        PersistentObject* obj = *it;
        if(std::find(mPersistentObjectNamesOnTile.begin(), mPersistentObjectNamesOnTile.end(), obj->getName()) != mPersistentObjectNamesOnTile.end())
        {
            ++it;
            continue;
        }

        // The object is not on this tile anymore, we remove it
        it = mPersistentObjectRegistered.erase(it);
        obj->removeFromGameMap();
        obj->deleteYourself();
    }

    if((tileType != Tile::TileType::claimed) || (seatId == 0))
        return;

    Seat* seat = getGameMap()->getSeatById(seatId);
    if(seat == nullptr)
        return;

    setSeat(seat);
    mClaimedPercentage = 1.0;
}

ODPacket& operator<<(ODPacket& os, const Tile::TileType& type)
{
    uint32_t intType = static_cast<Tile::TileType>(type);
    os << intType;
    return os;
}

ODPacket& operator>>(ODPacket& is, Tile::TileType& type)
{
    uint32_t intType;
    is >> intType;
    type = static_cast<Tile::TileType>(intType);
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
    t->mX = xLocation;
    t->mY = yLocation;
    t->mPosition = Ogre::Vector3(static_cast<Ogre::Real>(t->mX), static_cast<Ogre::Real>(t->mY), 0.0f);

    Tile::TileType tileType = static_cast<Tile::TileType>(Helper::toInt(elems[2]));
    t->setType(tileType);

    // If the tile type is lava or water, we ignore fullness
    switch(tileType)
    {
        case Tile::water:
        case Tile::lava:
            t->setFullnessValue(0.0);
            break;

        default:
            t->setFullnessValue(Helper::toDouble(elems[3]));
            break;
    }

    // If the tile is claimed, there can be an optional parameter with the seat id
    if(tileType != Tile::TileType::claimed || elems.size() < 5)
        return;

    int seatId = Helper::toInt(elems[4]);
    Seat* seat = t->getGameMap()->getSeatById(seatId);
    if(seat == nullptr)
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

//TODO: Turn this whole hardcoded thing into a static tileset configuration file.
std::string Tile::meshNameFromNeighbors(TileType myType, int fullnessMeshNumber,
                                        std::array<TileType, 8> neighbors,
                                        std::bitset<8> neighborsFullness, int& rt)
{
    // neighbors and neighborFullness arrays are filled this way:
    // x = given tile
    // Number = array index
    // -------------
    // | 1 | 2 | 3 |
    // -------------
    // | 0 | x | 4 |
    // -------------
    // | 7 | 6 | 5 |
    // -------------

    std::stringstream ss;
    //FIXME - define postfix somewhere
    int postfixInt = 0;
    unsigned char shiftedAroundBits;

    // get the integer from neighbors[], using it as a number coded in binary base
    for(int ii = 8; ii >=1; --ii)
    {
        postfixInt *=2;
        postfixInt += ((neighbors[(ii)%8] == myType &&  (!(myType!=water && myType!=lava)  || neighborsFullness[(ii)%8]  ))
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
    for(rt = 0; !Ogre::ResourceGroupManager::getSingletonPtr()->resourceExists("Graphics", ss.str()) && rt < 4; ++rt)
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

    // Second bad hack to workaround a bug with the Dirt tiles next to other tiles on the right
    // Since the corresponding file used must be turned by 180°, this ugly hack handles the rotation
    // manually.
    if (neighbors[2] == myType && neighbors[6] == myType && neighbors[4] == myType
        && (neighbors[0] != myType || !neighborsFullness[0])
        && neighborsFullness[2] && neighborsFullness[6] && neighborsFullness[4])
    {
        if (fullnessMeshNumber > 0 && (myType == Tile::dirt || myType == Tile::gold || myType == Tile::rock))
        {
            ss.str("");
            ss.clear();
            ss << "Dirt_10001111.mesh";
            rt = 3;
        }
    }

    // Third bad hack to workaround a bug with the Dirt tiles next to other tiles on the left
    // Since the corresponding file used must be turned by 180°, this ugly hack handles the rotation
    // manually.
    if (neighbors[2] == myType && neighbors[0] == myType && neighbors[6] == myType
        && (neighbors[4] != myType || !neighborsFullness[4])
        && neighborsFullness[2] && neighborsFullness[0] && neighborsFullness[6])
    {
        if (fullnessMeshNumber > 0 && (myType == Tile::dirt || myType == Tile::gold || myType == Tile::rock))
        {
            ss.str("");
            ss.clear();
            ss << "Dirt_10001111.mesh";
            rt = 1;
        }
    }

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

    RenderManager::getSingleton().rrRefreshTile(this, getGameMap()->getLocalPlayer());
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
}

void Tile::setSelected(bool ss, Player* pp)
{
    if (mSelected != ss)
    {
        mSelected = ss;

        RenderManager::getSingleton().rrTemporalMarkTile(this);
    }
}

void Tile::setMarkedForDiggingForAllPlayersExcept(bool s, Seat* exceptSeat)
{
    for (unsigned int i = 0, num = getGameMap()->numPlayers(); i < num; ++i)
    {
        Player* player = getGameMap()->getPlayer(i);
        if(exceptSeat == nullptr || (player->getSeat() != nullptr && !exceptSeat->isAlliedSeat(player->getSeat())))
            setMarkedForDigging(s, player);
    }
}

bool Tile::getMarkedForDigging(const Player *p) const
{
    if(std::find(mPlayersMarkingTile.begin(), mPlayersMarkingTile.end(), p) != mPlayersMarkingTile.end())
        return true;

    return false;
}

bool Tile::isMarkedForDiggingByAnySeat()
{
    return !mPlayersMarkingTile.empty();
}

bool Tile::addEntity(GameEntity *entity)
{
    if(!getGameMap()->isServerGameMap())
        return true;

    if(std::find(mEntitiesInTile.begin(), mEntitiesInTile.end(), entity) != mEntitiesInTile.end())
        return false;

    mEntitiesInTile.push_back(entity);
    return true;
}

bool Tile::removeEntity(GameEntity *entity)
{
    if(!getGameMap()->isServerGameMap())
        return true;

    std::vector<GameEntity*>::iterator it = std::find(mEntitiesInTile.begin(), mEntitiesInTile.end(), entity);
    if(it == mEntitiesInTile.end())
        return false;

    mEntitiesInTile.erase(it);
    return true;
}

void Tile::addPlayerMarkingTile(Player *p)
{
    mPlayersMarkingTile.push_back(p);
}

void Tile::removePlayerMarkingTile(Player *p)
{
    std::vector<Player*>::iterator it = std::find(mPlayersMarkingTile.begin(), mPlayersMarkingTile.end(), p);
    if(it == mPlayersMarkingTile.end())
        return;

    mPlayersMarkingTile.erase(it);
}

unsigned int Tile::numPlayersMarkingTile() const
{
    return mPlayersMarkingTile.size();
}

Player* Tile::getPlayerMarkingTile(int index)
{
    return mPlayersMarkingTile[index];
}

void Tile::addNeighbor(Tile *n)
{
    mNeighbors.push_back(n);
}

void Tile::claimForSeat(Seat* seat, double nDanceRate)
{
    // If the seat is allied, we add to it. If it is an enemy seat, we subtract from it.
    if (getSeat() != nullptr && getSeat()->isAlliedSeat(seat))
    {
        mClaimedPercentage += nDanceRate;
    }
    else
    {
        mClaimedPercentage -= nDanceRate;
        if (mClaimedPercentage <= 0.0)
        {
            // The tile is not yet claimed, but it is now an allied seat.
            mClaimedPercentage *= -1.0;
            setSeat(seat);
            // We set it to dirt. If it is claimed, it will be set correctly by claimTile
            setType(TileType::dirt);
            setDirtyForAllSeats();
        }
    }

    if ((getSeat() != nullptr) && (mClaimedPercentage >= 1.0) &&
        (getSeat()->isAlliedSeat(seat)))
    {
        claimTile(seat);
    }

    /*
    // TODO: This should rather add lights along claimed walls, and not on every walls. Maybe each 5 ones?
    // If this is the first time this tile has been claimed, emit a flash of light indicating that the tile was claimed.
    if (amountClaimed > 0.0 && claimLight == nullptr)
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

    // If an enemy player had marked this tile to dig, we disable it
    setMarkedForDiggingForAllPlayersExcept(false, seat);

    setDirtyForAllSeats();

    // Force all the neighbors to recheck their meshes as we have updated this tile.
    for (Tile* tile : mNeighbors)
    {
        // Update potential active spots.
        Building* building = tile->getCoveringBuilding();
        if (building != nullptr)
        {
            building->updateActiveSpots();
            building->createMesh();
        }
    }
}

double Tile::digOut(double digRate, bool doScaleDigRate)
{
    if (doScaleDigRate)
        digRate = scaleDigRate(digRate);

    double amountDug = 0.0;

    if (getFullness() == 0.0 || mType == lava || mType == water || mType == rock)
        return 0.0;

    if (digRate >= mFullness)
    {
        amountDug = mFullness;
        setFullness(0.0);

        setDirtyForAllSeats();

        for (Tile* tile : mNeighbors)
        {
            // Update potential active spots.
            Building* building = tile->getCoveringBuilding();
            if (building != nullptr)
            {
                building->updateActiveSpots();
                building->createMesh();
            }
        }
    }
    else
    {
        amountDug = digRate;
        setFullness(mFullness - digRate);
    }

    return amountDug;
}

double Tile::scaleDigRate(double digRate)
{
    switch (mType)
    {
        case claimed:
            return 0.2 * digRate;

        default:
            return digRate;
    }
}

Tile* Tile::getNeighbor(unsigned int index)
{
    OD_ASSERT_TRUE_MSG(index < mNeighbors.size(), "tile=" + displayAsString(this));
    if(index >= mNeighbors.size())
        return nullptr;

    return mNeighbors[index];
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

bool Tile::isClaimedForSeat(Seat* seat) const
{
    Seat* tileSeat = getSeat();
    if(tileSeat == nullptr)
        return false;

    if(mClaimedPercentage < 1.0)
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

void Tile::fillWithAttackableCreatures(std::vector<GameEntity*>& entities, Seat* seat, bool invert)
{
    for(GameEntity* entity : mEntitiesInTile)
    {
        OD_ASSERT_TRUE(entity != nullptr);
        if((entity == nullptr) || entity->getObjectType() != GameEntityType::creature)
            continue;

        if(!entity->isAttackable(this, seat))
            continue;

        // The invert flag is used to determine whether we want to return a list of the creatures
        // allied with supplied seat or the contrary.
        if ((invert && !entity->getSeat()->isAlliedSeat(seat)) || (!invert
                && entity->getSeat()->isAlliedSeat(seat)))
        {
            // Add the current creature
            if (std::find(entities.begin(), entities.end(), entity) == entities.end())
                entities.push_back(entity);
        }
    }
}

void Tile::fillWithAttackableRoom(std::vector<GameEntity*>& entities, Seat* seat, bool invert)
{
    Room* room = getCoveringRoom();
    if((room != nullptr) &&
       room->isAttackable(this, seat))
    {
        if ((invert && !room->getSeat()->isAlliedSeat(seat)) || (!invert
            && room->getSeat()->isAlliedSeat(seat)))
        {
            // If the room is not in the list already then add it.
            if (std::find(entities.begin(), entities.end(), room) == entities.end())
                entities.push_back(room);
        }
    }
}

void Tile::fillWithAttackableTrap(std::vector<GameEntity*>& entities, Seat* seat, bool invert)
{
    Trap* trap = getCoveringTrap();
    if((trap != nullptr) &&
       trap->isAttackable(this, seat))
    {
        if ((invert && !trap->getSeat()->isAlliedSeat(seat)) || (!invert
            && trap->getSeat()->isAlliedSeat(seat)))
        {
            // If the trap is not in the list already then add it.
            if (std::find(entities.begin(), entities.end(), trap) == entities.end())
                entities.push_back(trap);
        }
    }
}

void Tile::fillWithCarryableEntities(std::vector<GameEntity*>& entities)
{
    for(GameEntity* entity : mEntitiesInTile)
    {
        OD_ASSERT_TRUE(entity != nullptr);
        if(entity == nullptr)
            continue;

        if(entity->getEntityCarryType() == EntityCarryType::notCarryable)
            continue;

        if (std::find(entities.begin(), entities.end(), entity) == entities.end())
            entities.push_back(entity);
    }
}

void Tile::fillWithChickenEntities(std::vector<GameEntity*>& entities)
{
    for(GameEntity* entity : mEntitiesInTile)
    {
        OD_ASSERT_TRUE(entity != nullptr);
        if(entity == nullptr)
            continue;

        if(entity->getObjectType() != GameEntityType::renderedMovableEntity)
            continue;
        RenderedMovableEntity* rme = static_cast<RenderedMovableEntity*>(entity);
        if(rme->getRenderedMovableEntityType() != RenderedMovableEntityType::chickenEntity)
            continue;

        if (std::find(entities.begin(), entities.end(), entity) == entities.end())
            entities.push_back(entity);
    }
}

void Tile::fillWithCraftedTraps(std::vector<GameEntity*>& entities)
{
    for(GameEntity* entity : mEntitiesInTile)
    {
        OD_ASSERT_TRUE(entity != nullptr);
        if(entity == nullptr)
            continue;

        if(entity->getObjectType() != GameEntityType::renderedMovableEntity)
            continue;
        RenderedMovableEntity* rme = static_cast<RenderedMovableEntity*>(entity);
        if(rme->getRenderedMovableEntityType() != RenderedMovableEntityType::craftedTrap)
            continue;

        if (std::find(entities.begin(), entities.end(), entity) == entities.end())
            entities.push_back(entity);
    }
}

bool Tile::addTreasuryObject(TreasuryObject* obj)
{
    if(!getGameMap()->isServerGameMap())
        return true;

    if (std::find(mEntitiesInTile.begin(), mEntitiesInTile.end(), obj) != mEntitiesInTile.end())
        return false;

    // If there is already a treasury object, we merge it
    bool isMerged = false;
    for(GameEntity* entity : mEntitiesInTile)
    {
        OD_ASSERT_TRUE(entity != nullptr);
        if(entity == nullptr)
            continue;

        if(entity->getObjectType() != GameEntityType::renderedMovableEntity)
            continue;
        RenderedMovableEntity* rme = static_cast<RenderedMovableEntity*>(entity);
        if(rme->getRenderedMovableEntityType() != RenderedMovableEntityType::treasuryObject)
            continue;

        TreasuryObject* treasury = static_cast<TreasuryObject*>(entity);
        treasury->mergeGold(obj);
        isMerged = true;
        break;
    }

    if(!isMerged)
        mEntitiesInTile.push_back(obj);

    return true;
}

Room* Tile::getCoveringRoom() const
{
    if(mCoveringBuilding == nullptr)
        return nullptr;

    if(mCoveringBuilding->getObjectType() != GameEntityType::room)
        return nullptr;

    return static_cast<Room*>(mCoveringBuilding);
}

Trap* Tile::getCoveringTrap() const
{
    if(mCoveringBuilding == nullptr)
        return nullptr;

    if(mCoveringBuilding->getObjectType() != GameEntityType::trap)
        return nullptr;

    return static_cast<Trap*>(mCoveringBuilding);
}

void Tile::computeVisibleTiles()
{
    if(!getGameMap()->getIsFOWActivated())
    {
        // If the FOW is deactivated, we allow vision for every seat
        for(Seat* seat : getGameMap()->getSeats())
            notifyVision(seat);

        return;
    }

    if(mType != claimed)
        return;

    if(getSeat() == nullptr)
        return;

    // A claimed tile can see it self and its neighboors
    notifyVision(getSeat());
    for(Tile* tile : mNeighbors)
    {
        tile->notifyVision(getSeat());
    }
}

void Tile::clearVision()
{
    mSeatsWithVision.clear();
}

void Tile::notifyVision(Seat* seat)
{
    if(std::find(mSeatsWithVision.begin(), mSeatsWithVision.end(), seat) != mSeatsWithVision.end())
        return;

    seat->notifyVisionOnTile(this);
    mSeatsWithVision.push_back(seat);

    // We also notify vision for allied seats
    for(Seat* alliedSeat : seat->getAlliedSeats())
        notifyVision(alliedSeat);
}

void Tile::setSeats(const std::vector<Seat*>& seats)
{
    mTileChangedForSeats.clear();
    for(Seat* seat : seats)
    {
        // Every tile should be notified by default
        std::pair<Seat*, bool> p(seat, true);
        mTileChangedForSeats.push_back(p);
    }
}

bool Tile::hasChangedForSeat(Seat* seat) const
{
    for(const std::pair<Seat*, bool>& seatChanged : mTileChangedForSeats)
    {
        if(seatChanged.first != seat)
            continue;

        return seatChanged.second;
    }
    OD_ASSERT_TRUE_MSG(false, "Unknown seat id=" + Ogre::StringConverter::toString(seat->getId()));
    return false;
}

void Tile::changeNotifiedForSeat(Seat* seat)
{
    for(std::pair<Seat*, bool>& seatChanged : mTileChangedForSeats)
    {
        if(seatChanged.first != seat)
            continue;

        seatChanged.second = false;
        break;
    }
}

void Tile::setDirtyForAllSeats()
{
    if(!getGameMap()->isServerGameMap())
        return;

    for(std::pair<Seat*, bool>& seatChanged : mTileChangedForSeats)
        seatChanged.second = true;
}

void Tile::notifySeatsWithVision()
{
    for(GameEntity* entity : mEntitiesInTile)
    {
        entity->notifySeatsWithVision(mSeatsWithVision);
    }
}

bool Tile::registerPersistentObject(PersistentObject* obj)
{
    if(std::find(mPersistentObjectRegistered.begin(), mPersistentObjectRegistered.end(), obj) != mPersistentObjectRegistered.end())
        return false;

    mPersistentObjectRegistered.push_back(obj);

    if(getGameMap()->isServerGameMap())
    {
        for(std::pair<Seat*, bool>& seatChanged : mTileChangedForSeats)
        {
            if(!obj->isVisibleForSeat(seatChanged.first))
                continue;

            seatChanged.second = true;
        }
    }

    return true;
}

bool Tile::removePersistentObject(PersistentObject* obj)
{
    std::vector<PersistentObject*>::iterator it = std::find(mPersistentObjectRegistered.begin(), mPersistentObjectRegistered.end(), obj);
    if(it == mPersistentObjectRegistered.end())
        return false;

    mPersistentObjectRegistered.erase(it);
    setDirtyForAllSeats();
    return true;
}

std::string Tile::displayAsString(const Tile* tile)
{
    return "[" + Ogre::StringConverter::toString(tile->getX()) + ","
         + Ogre::StringConverter::toString(tile->getY())+ "]";
}
