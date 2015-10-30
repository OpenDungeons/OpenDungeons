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

#include "game/Seat.h"

#include "entities/Building.h"
#include "entities/CreatureDefinition.h"
#include "entities/Tile.h"
#include "game/Player.h"
#include "game/Research.h"
#include "game/ResearchManager.h"
#include "gamemap/GameMap.h"
#include "goals/Goal.h"
#include "network/ODServer.h"
#include "network/ServerNotification.h"
#include "render/RenderManager.h"
#include "rooms/Room.h"
#include "rooms/RoomManager.h"
#include "rooms/RoomType.h"
#include "spawnconditions/SpawnCondition.h"
#include "traps/Trap.h"
#include "traps/TrapManager.h"
#include "utils/ConfigManager.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"
#include "utils/Random.h"

#include <istream>
#include <ostream>

const std::string Seat::PLAYER_TYPE_HUMAN = "Human";
const std::string Seat::PLAYER_TYPE_AI = "AI";
const std::string Seat::PLAYER_TYPE_INACTIVE = "Inactive";
const std::string Seat::PLAYER_TYPE_CHOICE = "Choice";
const std::string Seat::PLAYER_FACTION_CHOICE = "Choice";

TileStateNotified::TileStateNotified():
    mTileVisual(TileVisual::nullTileVisual),
    mSeatIdOwner(-1),
    mMarkedForDigging(false),
    mVisionTurnLast(false),
    mVisionTurnCurrent(false),
    mBuilding(nullptr)
{
}


Seat::Seat(GameMap* gameMap) :
    mGameMap(gameMap),
    mPlayer(nullptr),
    mTeamId(-1),
    mMana(1000),
    mManaDelta(0),
    mStartingX(0),
    mStartingY(0),
    mGoldMined(0),
    mNumCreaturesFighters(0),
    mNumCreaturesFightersMax(0),
    mNumCreaturesWorkers(0),
    mDefaultWorkerClass(nullptr),
    mNumClaimedTiles(0),
    mHasGoalsChanged(true),
    mGold(0),
    mGoldMax(0),
    mId(-1),
    mTeamIndex(0),
    mNbRooms(std::vector<uint32_t>(static_cast<uint32_t>(RoomType::nbRooms), 0)),
    mIsDebuggingVision(false),
    mResearchPoints(0),
    mCurrentResearchType(ResearchType::nullResearchType),
    mCurrentResearchProgress(0.0f),
    mCurrentResearch(nullptr),
    mGuiResearchNeedsRefresh(false),
    mConfigPlayerId(-1),
    mConfigTeamId(-1),
    mConfigFactionIndex(-1),
    mKoCreatures(false)
{
}

void Seat::setTeamId(int teamId)
{
    OD_ASSERT_TRUE_MSG(std::find(mAvailableTeamIds.begin(), mAvailableTeamIds.end(),
        teamId) != mAvailableTeamIds.end(), "Unknown team id=" + Helper::toString(teamId)
        + ", for seat id=" + Helper::toString(getId()));
    OD_ASSERT_TRUE_MSG(teamId != 0 || isRogueSeat(), "Invalid rogue team id for seat id=" + Helper::toString(getId()));
    mTeamId = teamId;
}

void Seat::addGoal(Goal* g)
{
    mUncompleteGoals.push_back(g);
}

unsigned int Seat::numUncompleteGoals()
{
    unsigned int tempUnsigned = mUncompleteGoals.size();

    return tempUnsigned;
}

Goal* Seat::getUncompleteGoal(unsigned int index)
{
    if (index >= mUncompleteGoals.size())
        return nullptr;

    return mUncompleteGoals[index];
}

void Seat::clearUncompleteGoals()
{
    mUncompleteGoals.clear();
}

void Seat::clearCompletedGoals()
{
    mCompletedGoals.clear();
}

unsigned int Seat::numCompletedGoals()
{
    return mCompletedGoals.size();
}

Goal* Seat::getCompletedGoal(unsigned int index)
{
    if (index >= mCompletedGoals.size())
        return nullptr;

    return mCompletedGoals[index];
}

unsigned int Seat::numFailedGoals()
{
    return mFailedGoals.size();
}

Goal* Seat::getFailedGoal(unsigned int index)
{
    if (index >= mFailedGoals.size())
        return nullptr;

    return mFailedGoals[index];
}

unsigned int Seat::getNumClaimedTiles() const
{
    return mNumClaimedTiles;
}

void Seat::setNumClaimedTiles(const unsigned int& num)
{
    mNumClaimedTiles = num;
}

void Seat::incrementNumClaimedTiles()
{
    ++mNumClaimedTiles;
}

unsigned int Seat::checkAllCompletedGoals()
{
    // Loop over the goals vector and move any goals that have been met to the completed goals vector.
    std::vector<Goal*>::iterator currentGoal = mCompletedGoals.begin();
    while (currentGoal != mCompletedGoals.end())
    {
        // Start by checking if this previously met goal has now been unmet.
        if ((*currentGoal)->isUnmet(*this, *mGameMap))
        {
            mUncompleteGoals.push_back(*currentGoal);

            currentGoal = mCompletedGoals.erase(currentGoal);

            //Signal that the list of goals has changed.
            goalsHasChanged();
        }
        else
        {
            // Next check to see if this previously met goal has now been failed.
            if ((*currentGoal)->isFailed(*this, *mGameMap))
            {
                mFailedGoals.push_back(*currentGoal);

                currentGoal = mCompletedGoals.erase(currentGoal);

                //Signal that the list of goals has changed.
                goalsHasChanged();
            }
            else
            {
                ++currentGoal;
            }
        }
    }

    return numCompletedGoals();
}

bool Seat::getHasGoalsChanged()
{
    return mHasGoalsChanged;
}

void Seat::resetGoalsChanged()
{
    mHasGoalsChanged = false;
}

void Seat::goalsHasChanged()
{
    //Not locking here as this is supposed to be called from a function that already locks.
    mHasGoalsChanged = true;
}

bool Seat::isAlliedSeat(const Seat *seat) const
{
    return getTeamId() == seat->getTeamId();
}

bool Seat::canOwnedCreatureBePickedUpBy(const Seat* seat) const
{
    // Note : if we want to allow players to pickup allied creatures, we can do that here.
    if(this == seat)
        return true;

    return false;
}

bool Seat::canOwnedTileBeClaimedBy(const Seat* seat) const
{
    if(getTeamId() != seat->getTeamId())
        return true;

    return false;
}

bool Seat::canOwnedCreatureUseRoomFrom(const Seat* seat) const
{
    if(this == seat)
        return true;

    return false;
}

bool Seat::canBuildingBeDestroyedBy(const Seat* seat) const
{
    if(this == seat)
        return true;

    return false;
}

void Seat::addAlliedSeat(Seat* seat)
{
    mAlliedSeats.push_back(seat);
}

void Seat::clearTilesWithVision()
{
    if(mPlayer == nullptr)
        return;
    if(!mPlayer->getIsHuman())
        return;

    for(std::vector<TileStateNotified>& vec : mTilesStates)
    {
        for(TileStateNotified& p : vec)
        {
            p.mVisionTurnLast = p.mVisionTurnCurrent;
            p.mVisionTurnCurrent = false;
        }
    }
}

void Seat::notifyVisionOnTile(Tile* tile)
{
    if(mPlayer == nullptr)
        return;
    if(!mPlayer->getIsHuman())
        return;

    if(tile->getX() >= static_cast<int>(mTilesStates.size()))
    {
        OD_LOG_ERR("Tile=" + Tile::displayAsString(tile));
        return;
    }
    if(tile->getY() >= static_cast<int>(mTilesStates[tile->getX()].size()))
    {
        OD_LOG_ERR("Tile=" + Tile::displayAsString(tile));
        return;
    }

    TileStateNotified& tileState = mTilesStates[tile->getX()][tile->getY()];
    tileState.mVisionTurnCurrent = true;
}

