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

#include "entities/CreatureDefinition.h"

#include "game/Seat.h"
#include "gamemap/GameMap.h"

#include "goals/Goal.h"
#include "network/ODPacket.h"

#include "spawnconditions/SpawnCondition.h"

#include "network/ServerNotification.h"
#include "network/ODServer.h"

#include "rooms/Room.h"

#include "utils/ConfigManager.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"
#include "utils/Random.h"

const std::string Seat::PLAYER_TYPE_HUMAN = "Human";
const std::string Seat::PLAYER_TYPE_AI = "AI";
const std::string Seat::PLAYER_TYPE_INACTIVE = "Inactive";
const std::string Seat::PLAYER_TYPE_CHOICE = "Choice";
const std::string Seat::PLAYER_FACTION_CHOICE = "Choice";

Seat::Seat(GameMap* gameMap) :
    mGameMap(gameMap),
    mPlayer(nullptr),
    mTeamId(-1),
    mMana(1000),
    mManaDelta(0),
    mStartingX(0),
    mStartingY(0),
    mGoldMined(0),
    mNumCreaturesControlled(0),
    mStartingGold(0),
    mDefaultWorkerClass(nullptr),
    mNumClaimedTiles(0),
    mHasGoalsChanged(true),
    mGold(0),
    mId(-1),
    mNbTreasuries(0),
    mIsDebuggingVision(false)
{
}

void Seat::setMapSize(int x, int y)
{
    if(mPlayer == nullptr)
        return;
    if(!mPlayer->getIsHuman())
        return;

    mTilesVision = std::vector<std::vector<std::pair<bool, bool>>>(x);
    for(int xxx = 0; xxx < x; ++xxx)
    {
        mTilesVision[xxx] = std::vector<std::pair<bool, bool>>(y);
        for(int yyy = 0; yyy < y; ++yyy)
        {
            mTilesVision[xxx][yyy] = std::pair<bool, bool>(false, false);
        }
    }
}

