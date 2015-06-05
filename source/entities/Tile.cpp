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

#include "game/Player.h"
#include "game/Seat.h"

#include "gamemap/GameMap.h"

#include "network/ODPacket.h"

#include "utils/Helper.h"
#include "utils/LogManager.h"

#include <cstddef>
#include <bitset>
#include <istream>
#include <ostream>

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#define snprintf_is_banned_in_OD_code _snprintf
#endif

const std::string TILE_PREFIX = "Tile_";
const std::string TILE_SCANF = TILE_PREFIX + "%i_%i";

const uint32_t Tile::NO_FLOODFILL = 0;

Tile::Tile(GameMap* gameMap, bool isOnServerMap, int x, int y, TileType type, double fullness) :
    EntityBase({}, {}),
    mX                  (x),
    mY                  (y),
    mType               (type),
    mTileVisual         (TileVisual::nullTileVisual),
    mSelected           (false),
    mFullness           (fullness),
    mRefundPriceRoom    (0),
    mRefundPriceTrap    (0),
    mCoveringBuilding   (nullptr),
    mClaimedPercentage  (0.0),
    mScale              (Ogre::Vector3::ZERO),
    mIsRoom             (false),
    mIsTrap             (false),
    mLocalPlayerHasVision   (false),
    mGameMap(gameMap),
    mIsOnServerMap(isOnServerMap)
{
    setSeat(nullptr);
    computeTileVisual();
}

bool Tile::isDiggable(const Seat* seat) const
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
    if(mTileVisual != TileVisual::claimedFull)
    {
        OD_LOG_ERR("mTileVisual=" + tileVisualToString(mTileVisual));
        return false;
    }

    // If the wall is not claimed, it can be dug
    if(!isClaimed())
        return true;

    // It is claimed. If it is by the given seat, it can be dug
    if(isClaimedForSeat(seat))
        return true;

    return false;
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

