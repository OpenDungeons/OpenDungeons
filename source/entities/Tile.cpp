/*
 *  Copyright (C) 2011-2016  OpenDungeons Team
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

#include "entities/Building.h"
#include "entities/Creature.h"
#include "entities/GameEntityType.h"
#include "entities/TreasuryObject.h"
#include "game/Player.h"
#include "game/Seat.h"
#include "gamemap/GameMap.h"
#include "network/ODPacket.h"
#include "render/RenderManager.h"
#include "rooms/Room.h"
#include "sound/SoundEffectsManager.h"
#include "traps/Trap.h"
#include "utils/ConfigManager.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"

#include <cstddef>
#include <bitset>
#include <istream>
#include <ostream>

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#define snprintf_is_banned_in_OD_code _snprintf
#endif


const uint32_t Tile::NO_FLOODFILL = 0;
const std::string Tile::TILE_PREFIX = "Tile_";
const std::string Tile::TILE_SCANF = TILE_PREFIX + "%i_%i";

Tile::Tile(GameMap* gameMap, int x, int y, TileType type, double fullness) :
    GameEntity(gameMap, "", "", nullptr),
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
    mDisplayTileMesh    (true),
    mColorCustomMesh    (true),
    mHasBridge          (false),
    mLocalPlayerHasVision   (false),
    mNbWorkersDigging(0),
    mNbWorkersClaiming(0)
{
    computeTileVisual();
}

Tile::~Tile()
{
    if(!mEntitiesInTile.empty())
    {
        OD_LOG_ERR(getGameMap()->serverStr() + "tile=" + Tile::displayAsString(this) + ", size=" + Helper::toString(mEntitiesInTile.size()));
    }
}

GameEntityType Tile::getObjectType() const
{
    return GameEntityType::tile;
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
        case TileVisual::gemGround:
        case TileVisual::rockFull:
            return false;
        case TileVisual::goldFull:
        case TileVisual::dirtFull:
        case TileVisual::gemFull:
            return true;
        default:
            break;
    }

    // Should be claimed tile
    if(mTileVisual != TileVisual::claimedFull)
    {
        OD_LOG_ERR("tile=" + Tile::displayAsString(this) + ", mTileVisual=" + tileVisualToString(mTileVisual));
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
    if (getFullness() <= 0.0)
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

        case TileType::gem:
            return "Gem";

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

        case TileVisual::gemGround:
            return "gemGround";

        case TileVisual::gemFull:
            return "gemFull";

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
                + ", tile=" + Tile::displayAsString(this)
                + ", seatIndex=" + Helper::toString(seat->getTeamIndex()) + ", floodfillsize=" + Helper::toString(static_cast<uint32_t>(mFloodFillColor.size()))
                + ", fullness=" + Helper::toString(getFullness()));
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
                + ", tile=" + Tile::displayAsString(this)
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
                + ", tile=" + Tile::displayAsString(this)
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
                + ", tile=" + Tile::displayAsString(this)
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
                + ", tile=" + Tile::displayAsString(this)
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
    std::string str = "Floodfill : " + Tile::displayAsString(this)
        + " - type=" + Tile::tileVisualToString(getTileVisual())
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
    if(!getIsOnServerMap())
    {
        if(mTileVisual == TileVisual::claimedGround)
            return true;

        if(mTileVisual == TileVisual::claimedFull)
            return true;

        // For bridges
        if(getHasBridge())
            return true;

        return false;
    }

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
    OD_LOG_ERR("tile=" + Tile::displayAsString(this) + ", unknown seat id=" + Helper::toString(seat->getId()));
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
    switch(getType())
    {
        case TileType::dirt:
            if(mFullness > 0.0)
            {
                if(isClaimed())
                    mTileVisual = TileVisual::claimedFull;
                else
                    mTileVisual = TileVisual::dirtFull;
            }
            else
            {
                if(isClaimed())
                    mTileVisual = TileVisual::claimedGround;
                else
                    mTileVisual = TileVisual::dirtGround;
            }
            return;

        case TileType::rock:
            if(mFullness > 0.0)
                mTileVisual = TileVisual::rockFull;
            else
                mTileVisual = TileVisual::rockGround;
            return;

        case TileType::gold:
            if(mFullness > 0.0)
            {
                if(isClaimed())
                    mTileVisual = TileVisual::claimedFull;
                else
                    mTileVisual = TileVisual::goldFull;
            }
            else
            {
                if(isClaimed())
                    mTileVisual = TileVisual::claimedGround;
                else
                    mTileVisual = TileVisual::goldGround;
            }
            return;

        case TileType::water:
            mTileVisual = TileVisual::waterGround;
            return;

        case TileType::lava:
            mTileVisual = TileVisual::lavaGround;
            return;

        case TileType::gem:
            if(mFullness > 0.0)
                mTileVisual = TileVisual::gemFull;
            else
                mTileVisual = TileVisual::gemGround;
            return;

        default:
            OD_LOG_ERR("Computing tile visual for unknown tile type tile=" + Tile::displayAsString(this) + ", TileType=" + tileTypeToString(getType()));
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
                + ", tile=" + Tile::displayAsString(this)
                + ", seatIndex=" + Helper::toString(seat->getTeamIndex()) + ", floodfillsize=" + Helper::toString(static_cast<uint32_t>(mFloodFillColor.size()))
                + ", fullness=" + Helper::toString(getFullness()));
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
                + ", tile=" + Tile::displayAsString(this)
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

bool Tile::shouldColorTileMesh() const
{
    // We only set color for claimed tiles
    switch(getTileVisual())
    {
        case TileVisual::claimedGround:
        case TileVisual::claimedFull:
            return true;
        default:
            return false;
    }
}

void Tile::exportToStream(Tile* tile, std::ostream& os)
{
    tile->exportToStream(os);
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
        fireTileSound(TileSound::Digged);

        if(!getGameMap()->isInEditorMode())
        {
            // Do a flood fill to update the contiguous region touching the tile.
            for(Seat* seat : getGameMap()->getSeats())
                getGameMap()->refreshFloodFill(seat, this);
        }
    }
}

void Tile::createMeshLocal()
{
    GameEntity::createMeshLocal();

    if(getIsOnServerMap())
        return;

    RenderManager::getSingleton().rrCreateTile(*this, *getGameMap(), *getGameMap()->getLocalPlayer());
}

void Tile::destroyMeshLocal()
{
    GameEntity::destroyMeshLocal();

    if(getIsOnServerMap())
        return;

    RenderManager::getSingleton().rrDestroyTile(*this);
}

bool Tile::isBuildableUpon(Seat* seat) const
{
    if(isFullTile())
        return false;
    if(getIsBuilding())
        return false;
    if(!isClaimedForSeat(seat))
        return false;

    return true;
}

void Tile::setCoveringBuilding(Building *building)
{
    if(mCoveringBuilding == building)
        return;

    // We set the tile as dirty for all seats if needed (we have to check because we
    // don't want to refresh tiles for traps for enemy players)
    if(mCoveringBuilding != nullptr)
    {
        for(std::pair<Seat*, bool>& seatChanged : mTileChangedForSeats)
        {
            if(!mCoveringBuilding->shouldSetCoveringTileDirty(seatChanged.first, this))
                continue;

            seatChanged.second = true;
        }
    }
    mCoveringBuilding = building;
    mIsRoom = false;
    if(getCoveringRoom() != nullptr)
    {
        mIsRoom = true;
        fireTileSound(TileSound::BuildRoom);
    }

    mIsTrap = false;
    if(getCoveringTrap() != nullptr)
    {
        mIsTrap = true;
        fireTileSound(TileSound::BuildTrap);
    }

    if(mCoveringBuilding != nullptr)
    {
        for(std::pair<Seat*, bool>& seatChanged : mTileChangedForSeats)
        {
            if(!mCoveringBuilding->shouldSetCoveringTileDirty(seatChanged.first, this))
                continue;

            seatChanged.second = true;
        }

        // Set the tile as claimed and of the team color of the building
        setSeat(mCoveringBuilding->getSeat());
        mClaimedPercentage = 1.0;
    }
}

bool Tile::isGroundClaimable(Seat* seat) const
{
    if(getFullness() > 0.0)
        return false;

    if(getCoveringBuilding() != nullptr)
        return getCoveringBuilding()->isClaimable(seat);

    if(mType != TileType::dirt && mType != TileType::gold)
        return false;

    if(isClaimedForSeat(seat))
        return false;

    return true;
}

void Tile::exportToPacketForUpdate(ODPacket& os, const Seat* seat) const
{
    GameEntity::exportToPacketForUpdate(os, seat);

    seat->exportTileToPacket(os, this);
}

void Tile::updateFromPacket(ODPacket& is)
{
    GameEntity::updateFromPacket(is);

    // This function should read parameters as sent by Tile::exportToPacketForUpdate
    int seatId;
    std::string meshName;
    std::stringstream ss;

    // We set the seat if there is one
    OD_ASSERT_TRUE(is >> mIsRoom);
    OD_ASSERT_TRUE(is >> mIsTrap);
    OD_ASSERT_TRUE(is >> mRefundPriceRoom);
    OD_ASSERT_TRUE(is >> mRefundPriceTrap);

    OD_ASSERT_TRUE(is >> mDisplayTileMesh);
    OD_ASSERT_TRUE(is >> mColorCustomMesh);
    OD_ASSERT_TRUE(is >> mHasBridge);

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

void Tile::refreshMesh()
{
    if (!isMeshExisting())
        return;

    if(getIsOnServerMap())
        return;

    RenderManager::getSingleton().rrRefreshTile(*this, *getGameMap(), *getGameMap()->getLocalPlayer());
}

void Tile::setSelected(bool ss, const Player* pp)
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


bool Tile::addEntity(GameEntity *entity)
{
    if(std::find(mEntitiesInTile.begin(), mEntitiesInTile.end(), entity) != mEntitiesInTile.end())
    {
        OD_LOG_ERR(getGameMap()->serverStr() + "Trying to insert twice entity=" + entity->getName() + " on tile=" + Tile::displayAsString(this));
        return false;
    }

    mEntitiesInTile.push_back(entity);
    return true;
}

void Tile::removeEntity(GameEntity *entity)
{
    std::vector<GameEntity*>::iterator it = std::find(mEntitiesInTile.begin(), mEntitiesInTile.end(), entity);
    if(it == mEntitiesInTile.end())
    {
        OD_LOG_ERR(getGameMap()->serverStr() + "Trying to remove not inserted entity=" + entity->getName() + " from tile=" + Tile::displayAsString(this));
        return;
    }

    mEntitiesInTile.erase(it);
}


void Tile::claimForSeat(Seat* seat, double nDanceRate)
{
    // If there is a claimable building, we claim it
    if((getCoveringBuilding() != nullptr) &&
        (getCoveringBuilding()->isClaimable(seat)))
    {
        getCoveringBuilding()->claimForSeat(seat, this, nDanceRate);
        return;
    }

    // Claiming walls is less efficient than claiming ground
    if(getFullness() > 0)
        nDanceRate *= ConfigManager::getSingleton().getClaimingWallPenalty();

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
            // We notify the old seat that the tile is lost
            if(getSeat() != nullptr)
                getSeat()->notifyTileClaimedByEnemy(this);

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
}

void Tile::claimTile(Seat* seat)
{
    // Claim the tile.
    // We need this because if we are a client, the tile may be from a non allied seat
    setSeat(seat);
    mClaimedPercentage = 1.0;

    if(isFullTile())
        fireTileSound(TileSound::ClaimWall);
    else
        fireTileSound(TileSound::ClaimGround);

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

double Tile::digOut(double digRate)
{
    // We scle dig rate depending on the tile type
    double digRateScaled;
    double fullnessLost;
    switch(getTileVisual())
    {
        case TileVisual::claimedFull:
            digRateScaled = digRate * 0.2;
            fullnessLost = digRate * 0.2;
            break;
        case TileVisual::dirtFull:
        case TileVisual::goldFull:
            digRateScaled = digRate;
            fullnessLost = digRate;
            break;
        case TileVisual::gemFull:
            digRateScaled = digRate;
            fullnessLost = 0.0;
            break;
        default:
            // Non diggable type!
            OD_LOG_ERR("Wrong tile visual for digging tile=" + Tile::displayAsString(this) + ", visual=" + tileVisualToString(getTileVisual()));
            return 0.0;
    }

    // Nothing to dig
    if(fullnessLost <= 0.0)
        return digRateScaled;

    if(mFullness <= 0.0)
    {
        OD_LOG_ERR("tile=" + Tile::displayAsString(this) + ", mFullness=" + Helper::toString(mFullness));
        return 0.0;
    }

    if(fullnessLost >= mFullness)
    {
        digRateScaled = mFullness;
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
        return digRateScaled;
    }

    digRateScaled = fullnessLost;
    setFullness(mFullness - fullnessLost);
    return digRateScaled;
}

void Tile::fillWithCarryableEntities(Creature* carrier, std::vector<GameEntity*>& entities)
{
    for(GameEntity* entity : mEntitiesInTile)
    {
        if(entity == nullptr)
        {
            OD_LOG_ERR("unexpected null entity in tile=" + Tile::displayAsString(this));
            continue;
        }

        // We check if the entity is already being handled by another creature
        if(entity->getCarryLock(*carrier))
            continue;

        if(entity->getEntityCarryType(carrier) == EntityCarryType::notCarryable)
            continue;

        if (std::find(entities.begin(), entities.end(), entity) == entities.end())
            entities.push_back(entity);
    }
}

bool Tile::isEntityOnTile(GameEntity* entity) const
{
    if(entity == nullptr)
        return false;

    switch(entity->getObjectType())
    {
        case GameEntityType::room:
            return (entity == getCoveringRoom());
        case GameEntityType::trap:
            return (entity == getCoveringTrap());
        default:
            break;
    }
    for(GameEntity* tmpEntity : mEntitiesInTile)
    {
        if(tmpEntity == entity)
            return true;
    }
    return false;
}

uint32_t Tile::countEntitiesOnTile(GameEntityType entityType) const
{
    uint32_t nbItems = 0;
    for(GameEntity* entity : mEntitiesInTile)
    {
        if(entity == nullptr)
        {
            OD_LOG_ERR("unexpected null entity in tile=" + Tile::displayAsString(this));
            continue;
        }

        if(entity->getObjectType() != entityType)
            continue;

        ++nbItems;
    }

    return nbItems;
}

void Tile::fillWithEntities(std::vector<GameEntity*>& entities, SelectionEntityWanted entityWanted, Player* player)
{
    for(GameEntity* entity : mEntitiesInTile)
    {
        if(entity == nullptr)
        {
            OD_LOG_ERR("unexpected null entity in tile=" + Tile::displayAsString(this));
            continue;
        }

        switch(entityWanted)
        {
            case SelectionEntityWanted::any:
            {
                // We accept any entity
                break;
            }
            case SelectionEntityWanted::creatureAliveOwned:
            {
                if(entity->getObjectType() != GameEntityType::creature)
                    continue;

                if(player->getSeat() != entity->getSeat())
                    continue;

                Creature* creature = static_cast<Creature*>(entity);
                if(!creature->isAlive())
                    continue;

                break;
            }
            case SelectionEntityWanted::chicken:
            {
                if(entity->getObjectType() != GameEntityType::chickenEntity)
                    continue;

                break;
            }
            case SelectionEntityWanted::treasuryObjects:
            {
                if(entity->getObjectType() != GameEntityType::treasuryObject)
                    continue;

                break;
            }
            case SelectionEntityWanted::creatureAliveOwnedHurt:
            {
                if(entity->getObjectType() != GameEntityType::creature)
                    continue;

                if(player->getSeat() != entity->getSeat())
                    continue;

                Creature* creature = static_cast<Creature*>(entity);
                if(!creature->isAlive())
                    continue;

                if(!creature->isHurt())
                    continue;

                break;
            }
            case SelectionEntityWanted::creatureAliveAllied:
            {
                if(entity->getObjectType() != GameEntityType::creature)
                    continue;

                if(entity->getSeat() == nullptr)
                    continue;

                if(!player->getSeat()->isAlliedSeat(entity->getSeat()))
                    continue;

                Creature* creature = static_cast<Creature*>(entity);
                if(!creature->isAlive())
                    continue;

                break;
            }
            case SelectionEntityWanted::creatureAliveEnemy:
            {
                if(entity->getObjectType() != GameEntityType::creature)
                    continue;

                if(entity->getSeat() == nullptr)
                    continue;

                if(player->getSeat()->isAlliedSeat(entity->getSeat()))
                    continue;

                Creature* creature = static_cast<Creature*>(entity);
                if(!creature->isAlive())
                    continue;

                break;
            }
            case SelectionEntityWanted::creatureAlive:
            {
                if(entity->getObjectType() != GameEntityType::creature)
                    continue;

                Creature* creature = static_cast<Creature*>(entity);
                if(!creature->isAlive())
                    continue;

                break;
            }
            case SelectionEntityWanted::creatureAliveOrDead:
            {
                if(entity->getObjectType() != GameEntityType::creature)
                    continue;

                break;
            }
            case SelectionEntityWanted::creatureAliveInOwnedPrisonHurt:
            {
                if(entity->getObjectType() != GameEntityType::creature)
                    continue;

                Creature* creature = static_cast<Creature*>(entity);
                if(!creature->isAlive())
                    continue;

                if(!creature->isInPrison())
                    continue;

                if(!creature->getSeatPrison()->canOwnedCreatureBePickedUpBy(player->getSeat()))
                    continue;

                break;
            }
            case SelectionEntityWanted::creatureAliveEnemyAttackable:
            {
                if(entity->getObjectType() != GameEntityType::creature)
                    continue;

                if(entity->getSeat() == nullptr)
                    continue;

                if(player->getSeat()->isAlliedSeat(entity->getSeat()))
                    continue;

                Creature* creature = static_cast<Creature*>(entity);
                if(!creature->isAlive())
                    continue;

                if(!creature->isAttackable(this, player->getSeat()))
                    continue;

                break;
            }
            default:
            {
                static bool logMsg = false;
                if(!logMsg)
                {
                    logMsg = true;
                    OD_LOG_ERR("Wrong SelectionEntityWanted int=" + Helper::toString(static_cast<uint32_t>(entityWanted)));
                }
                continue;
            }
        }

        if (std::find(entities.begin(), entities.end(), entity) != entities.end())
            continue;

        entities.push_back(entity);
    }
}

bool Tile::addTreasuryObject(TreasuryObject* obj)
{
    if (std::find(mEntitiesInTile.begin(), mEntitiesInTile.end(), obj) != mEntitiesInTile.end())
    {
        OD_LOG_ERR(getGameMap()->serverStr() + "Trying to insert twice treasury=" + obj->getName() + " on tile=" + Tile::displayAsString(this));
        return false;
    }

    if(!getIsOnServerMap())
    {
        // On client side, we add the entity to tile. Merging is relevant on server side only
        mEntitiesInTile.push_back(obj);
        return true;
    }

    // If there is already a treasury object, we merge it
    bool isMerged = false;
    for(GameEntity* entity : mEntitiesInTile)
    {
        if(entity == nullptr)
        {
            OD_LOG_ERR("unexpected null entity in tile=" + Tile::displayAsString(this));
            continue;
        }

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

bool Tile::checkCoveringRoomType(RoomType type) const
{
    Room* coveringRoom = getCoveringRoom();
    if(coveringRoom == nullptr)
        return false;

    return (coveringRoom->getType() == type);
}

Trap* Tile::getCoveringTrap() const
{
    if(mCoveringBuilding == nullptr)
        return nullptr;

    if(mCoveringBuilding->getObjectType() != GameEntityType::trap)
        return nullptr;

    return static_cast<Trap*>(mCoveringBuilding);
}

bool Tile::checkCoveringTrapType(TrapType type) const
{
    Trap* coveringTrap = getCoveringTrap();
    if(coveringTrap == nullptr)
        return false;

    return (coveringTrap->getType() == type);
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

void Tile::setDirtyForAllSeats()
{
    if(!getIsOnServerMap())
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
}


bool Tile::isFullTile() const
{
    if(getIsOnServerMap())
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

bool Tile::permitsVision()
{
    if(isFullTile())
        return false;

    if((getCoveringBuilding() != nullptr) &&
       (!getCoveringBuilding()->permitsVision(this)))
    {
        return false;
    }

    return true;
}

void Tile::fireTileSound(TileSound sound)
{
    std::string soundFamily;
    switch(sound)
    {
        case TileSound::ClaimGround:
            soundFamily = "ClaimTile";
            break;
        case TileSound::ClaimWall:
            soundFamily = "ClaimTile";
            break;
        case TileSound::Digged:
            soundFamily = "RocksFalling";
            break;
        case TileSound::BuildRoom:
            soundFamily = "BuildRoom";
            break;
        case TileSound::BuildTrap:
            soundFamily = "BuildTrap";
            break;
        default:
            OD_LOG_ERR("Wrong TileSound value=" + Helper::toString(static_cast<uint32_t>(sound)));
            return;
    }

    getGameMap()->fireGameSound(*this, soundFamily);
}

double Tile::getCreatureSpeedDefault(const Creature* creature) const
{
    // If we are on a full tile, we set the speed to ground speed. That can happen
    // on client side if there is a desynchro between server and client and the
    // creature is not exactly on the same tile
    if(!getIsOnServerMap() && isFullTile())
        return creature->getMoveSpeedGround();

    switch(getTileVisual())
    {
        case TileVisual::dirtGround:
        case TileVisual::goldGround:
        case TileVisual::rockGround:
        case TileVisual::claimedGround:
            return creature->getMoveSpeedGround();
        case TileVisual::waterGround:
            return creature->getMoveSpeedWater();
        case TileVisual::lavaGround:
            return creature->getMoveSpeedLava();
        default:
            return 0.0;
    }
}

bool Tile::canWorkerClaim(const Creature& worker)
{
    if(mNbWorkersClaiming < ConfigManager::getSingleton().getNbWorkersClaimSameTile())
        return true;

    return false;
}

bool Tile::addWorkerClaiming(const Creature& worker)
{
    if(!canWorkerClaim(worker))
        return false;

    ++mNbWorkersClaiming;
    return true;
}

bool Tile::removeWorkerClaiming(const Creature& worker)
{
    // Sanity check
    if(mNbWorkersClaiming <= 0)
    {
        OD_LOG_ERR("Cannot remove worker=" + worker.getName() + ", tile=" + Tile::displayAsString(this));
        return false;
    }

    --mNbWorkersClaiming;
    return true;
}

bool Tile::canWorkerDig(const Creature& worker)
{
    if(mNbWorkersDigging < ConfigManager::getSingleton().getNbWorkersDigSameTile())
        return true;

    return false;
}

bool Tile::addWorkerDigging(const Creature& worker)
{
    if(!canWorkerDig(worker))
        return false;

    ++mNbWorkersDigging;
    return true;
}

bool Tile::removeWorkerDigging(const Creature& worker)
{
    // Sanity check
    if(mNbWorkersDigging <= 0)
    {
        OD_LOG_ERR("Cannot remove worker=" + worker.getName() + ", tile=" + Tile::displayAsString(this));
        return false;
    }

    --mNbWorkersDigging;
    return true;
}

std::string Tile::displayAsString(const Tile* tile)
{
    if(tile == nullptr)
        return "nullptr";

    return "[" + Helper::toString(tile->getX()) + ","
         + Helper::toString(tile->getY())+ "]";
}
