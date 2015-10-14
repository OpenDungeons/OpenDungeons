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

#include "entities/Tile.h"
#include "game/Player.h"
#include "game/Research.h"
#include "goals/Goal.h"
#include "rooms/RoomType.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"

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