void Tile::exportToPacket(ODPacket& os) const
{
    //Check to make sure this function is not called. Seat argument should be used instead
    OD_LOG_ERR("tile=" + displayAsString(this));
    throw std::runtime_error("ERROR: Wrong packet export function used for tile");
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

void Tile::setMarkedForDigging(bool ss, const Player *pp)
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

void Tile::addPlayerMarkingTile(const Player *p)
{
    mPlayersMarkingTile.push_back(p);
}

void Tile::removePlayerMarkingTile(const Player *p)
{
    auto it = std::find(mPlayersMarkingTile.begin(), mPlayersMarkingTile.end(), p);
    if(it == mPlayersMarkingTile.end())
        return;

    mPlayersMarkingTile.erase(it);
}

void Tile::addNeighbor(Tile *n)
{
    mNeighbors.push_back(n);
}

double Tile::scaleDigRate(double digRate)
{
    if(!isClaimed())
        return digRate;

    return 0.2 * digRate;
}

Tile* Tile::getNeighbor(unsigned int index)
{
    if(index >= mNeighbors.size())
    {
        OD_LOG_ERR("tile=" + displayAsString(this) + ", index=" + Helper::toString(index) + ", size=" + Helper::toString(mNeighbors.size()));
        return nullptr;
    }

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
    return Helper::toString(static_cast<uint32_t>(type));
}

bool Tile::isFloodFillFilled(Seat* seat) const
{
    if(getFullness() > 0.0)
        return true;

    switch(getType())
    {
        case TileType::dirt:
        case TileType::gold:
        case TileType::rock:
        {
            if((getFloodFillValue(seat, FloodFillType::ground) != NO_FLOODFILL) &&
               (getFloodFillValue(seat, FloodFillType::groundWater) != NO_FLOODFILL) &&
               (getFloodFillValue(seat, FloodFillType::groundLava) != NO_FLOODFILL) &&
               (getFloodFillValue(seat, FloodFillType::groundWaterLava) != NO_FLOODFILL))
            {
                return true;
            }
            break;
        }
        case TileType::water:
        {
            if((getFloodFillValue(seat, FloodFillType::groundWater) != NO_FLOODFILL) &&
               (getFloodFillValue(seat, FloodFillType::groundWaterLava) != NO_FLOODFILL))
            {
                return true;
            }
            break;
        }
        case TileType::lava:
        {
            if((getFloodFillValue(seat, FloodFillType::groundLava) != NO_FLOODFILL) &&
               (getFloodFillValue(seat, FloodFillType::groundWaterLava) != NO_FLOODFILL))
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

bool Tile::isSameFloodFill(Seat* seat, FloodFillType type, Tile* tile) const
{
    return getFloodFillValue(seat, type) == tile->getFloodFillValue(seat, type);
}

void Tile::resetFloodFill()
{
    for(std::vector<uint32_t>& values : mFloodFillColor)
    {
        for(uint32_t& floodFillValue : values)
        {
            floodFillValue = NO_FLOODFILL;
        }
    }
}

bool Tile::updateFloodFillFromTile(Seat* seat, FloodFillType type, Tile* tile)
{
    if(seat->getTeamIndex() >= mFloodFillColor.size())
    {
        static bool logMsg = false;
        if(!logMsg)
        {
            logMsg = true;
            OD_LOG_ERR("Wrong floodfill seat index seatId=" + Helper::toString(seat->getId())
                + ", seatIndex=" + Helper::toString(seat->getTeamIndex()) + ", floodfillsize=" + Helper::toString(static_cast<uint32_t>(mFloodFillColor.size())));
        }
        return false;
    }

    std::vector<uint32_t>& values = mFloodFillColor[seat->getTeamIndex()];
    uint32_t intType = static_cast<uint32_t>(type);
    if(intType >= values.size())
    {
        static bool logMsg = false;
        if(!logMsg)
        {
            logMsg = true;
            OD_LOG_ERR("Wrong floodfill seat index seatId=" + Helper::toString(seat->getId())
                + ", intType=" + Helper::toString(intType) + ", floodfillsize=" + Helper::toString(static_cast<uint32_t>(values.size())));
        }
        return false;
    }

    if((values[intType] != NO_FLOODFILL) ||
       (tile->getFloodFillValue(seat, type) == NO_FLOODFILL))
    {
        return false;
    }

    values[intType] = tile->getFloodFillValue(seat, type);
    return true;
}

void Tile::replaceFloodFill(Seat* seat, FloodFillType type, uint32_t newValue)
{
    if(seat->getTeamIndex() >= mFloodFillColor.size())
    {
        static bool logMsg = false;
        if(!logMsg)
        {
            logMsg = true;
            OD_LOG_ERR("Wrong floodfill seat index seatId=" + Helper::toString(seat->getId())
                + ", seatIndex=" + Helper::toString(seat->getTeamIndex()) + ", floodfillsize=" + Helper::toString(static_cast<uint32_t>(mFloodFillColor.size())));
        }
        return;
    }

    std::vector<uint32_t>& values = mFloodFillColor[seat->getTeamIndex()];
    uint32_t intType = static_cast<uint32_t>(type);
    if(intType >= values.size())
    {
        static bool logMsg = false;
        if(!logMsg)
        {
            logMsg = true;
            OD_LOG_ERR("Wrong floodfill seat index seatId=" + Helper::toString(seat->getId())
                + ", intType=" + Helper::toString(intType) + ", floodfillsize=" + Helper::toString(static_cast<uint32_t>(values.size())));
        }
        return;
    }

    values[intType] = newValue;
}

void Tile::copyFloodFillToOtherSeats(Seat* seatToCopy)
{
    if(seatToCopy->getTeamIndex() >= mFloodFillColor.size())
    {
        static bool logMsg = false;
        if(!logMsg)
        {
            logMsg = true;
            OD_LOG_ERR("Wrong floodfill seat index seatId=" + Helper::toString(seatToCopy->getId())
                + ", seatIndex=" + Helper::toString(seatToCopy->getTeamIndex()) + ", floodfillsize=" + Helper::toString(static_cast<uint32_t>(mFloodFillColor.size())));
        }
        return;
    }

    std::vector<uint32_t>& valuesToCopy = mFloodFillColor[seatToCopy->getTeamIndex()];
    for(uint32_t indexFloodFill = 0; indexFloodFill < mFloodFillColor.size(); ++indexFloodFill)
    {
        if(seatToCopy->getTeamIndex() == indexFloodFill)
            continue;

        std::vector<uint32_t>& values = mFloodFillColor[indexFloodFill];
        for(uint32_t intType = 0; intType < static_cast<uint32_t>(FloodFillType::nbValues); ++intType)
            values[intType] = valuesToCopy[intType];

    }
}

void Tile::logFloodFill() const
{
    std::string str = "Tile floodfill : " + Tile::displayAsString(this)
        + " - fullness=" + Helper::toString(getFullness())
        + " - seatId=" + std::string(getSeat() == nullptr ? "-1" : Helper::toString(getSeat()->getId()));
    for(const std::vector<uint32_t>& values : mFloodFillColor)
    {
        int cpt = 0;
        for(const uint32_t& floodFill : values)
        {
            str += ", [" + Helper::toString(cpt) + "]=" + Helper::toString(floodFill);
            ++cpt;
        }
    }
    OD_LOG_INF(str);
}

bool Tile::isClaimedForSeat(const Seat* seat) const
{
    if(!isClaimed())
        return false;

    if(getSeat()->canOwnedTileBeClaimedBy(seat))
        return false;

    return true;
}

bool Tile::isClaimed() const
{
    if(!mIsOnServerMap)
        return ((mTileVisual == TileVisual::claimedGround) || (mTileVisual == TileVisual::claimedFull));

    if(getSeat() == nullptr)
        return false;

    if(mClaimedPercentage < 1.0)
        return false;

    return true;
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
    OD_LOG_ERR("Unknown seat id=" + Helper::toString(seat->getId()));
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

uint32_t Tile::getFloodFillValue(Seat* seat, FloodFillType type) const
{
    if(seat->getTeamIndex() >= mFloodFillColor.size())
    {
        static bool logMsg = false;
        if(!logMsg)
        {
            logMsg = true;
            OD_LOG_ERR("Wrong floodfill seat index seatId=" + Helper::toString(seat->getId())
                + ", seatIndex=" + Helper::toString(seat->getTeamIndex()) + ", floodfillsize=" + Helper::toString(static_cast<uint32_t>(mFloodFillColor.size())));
        }
        return NO_FLOODFILL;
    }

    const std::vector<uint32_t>& values = mFloodFillColor[seat->getTeamIndex()];
    uint32_t intType = static_cast<uint32_t>(type);
    if(intType >= values.size())
    {
        static bool logMsg = false;
        if(!logMsg)
        {
            logMsg = true;
            OD_LOG_ERR("Wrong floodfill seat index seatId=" + Helper::toString(seat->getId())
                + ", intType=" + Helper::toString(intType) + ", floodfillsize=" + Helper::toString(static_cast<uint32_t>(values.size())));
        }
        return NO_FLOODFILL;
    }

    return values.at(intType);
}

void Tile::setTeamsNumber(uint32_t nbTeams)
{
    mFloodFillColor = std::vector<std::vector<uint32_t>>(nbTeams, std::vector<uint32_t>(static_cast<uint32_t>(FloodFillType::nbValues), NO_FLOODFILL));
}

std::string Tile::displayAsString(const Tile* tile)
{
    return "[" + Helper::toString(tile->getX()) + ","
         + Helper::toString(tile->getY())+ "]";
}
