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

#include "rooms/Room.h"

#include "traps/Trap.h"

#include "utils/Helper.h"
#include "utils/LogManager.h"

#include <cstddef>
#include <bitset>

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#define snprintf_is_banned_in_OD_code _snprintf
#endif

const std::string TILE_PREFIX = "Tile_";
const std::string TILE_SCANF = TILE_PREFIX + "%i_%i";

Tile::Tile(GameMap* gameMap, int x, int y, TileType type, double fullness) :
    GameEntity          (gameMap),
    mX                  (x),
    mY                  (y),
    mType               (type),
    mTileVisual         (TileVisual::nullTileVisual),
    mSelected           (false),
    mFullness           (fullness),
    mCoveringBuilding   (nullptr),
    mFloodFillColor     (std::vector<int>(static_cast<int>(FloodFillType::nbValues), -1)),
    mClaimedPercentage  (0.0),
    mScale              (Ogre::Vector3::ZERO),
    mIsBuilding         (false),
    mLocalPlayerHasVision   (false)
{
    setSeat(nullptr);
    computeTileVisual();
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
}

bool Tile::isBuildableUpon(Seat* seat) const
{
    if(isFullTile())
        return false;
    if(mIsBuilding)
        return false;
    if(!isClaimedForSeat(seat))
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

    // Some buildings (like traps) might not want to refresh the tile for enemy seats
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
}

bool Tile::isDiggable(Seat* seat) const
{
    // Handle non claimed
    switch(mTileVisual)
    {
        case TileVisual::claimedGround:
        case TileVisual::dirtGround:
        case TileVisual::goldGround:
        case TileVisual::lavaGround:
        case TileVisual::waterGround:
        case TileVisual::rockGround:
        case TileVisual::rockFull:
            return false;
        case TileVisual::goldFull:
        case TileVisual::dirtFull:
            return true;
        default:
            break;
    }

    // Should be claimed tile
    OD_ASSERT_TRUE_MSG(mTileVisual == TileVisual::claimedFull, "mTileVisual=" + tileVisualToString(mTileVisual));

    // If the wall is not claimed, it can be dug
    if(!isClaimed())
        return true;

    // It is claimed. If it is by the given seat, it can be dug
    if(isClaimedForSeat(seat))
        return true;

    return false;
}

bool Tile::isGroundClaimable(Seat* seat) const
{
    if(getFullness() > 0.0)
        return false;

    if(mType != TileType::dirt && mType != TileType::gold)
        return false;

    if(getCoveringRoom() != nullptr)
        return false;

    if(isClaimedForSeat(seat))
        return false;

    return true;
}

bool Tile::isWallClaimable(Seat* seat)
{
    if (getFullness() == 0.0)
        return false;

    if (mType == TileType::lava || mType == TileType::water || mType == TileType::rock || mType == TileType::gold)
        return false;

    // Check whether at least one neighbor is a claimed ground tile of the given seat
    // which is a condition to permit claiming the given wall tile.
    bool foundClaimedGroundTile = false;
    for (Tile* tile : mNeighbors)
    {
        if (tile->getFullness() > 0.0)
            continue;

        if (!tile->isClaimedForSeat(seat))
            continue;

        foundClaimedGroundTile = true;
        break;
    }

    if (foundClaimedGroundTile == false)
        return false;

    // If the tile is not claimed, it is claimable
    if (!isClaimed())
        return true;

    // We check if the tile is already claimed for our seat
    if (isClaimedForSeat(seat))
        return false;

    // The tile is claimed by another team. We check if there is a claimed ground tile
    // claimed by the same team. If not, we can claim. If yes, we cannot
    Seat* tileSeat = getSeat();
    if (tileSeat == nullptr)
        return true;

    foundClaimedGroundTile = false;
    for (Tile* tile : mNeighbors)
    {
        if (tile->getFullness() > 0.0)
            continue;

        if (!tile->isClaimedForSeat(tileSeat))
            continue;

        foundClaimedGroundTile = true;
        break;
    }

    if (foundClaimedGroundTile)
        return false;

    return true;
}