void Seat::notifyTileClaimedByEnemy(Tile* tile)
{
    if(mPlayer == nullptr)
        return;
    if(!mPlayer->getIsHuman())
        return;

    if(tile->getX() >= static_cast<int>(mTilesStates.size()))
    {
        OD_LOG_ERR("Tile=" + Tile::displayAsString(tile));
        return;
    }
    if(tile->getY() >= static_cast<int>(mTilesStates[tile->getX()].size()))
    {
        OD_LOG_ERR("Tile=" + Tile::displayAsString(tile));
        return;
    }

    TileStateNotified& tileState = mTilesStates[tile->getX()][tile->getY()];

    // By default, we set the tile like if it was not claimed anymore
    tileState.mSeatIdOwner = -1;
    tileState.mTileVisual = TileVisual::dirtGround;
    tileState.mVisionTurnCurrent = true;
}

const std::string Seat::getFactionFromLine(const std::string& line)
{
    const uint32_t indexFactionInLine = 3;
    std::vector<std::string> elems = Helper::split(line, '\t');
    if(elems.size() > indexFactionInLine)
        return elems[indexFactionInLine];

    OD_LOG_ERR("line=" + line);
    return std::string();
}

void Seat::refreshFromSeat(Seat* s)
{
    // We only refresh data that changes over time (gold, mana, ...)
    mGold = s->mGold;
    mGoldMax = s->mGoldMax;
    mMana = s->mMana;
    mManaDelta = s->mManaDelta;
    mNumClaimedTiles = s->mNumClaimedTiles;
    mNumCreaturesFighters = s->mNumCreaturesFighters;
    mNumCreaturesFightersMax = s->mNumCreaturesFightersMax;
    mNumCreaturesWorkers = s->mNumCreaturesWorkers;
    mHasGoalsChanged = s->mHasGoalsChanged;
    mNbRooms = s->mNbRooms;
    mCurrentResearchType = s->mCurrentResearchType;
    mCurrentResearchProgress = s->mCurrentResearchProgress;
}

bool Seat::takeMana(double mana)
{
    if(mana > mMana)
        return false;

    mMana -= mana;
    return true;
}

uint32_t Seat::getNbRooms(RoomType roomType) const
{
    uint32_t index = static_cast<uint32_t>(roomType);
    if(index >= mNbRooms.size())
    {
        OD_LOG_ERR("wrong index=" + Helper::toString(index) + ", size=" + Helper::toString(mNbRooms.size()));
        return 0;
    }

    return mNbRooms.at(index);
}

bool Seat::sortForMapSave(Seat* s1, Seat* s2)
{
    return s1->mId < s2->mId;
}

void Seat::setPlayer(Player* player)
{
    if(mPlayer != nullptr)
    {
        OD_LOG_ERR("A player=" + mPlayer->getNick() + " already on seat id="
            + Helper::toString(getId()) + ", newNick=" + player->getNick());
    }

    mPlayer = player;
    player->mSeat = this;
}

bool Seat::hasVisionOnTile(Tile* tile)
{
    if(!mGameMap->isServerGameMap())
    {
        // On client side, we check only for the local player
        if(this != mGameMap->getLocalPlayer()->getSeat())
            return false;

        return tile->getLocalPlayerHasVision();
    }

    // AI players have vision on every tile
    if(mPlayer == nullptr)
        return true;
    if(!mPlayer->getIsHuman())
        return true;

    if(tile->getX() >= static_cast<int>(mTilesStates.size()))
    {
        OD_LOG_ERR("Tile=" + Tile::displayAsString(tile));
        return false;
    }
    if(tile->getY() >= static_cast<int>(mTilesStates[tile->getX()].size()))
    {
        OD_LOG_ERR("Tile=" + Tile::displayAsString(tile));
        return false;
    }

    TileStateNotified& stateTile = mTilesStates[tile->getX()][tile->getY()];

    return stateTile.mVisionTurnCurrent;
}

void Seat::initSeat()
{
    if(getPlayer() == nullptr)
        return;

    ConfigManager& config = ConfigManager::getSingleton();

    if(isRogueSeat())
    {
        std::string defaultWorkerClass = config.getRogueWorkerClass();
        mDefaultWorkerClass = mGameMap->getClassDescription(defaultWorkerClass);
        OD_ASSERT_TRUE_MSG(mDefaultWorkerClass != nullptr, "No valid default worker class for rogue seat: " + defaultWorkerClass);
        return;
    }

    // Spawn pool initialisation
    const std::vector<std::string>& pool = config.getFactionSpawnPool(mFaction);
    OD_ASSERT_TRUE_MSG(!pool.empty(), "Empty spawn pool seatId=" + Helper::toString(mId) + ", faction=" + mFaction);
    for(const std::string& defName : pool)
    {
        const CreatureDefinition* def = mGameMap->getClassDescription(defName);
        if(def == nullptr)
        {
            OD_LOG_ERR("defName=" + defName);
            continue;
        }

        mSpawnPool.push_back(std::pair<const CreatureDefinition*, bool>(def, false));
    }

    // Get the default worker class
    std::string defaultWorkerClass = config.getFactionWorkerClass(mFaction);
    mDefaultWorkerClass = mGameMap->getClassDescription(defaultWorkerClass);
    OD_ASSERT_TRUE_MSG(mDefaultWorkerClass != nullptr, "No valid default worker class for seatId=" + Helper::toString(mId) + ", faction: " + mFaction);

    // We use a temporary vector to allow the corresponding functions to check the vector validity
    // and reject its content if it is not valid
    std::vector<ResearchType> researches = mResearchDone;
    mResearchDone.clear();
    setResearchesDone(researches);
    researches = mResearchPending;
    mResearchPending.clear();
    setResearchTree(researches);

    // We restore the tiles if any
    if(!mTilesStateLoaded.empty())
    {
        std::vector<Tile*> tilesRefresh;
        std::vector<Tile*> tilesMark;

        for(std::pair<std::pair<int, int> const, TileStateNotified>& p : mTilesStateLoaded)
        {
            Tile* tile = mGameMap->getTile(p.first.first, p.first.second);
            if(tile == nullptr)
            {
                OD_LOG_ERR("tile=" + Tile::displayAsString(tile));
                continue;
            }

            // We check if the tile is marked
            if(p.second.mMarkedForDigging)
                tilesMark.push_back(tile);

            // Other tiles than goldFull, dirtFull and rockFull need to be notified to refresh
            switch(p.second.mTileVisual)
            {
                case TileVisual::nullTileVisual:
                case TileVisual::goldFull:
                case TileVisual::dirtFull:
                case TileVisual::rockFull:
                    break;
                default:
                    tilesRefresh.push_back(tile);
                    break;
            }
        }

        if(!tilesRefresh.empty())
        {
            ServerNotification *serverNotification = new ServerNotification(
                ServerNotificationType::refreshTiles, getPlayer());
            uint32_t nbTiles = tilesRefresh.size();
            serverNotification->mPacket << nbTiles;
            for(Tile* tile : tilesRefresh)
            {
                std::pair<int, int> tileCoords(tile->getX(), tile->getY());
                TileStateNotified& tileState = mTilesStateLoaded[tileCoords];

                // We set the tile visual to make sure the tile state is exported if
                // game is saved again
                if(tile->getX() >= static_cast<int>(mTilesStates.size()))
                {
                    OD_LOG_ERR("Tile=" + Tile::displayAsString(tile));
                    continue;
                }
                if(tile->getY() >= static_cast<int>(mTilesStates[tile->getX()].size()))
                {
                    OD_LOG_ERR("Tile=" + Tile::displayAsString(tile));
                    continue;
                }
                mTilesStates[tile->getX()][tile->getY()] = tileState;

                // Then, we export tile state to the client
                mGameMap->tileToPacket(serverNotification->mPacket, tile);
                tile->exportToPacketForUpdate(serverNotification->mPacket, this);
            }
            ODServer::getSingleton().queueServerNotification(serverNotification);
        }

        getPlayer()->markTilesForDigging(true, tilesMark, false);
    }
}

