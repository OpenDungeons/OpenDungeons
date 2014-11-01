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

#include "entities/CreatureDefinition.h"

#include "game/Seat.h"
#include "gamemap/GameMap.h"

#include "goals/Goal.h"
#include "network/ODPacket.h"

#include "spawnconditions/SpawnCondition.h"

#include "utils/ConfigManager.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"
#include "utils/Random.h"

Seat::Seat(GameMap* gameMap) :
    mTeamId(0),
    mStartingX(0),
    mStartingY(0),
    mMana(1000),
    mManaDelta(0),
    mHp(1000),
    mGoldMined(0),
    mNumCreaturesControlled(0),
    mGameMap(gameMap),
    mNumClaimedTiles(0),
    mHasGoalsChanged(true),
    mGold(0),
    mId(-1)
{
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
        return NULL;

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
        return NULL;

    return mCompletedGoals[index];
}

unsigned int Seat::numFailedGoals()
{
    return mFailedGoals.size();
}

Goal* Seat::getFailedGoal(unsigned int index)
{
    if (index >= mFailedGoals.size())
        return NULL;

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

void Seat::addSpawnableCreature(const std::string& creature_name)
{
    const CreatureDefinition* def = ConfigManager::getSingleton().getCreatureDefinition(creature_name);
    OD_ASSERT_TRUE_MSG(def != nullptr, "creature_name=" + creature_name);
    if(def == nullptr)
        return;
    mSpawnPool.push_back(std::pair<const CreatureDefinition*, bool>(def, false));
}


void Seat::writeSpawnPool(std::ofstream& file) const
{
    for(std::pair<const CreatureDefinition*, bool> def : mSpawnPool)
    {
        file << def.first->getClassName() << std::endl;
    }
}

const CreatureDefinition* Seat::getNextCreatureClassToSpawn()
{
    std::vector<std::pair<const CreatureDefinition*, int32_t> > defSpawnable;
    int32_t nbPointsTotal = 0;
    for(std::pair<const CreatureDefinition*, bool>& def : mSpawnPool)
    {
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

    // We choose randomly a creature to spawn according to there points
    int32_t cpt = Random::Int(0, nbPointsTotal);
    for(std::pair<const CreatureDefinition*, int32_t>& def : defSpawnable)
    {
        if(cpt < def.second)
            return def.first;

        cpt -= def.second;
    }

    // It is not normal to come here
    OD_ASSERT_TRUE(false);
    return nullptr;
}

std::string Seat::getFormat()
{
    return "seatId\tteamId\tfaction\tstartingX\tstartingY\tcolor\tstartingGold";
}

ODPacket& operator<<(ODPacket& os, Seat *s)
{
    os << s->mId << s->mTeamId << s->mFaction << s->mStartingX
       << s->mStartingY;
    os << s->mColorId;
    os << s->mGold << s->mMana << s->mManaDelta << s->mNumClaimedTiles;
    os << s->mHasGoalsChanged;

    return os;
}

ODPacket& operator>>(ODPacket& is, Seat *s)
{
    is >> s->mId >> s->mTeamId >> s->mFaction >> s->mStartingX >> s->mStartingY;
    is >> s->mColorId;
    is >> s->mGold >> s->mMana >> s->mManaDelta >> s->mNumClaimedTiles;
    is >> s->mHasGoalsChanged;
    s->mColorValue = ConfigManager::getSingleton().getColorFromId(s->mColorId);

    return is;
}

const std::string Seat::getFactionFromLine(const std::string& line)
{
    std::vector<std::string> elems = Helper::split(line, '\t');
    OD_ASSERT_TRUE_MSG(elems.size() > 2, "line=" + line);
    if(elems.size() > 2)
        return elems[2];

    return std::string();
}

void Seat::loadFromLine(const std::string& line, Seat *s)
{
    std::vector<std::string> elems = Helper::split(line, '\t');

    s->mId = Helper::toInt(elems[0]);
    s->mTeamId = Helper::toInt(elems[1]);
    s->mFaction = elems[2];
    s->mStartingX = Helper::toInt(elems[3]);
    s->mStartingY = Helper::toInt(elems[4]);
    s->mColorId = elems[5];
    s->mStartingGold = Helper::toInt(elems[6]);
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
}

bool Seat::sortForMapSave(Seat* s1, Seat* s2)
{
    return s1->mId < s2->mId;
}

std::ostream& operator<<(std::ostream& os, Seat *s)
{
    os << s->mId << "\t" << s->mTeamId << "\t" << s->mFaction << "\t" << s->mStartingX
       << "\t"<< s->mStartingY;
    os << "\t" << s->mColorId;
    os << "\t" << s->mStartingGold;
    return os;
}

std::istream& operator>>(std::istream& is, Seat *s)
{
    is >> s->mId >> s->mTeamId >> s->mFaction >> s->mStartingX >> s->mStartingY;
    is >> s->mColorId;
    is >> s->mStartingGold;
    s->mColorValue = ConfigManager::getSingleton().getColorFromId(s->mColorId);
    return is;
}