bool Tile::isWallClaimedForSeat(Seat* seat)
{
    if (getFullness() == 0.0)
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

std::string Tile::getFormat()
{
    return "posX\tposY\ttype\tfullness\tseatId(optional)";
}

void Tile::exportToStream(std::ostream& os) const
{
    os << getX() << "\t" << getY() << "\t";
    os << getType() << "\t" << getFullness();
    if(getSeat() == nullptr)
        return;

    os << "\t" << getSeat()->getId();
}

void Tile::exportTileToPacket(ODPacket& os, Seat* seat)
{
    Seat* tileSeat = getSeat();
    int seatId = -1;
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
        mScale = Ogre::Vector3::ZERO;
    }

    os << mIsBuilding;

    os << seatId;
    os << meshName;
    os << mScale;
    os << mTileVisual;

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

void Tile::exportToPacket(ODPacket& os) const
{
    //Check to make sure this function is not called. The function taking a
    //seat argument should be used instead
    throw std::runtime_error("ERROR: Wrong packet export function used for tile");
}

void Tile::updateFromPacket(ODPacket& is)
{
    int seatId;
    std::string meshName;
    std::stringstream ss;

    // We set the seat if there is one
    OD_ASSERT_TRUE(is >> mIsBuilding);

    OD_ASSERT_TRUE(is >> seatId);

    OD_ASSERT_TRUE(is >> meshName);
    setMeshName(meshName);

    OD_ASSERT_TRUE(is >> mScale);

    ss.str(std::string());
    ss << TILE_PREFIX;
    ss << getX();
    ss << "_";
    ss << getY();

    setName(ss.str());

    OD_ASSERT_TRUE(is >> mTileVisual);

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

    if(seatId == -1)
    {
        setSeat(nullptr);
    }
    else
    {
        Seat* seat = getGameMap()->getSeatById(seatId);
        if(seat != nullptr)
            setSeat(seat);

    }

    // We need to check if the tile is unmarked after reading the needed information.
    if(getMarkedForDigging(getGameMap()->getLocalPlayer()) &&
       !isDiggable(getGameMap()->getLocalPlayer()->getSeat()))
    {
        removePlayerMarkingTile(getGameMap()->getLocalPlayer());
    }
}

ODPacket& operator<<(ODPacket& os, const TileType& type)
{
    uint32_t intType = static_cast<uint32_t>(type);
    os << intType;
    return os;
}

ODPacket& operator>>(ODPacket& is, TileType& type)
{
    uint32_t intType;
    is >> intType;
    type = static_cast<TileType>(intType);
    return is;
}

std::ostream& operator<<(std::ostream& os, const TileType& type)
{
    uint32_t intType = static_cast<uint32_t>(type);
    os << intType;
    return os;
}

std::istream& operator>>(std::istream& is, TileType& type)
{
    uint32_t intType;
    is >> intType;
    type = static_cast<TileType>(intType);
    return is;
}

ODPacket& operator<<(ODPacket& os, const TileVisual& type)
{
    uint32_t intType = static_cast<uint32_t>(type);
    os << intType;
    return os;
}

ODPacket& operator>>(ODPacket& is, TileVisual& type)
{
    uint32_t intType;
    is >> intType;
    type = static_cast<TileVisual>(intType);
    return is;
}

std::ostream& operator<<(std::ostream& os, const TileVisual& type)
{
    uint32_t intType = static_cast<uint32_t>(type);
    os << intType;
    return os;
}

std::istream& operator>>(std::istream& is, TileVisual& type)
{
    uint32_t intType;
    is >> intType;
    type = static_cast<TileVisual>(intType);
    return is;
}

void Tile::loadFromLine(const std::string& line, Tile *t)
{
    std::vector<std::string> elems = Helper::split(line, '\t');

    int xLocation = Helper::toInt(elems[0]);
    int yLocation = Helper::toInt(elems[1]);

    std::stringstream tileName("");
    tileName << TILE_PREFIX;
    tileName << xLocation;
    tileName << "_";
    tileName << yLocation;

    t->setName(tileName.str());
    t->mX = xLocation;
    t->mY = yLocation;
    t->mPosition = Ogre::Vector3(static_cast<Ogre::Real>(t->mX), static_cast<Ogre::Real>(t->mY), 0.0f);

    TileType tileType = static_cast<TileType>(Helper::toInt(elems[2]));
    t->setType(tileType);

    // If the tile type is lava or water, we ignore fullness
    double fullness;
    switch(tileType)
    {
        case TileType::water:
        case TileType::lava:
            fullness = 0.0;
            break;

        default:
            fullness = Helper::toDouble(elems[3]);
            break;
    }
    t->setFullnessValue(fullness);

    bool shouldSetSeat = false;
    // We allow to set seat if the tile is dirt (full or not) or if it is gold (ground only)
    if(elems.size() >= 5)
    {
        if(tileType == TileType::dirt)
        {
            shouldSetSeat = true;
        }
        else if((tileType == TileType::gold) &&
                (fullness == 0.0))
        {
            shouldSetSeat = true;
        }
    }

    if(!shouldSetSeat)
    {
        t->setSeat(nullptr);
        return;
    }

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
        case TileType::dirt:
            return "Dirt";

        case TileType::rock:
            return "Rock";

        case TileType::gold:
            return "Gold";

        case TileType::water:
            return "Water";

        case TileType::lava:
            return "Lava";

        default:
            return "Unknown tile type=" + Helper::toString(static_cast<uint32_t>(t));
    }
}