void Seat::setMapSize(int x, int y)
{
    if(mPlayer == nullptr)
        return;
    if(!mPlayer->getIsHuman())
        return;

    mTilesStates = std::vector<std::vector<TileStateNotified>>(x, std::vector<TileStateNotified>(y));
    // By default, we know that rock (ground & full) will be set as rock full tiles,
    // gold (ground & full) will be set as gold full tiles,
    // other tiles will be set as dirt full tiles
    for(int xxx = 0; xxx < x; ++xxx)
    {
        for(int yyy = 0; yyy < y; ++yyy)
        {
            Tile* tile = mGameMap->getTile(xxx, yyy);
            if(tile == nullptr)
                continue;

            if(tile->getType() == TileType::gold)
            {
                mTilesStates[xxx][yyy].mTileVisual = TileVisual::goldFull;
                continue;
            }

            if(tile->getType() == TileType::rock)
            {
                mTilesStates[xxx][yyy].mTileVisual = TileVisual::rockFull;
                continue;
            }

            mTilesStates[xxx][yyy].mTileVisual = TileVisual::dirtFull;
        }
    }
}

unsigned int Seat::checkAllGoals()
{
    // Loop over the goals vector and move any goals that have been met to the completed goals vector.
    std::vector<Goal*> goalsToAdd;
    std::vector<Goal*>::iterator currentGoal = mUncompleteGoals.begin();
    while (currentGoal != mUncompleteGoals.end())
    {
        Goal* goal = *currentGoal;
        // Start by checking if the goal has been met by this seat.
        if (goal->isMet(*this, *mGameMap))
        {
            mCompletedGoals.push_back(goal);

            // Add any subgoals upon completion to the list of outstanding goals.
            for (unsigned int i = 0; i < goal->numSuccessSubGoals(); ++i)
                goalsToAdd.push_back(goal->getSuccessSubGoal(i));

            currentGoal = mUncompleteGoals.erase(currentGoal);

            goalsHasChanged();

            // Tells the player an objective has been met.
            if(mGameMap->getTurnNumber() > 5 && getPlayer() != nullptr && getPlayer()->getIsHuman())
            {
                ServerNotification *serverNotification = new ServerNotification(
                    ServerNotificationType::chatServer, getPlayer());

                serverNotification->mPacket << "You have met an objective." << EventShortNoticeType::aboutObjectives;
                ODServer::getSingleton().queueServerNotification(serverNotification);
            }
        }
        else
        {
            // If the goal has not been met, check to see if it cannot be met in the future.
            if (goal->isFailed(*this, *mGameMap))
            {
                mFailedGoals.push_back(goal);

                // Add any subgoals upon completion to the list of outstanding goals.
                for (unsigned int i = 0; i < goal->numFailureSubGoals(); ++i)
                    goalsToAdd.push_back(goal->getFailureSubGoal(i));

                currentGoal = mUncompleteGoals.erase(currentGoal);
                goalsHasChanged();

                // Tells the player an objective has been failed.
                if(mGameMap->getTurnNumber() > 5 && getPlayer() != nullptr && getPlayer()->getIsHuman())
                {
                    ServerNotification *serverNotification = new ServerNotification(
                        ServerNotificationType::chatServer, getPlayer());

                    serverNotification->mPacket << "You have FAILED an objective!" << EventShortNoticeType::majorGameEvent;
                    ODServer::getSingleton().queueServerNotification(serverNotification);
                }
            }
            else
            {
                // The goal has not been met but has also not been definitively failed, continue on to the next goal in the list.
                ++currentGoal;
            }
        }
    }

    for(std::vector<Goal*>::iterator it = goalsToAdd.begin(); it != goalsToAdd.end(); ++it)
    {
        Goal* goal = *it;
        mUncompleteGoals.push_back(goal);
    }

    return numUncompleteGoals();
}

void Seat::notifyChangedVisibleTiles()
{
    if(mPlayer == nullptr)
        return;
    if(!mPlayer->getIsHuman())
        return;

    std::vector<Tile*> tilesToNotify;
    int xMax = static_cast<int>(mTilesStates.size());
    for(int xxx = 0; xxx < xMax; ++xxx)
    {
        int yMax = static_cast<int>(mTilesStates[xxx].size());
        for(int yyy = 0; yyy < yMax; ++yyy)
        {
            if(!mTilesStates[xxx][yyy].mVisionTurnCurrent)
                continue;

            Tile* tile = mGameMap->getTile(xxx, yyy);
            if(!tile->hasChangedForSeat(this))
                continue;

            tilesToNotify.push_back(tile);
            tile->changeNotifiedForSeat(this);
        }
    }

    if(tilesToNotify.empty())
        return;

    uint32_t nbTiles = tilesToNotify.size();
    ServerNotification *serverNotification = new ServerNotification(
        ServerNotificationType::refreshTiles, getPlayer());
    serverNotification->mPacket << nbTiles;
    for(Tile* tile : tilesToNotify)
    {
        mGameMap->tileToPacket(serverNotification->mPacket, tile);
        updateTileStateForSeat(tile);
        tile->exportToPacketForUpdate(serverNotification->mPacket, this);
    }
    ODServer::getSingleton().queueServerNotification(serverNotification);
}

void Seat::stopVisualDebugEntities()
{
    if(mGameMap->isServerGameMap())
        return;

    mIsDebuggingVision = false;

    for (Tile* tile : mVisualDebugEntityTiles)
    {
        if (tile == nullptr)
            continue;

        RenderManager::getSingleton().rrDestroySeatVisionVisualDebug(getId(), tile);
    }
    mVisualDebugEntityTiles.clear();
}

void Seat::refreshVisualDebugEntities(const std::vector<Tile*>& tiles)
{
    if(mGameMap->isServerGameMap())
        return;

    mIsDebuggingVision = true;

    for (Tile* tile : tiles)
    {
        // We check if the visual debug is already on this tile
        if(std::find(mVisualDebugEntityTiles.begin(), mVisualDebugEntityTiles.end(), tile) != mVisualDebugEntityTiles.end())
            continue;

        RenderManager::getSingleton().rrCreateSeatVisionVisualDebug(getId(), tile);

        mVisualDebugEntityTiles.push_back(tile);
    }

    // now, we check if visual debug should be removed from a tile
    for (std::vector<Tile*>::iterator it = mVisualDebugEntityTiles.begin(); it != mVisualDebugEntityTiles.end();)
    {
        Tile* tile = *it;
        if(std::find(tiles.begin(), tiles.end(), tile) != tiles.end())
        {
            ++it;
            continue;
        }

        it = mVisualDebugEntityTiles.erase(it);

        RenderManager::getSingleton().rrDestroySeatVisionVisualDebug(getId(), tile);
    }
}

