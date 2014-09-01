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

#include "Seat.h"

#include "Goal.h"
#include "Helper.h"

Seat::Seat() :
        mColor(0),
        mStartingX(0),
        mStartingY(0),
        mMana(1000),
        mManaDelta(0),
        mHp(1000),
        mGold(0),
        mGoldMined(0),
        mNumCreaturesControlled(0),
        mNumClaimedTiles(0),
        mHasGoalsChanged(true)
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
    std::vector<Goal*>::iterator currentGoal = mUncompleteGoals.begin();
    while (currentGoal != mUncompleteGoals.end())
    {
        // Start by checking if the goal has been met by this seat.
        if ((*currentGoal)->isMet(this))
        {
            mCompletedGoals.push_back(*currentGoal);

            // Add any subgoals upon completion to the list of outstanding goals.
            for (unsigned int i = 0; i < (*currentGoal)->numSuccessSubGoals(); ++i)
                mUncompleteGoals.push_back((*currentGoal)->getSuccessSubGoal(i));

            currentGoal = mUncompleteGoals.erase(currentGoal);

        }
        else
        {
            // If the goal has not been met, check to see if it cannot be met in the future.
            if ((*currentGoal)->isFailed(this))
            {
                mFailedGoals.push_back(*currentGoal);

                // Add any subgoals upon completion to the list of outstanding goals.
                for (unsigned int i = 0; i < (*currentGoal)->numFailureSubGoals(); ++i)
                    mUncompleteGoals.push_back((*currentGoal)->getFailureSubGoal(i));

                currentGoal = mUncompleteGoals.erase(currentGoal);
            }
            else
            {
                // The goal has not been met but has also not been definitively failed, continue on to the next goal in the list.
                ++currentGoal;
            }
        }
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

std::string Seat::getFormat()
{
    return "color\tfaction\tstartingX\tstartingY\tcolorR\tcolorG\tcolorB";
}

ODPacket& operator<<(ODPacket& os, Seat *s)
{
    os << s->mColor << s->mFaction << s->mStartingX
       << s->mStartingY;
    os << s->mColorValue.r << s->mColorValue.g
       << s->mColorValue.b;
    os << s->mGold << s->mMana << s->mManaDelta << s->mNumClaimedTiles;
    os << s->mHasGoalsChanged;

    return os;
}

ODPacket& operator>>(ODPacket& is, Seat *s)
{
    is >> s->mColor >> s->mFaction >> s->mStartingX >> s->mStartingY;
    is >> s->mColorValue.r >> s->mColorValue.g >> s->mColorValue.b;
    is >> s->mGold >> s->mMana >> s->mManaDelta >> s->mNumClaimedTiles;
    is >> s->mHasGoalsChanged;
    s->mColorValue.a = 1.0;

    return is;
}

void Seat::loadFromLine(const std::string& line, Seat *s)
{
    std::vector<std::string> elems = Helper::split(line, '\t');

    s->mColor = Helper::toInt(elems[0]);
    s->mFaction = elems[1];
    s->mStartingX = Helper::toInt(elems[2]);
    s->mStartingY = Helper::toInt(elems[3]);
    s->mColorValue.r = Helper::toDouble(elems[4]);
    s->mColorValue.g = Helper::toDouble(elems[5]);
    s->mColorValue.b = Helper::toDouble(elems[6]);
    s->mStartingGold = Helper::toInt(elems[7]);
    s->mColorValue.a = 1.0;
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