std::string Tile::tileVisualToString(TileVisual tileVisual)
{
    switch (tileVisual)
    {
        case TileVisual::nullTileVisual:
            return "nullTileVisual";

        case TileVisual::dirtGround:
            return "dirtGround";

        case TileVisual::dirtFull:
            return "dirtFull";

        case TileVisual::rockGround:
            return "rockGround";

        case TileVisual::rockFull:
            return "rockFull";

        case TileVisual::goldGround:
            return "goldGround";

        case TileVisual::goldFull:
            return "goldFull";

        case TileVisual::waterGround:
            return "waterGround";

        case TileVisual::lavaGround:
            return "lavaGround";

        case TileVisual::claimedGround:
            return "claimedGround";

        case TileVisual::claimedFull:
            return "claimedFull";

        default:
            return "Unknown tile type=" + Helper::toString(static_cast<uint32_t>(tileVisual));
    }
}

TileVisual Tile::tileVisualFromString(const std::string& strTileVisual)
{
    uint32_t nb = static_cast<uint32_t>(TileVisual::countTileVisual);
    for(uint32_t k = 0; k < nb; ++k)
    {
        TileVisual tileVisual = static_cast<TileVisual>(k);
        if(strTileVisual.compare(tileVisualToString(tileVisual)) == 0)
            return tileVisual;
    }

    return TileVisual::nullTileVisual;
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
    for (Player* player : getGameMap()->getPlayers())
    {
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
            computeTileVisual();
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

    // If an enemy player had marked this tile to dig, we disable it
    setMarkedForDiggingForAllPlayersExcept(false, seat);

    computeTileVisual();
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

void Tile::unclaimTile()
{
    // Unclaim the tile.
    setSeat(nullptr);
    mClaimedPercentage = 0.0;

    computeTileVisual();
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

    if (getFullness() == 0.0 || mType == TileType::lava || mType == TileType::water || mType == TileType::rock)
        return 0.0;

    if (digRate >= mFullness)
    {
        amountDug = mFullness;
        setFullness(0.0);

        computeTileVisual();
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
    if(!isClaimed())
        return digRate;

    return 0.2 * digRate;
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
    ss << TILE_PREFIX;
    ss << x;
    ss << "_";
    ss << y;
    return ss.str();
}

bool Tile::checkTileName(const std::string& tileName, int& x, int& y)
{
    if (tileName.compare(0, TILE_PREFIX.length(), TILE_PREFIX) != 0)
        return false;

    if(sscanf(tileName.c_str(), TILE_SCANF.c_str(), &x, &y) != 2)
        return false;

    return true;
}

std::string Tile::toString(FloodFillType type)
{
    return Helper::toString(toUInt32(type));
}

bool Tile::isFloodFillFilled() const
{
    if(getFullness() > 0.0)
        return true;

    switch(getType())
    {
        case TileType::dirt:
        case TileType::gold:
        {
            if((mFloodFillColor[toUInt32(FloodFillType::ground)] != -1) &&
               (mFloodFillColor[toUInt32(FloodFillType::groundWater)] != -1) &&
               (mFloodFillColor[toUInt32(FloodFillType::groundLava)] != -1) &&
               (mFloodFillColor[toUInt32(FloodFillType::groundWaterLava)] != -1))
            {
                return true;
            }
            break;
        }
        case TileType::water:
        {
            if((mFloodFillColor[toUInt32(FloodFillType::groundWater)] != -1) &&
               (mFloodFillColor[toUInt32(FloodFillType::groundWaterLava)] != -1))
            {
                return true;
            }
            break;
        }
        case TileType::lava:
        {
            if((mFloodFillColor[toUInt32(FloodFillType::groundLava)] != -1) &&
               (mFloodFillColor[toUInt32(FloodFillType::groundWaterLava)] != -1))
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

bool Tile::isSameFloodFill(FloodFillType type, Tile* tile) const
{
    return mFloodFillColor[toUInt32(type)] == tile->mFloodFillColor[toUInt32(type)];
}

void Tile::resetFloodFill()
{
    for(int& floodFillValue : mFloodFillColor)
    {
        floodFillValue = -1;
    }
}

int Tile::floodFillValue(FloodFillType type) const
{
    uint32_t intFloodFill = toUInt32(type);
    OD_ASSERT_TRUE_MSG(intFloodFill < mFloodFillColor.size(), Helper::toString(intFloodFill));
    if(intFloodFill >= mFloodFillColor.size())
        return -1;

    return mFloodFillColor[intFloodFill];
}

bool Tile::updateFloodFillFromTile(FloodFillType type, Tile* tile)
{
    if((floodFillValue(type) != -1) ||
       (tile->floodFillValue(type) == -1))
    {
        return false;
    }

    mFloodFillColor[toUInt32(type)] = tile->mFloodFillColor[toUInt32(type)];
    return true;
}

void Tile::replaceFloodFill(FloodFillType type, int newValue)
{
    mFloodFillColor[toUInt32(type)] = newValue;
}

void Tile::logFloodFill() const
{
    std::string str = "Tile floodfill : " + Tile::displayAsString(this)
        + " - fullness=" + Helper::toString(getFullness())
        + " - seatId=" + std::string(getSeat() == nullptr ? "-1" : Helper::toString(getSeat()->getId()));
    int cpt = 0;
    for(const int& floodFill : mFloodFillColor)
    {
        str += ", [" + Helper::toString(cpt) + "]=" +
            Helper::toString(floodFill);
        ++cpt;
    }
    LogManager::getSingleton().logMessage(str);
}

bool Tile::isClaimedForSeat(Seat* seat) const
{
    if(!isClaimed())
        return false;

    if(getSeat()->canOwnedTileBeClaimedBy(seat))
        return false;

    return true;
}

bool Tile::isClaimed() const
{
    if(!getGameMap()->isServerGameMap())
        return ((mTileVisual == TileVisual::claimedGround) || (mTileVisual == TileVisual::claimedFull));

    if(getSeat() == nullptr)
        return false;

    if(mClaimedPercentage < 1.0)
        return false;

    return true;
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

        if(entity->getObjectType() != GameEntityType::chickenEntity)
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

        if(entity->getObjectType() != GameEntityType::craftedTrap)
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

        if(entity->getObjectType() != GameEntityType::treasuryObject)
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

    if(!isClaimed())
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

void Tile::notifyEntitiesSeatsWithVision()
{
    for(GameEntity* entity : mEntitiesInTile)
    {
        entity->notifySeatsWithVision(mSeatsWithVision);
    }

    if(getCoveringBuilding() != nullptr)
        getCoveringBuilding()->notifySeatsVisionOnTile(mSeatsWithVision, this);
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

void Tile::computeTileVisual()
{
    if(isClaimed())
    {
        if(mFullness > 0.0)
            mTileVisual = TileVisual::claimedFull;
        else
            mTileVisual = TileVisual::claimedGround;
        return;
    }

    switch(getType())
    {
        case TileType::dirt:
            if(mFullness > 0.0)
                mTileVisual = TileVisual::dirtFull;
            else
                mTileVisual = TileVisual::dirtGround;
            return;

        case TileType::rock:
            if(mFullness > 0.0)
                mTileVisual = TileVisual::rockFull;
            else
                mTileVisual = TileVisual::rockGround;
            return;

        case TileType::gold:
            if(mFullness > 0.0)
                mTileVisual = TileVisual::goldFull;
            else
                mTileVisual = TileVisual::goldGround;
            return;

        case TileType::water:
            mTileVisual = TileVisual::waterGround;
            return;

        case TileType::lava:
            mTileVisual = TileVisual::lavaGround;
            return;

        default:
            mTileVisual = TileVisual::nullTileVisual;
            return;
    }
}

bool Tile::isFullTile() const
{
    if(getGameMap()->isServerGameMap())
    {
        return getFullness() > 0.0;
    }
    else
    {
        switch(mTileVisual)
        {
            case TileVisual::claimedFull:
            case TileVisual::dirtFull:
            case TileVisual::goldFull:
            case TileVisual::rockFull:
                return true;
            default:
                return false;
        }
    }
}

std::string Tile::displayAsString(const Tile* tile)
{
    return "[" + Ogre::StringConverter::toString(tile->getX()) + ","
         + Ogre::StringConverter::toString(tile->getY())+ "]";
}