void Seat::displaySeatVisualDebug(bool enable)
{
    if(!mGameMap->isServerGameMap())
        return;

    // Visual debugging do not work for AI players (otherwise, we would have to use
    // mTilesStates for them which would be memory consuming)
    if(mPlayer == nullptr)
        return;
    if(!mPlayer->getIsHuman())
        return;

    mIsDebuggingVision = enable;
    int seatId = getId();
    if(enable)
    {
        std::vector<Tile*> tiles;
        int xMax = static_cast<int>(mTilesStates.size());
        for(int xxx = 0; xxx < xMax; ++xxx)
        {
            int yMax = static_cast<int>(mTilesStates[xxx].size());
            for(int yyy = 0; yyy < yMax; ++yyy)
            {
                if(!mTilesStates[xxx][yyy].mVisionTurnCurrent)
                    continue;

                Tile* tile = mGameMap->getTile(xxx, yyy);
                tiles.push_back(tile);
            }
        }
        uint32_t nbTiles = tiles.size();
        ServerNotification *serverNotification = new ServerNotification(
            ServerNotificationType::refreshSeatVisDebug, nullptr);
        serverNotification->mPacket << seatId;
        serverNotification->mPacket << true;
        serverNotification->mPacket << nbTiles;
        for(Tile* tile : tiles)
        {
            mGameMap->tileToPacket(serverNotification->mPacket, tile);
        }
        ODServer::getSingleton().queueServerNotification(serverNotification);
    }
    else
    {
        ServerNotification *serverNotification = new ServerNotification(
            ServerNotificationType::refreshSeatVisDebug, nullptr);
        serverNotification->mPacket << seatId;
        serverNotification->mPacket << false;
        ODServer::getSingleton().queueServerNotification(serverNotification);
    }
}

void Seat::sendVisibleTiles()
{
    if(!mGameMap->isServerGameMap())
        return;

    if(getPlayer() == nullptr)
        return;

    if(!getPlayer()->getIsHuman())
        return;

    uint32_t nbTiles;
    ServerNotification *serverNotification = new ServerNotification(
        ServerNotificationType::refreshVisibleTiles, getPlayer());
    std::vector<Tile*> tilesVisionGained;
    std::vector<Tile*> tilesVisionLost;
    // Tiles we gained vision
    int xMax = static_cast<int>(mTilesStates.size());
    for(int xxx = 0; xxx < xMax; ++xxx)
    {
        int yMax = static_cast<int>(mTilesStates[xxx].size());
        for(int yyy = 0; yyy < yMax; ++yyy)
        {
            if(mTilesStates[xxx][yyy].mVisionTurnCurrent == mTilesStates[xxx][yyy].mVisionTurnLast)
                continue;

            Tile* tile = mGameMap->getTile(xxx, yyy);
            if(mTilesStates[xxx][yyy].mVisionTurnCurrent)
            {
                // Vision gained
                tilesVisionGained.push_back(tile);
            }
            else
            {
                // Vision lost
                tilesVisionLost.push_back(tile);
            }
        }
    }

    // Notify tiles we gained vision
    nbTiles = tilesVisionGained.size();
    serverNotification->mPacket << nbTiles;
    for(Tile* tile : tilesVisionGained)
    {
        mGameMap->tileToPacket(serverNotification->mPacket, tile);
    }

    // Notify tiles we lost vision
    nbTiles = tilesVisionLost.size();
    serverNotification->mPacket << nbTiles;
    for(Tile* tile : tilesVisionLost)
    {
        mGameMap->tileToPacket(serverNotification->mPacket, tile);
    }
    ODServer::getSingleton().queueServerNotification(serverNotification);
}

void Seat::computeSeatBeginTurn()
{
    if(mPlayer != nullptr)
    {
        std::fill(mNbRooms.begin(), mNbRooms.end(), 0);
        for(Room* room : mGameMap->getRooms())
        {
            if(room->getSeat() != this)
                continue;

            uint32_t index = static_cast<uint32_t>(room->getType());
            if(index >= mNbRooms.size())
            {
                OD_LOG_ERR("wrong index=" + Helper::toString(index) + ", size=" + Helper::toString(mNbRooms.size()));
                return;
            }
            ++mNbRooms[index];
        }
    }
}


Seat* Seat::createRogueSeat(GameMap* gameMap)
{
    Seat* seat = new Seat(gameMap);
    seat->mId = 0;
    seat->mTeamId = 0;
    seat->mAvailableTeamIds.push_back(0);
    seat->mPlayerType = PLAYER_TYPE_INACTIVE;
    seat->mStartingX = 0;
    seat->mStartingY = 0;
    seat->mGold = 0;
    seat->mGoldMax = 0;
    seat->mGoldMined = 0;
    seat->mColorId = "0";
    seat->mMana = 0;

    // In editor, we do not add the player on rogue seat because that's where the human player will be since that's the
    // only seat we are sure to exist
    if(!gameMap->isInEditorMode())
    {
        Player* inactivePlayer = new Player(gameMap, 0);
        inactivePlayer->setNick("Inactive rogue AI");
        gameMap->addPlayer(inactivePlayer);
        seat->setPlayer(inactivePlayer);
    }

    return seat;
}