void Seat::setTeamId(int teamId)
{
    OD_ASSERT_TRUE_MSG(std::find(mAvailableTeamIds.begin(), mAvailableTeamIds.end(),
        teamId) != mAvailableTeamIds.end(), "Unknown team id=" + Ogre::StringConverter::toString(teamId)
        + ", for seat id=" + Ogre::StringConverter::toString(getId()));
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

unsigned int Seat::getNumClaimedTiles()
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

unsigned int Seat::checkAllGoals()
{
    // Loop over the goals vector and move any goals that have been met to the completed goals vector.
    std::vector<Goal*> goalsToAdd;
    std::vector<Goal*>::iterator currentGoal = mUncompleteGoals.begin();
    while (currentGoal != mUncompleteGoals.end())
    {
        Goal* goal = *currentGoal;
        // Start by checking if the goal has been met by this seat.
        if (goal->isMet(this))
        {
            mCompletedGoals.push_back(goal);

            // Add any subgoals upon completion to the list of outstanding goals.
            for (unsigned int i = 0; i < goal->numSuccessSubGoals(); ++i)
                goalsToAdd.push_back(goal->getSuccessSubGoal(i));

            currentGoal = mUncompleteGoals.erase(currentGoal);

        }
        else
        {
            // If the goal has not been met, check to see if it cannot be met in the future.
            if (goal->isFailed(this))
            {
                mFailedGoals.push_back(goal);

                // Add any subgoals upon completion to the list of outstanding goals.
                for (unsigned int i = 0; i < goal->numFailureSubGoals(); ++i)
                    goalsToAdd.push_back(goal->getFailureSubGoal(i));

                currentGoal = mUncompleteGoals.erase(currentGoal);
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

unsigned int Seat::checkAllCompletedGoals()
{
    // Loop over the goals vector and move any goals that have been met to the completed goals vector.
    std::vector<Goal*>::iterator currentGoal = mCompletedGoals.begin();
    while (currentGoal != mCompletedGoals.end())
    {
        // Start by checking if this previously met goal has now been unmet.
        if ((*currentGoal)->isUnmet(this))
        {
            mUncompleteGoals.push_back(*currentGoal);

            currentGoal = mCompletedGoals.erase(currentGoal);

            //Signal that the list of goals has changed.
            goalsHasChanged();
        }
        else
        {
            // Next check to see if this previously met goal has now been failed.
            if ((*currentGoal)->isFailed(this))
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

bool Seat::isAlliedSeat(Seat *seat)
{
    return getTeamId() == seat->getTeamId();
}

bool Seat::canOwnedCreatureBePickedUpBy(Seat* seat)
{
    // Note : if we want to allow players to pickup allied creatures, we can do that here.
    if(this == seat)
        return true;

    return false;
}

bool Seat::canOwnedTileBeClaimedBy(Seat* seat)
{
    if(getTeamId() != seat->getTeamId())
        return true;

    return false;
}

bool Seat::canOwnedCreatureUseRoomFrom(Seat* seat)
{
    if(this == seat)
        return true;

    return false;
}

bool Seat::canRoomBeDestroyedBy(Seat* seat)
{
    if(this == seat)
        return true;

    return false;
}

bool Seat::canTrapBeDestroyedBy(Seat* seat)
{
    if(this == seat)
        return true;

    return false;
}

void Seat::setPlayer(Player* player)
{
    OD_ASSERT_TRUE_MSG(mPlayer == nullptr, "A player=" + mPlayer->getNick() + " already on seat id="
        + Ogre::StringConverter::toString(getId()));

    mPlayer = player;
    mPlayer->mSeat = this;
}


void Seat::addAlliedSeat(Seat* seat)
{
    mAlliedSeats.push_back(seat);
}


void Seat::initSpawnPool()
{
    ConfigManager& config = ConfigManager::getSingleton();
    const std::vector<std::string>& pool = config.getFactionSpawnPool(mFaction);
    OD_ASSERT_TRUE_MSG(!pool.empty(), "Empty spawn pool for faction=" + mFaction);
    for(const std::string& defName : pool)
    {
        const CreatureDefinition* def = mGameMap->getClassDescription(defName);
        OD_ASSERT_TRUE_MSG(def != nullptr, "defName=" + defName);
        if(def == nullptr)
            continue;

        mSpawnPool.push_back(std::pair<const CreatureDefinition*, bool>(def, false));
    }

    // Get the default worker class
    std::string defaultWorkerClass = config.getFactionWorkerClass(mFaction);
    mDefaultWorkerClass = mGameMap->getClassDescription(defaultWorkerClass);
    OD_ASSERT_TRUE_MSG(mDefaultWorkerClass != nullptr, "No valid default worker class for faction: " + mFaction);
}

const CreatureDefinition* Seat::getNextFighterClassToSpawn()
{
    std::vector<std::pair<const CreatureDefinition*, int32_t> > defSpawnable;
    int32_t nbPointsTotal = 0;
    for(std::pair<const CreatureDefinition*, bool>& def : mSpawnPool)
    {
        // Only check for fighter creatures.
        if (!def.first || def.first->isWorker())
            continue;

        const std::vector<const SpawnCondition*>& conditions = ConfigManager::getSingleton().getCreatureSpawnConditions(def.first);
        int32_t nbPointsConditions = 0;
        for(const SpawnCondition* condition : conditions)
        {
            int32_t nbPointsCondition = 0;
            if(!condition->computePointsForSeat(mGameMap, this, nbPointsCondition))
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

        // Check if it is the first time this conditions have been fullfilled. If yes, we force this creature to spawn
        if(!def.second && !conditions.empty())
        {
            def.second = true;
            return def.first;
        }
        nbPointsConditions += ConfigManager::getSingleton().getBaseSpawnPoint();
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
    OD_ASSERT_TRUE_MSG(false, "seatId=" + Ogre::StringConverter::toString(getId()));
    return nullptr;
}

void Seat::clearTilesWithVision()
{
    if(mPlayer == nullptr)
        return;
    if(!mPlayer->getIsHuman())
        return;

    for(std::vector<std::pair<bool, bool>>& vec : mTilesVision)
    {
        for(std::pair<bool, bool>& p : vec)
        {
            p.first = p.second;
            p.second = false;
        }
    }
}

void Seat::notifyVisionOnTile(Tile* tile)
{
    if(mPlayer == nullptr)
        return;
    if(!mPlayer->getIsHuman())
        return;

    OD_ASSERT_TRUE_MSG(tile->getX() < static_cast<int>(mTilesVision.size()), "Tile=" + Tile::displayAsString(tile));
    OD_ASSERT_TRUE_MSG(tile->getY() < static_cast<int>(mTilesVision[tile->getX()].size()), "Tile=" + Tile::displayAsString(tile));
    std::pair<bool, bool>& p = mTilesVision[tile->getX()][tile->getY()];
    p.second = true;
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

    OD_ASSERT_TRUE_MSG(tile->getX() < static_cast<int>(mTilesVision.size()), "Tile=" + Tile::displayAsString(tile));
    OD_ASSERT_TRUE_MSG(tile->getY() < static_cast<int>(mTilesVision[tile->getX()].size()), "Tile=" + Tile::displayAsString(tile));
    std::pair<bool, bool>& p = mTilesVision[tile->getX()][tile->getY()];

    return p.second;
}

void Seat::notifyChangedVisibleTiles()
{
    if(mPlayer == nullptr)
        return;
    if(!mPlayer->getIsHuman())
        return;

    std::vector<Tile*> tilesToNotify;
    int xMax = static_cast<int>(mTilesVision.size());
    for(int xxx = 0; xxx < xMax; ++xxx)
    {
        int yMax = static_cast<int>(mTilesVision[xxx].size());
        for(int yyy = 0; yyy < yMax; ++yyy)
        {
            if(!mTilesVision[xxx][yyy].second)
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
        tile->exportToPacket(serverNotification->mPacket, this);
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
    // mTilesVision for them which would be memory consuming)
    if(mPlayer == nullptr)
        return;
    if(!mPlayer->getIsHuman())
        return;

    mIsDebuggingVision = enable;
    int seatId = getId();
    if(enable)
    {
        std::vector<Tile*> tiles;
        int xMax = static_cast<int>(mTilesVision.size());
        for(int xxx = 0; xxx < xMax; ++xxx)
        {
            int yMax = static_cast<int>(mTilesVision[xxx].size());
            for(int yyy = 0; yyy < yMax; ++yyy)
            {
                if(!mTilesVision[xxx][yyy].second)
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
    int xMax = static_cast<int>(mTilesVision.size());
    for(int xxx = 0; xxx < xMax; ++xxx)
    {
        int yMax = static_cast<int>(mTilesVision[xxx].size());
        for(int yyy = 0; yyy < yMax; ++yyy)
        {
            if(mTilesVision[xxx][yyy].second == mTilesVision[xxx][yyy].first)
                continue;

            Tile* tile = mGameMap->getTile(xxx, yyy);
            if(mTilesVision[xxx][yyy].second)
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

void Seat::computeSeatBeforeSendingToClient()
{
    if(mPlayer != nullptr)
    {
        mNbTreasuries = mGameMap->numRoomsByTypeAndSeat(RoomType::treasury, this);
    }
}

std::string Seat::getFormat()
{
    return "seatId\tteamId\tplayer\tfaction\tstartingX\tstartingY\tcolor\tstartingGold";
}

ODPacket& operator<<(ODPacket& os, Seat *s)
{
    os << s->mId << s->mTeamId << s->mPlayerType << s->mFaction << s->mStartingX
       << s->mStartingY;
    os << s->mColorId;
    os << s->mGold << s->mMana << s->mManaDelta << s->mNumClaimedTiles;
    os << s->mHasGoalsChanged;
    os << s->mNbTreasuries;
    uint32_t nb = s->mAvailableTeamIds.size();
    os << nb;
    for(int teamId : s->mAvailableTeamIds)
        os << teamId;

    return os;
}

ODPacket& operator>>(ODPacket& is, Seat *s)
{
    is >> s->mId >> s->mTeamId >> s->mPlayerType;
    is >> s->mFaction >> s->mStartingX >> s->mStartingY;
    is >> s->mColorId;
    is >> s->mGold >> s->mMana >> s->mManaDelta >> s->mNumClaimedTiles;
    is >> s->mHasGoalsChanged;
    is >> s->mNbTreasuries;
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

    return is;
}

const std::string Seat::getFactionFromLine(const std::string& line)
{
    const uint32_t indexFactionInLine = 3;
    std::vector<std::string> elems = Helper::split(line, '\t');
    OD_ASSERT_TRUE_MSG(elems.size() > indexFactionInLine, "line=" + line);
    if(elems.size() > indexFactionInLine)
        return elems[indexFactionInLine];

    return std::string();
}

Seat* Seat::getRogueSeat(GameMap* gameMap)
{
    Seat* seat = new Seat(gameMap);
    seat->mId = 0;
    seat->mTeamId = 0;
    seat->mAvailableTeamIds.push_back(0);
    seat->mPlayerType = PLAYER_TYPE_INACTIVE;
    seat->mStartingX = 0;
    seat->mStartingY = 0;
    seat->mStartingGold = 0;
    return seat;
}

void Seat::loadFromLine(const std::string& line, Seat *s)
{
    std::vector<std::string> elems = Helper::split(line, '\t');

    int32_t i = 0;
    s->mId = Helper::toInt(elems[i++]);
    OD_ASSERT_TRUE_MSG(s->mId != 0, "Forbidden seatId for line=" + line);
    std::vector<std::string> teamIds = Helper::split(elems[i++], '/');
    for(const std::string& strTeamId : teamIds)
    {
        int teamId = Helper::toInt(strTeamId);
        OD_ASSERT_TRUE_MSG(teamId != 0, "Forbidden teamId for line=" + line);
        if(teamId == 0)
            continue;

        s->mAvailableTeamIds.push_back(teamId);
    }
    s->mPlayerType = elems[i++];
    s->mFaction = elems[i++];
    s->mStartingX = Helper::toInt(elems[i++]);
    s->mStartingY = Helper::toInt(elems[i++]);
    s->mColorId = elems[i++];
    s->mStartingGold = Helper::toInt(elems[i++]);
    s->mColorValue = ConfigManager::getSingleton().getColorFromId(s->mColorId);
}

void Seat::refreshFromSeat(Seat* s)
{
    // We only refresh data that changes over time (gold, mana, ...)
    mGold = s->mGold;
    mMana = s->mMana;
    mManaDelta = s->mManaDelta;
    mNumClaimedTiles = s->mNumClaimedTiles;
    mHasGoalsChanged = s->mHasGoalsChanged;
    mNbTreasuries = s->mNbTreasuries;
}

bool Seat::takeMana(double mana)
{
    if(mana > mMana)
        return false;

    mMana -= mana;
    return true;
}

bool Seat::sortForMapSave(Seat* s1, Seat* s2)
{
    return s1->mId < s2->mId;
}

std::ostream& operator<<(std::ostream& os, Seat *s)
{
    os << s->mId;
    // If the team id is set, we save it. Otherwise, we save all the available team ids
    // That way, save map will work in both editor and in game.
    if(s->mTeamId != -1)
    {
        os << "\t" << s->mTeamId;
    }
    else
    {
        int cpt = 0;
        for(int teamId : s->mAvailableTeamIds)
        {
            if(cpt == 0)
                os << "\t";
            else
                os << "/";

            os << teamId;
            ++cpt;
        }
    }
    os << "\t" << s->mPlayerType << "\t" << s->mFaction << "\t" << s->mStartingX
       << "\t"<< s->mStartingY;
    os << "\t" << s->mColorId;
    os << "\t" << s->mStartingGold;
    return os;
}
