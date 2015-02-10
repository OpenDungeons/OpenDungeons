/*!
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

#include "ai/KeeperAI.h"

#include "entities/Creature.h"
#include "gamemap/GameMap.h"
#include "entities/Tile.h"
#include "rooms/Room.h"
#include "rooms/RoomCrypt.h"
#include "rooms/RoomDormitory.h"
#include "rooms/RoomForge.h"
#include "rooms/RoomHatchery.h"
#include "rooms/RoomLibrary.h"
#include "rooms/RoomTrainingHall.h"
#include "rooms/RoomTreasury.h"
#include "spell/Spell.h"
#include "utils/LogManager.h"
#include "utils/Random.h"

#include <vector>

KeeperAI::KeeperAI(GameMap& gameMap, Player& player, const std::string& parameters):
    BaseAI(gameMap, player, parameters),
    mCooldownCheckTreasury(0),
    mCooldownLookingForRooms(0),
    mRoomPosX(-1),
    mRoomPosY(-1),
    mRoomSize(-1),
    mNoMoreReachableGold(false),
    mCooldownLookingForGold(0),
    mCooldownDefense(0),
    mCooldownWorkers(0)
{
}

bool KeeperAI::doTurn(double frameTime)
{
    // If we have no dungeon temple, we are dead
    if(getDungeonTemple() == nullptr)
        return false;

    saveWoundedCreatures();

    handleDefense();

    if (handleWorkers())
        return true;

    if (checkTreasury())
        return true;

    if (handleRooms())
        return true;

    if (lookForGold())
        return true;

    return true;
}

bool KeeperAI::checkTreasury()
{
    // If the treasury gets destroyed, we don't want the AI to build each turn the
    // free treasury
    if(mCooldownCheckTreasury > 0)
    {
        --mCooldownCheckTreasury;
        return false;
    }

    std::vector<Room*> treasuriesOwned = mGameMap.getRoomsByTypeAndSeat(RoomType::treasury,
        mPlayer.getSeat());

    int nbTilesTreasuries = 0;
    for(Room* treasury : treasuriesOwned)
        nbTilesTreasuries += treasury->numCoveredTiles();

    // We want at least 3 tiles for a treasury
    if(nbTilesTreasuries >= 3)
        return false;

    // The treasury is too small, we try to increase it
    int totalGold = 0;
    for(Room* treasury : treasuriesOwned)
    {
        RoomTreasury* rt = static_cast<RoomTreasury*>(treasury);
        totalGold += rt->getTotalGold();
    }

    // A treasury can be built if we have none (in this case, it is free). Otherwise,
    // we check if we have enough gold
    if(nbTilesTreasuries > 0 && totalGold < Room::costPerTile(RoomType::treasury))
        return false;

    Tile* central = getDungeonTemple()->getCentralTile();

    Creature* kobold = mGameMap.getKoboldForPathFinding(mPlayer.getSeat());
    if (kobold == nullptr)
        return false;

    // We try in priority to gold next to an existing treasury
    for(Room* treasury : treasuriesOwned)
    {
        for(Tile* tile : treasury->getCoveredTiles())
        {
            for(Tile* neigh : tile->getAllNeighbors())
            {
                if(neigh->isBuildableUpon() && neigh->isClaimedForSeat(mPlayer.getSeat()) &&
                   mGameMap.pathExists(kobold, central, neigh))
                {
                    std::vector<Tile*> tiles;
                    int goldRequired;
                    mGameMap.fillBuildableTilesAndPriceForPlayerInArea(neigh->getX(), neigh->getY(),
                        neigh->getX(), neigh->getY(), &mPlayer, RoomType::treasury, tiles, goldRequired);
                    if (tiles.empty())
                        return false;

                    if(!mGameMap.withdrawFromTreasuries(goldRequired, mPlayer.getSeat()))
                        return false;

                    Room* room = new RoomTreasury(&mGameMap);
                    buildRoom(room, tiles);
                    mCooldownCheckTreasury = Random::Int(10,30);
                    return true;
                }
            }
        }
    }

    int widerSide = mGameMap.getMapSizeX() > mGameMap.getMapSizeY() ?
        mGameMap.getMapSizeX() : mGameMap.getMapSizeY();

    // If we have found no tile available to an existing treasury, we search for the closest
    // buildable claimed tile available
    Tile* firstAvailableTile = nullptr;
    for(int32_t distance = 1; distance < widerSide; ++distance)
    {
        for(int k = 0; k <= distance; ++k)
        {
            Tile* t;
            // North-East
            t = mGameMap.getTile(central->getX() + k, central->getY() + distance);
            if(t != nullptr && t->isBuildableUpon() && t->isClaimedForSeat(mPlayer.getSeat()) &&
               mGameMap.pathExists(kobold, central, t))
            {
                firstAvailableTile = t;
                break;
            }
            // North-West
            if(k > 0)
            {
                t = mGameMap.getTile(central->getX() - k, central->getY() + distance);
                if(t != nullptr && t->isBuildableUpon() && t->isClaimedForSeat(mPlayer.getSeat()) &&
                   mGameMap.pathExists(kobold, central, t))
                {
                    firstAvailableTile = t;
                    break;
                }
            }
            // South-East
            t = mGameMap.getTile(central->getX() + k, central->getY() - distance);
            if(t != nullptr && t->isBuildableUpon() && t->isClaimedForSeat(mPlayer.getSeat()) &&
               mGameMap.pathExists(kobold, central, t))
            {
                firstAvailableTile = t;
                break;
            }
            // South-West
            if(k > 0)
            {
                t = mGameMap.getTile(central->getX() - k, central->getY() - distance);
                if(t != nullptr && t->isBuildableUpon() && t->isClaimedForSeat(mPlayer.getSeat()) &&
                   mGameMap.pathExists(kobold, central, t))
                {
                    firstAvailableTile = t;
                    break;
                }
            }
            // East-North
            t = mGameMap.getTile(central->getX() + distance, central->getY() + k);
            if(t != nullptr && t->isBuildableUpon() && t->isClaimedForSeat(mPlayer.getSeat()) &&
               mGameMap.pathExists(kobold, central, t))
            {
                firstAvailableTile = t;
                break;
            }
            // East-South
            if(k > 0)
            {
                t = mGameMap.getTile(central->getX() + distance, central->getY() - k);
                if(t != nullptr && t->isBuildableUpon() && t->isClaimedForSeat(mPlayer.getSeat()) &&
                   mGameMap.pathExists(kobold, central, t))
                {
                    firstAvailableTile = t;
                    break;
                }
            }
            // West-North
            t = mGameMap.getTile(central->getX() - distance, central->getY() + k);
            if(t != nullptr && t->isBuildableUpon() && t->isClaimedForSeat(mPlayer.getSeat()) &&
               mGameMap.pathExists(kobold, central, t))
            {
                firstAvailableTile = t;
                break;
            }
            // West-South
            if(k > 0)
            {
                t = mGameMap.getTile(central->getX() - distance, central->getY() - k);
                if(t != nullptr && t->isBuildableUpon() && t->isClaimedForSeat(mPlayer.getSeat()) &&
                   mGameMap.pathExists(kobold, central, t))
                {
                    firstAvailableTile = t;
                    break;
                }
            }
        }

        if(firstAvailableTile != nullptr)
            break;
    }

    // We couldn't find any available tile T_T
    // We return true to avoid doing something else to let kobolds claim
    if(firstAvailableTile == nullptr)
        return true;

    std::vector<Tile*> tiles;
    int goldRequired;
    mGameMap.fillBuildableTilesAndPriceForPlayerInArea(firstAvailableTile->getX(), firstAvailableTile->getY(),
        firstAvailableTile->getX(), firstAvailableTile->getY(), &mPlayer, RoomType::treasury, tiles, goldRequired);
    if (tiles.empty())
        return false;

    if(!mGameMap.withdrawFromTreasuries(goldRequired, mPlayer.getSeat()))
        return false;

    Room* room = new RoomTreasury(&mGameMap);
    buildRoom(room, tiles);
    mCooldownCheckTreasury = Random::Int(10,30);
    return true;
}

bool KeeperAI::handleRooms()
{
    if(mCooldownLookingForRooms > 0)
    {
        --mCooldownLookingForRooms;
        return false;
    }

    mCooldownLookingForRooms = Random::Int(30,60);

    // We check if the last built room is done
    if(mRoomSize != -1)
    {
        Tile* tile = mGameMap.getTile(mRoomPosX, mRoomPosY);
        OD_ASSERT_TRUE(tile != nullptr);
        if(tile == nullptr)
        {
            mRoomSize = -1;
            return false;
        }
        int32_t points;
        if(!computePointsForRoom(tile, mPlayer.getSeat(), mRoomSize, true, false, points))
        {
            // The room is not valid anymore (may be claimed or built by somebody else). We redo
            mRoomSize = -1;
            return false;
        }

        if(buildMostNeededRoom())
        {
            mRoomSize = -1;
            return true;
        }

        return false;
    }

    Tile* central = getDungeonTemple()->getCentralTile();
    int32_t bestX = 0;
    int32_t bestY = 0;
    if(!findBestPlaceForRoom(central, mPlayer.getSeat(), 5, true, bestX, bestY))
        return false;

    mRoomSize = 5;
    mRoomPosX = bestX;
    mRoomPosY = bestY;

    Tile* tileDest = mGameMap.getTile(mRoomPosX, mRoomPosY);
    OD_ASSERT_TRUE_MSG(tileDest != nullptr, "tileDest=" + Tile::displayAsString(tileDest));
    if(!digWayToTile(central, tileDest))
        return false;

    for(int xx = 0; xx < mRoomSize; ++xx)
    {
        for(int yy = 0; yy < mRoomSize; ++yy)
        {
            Tile* tile = mGameMap.getTile(mRoomPosX + xx, mRoomPosY + yy);
            OD_ASSERT_TRUE(tile != nullptr);
            if(tile == nullptr)
                continue;

            tile->setMarkedForDigging(true, &mPlayer);
        }
    }

    return true;
}

bool KeeperAI::lookForGold()
{
    if (mNoMoreReachableGold)
        return false;

    if(mCooldownLookingForGold > 0)
    {
        --mCooldownLookingForGold;
        return false;
    }

    mCooldownLookingForGold = Random::Int(70,120);

    // Do we need gold ?
    int emptyStorage = 0;
    std::vector<Room*> treasuriesOwned = mGameMap.getRoomsByTypeAndSeat(RoomType::treasury,
        mPlayer.getSeat());
    for(Room* room : treasuriesOwned)
    {
        RoomTreasury* rt = static_cast<RoomTreasury*>(room);
        emptyStorage += rt->emptyStorageSpace();
    }

    // No need to search for gold
    if(emptyStorage < 100)
        return false;

    Tile* central = getDungeonTemple()->getCentralTile();
    int widerSide = mGameMap.getMapSizeX() > mGameMap.getMapSizeY() ?
        mGameMap.getMapSizeX() : mGameMap.getMapSizeY();

    // We search for the closest gold tile
    Tile* firstGoldTile = nullptr;
    for(int32_t distance = 1; distance < widerSide; ++distance)
    {
        for(int k = 0; k <= distance; ++k)
        {
            Tile* t;
            // North-East
            t = mGameMap.getTile(central->getX() + k, central->getY() + distance);
            if(t != nullptr && t->getType() == TileType::gold && t->getFullness() > 0.0)
            {
                // If we already have a tile at same distance, we randomly change to
                // try to not be too predictable
                if((firstGoldTile == nullptr) || (Random::Uint(1,2) == 1))
                    firstGoldTile = t;
            }
            // North-West
            if(k > 0)
            {
                t = mGameMap.getTile(central->getX() - k, central->getY() + distance);
                if(t != nullptr && t->getType() == TileType::gold && t->getFullness() > 0.0)
                {
                    if((firstGoldTile == nullptr) || (Random::Uint(1,2) == 1))
                        firstGoldTile = t;
                }
            }
            // South-East
            t = mGameMap.getTile(central->getX() + k, central->getY() - distance);
            if(t != nullptr && t->getType() == TileType::gold && t->getFullness() > 0.0)
            {
                if((firstGoldTile == nullptr) || (Random::Uint(1,2) == 1))
                    firstGoldTile = t;
            }
            // South-West
            if(k > 0)
            {
                t = mGameMap.getTile(central->getX() - k, central->getY() - distance);
                if(t != nullptr && t->getType() == TileType::gold && t->getFullness() > 0.0)
                {
                    if((firstGoldTile == nullptr) || (Random::Uint(1,2) == 1))
                        firstGoldTile = t;
                }
            }
            // East-North
            t = mGameMap.getTile(central->getX() + distance, central->getY() + k);
            if(t != nullptr && t->getType() == TileType::gold && t->getFullness() > 0.0)
            {
                if((firstGoldTile == nullptr) || (Random::Uint(1,2) == 1))
                    firstGoldTile = t;
            }
            // East-South
            if(k > 0)
            {
                t = mGameMap.getTile(central->getX() + distance, central->getY() - k);
                if(t != nullptr && t->getType() == TileType::gold && t->getFullness() > 0.0)
                {
                    if((firstGoldTile == nullptr) || (Random::Uint(1,2) == 1))
                        firstGoldTile = t;
                }
            }
            // West-North
            t = mGameMap.getTile(central->getX() - distance, central->getY() + k);
            if(t != nullptr && t->getType() == TileType::gold && t->getFullness() > 0.0)
            {
                if((firstGoldTile == nullptr) || (Random::Uint(1,2) == 1))
                    firstGoldTile = t;
            }
            // West-South
            if(k > 0)
            {
                t = mGameMap.getTile(central->getX() - distance, central->getY() - k);
                if(t != nullptr && t->getType() == TileType::gold && t->getFullness() > 0.0)
                {
                    if((firstGoldTile == nullptr) || (Random::Uint(1,2) == 1))
                        firstGoldTile = t;
                }
            }

            if(firstGoldTile != nullptr)
                break;
        }

        // If we found a tile, no need to continue
        if(firstGoldTile != nullptr)
            break;
    }

    // No more gold
    if (firstGoldTile == nullptr)
    {
        mNoMoreReachableGold = true;
        return false;
    }

    if(!digWayToTile(central, firstGoldTile))
    {
        mNoMoreReachableGold = true;
        return false;
    }

    // If the neighbors are gold, we dig them
    const int levelTilesDig = 2;
    std::set<Tile*> tilesDig;
    // Because we can't insert Tiles in tilesDig while iterating, we use a copy: tilesDigTmp
    std::set<Tile*> tilesDigTmp;
    tilesDig.insert(firstGoldTile);

    for(int i = 0; i < levelTilesDig; ++i)
    {
        tilesDigTmp = tilesDig;
        for(Tile* tile : tilesDigTmp)
        {
            for(Tile* neigh : tile->getAllNeighbors())
            {
                if(neigh->getType() == TileType::gold && neigh->getFullness() > 0.0)
                    tilesDig.insert(neigh);
            }
        }
    }

    for(Tile* tile : tilesDig)
        tile->setMarkedForDigging(true, &mPlayer);

    return true;
}

bool KeeperAI::buildMostNeededRoom()
{
    std::vector<Room*> rooms;

    // Dormitory
    rooms = mGameMap.getRoomsByTypeAndSeat(RoomType::dormitory, mPlayer.getSeat());
    uint32_t nbDormitory = rooms.size();
    if(nbDormitory == 0)
    {
        std::vector<Tile*> tiles;
        int goldRequired;
        mGameMap.fillBuildableTilesAndPriceForPlayerInArea(mRoomPosX, mRoomPosY, mRoomPosX + mRoomSize - 1,
            mRoomPosY + mRoomSize - 1, &mPlayer, RoomType::dormitory, tiles, goldRequired);
        if (tiles.size() < static_cast<uint32_t>(mRoomSize * mRoomSize))
            return false;

        if(!mGameMap.withdrawFromTreasuries(goldRequired, mPlayer.getSeat()))
            return false;

        Room* room = new RoomDormitory(&mGameMap);
        buildRoom(room, tiles);
        return true;
    }

    std::vector<Room*> treasuriesOwned = mGameMap.getRoomsByTypeAndSeat(RoomType::treasury,
        mPlayer.getSeat());
    int emptyStorage = 0;
    int totalGold = 0;
    for(Room* room : treasuriesOwned)
    {
        RoomTreasury* rt = static_cast<RoomTreasury*>(room);
        emptyStorage += rt->emptyStorageSpace();
        totalGold += rt->getTotalGold();
    }

    if((emptyStorage < 100) && (totalGold < 20000))
    {
        std::vector<Tile*> tiles;
        int goldRequired;
        mGameMap.fillBuildableTilesAndPriceForPlayerInArea(mRoomPosX, mRoomPosY, mRoomPosX + mRoomSize - 1,
            mRoomPosY + mRoomSize - 1, &mPlayer, RoomType::treasury, tiles, goldRequired);
        if (tiles.size() < static_cast<uint32_t>(mRoomSize * mRoomSize))
            return false;

        if(!mGameMap.withdrawFromTreasuries(goldRequired, mPlayer.getSeat()))
            return false;

        Room* room = new RoomTreasury(&mGameMap);
        buildRoom(room, tiles);
        return true;
    }

    // hatchery
    rooms = mGameMap.getRoomsByTypeAndSeat(RoomType::hatchery, mPlayer.getSeat());
    if(rooms.empty())
    {
        std::vector<Tile*> tiles;
        int goldRequired;
        mGameMap.fillBuildableTilesAndPriceForPlayerInArea(mRoomPosX, mRoomPosY, mRoomPosX + mRoomSize - 1,
            mRoomPosY + mRoomSize - 1, &mPlayer, RoomType::hatchery, tiles, goldRequired);
        if (tiles.size() < static_cast<uint32_t>(mRoomSize * mRoomSize))
            return false;

        if(!mGameMap.withdrawFromTreasuries(goldRequired, mPlayer.getSeat()))
            return false;

        Room* room = new RoomHatchery(&mGameMap);
        buildRoom(room, tiles);
        return true;
    }

    // trainingHall
    rooms = mGameMap.getRoomsByTypeAndSeat(RoomType::trainingHall, mPlayer.getSeat());
    if(rooms.empty())
    {
        std::vector<Tile*> tiles;
        int goldRequired;
        mGameMap.fillBuildableTilesAndPriceForPlayerInArea(mRoomPosX, mRoomPosY, mRoomPosX + mRoomSize - 1,
            mRoomPosY + mRoomSize - 1, &mPlayer, RoomType::trainingHall, tiles, goldRequired);
        if (tiles.size() < static_cast<uint32_t>(mRoomSize * mRoomSize))
            return false;

        if(!mGameMap.withdrawFromTreasuries(goldRequired, mPlayer.getSeat()))
            return false;

        Room* room = new RoomTrainingHall(&mGameMap);
        buildRoom(room, tiles);
        return true;
    }

    // forge
    rooms = mGameMap.getRoomsByTypeAndSeat(RoomType::forge, mPlayer.getSeat());
    if(rooms.empty())
    {
        std::vector<Tile*> tiles;
        int goldRequired;
        mGameMap.fillBuildableTilesAndPriceForPlayerInArea(mRoomPosX, mRoomPosY, mRoomPosX + mRoomSize - 1,
            mRoomPosY + mRoomSize - 1, &mPlayer, RoomType::forge, tiles, goldRequired);
        if (tiles.size() < static_cast<uint32_t>(mRoomSize * mRoomSize))
            return false;

        if(!mGameMap.withdrawFromTreasuries(goldRequired, mPlayer.getSeat()))
            return false;

        Room* room = new RoomForge(&mGameMap);
        buildRoom(room, tiles);
        return true;
    }

    // library
    rooms = mGameMap.getRoomsByTypeAndSeat(RoomType::library, mPlayer.getSeat());
    if(rooms.empty())
    {
        std::vector<Tile*> tiles;
        int goldRequired;
        mGameMap.fillBuildableTilesAndPriceForPlayerInArea(mRoomPosX, mRoomPosY, mRoomPosX + mRoomSize - 1,
            mRoomPosY + mRoomSize - 1, &mPlayer, RoomType::library, tiles, goldRequired);
        if (tiles.size() < static_cast<uint32_t>(mRoomSize * mRoomSize))
            return false;

        if(!mGameMap.withdrawFromTreasuries(goldRequired, mPlayer.getSeat()))
            return false;

        Room* room = new RoomLibrary(&mGameMap);
        buildRoom(room, tiles);
        return true;
    }

    // Once we have done all the basic buildings, we go for another dormitory
    if(nbDormitory == 1)
    {
        std::vector<Tile*> tiles;
        int goldRequired;
        mGameMap.fillBuildableTilesAndPriceForPlayerInArea(mRoomPosX, mRoomPosY, mRoomPosX + mRoomSize - 1,
            mRoomPosY + mRoomSize - 1, &mPlayer, RoomType::dormitory, tiles, goldRequired);
        if (tiles.size() < static_cast<uint32_t>(mRoomSize * mRoomSize))
            return false;

        if(!mGameMap.withdrawFromTreasuries(goldRequired, mPlayer.getSeat()))
            return false;

        Room* room = new RoomDormitory(&mGameMap);
        buildRoom(room, tiles);
        return true;
    }

    // Crypt
    rooms = mGameMap.getRoomsByTypeAndSeat(RoomType::crypt, mPlayer.getSeat());
    if(rooms.empty())
    {
        std::vector<Tile*> tiles;
        int goldRequired;
        mGameMap.fillBuildableTilesAndPriceForPlayerInArea(mRoomPosX, mRoomPosY, mRoomPosX + mRoomSize - 1,
            mRoomPosY + mRoomSize - 1, &mPlayer, RoomType::crypt, tiles, goldRequired);
        if (tiles.size() < static_cast<uint32_t>(mRoomSize * mRoomSize))
            return false;

        if(!mGameMap.withdrawFromTreasuries(goldRequired, mPlayer.getSeat()))
            return false;

        Room* room = new RoomCrypt(&mGameMap);
        buildRoom(room, tiles);
        return true;
    }

    return true;
}

void KeeperAI::saveWoundedCreatures()
{
    Tile* dungeonTempleTile = getDungeonTemple()->getCentralTile();
    OD_ASSERT_TRUE(dungeonTempleTile != nullptr);

    Seat* seat = mPlayer.getSeat();
    std::vector<Creature*> creatures = mGameMap.getCreaturesBySeat(seat);
    for(Creature* creature : creatures)
    {
        // We take away fleeing creatures not too near our dungeon heart
        if(!creature->isActionInList(CreatureActionType::flee))
            continue;
        Tile* tile = creature->getPositionTile();
        if(tile == nullptr)
            continue;

        if((std::abs(dungeonTempleTile->getX() - tile->getX()) <= 5) &&
           (std::abs(dungeonTempleTile->getY() - tile->getY()) <= 5))
        {
            // We are too close from our dungeon heart to be picked up
            continue;
        }

        if(!creature->tryPickup(seat))
            continue;

        mPlayer.pickUpEntity(creature);

        OD_ASSERT_TRUE(mPlayer.dropHand(dungeonTempleTile) == creature);
    }
}

void KeeperAI::handleDefense()
{
    if(mCooldownDefense > 0)
    {
        --mCooldownDefense;
        return;
    }

    Tile* dungeonTempleTile = getDungeonTemple()->getCentralTile();
    OD_ASSERT_TRUE(dungeonTempleTile != nullptr);

    Seat* seat = mPlayer.getSeat();
    // We drop creatures nearby owned or allied attacked creatures
    std::vector<Creature*> creatures = mGameMap.getCreaturesByAlliedSeat(seat);
    for(Creature* creature : creatures)
    {
        // We check if a creature is fighting near a claimed tile. If yes, we drop a creature nearby
        if(!creature->isActionInList(CreatureActionType::fight))
            continue;

        Tile* tile = creature->getPositionTile();
        if(tile == nullptr)
            continue;

        Creature* creatureToDrop = mGameMap.getFighterToPickupBySeat(seat);
        if(creatureToDrop == nullptr)
            continue;

        if(!creatureToDrop->tryPickup(seat))
            continue;

        for(Tile* neigh : tile->getAllNeighbors())
        {
            if(creatureToDrop->tryDrop(seat, neigh))
            {
                mPlayer.pickUpEntity(creatureToDrop);
                OD_ASSERT_TRUE(mPlayer.dropHand(neigh) == creatureToDrop);
                mCooldownDefense = Random::Int(0,5);
                return;
            }
        }
    }
}

bool KeeperAI::handleWorkers()
{
    if(mCooldownWorkers > 0)
    {
        --mCooldownWorkers;
        return false;
    }

    mCooldownWorkers = Random::Int(3,10);

    Tile* central = getDungeonTemple()->getCentralTile();
    std::vector<Tile*> tiles;
    tiles.push_back(central);
    int summonCost = Spell::getSpellCost(&mGameMap, SpellType::summonWorker, tiles, &mPlayer);
    int mana = static_cast<int>(mPlayer.getSeat()->getMana());
    if(mana < summonCost)
        return false;

    // If we have less than 4 workers or we have the chance, we summon
    int nbWorkers = mGameMap.getNbWorkersForSeat(mPlayer.getSeat());
    if((nbWorkers < 4) ||
       (Random::Int(0, nbWorkers * 3) == 0))
    {
        mPlayer.getSeat()->takeMana(mana);
        Spell::castSpell(&mGameMap, SpellType::summonWorker, tiles, &mPlayer);
        return true;
    }

    return false;
}