bool Seat::importSeatFromStream(std::istream& is)
{
    std::string str;
    OD_ASSERT_TRUE(is >> str);
    if(str != "seatId")
    {
        OD_LOG_INF("WARNING: expected seatId and read " + str);
        return false;
    }
    OD_ASSERT_TRUE(is >> mId);
    if(mId == 0)
    {
        OD_LOG_INF("WARNING: Forbidden seatId used");
        return false;
    }

    OD_ASSERT_TRUE(is >> str);
    if(str != "teamId")
    {
        OD_LOG_INF("WARNING: expected teamId and read " + str);
        return false;
    }
    OD_ASSERT_TRUE(is >> str);

    std::vector<std::string> teamIds = Helper::split(str, '/');
    for(const std::string& strTeamId : teamIds)
    {
        int teamId = Helper::toInt(strTeamId);
        if(teamId == 0)
        {
            OD_LOG_INF("WARNING: forbidden teamId in seat id=" + Helper::toString(mId));
            continue;
        }

        mAvailableTeamIds.push_back(teamId);
    }

    OD_ASSERT_TRUE(is >> str);
    if(str != "player")
    {
        OD_LOG_INF("WARNING: expected player and read " + str);
        return false;
    }
    OD_ASSERT_TRUE(is >> mPlayerType);

    OD_ASSERT_TRUE(is >> str);
    if(str != "faction")
    {
        OD_LOG_INF("WARNING: expected faction and read " + str);
        return false;
    }
    OD_ASSERT_TRUE(is >> mFaction);

    OD_ASSERT_TRUE(is >> str);
    if(str != "startingX")
    {
        OD_LOG_INF("WARNING: expected startingX and read " + str);
        return false;
    }
    OD_ASSERT_TRUE(is >> mStartingX);

    OD_ASSERT_TRUE(is >> str);
    if(str != "startingY")
    {
        OD_LOG_INF("WARNING: expected startingY and read " + str);
        return false;
    }
    OD_ASSERT_TRUE(is >> mStartingY);

    OD_ASSERT_TRUE(is >> str);
    if(str != "colorId")
    {
        OD_LOG_INF("WARNING: expected colorId and read " + str);
        return false;
    }
    OD_ASSERT_TRUE(is >> mColorId);

    OD_ASSERT_TRUE(is >> str);
    if(str != "gold")
    {
        OD_LOG_INF("WARNING: expected gold and read " + str);
        return false;
    }
    OD_ASSERT_TRUE(is >> mGold);

    OD_ASSERT_TRUE(is >> str);
    if(str != "goldMined")
    {
        OD_LOG_INF("WARNING: expected goldMined and read " + str);
        return false;
    }
    OD_ASSERT_TRUE(is >> mGoldMined);

    OD_ASSERT_TRUE(is >> str);
    if(str != "mana")
    {
        OD_LOG_INF("WARNING: expected mana and read " + str);
        return false;
    }
    OD_ASSERT_TRUE(is >> mMana);

    mColorValue = ConfigManager::getSingleton().getColorFromId(mColorId);

    uint32_t nbResearch = static_cast<uint32_t>(ResearchType::countResearch);
    OD_ASSERT_TRUE(is >> str);
    if(str != "[ResearchDone]")
    {
        OD_LOG_INF("WARNING: expected [ResearchDone] and read " + str);
        return false;
    }
    while(true)
    {
        OD_ASSERT_TRUE(is >> str);
        if(str == "[/ResearchDone]")
            break;

        for(uint32_t i = 0; i < nbResearch; ++i)
        {
            ResearchType type = static_cast<ResearchType>(i);
            if(type == ResearchType::nullResearchType)
                continue;
            if(str.compare(Research::researchTypeToString(type)) != 0)
                continue;

            if(std::find(mResearchDone.begin(), mResearchDone.end(), type) != mResearchDone.end())
                break;

            // We found a valid research
            mResearchDone.push_back(type);

            break;
        }
    }

    OD_ASSERT_TRUE(is >> str);
    if(str != "[ResearchNotAllowed]")
    {
        OD_LOG_INF("WARNING: expected [ResearchNotAllowed] and read " + str);
        return false;
    }

    while(true)
    {
        OD_ASSERT_TRUE(is >> str);
        if(str == "[/ResearchNotAllowed]")
            break;

        for(uint32_t i = 0; i < nbResearch; ++i)
        {
            ResearchType type = static_cast<ResearchType>(i);
            if(type == ResearchType::nullResearchType)
                continue;
            if(str.compare(Research::researchTypeToString(type)) != 0)
                continue;

            // We do not allow a research to be done and not allowed
            if(std::find(mResearchDone.begin(), mResearchDone.end(), type) != mResearchDone.end())
                break;
            if(std::find(mResearchNotAllowed.begin(), mResearchNotAllowed.end(), type) != mResearchNotAllowed.end())
                break;

            // We found a valid research
            mResearchNotAllowed.push_back(type);

            break;
        }
    }

    OD_ASSERT_TRUE(is >> str);
    if(str != "[ResearchPending]")
    {
        OD_LOG_INF("WARNING: expected [ResearchPending] and read " + str);
        return false;
    }

    while(true)
    {
        OD_ASSERT_TRUE(is >> str);
        if(str == "[/ResearchPending]")
            break;

        for(uint32_t i = 0; i < nbResearch; ++i)
        {
            ResearchType type = static_cast<ResearchType>(i);
            if(type == ResearchType::nullResearchType)
                continue;
            if(str.compare(Research::researchTypeToString(type)) != 0)
                continue;

            // We do not allow researches already done or not allowed
            if(std::find(mResearchDone.begin(), mResearchDone.end(), type) != mResearchDone.end())
                break;
            if(std::find(mResearchNotAllowed.begin(), mResearchNotAllowed.end(), type) != mResearchNotAllowed.end())
                break;
            if(std::find(mResearchPending.begin(), mResearchPending.end(), type) != mResearchPending.end())
                break;

            // We found a valid research
            mResearchPending.push_back(type);
        }
    }

    // Note: At this point, we are reading seats but the tiles may change during map load.
    // That's why we cannot keep pointers to tiles. However, map size is already set
    // so we can use it
    uint32_t nb = static_cast<uint32_t>(TileVisual::countTileVisual);
    for(uint32_t k = 0; k < nb; ++k)
    {
        TileVisual tileVisual = static_cast<TileVisual>(k);
        // Full dirt tiles, full gold tiles and full rock tiles are automatically
        // set so we don't have to bother about them
        switch(tileVisual)
        {
            case TileVisual::nullTileVisual:
            case TileVisual::goldFull:
            case TileVisual::dirtFull:
            case TileVisual::rockFull:
                continue;

            default:
                break;
        }
        int ret = readTilesVisualInitialStates(tileVisual, is);
        if(ret == 0)
            return true;
        else if(ret == -1)
            return false;
    }

    OD_ASSERT_TRUE(is >> str);
    if(str != "[markedTiles]")
    {
        OD_LOG_INF("WARNING: expected [markedTiles] and read " + str);
        return false;
    }

    while(true)
    {
        OD_ASSERT_TRUE(is >> str);
        if(str == "[/markedTiles]")
            break;

        std::pair<int, int> tilecoords;
        tilecoords.first = Helper::toInt(str);
        OD_ASSERT_TRUE(is >> str);
        tilecoords.second = Helper::toInt(str);

        TileStateNotified& tileState = mTilesStateLoaded[tilecoords];
        tileState.mMarkedForDigging = true;
    }

    // The next line should be a seat end tag
    OD_ASSERT_TRUE(is >> str);
    if(str != "[/Seat]")
    {
        OD_LOG_INF("WARNING: expected [/Seat] and read " + str);
        return false;
    }

    return true;
}


bool Seat::exportSeatToStream(std::ostream& os) const
{
    os << "seatId\t";
    os << mId;
    os << std::endl;
    // If the team id is set, we save it. Otherwise, we save all the available team ids
    // That way, save map will work in both editor and in game.
    os << "teamId\t";
    if(mTeamId != -1)
    {
        os << mTeamId;
    }
    else
    {
        int cpt = 0;
        for(int teamId : mAvailableTeamIds)
        {
            if(cpt > 0)
                os << "/";

            os << teamId;
            ++cpt;
        }
    }
    os << std::endl;

    // On editor, we write the original player type. If we are saving a game, we keep the assigned type
    if((mGameMap->isInEditorMode()) ||
        (getPlayer() == nullptr))
    {
        os << "player\t";
        os << mPlayerType;
        os << std::endl;
    }
    else
    {
        os << "player\t";
        if(getPlayer()->getIsHuman())
            os << PLAYER_TYPE_HUMAN;
        else
            os << PLAYER_TYPE_AI;

        os << std::endl;
    }

    os << "faction\t";
    os << mFaction;
    os << std::endl;

    os << "startingX\t";
    os << mStartingX;
    os << std::endl;

    os << "startingY\t";
    os << mStartingY;
    os << std::endl;

    os << "colorId\t";
    os << mColorId;
    os << std::endl;

    os << "gold\t";
    os << mGold;
    os << std::endl;

    os << "goldMined\t";
    os << mGoldMined;
    os << std::endl;

    os << "mana\t";
    os << mMana;
    os << std::endl;

    os << "[ResearchDone]" << std::endl;
    for(ResearchType type : mResearchDone)
    {
        os << Research::researchTypeToString(type) << std::endl;
    }
    os << "[/ResearchDone]" << std::endl;

    os << "[ResearchNotAllowed]" << std::endl;
    for(ResearchType type : mResearchNotAllowed)
    {
        os << Research::researchTypeToString(type) << std::endl;
    }
    os << "[/ResearchNotAllowed]" << std::endl;

    os << "[ResearchPending]" << std::endl;
    for(ResearchType type : mResearchPending)
    {
        os << Research::researchTypeToString(type) << std::endl;
    }
    os << "[/ResearchPending]" << std::endl;

    // In editor mode, we don't save tile states
    if(mGameMap->isInEditorMode())
        return true;

    // Tile states are only saved for human players
    if((getPlayer() == nullptr) ||
        (!getPlayer()->getIsHuman()))
    {
        return true;
    }

    // We save the visible tiles last state
    uint32_t nb = static_cast<uint32_t>(TileVisual::countTileVisual);
    for(uint32_t k = 0; k < nb; ++k)
    {
        TileVisual tileVisual = static_cast<TileVisual>(k);
        // Full dirt tiles, full gold tiles and full rock tiles are automatically
        // set so we don't have to bother about them
        switch(tileVisual)
        {
            case TileVisual::nullTileVisual:
            case TileVisual::goldFull:
            case TileVisual::dirtFull:
            case TileVisual::rockFull:
                continue;

            default:
                break;
        }
        exportTilesVisualInitialStates(tileVisual, os);
    }

    os << "[markedTiles]" << std::endl;
    for(uint32_t xxx = 0; xxx < mTilesStates.size(); ++xxx)
    {
        for(uint32_t yyy = 0; yyy < mTilesStates[xxx].size(); ++yyy)
        {
            const TileStateNotified& tileState = mTilesStates[xxx][yyy];
            if(!tileState.mMarkedForDigging)
                continue;

            os << xxx << "\t" << yyy << std::endl;
        }
    }
    os << "[/markedTiles]" << std::endl;

    return true;
}

void Seat::exportTilesVisualInitialStates(TileVisual tileVisual, std::ostream& os) const
{
    os << "[" + Tile::tileVisualToString(tileVisual) + "]" << std::endl;

    for(uint32_t xxx = 0; xxx < mTilesStates.size(); ++xxx)
    {
        for(uint32_t yyy = 0; yyy < mTilesStates[xxx].size(); ++yyy)
        {
            const TileStateNotified& tileState = mTilesStates[xxx][yyy];
            if(tileState.mTileVisual != tileVisual)
                continue;

            os << xxx << "\t" << yyy << "\t" << tileState.mSeatIdOwner << std::endl;
        }
    }

    os << "[/" + Tile::tileVisualToString(tileVisual) + "]" << std::endl;
}

bool Seat::addResearch(ResearchType type)
{
    if(std::find(mResearchDone.begin(), mResearchDone.end(), type) != mResearchDone.end())
        return false;

    std::vector<ResearchType> researchDone = mResearchDone;
    researchDone.push_back(type);
    setResearchesDone(researchDone);

    // Tells the player a new room/trap/spell is available.
    if(getPlayer() != nullptr && getPlayer()->getIsHuman())
    {
        ServerNotification *serverNotification = new ServerNotification(
            ServerNotificationType::chatServer, getPlayer());

        std::string msg = Research::researchTypeToPlayerVisibleString(type) + " is now available.";
        serverNotification->mPacket << msg << EventShortNoticeType::aboutResearches;
        ODServer::getSingleton().queueServerNotification(serverNotification);
    }

    return true;
}

bool Seat::isResearchDone(ResearchType type) const
{
    for(ResearchType researchDone : mResearchDone)
    {
        if(researchDone == type)
            return true;
    }

    return false;
}

uint32_t Seat::isResearchPending(ResearchType resType) const
{
    uint32_t queueNumber = 1;
    for (ResearchType pendingRes : mResearchPending)
    {
        if (pendingRes == resType)
            return queueNumber;
        ++queueNumber;
    }
    return 0;
}

ResearchType Seat::getFirstResearchPending() const
{
    if(mResearchPending.empty())
        return ResearchType::nullResearchType;

    return mResearchPending.at(0);
}

void Seat::addResearchPoints(int32_t points)
{
    // Even if we are not searching anything, we allow to bring back a research book if
    // we find any
    mResearchPoints += points;
    if(mCurrentResearch == nullptr)
    {
        mCurrentResearchType = ResearchType::nullResearchType;
        return;
    }

    if(mResearchPoints < mCurrentResearch->getNeededResearchPoints())
    {
        mCurrentResearchType = mCurrentResearch->getType();
        mCurrentResearchProgress = static_cast<float>(mResearchPoints) / static_cast<float>(mCurrentResearch->getNeededResearchPoints());
        return;
    }

    // The current research is complete. We add it to the available research list
    mResearchPoints -= mCurrentResearch->getNeededResearchPoints();
    addResearch(mCurrentResearch->getType());

    // We set the next research
    setNextResearch(mCurrentResearch->getType());
}

bool Seat::getCurrentResearchProgress(ResearchType& type, float& progress) const
{
    if(mCurrentResearchType == ResearchType::nullResearchType)
        return false;

    type = mCurrentResearchType;
    progress = mCurrentResearchProgress;
    return true;
}

void Seat::setNextResearch(ResearchType researchedType)
{
    if(!mGameMap->isServerGameMap())
        return;

    mCurrentResearch = nullptr;
    if(mResearchPending.empty())
    {
        mCurrentResearchType = ResearchType::nullResearchType;

        // Notify the player that no research is in the queue if there are still available researches
        if(ResearchManager::isAllResearchesDoneForSeat(this))
            return;

        if(getPlayer() == nullptr)
            return;
        if(!getPlayer()->getIsHuman())
            return;
        if(getNbRooms(RoomType::library) <= 0)
            return;

        getPlayer()->notifyNoResearchInQueue();
        return;
    }

    // We search for the first pending research we don't own a corresponding ResearchEntity
    ResearchType researchType = ResearchType::nullResearchType;
    for(ResearchType pending : mResearchPending)
    {
        if(pending == researchedType)
            continue;

        researchType = pending;

        if(researchType != ResearchType::nullResearchType)
            break;
    }

    if(researchType == ResearchType::nullResearchType)
    {
        mCurrentResearchType = ResearchType::nullResearchType;
        return;
    }

    // We have found a fitting research. We retrieve the corresponding Research
    // object and start working on that
    mCurrentResearch = ResearchManager::getResearch(researchType);
    if(mCurrentResearch == nullptr)
    {
        mCurrentResearchType = ResearchType::nullResearchType;
        return;
    }

    mCurrentResearchType = mCurrentResearch->getType();
    mCurrentResearchProgress = static_cast<float>(mResearchPoints) / static_cast<float>(mCurrentResearch->getNeededResearchPoints());
}

void Seat::setResearchesDone(const std::vector<ResearchType>& researches)
{
    mResearchDone = researches;
    // We remove the researches done from the pending researches (if it was there,
    // which may not be true if the research list changed after creating the
    // researchEntity for example)
    for(ResearchType type : researches)
    {
        auto research = std::find(mResearchPending.begin(), mResearchPending.end(), type);
        if(research == mResearchPending.end())
            continue;

        mResearchPending.erase(research);
    }

    if(mGameMap->isServerGameMap())
    {
        if((getPlayer() != nullptr) && getPlayer()->getIsHuman())
        {
            // We notify the client
            ServerNotification *serverNotification = new ServerNotification(
                ServerNotificationType::researchesDone, getPlayer());

            uint32_t nbItems = mResearchDone.size();
            serverNotification->mPacket << nbItems;
            for(ResearchType research : mResearchDone)
                serverNotification->mPacket << research;

            ODServer::getSingleton().queueServerNotification(serverNotification);
        }
    }
    else
    {
        // We notify the mode that the available researches changed. This way, it will
        // be able to update the UI as needed
        mGuiResearchNeedsRefresh = true;
    }
}

void Seat::setResearchTree(const std::vector<ResearchType>& researches)
{
    if(mGameMap->isServerGameMap())
    {
        // We check if all the researches in the vector are allowed. If not, we don't update the list
        std::vector<ResearchType> researchesDoneInTree = mResearchDone;
        for(ResearchType researchType : researches)
        {
            // We check if the research is allowed
            if(std::find(mResearchNotAllowed.begin(), mResearchNotAllowed.end(), researchType) != mResearchNotAllowed.end())
            {
                // Invalid research. This might be allowed in the gui to enter invalid
                // values. In this case, we should remove the assert
                OD_LOG_ERR("Unallowed research: " + Research::researchTypeToString(researchType));
                return;
            }
            const Research* research = ResearchManager::getResearch(researchType);
            if(research == nullptr)
            {
                // We found an unknown research
                OD_LOG_ERR("Unknown research: " + Research::researchTypeToString(researchType));
                return;
            }

            if(!research->canBeResearched(researchesDoneInTree))
            {
                // Invalid research. This might happen if the level has a research pending with a non researchable dependency.
                // In this case, we don't use the research tree
                OD_LOG_ERR("Unallowed research: " + Research::researchTypeToString(researchType));
                return;
            }

            // This research is valid. We add it in the list and we check if the next one also is
            researchesDoneInTree.push_back(researchType);
        }

        mResearchPending = researches;
        if((getPlayer() != nullptr) && getPlayer()->getIsHuman())
        {
            // We notify the client
            ServerNotification *serverNotification = new ServerNotification(
                ServerNotificationType::researchTree, getPlayer());

            uint32_t nbItems = mResearchPending.size();
            serverNotification->mPacket << nbItems;
            for(ResearchType research : mResearchPending)
                serverNotification->mPacket << research;

            ODServer::getSingleton().queueServerNotification(serverNotification);
        }

        // We start working on the research tree
        setNextResearch(ResearchType::nullResearchType);
    }
    else
    {
        // On client side, no need to check if the research tree is allowed
        mResearchPending = researches;

        // Makes the client gui update.
        mGuiResearchNeedsRefresh = true;
    }
}

void Seat::updateTileStateForSeat(Tile* tile)
{
    if(tile->getX() >= static_cast<int>(mTilesStates.size()))
    {
        OD_LOG_ERR("Tile=" + Tile::displayAsString(tile));
        return;
    }
    if(tile->getY() >= static_cast<int>(mTilesStates[tile->getX()].size()))
    {
        OD_LOG_ERR("Tile=" + Tile::displayAsString(tile));
        return;
    }

    TileStateNotified& tileState = mTilesStates[tile->getX()][tile->getY()];
    tileState.mTileVisual = tile->getTileVisual();
    switch(tileState.mTileVisual)
    {
        case TileVisual::claimedFull:
        case TileVisual::claimedGround:
            if(tile->getSeat() == nullptr)
            {
                OD_LOG_ERR("Tile=" + Tile::displayAsString(tile));
            }
            else
            {
                tileState.mSeatIdOwner = tile->getSeat()->getId();
            }
            break;
        case TileVisual::waterGround:
        case TileVisual::lavaGround:
            if(tile->getCoveringBuilding() != nullptr)
            {
                if(tile->getSeat() == nullptr)
                {
                    OD_LOG_ERR("Tile=" + Tile::displayAsString(tile));
                }
                else
                {
                    tileState.mSeatIdOwner = tile->getSeat()->getId();
                }
            }
            break;
        default:
            tileState.mSeatIdOwner = -1;
            break;
    }

    if(tile->getCoveringBuilding() == tileState.mBuilding)
        return;

    if(tileState.mBuilding != nullptr)
        tileState.mBuilding->notifySeatVision(tile, this);

    if((tile->getCoveringBuilding() != nullptr) &&
        (tile->getCoveringBuilding()->isTileVisibleForSeat(tile, this)))
    {
        tileState.mBuilding = tile->getCoveringBuilding();
        tileState.mBuilding->notifySeatVision(tile, this);
    }
    else
    {
        tileState.mBuilding = nullptr;
    }
}

void Seat::setVisibleBuildingOnTile(Building* building, Tile* tile)
{
    if(getPlayer() == nullptr)
        return;
    if(!getPlayer()->getIsHuman())
        return;

    if(tile->getX() >= static_cast<int>(mTilesStates.size()))
    {
        OD_LOG_ERR("Tile=" + Tile::displayAsString(tile));
        return;
    }
    if(tile->getY() >= static_cast<int>(mTilesStates[tile->getX()].size()))
    {
        OD_LOG_ERR("Tile=" + Tile::displayAsString(tile));
        return;
    }

    TileStateNotified& tileState = mTilesStates[tile->getX()][tile->getY()];

    if(building == tileState.mBuilding)
        return;

    tileState.mBuilding = building;
    tileState.mSeatIdOwner = building->getSeat()->getId();
}

void Seat::exportTileToPacket(ODPacket& os, const Tile* tile) const
{
    if(getPlayer() == nullptr)
    {
        OD_LOG_ERR("SeatId=" + Helper::toString(getId()));
        return;
    }
    if(!getPlayer()->getIsHuman())
    {
        OD_LOG_ERR("SeatId=" + Helper::toString(getId()));
        return;
    }

    if(tile->getX() >= static_cast<int>(mTilesStates.size()))
    {
        OD_LOG_ERR("Tile=" + Tile::displayAsString(tile));
        return;
    }
    if(tile->getY() >= static_cast<int>(mTilesStates[tile->getX()].size()))
    {
        OD_LOG_ERR("Tile=" + Tile::displayAsString(tile));
        return;
    }

    const TileStateNotified& tileState = mTilesStates[tile->getX()][tile->getY()];

    int tileSeatId = -1;
    // We only pass the tile seat to the client if the tile is fully claimed
    switch(tileState.mTileVisual)
    {
        case TileVisual::claimedGround:
        case TileVisual::claimedFull:
            tileSeatId = tileState.mSeatIdOwner;
            break;
        case TileVisual::waterGround:
        case TileVisual::lavaGround:
            if(tileState.mBuilding != nullptr)
                tileSeatId = tileState.mSeatIdOwner;
            break;
        default:
            break;
    }

    std::string meshName;
    Ogre::Vector3 scale;

    if((tileState.mBuilding != nullptr) &&
       !tileState.mBuilding->getMeshName().empty())
    {
        meshName = tileState.mBuilding->getMeshName() + ".mesh";
        scale = tileState.mBuilding->getScale();
    }
    else
    {
        // We set an empty mesh so that the client can compute the tile itself
        meshName.clear();
        scale = Ogre::Vector3::ZERO;
    }
    bool isRoom = false;
    bool isTrap = false;
    bool displayTileMesh = true;
    bool colorCustomMesh = false;
    bool hasBridge = false;

    uint32_t refundPriceRoom = 0;
    uint32_t refundPriceTrap = 0;
    if(tileState.mBuilding != nullptr)
    {
        displayTileMesh = tileState.mBuilding->displayTileMesh();
        colorCustomMesh = tileState.mBuilding->colorCustomMesh();

        if(tileState.mBuilding->getObjectType() == GameEntityType::room)
        {
            isRoom = true;
            Room* room = static_cast<Room*>(tileState.mBuilding);
            if(room->getSeat() == this)
                refundPriceRoom = (RoomManager::costPerTile(room->getType()) / 2);

            hasBridge = room->isBridge();
        }
        else if(tileState.mBuilding->getObjectType() == GameEntityType::trap)
        {
            isTrap = true;
            Trap* trap = static_cast<Trap*>(tileState.mBuilding);
            if(trap->getSeat() == this)
                refundPriceTrap = (TrapManager::costPerTile(trap->getType()) / 2);
        }
    }
    os << isRoom;
    os << isTrap;
    os << refundPriceRoom;
    os << refundPriceTrap;
    os << displayTileMesh;
    os << colorCustomMesh;
    os << hasBridge;
    os << tileSeatId;
    os << meshName;
    os << scale;
    os << tileState.mTileVisual;
}

void Seat::notifyBuildingRemovedFromGameMap(Building* building, Tile* tile)
{
    if(getPlayer() == nullptr)
        return;
    if(!getPlayer()->getIsHuman())
        return;

    if(tile->getX() >= static_cast<int>(mTilesStates.size()))
    {
        OD_LOG_ERR("Tile=" + Tile::displayAsString(tile));
        return;
    }
    if(tile->getY() >= static_cast<int>(mTilesStates[tile->getX()].size()))
    {
        OD_LOG_ERR("Tile=" + Tile::displayAsString(tile));
        return;
    }

    TileStateNotified& tileState = mTilesStates[tile->getX()][tile->getY()];
    if(tileState.mBuilding == building)
        tileState.mBuilding = nullptr;
}

void Seat::tileMarkedDiggingNotifiedToPlayer(Tile* tile, bool isDigSet)
{
    if(getPlayer() == nullptr)
        return;
    if(!getPlayer()->getIsHuman())
        return;

    if(tile->getX() >= static_cast<int>(mTilesStates.size()))
    {
        OD_LOG_ERR("Tile=" + Tile::displayAsString(tile));
        return;
    }
    if(tile->getY() >= static_cast<int>(mTilesStates[tile->getX()].size()))
    {
        OD_LOG_ERR("Tile=" + Tile::displayAsString(tile));
        return;
    }

    TileStateNotified& tileState = mTilesStates[tile->getX()][tile->getY()];
    tileState.mMarkedForDigging = isDigSet;
}

bool Seat::isTileDiggableForClient(Tile* tile) const
{
    if(!getPlayer()->getIsHuman())
        return false;
    if(tile->getX() >= static_cast<int>(mTilesStates.size()))
    {
        OD_LOG_ERR("Tile=" + Tile::displayAsString(tile));
        return false;
    }
    if(tile->getY() >= static_cast<int>(mTilesStates[tile->getX()].size()))
    {
        OD_LOG_ERR("Tile=" + Tile::displayAsString(tile));
        return false;
    }

    const TileStateNotified& tileState = mTilesStates[tile->getX()][tile->getY()];
    // Handle non claimed
    switch(tileState.mTileVisual)
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
    if(tileState.mTileVisual != TileVisual::claimedFull)
    {
        OD_LOG_ERR("mTileVisual=" + Tile::tileVisualToString(tileState.mTileVisual));
        return false;
    }

    // It is claimed. If it is by the given seat team, it can be dug
    Seat* seat = mGameMap->getSeatById(tileState.mSeatIdOwner);
    if(!canOwnedTileBeClaimedBy(seat))
        return true;

    return false;
}

const CreatureDefinition* Seat::getNextFighterClassToSpawn(const GameMap& gameMap, const ConfigManager& configManager)
{
    std::vector<std::pair<const CreatureDefinition*, int32_t> > defSpawnable;
    int32_t nbPointsTotal = 0;
    for(std::pair<const CreatureDefinition*, bool>& def : mSpawnPool)
    {
        // Only check for fighter creatures.
        if (!def.first || def.first->isWorker())
            continue;

        const std::vector<const SpawnCondition*>& conditions = configManager.getCreatureSpawnConditions(def.first);
        int32_t nbPointsConditions = 0;
        for(const SpawnCondition* condition : conditions)
        {
            int32_t nbPointsCondition = 0;
            if(!condition->computePointsForSeat(gameMap, *this, nbPointsCondition))
            {
                nbPointsConditions = -1;
                break;
            }
            nbPointsConditions += nbPointsCondition;
        }

        // Check if the creature can spawn. nbPointsConditions < 0 can happen if a condition is not met or if there are too many
        // negative points. In both cases, we don't want the creature to spawn
        if(nbPointsConditions < 0)
            continue;

        // Check if it is the first time this conditions have been fulfilled. If yes, we force this creature to spawn
        if(!def.second && !conditions.empty())
        {
            def.second = true;
            return def.first;
        }
        nbPointsConditions += configManager.getBaseSpawnPoint();
        defSpawnable.push_back(std::pair<const CreatureDefinition*, int32_t>(def.first, nbPointsConditions));
        nbPointsTotal += nbPointsConditions;
    }

    if(defSpawnable.empty())
        return nullptr;

    // We choose randomly a creature to spawn according to their points
    int32_t cpt = Random::Int(0, nbPointsTotal - 1);
    for(std::pair<const CreatureDefinition*, int32_t>& def : defSpawnable)
    {
        if(cpt < def.second)
            return def.first;

        cpt -= def.second;
    }

    // It is not normal to come here
    OD_LOG_ERR("seatId=" + Helper::toString(getId()));
    return nullptr;
}

ODPacket& operator<<(ODPacket& os, Seat *s)
{
    os << s->mId << s->mTeamId << s->mPlayerType << s->mFaction << s->mStartingX
       << s->mStartingY;
    os << s->mColorId;
    os << s->mGold << s->mGoldMax;
    os << s->mMana << s->mManaDelta << s->mNumClaimedTiles;
    os << s->mNumCreaturesFighters << s->mNumCreaturesFightersMax;
    os << s->mNumCreaturesWorkers;
    os << s->mHasGoalsChanged;
    for(const uint32_t& nbRooms : s->mNbRooms)
    {
        os << nbRooms;
    }
    os << s->mCurrentResearchType;
    os << s->mCurrentResearchProgress;
    uint32_t nb;
    nb  = s->mAvailableTeamIds.size();
    os << nb;
    for(int teamId : s->mAvailableTeamIds)
        os << teamId;

    nb = s->mResearchNotAllowed.size();
    os << nb;
    for(ResearchType resType : s->mResearchNotAllowed)
        os << resType;

    return os;
}

ODPacket& operator>>(ODPacket& is, Seat *s)
{
    is >> s->mId >> s->mTeamId >> s->mPlayerType;
    is >> s->mFaction >> s->mStartingX >> s->mStartingY;
    is >> s->mColorId;
    is >> s->mGold >> s->mGoldMax;
    is >> s->mMana >> s->mManaDelta >> s->mNumClaimedTiles;
    is >> s->mNumCreaturesFighters >> s->mNumCreaturesFightersMax;
    is >> s->mNumCreaturesWorkers;
    is >> s->mHasGoalsChanged;
    for(uint32_t& nbRooms : s->mNbRooms)
    {
        is >> nbRooms;
    }
    is >> s->mCurrentResearchType;
    is >> s->mCurrentResearchProgress;
    s->mColorValue = ConfigManager::getSingleton().getColorFromId(s->mColorId);
    uint32_t nb;
    is >> nb;
    while(nb > 0)
    {
        --nb;
        int teamId;
        is >> teamId;
        s->mAvailableTeamIds.push_back(teamId);
    }

    is >> nb;
    while(nb > 0)
    {
        --nb;
        ResearchType resType;
        is >> resType;
        s->mResearchNotAllowed.push_back(resType);
    }

    return is;
}

int Seat::readTilesVisualInitialStates(TileVisual tileVisual, std::istream& is)
{
    // We check if it is the Seat end tag
    std::string str;
    OD_ASSERT_TRUE(is >> str);
    if (str == "[/Seat]")
        return 0;

    if(str != "[" + Tile::tileVisualToString(tileVisual) + "]")
    {
        OD_LOG_INF("WARNING: expected [" + Tile::tileVisualToString(tileVisual) + "] and read " + str);
        return -1;
    }


    while(true)
    {
        OD_ASSERT_TRUE(is >> str);
        if(str == "[/" + Tile::tileVisualToString(tileVisual) + "]")
            break;

        std::pair<int, int> tilecoords;
        tilecoords.first = Helper::toInt(str);

        OD_ASSERT_TRUE(is >> str);
        tilecoords.second = Helper::toInt(str);

        int seatId;
        OD_ASSERT_TRUE(is >> seatId);

        TileStateNotified& tileState = mTilesStateLoaded[tilecoords];
        tileState.mTileVisual = tileVisual;
        tileState.mSeatIdOwner = seatId;
    }

    return 1;
}

void Seat::setPlayerSettings(bool koCreatures)
{
    mKoCreatures = koCreatures;
    if(!mGameMap->isServerGameMap())
        return;

    if(getPlayer() == nullptr)
        return;

    // We send a message to the client to update his settings
    ServerNotification *serverNotification = new ServerNotification(
        ServerNotificationType::setPlayerSettings, getPlayer());

    serverNotification->mPacket << mKoCreatures;
    ODServer::getSingleton().queueServerNotification(serverNotification);
}
