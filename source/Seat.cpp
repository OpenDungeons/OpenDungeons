#include "Seat.h"
#include "Goal.h"

#include "Helper.h"

Seat::Seat() :
        color(0),
        startingX(0),
        startingY(0),
        mana(1000),
        manaDelta(0),
        hp(1000),
        gold(0),
        goldMined(0),
        numCreaturesControlled(0),
        factionHumans(0.0),
        factionCorpars(0.0),
        factionUndead(0.0),
        factionConstructs(0.0),
        factionDenizens(0.0),
        alignmentAltruism(0.0),
        alignmentOrder(0.0),
        alignmentPeace(0.0),
        numClaimedTiles(0),
        hasGoalsChanged(true)
{
}

/** \brief Adds a goal to the vector of goals which must be completed by this seat before it can be declared a winner.
 *
 */
void Seat::addGoal(Goal *g)
{
    goals.push_back(g);
}

/** \brief A simple accessor function to return the number of goals which must be completed by this seat before it can be declared a winner.
 *
 */
unsigned int Seat::numGoals()
{
    unsigned int tempUnsigned = goals.size();

    return tempUnsigned;
}

/** \brief A simple accessor function to allow for looping over the goals which must be completed by this seat before it can be declared a winner.
 *
 */
Goal* Seat::getGoal(unsigned int index)
{
    if (index >= goals.size())
        return NULL;

    return goals[index];
}

/** \brief A simple mutator to clear the vector of unmet goals.
 *
 */
void Seat::clearGoals()
{
    goals.clear();
}

/** \brief A simple mutator to clear the vector of met goals.
 *
 */
void Seat::clearCompletedGoals()
{
    completedGoals.clear();
}

/** \brief A simple accessor function to return the number of goals completed by this seat.
 *
 */
unsigned int Seat::numCompletedGoals()
{
    return completedGoals.size();
}

/** \brief A simple accessor function to allow for looping over the goals completed by this seat.
 *
 */
Goal* Seat::getCompletedGoal(unsigned int index)
{
    if (index >= completedGoals.size())
        return NULL;

    return completedGoals[index];
}

/** \brief A simple accessor function to return the number of goals failed by this seat.
 *
 */
unsigned int Seat::numFailedGoals()
{
    return failedGoals.size();
}

/** \brief A simple accessor function to allow for looping over the goals failed by this seat.
 *
 */
Goal* Seat::getFailedGoal(unsigned int index)
{
    if (index >= failedGoals.size())
        return NULL;

    return failedGoals[index];
}

unsigned int Seat::getNumClaimedTiles()
{
    return numClaimedTiles;
}

void Seat::setNumClaimedTiles(const unsigned int& num)
{
    numClaimedTiles = num;
}

/** \brief Increment the number of claimed tiles by 1
 *
 */
void Seat::incrementNumClaimedTiles()
{
    ++numClaimedTiles;
}

/** \brief Loop over the vector of unmet goals and call the isMet() and isFailed() functions on
 * each one, if it is met move it to the completedGoals vector.
 *
 */
unsigned int Seat::checkAllGoals()
{
    // Loop over the goals vector and move any goals that have been met to the completed goals vector.
    std::vector<Goal*>::iterator currentGoal = goals.begin();
    while (currentGoal != goals.end())
    {
        // Start by checking if the goal has been met by this seat.
        if ((*currentGoal)->isMet(this))
        {
            completedGoals.push_back(*currentGoal);

            // Add any subgoals upon completion to the list of outstanding goals.
            for (unsigned int i = 0; i < (*currentGoal)->numSuccessSubGoals(); ++i)
                goals.push_back((*currentGoal)->getSuccessSubGoal(i));

            //FIXME: This is probably a memory leak since the goal is created on the heap and should probably be deleted here.
            currentGoal = goals.erase(currentGoal);

        }
        else
        {
            // If the goal has not been met, check to see if it cannot be met in the future.
            if ((*currentGoal)->isFailed(this))
            {
                failedGoals.push_back(*currentGoal);

                // Add any subgoals upon completion to the list of outstanding goals.
                for (unsigned int i = 0; i
                        < (*currentGoal)->numFailureSubGoals(); ++i)
                    goals.push_back((*currentGoal)->getFailureSubGoal(i));

                //FIXME: This is probably a memory leak since the goal is created on the heap and should probably be deleted here.
                currentGoal = goals.erase(currentGoal);
            }
            else
            {
                // The goal has not been met but has also not been definitively failed, continue on to the next goal in the list.
                ++currentGoal;
            }
        }
    }

    return numGoals();
}

/** \brief Loop over the vector of met goals and call the isUnmet() function on each one, if any of them are no longer satisfied move them back to the goals vector.
 *
 */
unsigned int Seat::checkAllCompletedGoals()
{
    // Loop over the goals vector and move any goals that have been met to the completed goals vector.
    std::vector<Goal*>::iterator currentGoal = completedGoals.begin();
    while (currentGoal != completedGoals.end())
    {
        // Start by checking if this previously met goal has now been unmet.
        if ((*currentGoal)->isUnmet(this))
        {
            goals.push_back(*currentGoal);

            currentGoal = completedGoals.erase(currentGoal);

            //Signal that the list of goals has changed.
            goalsHasChanged();
        }
        else
        {
            // Next check to see if this previously met goal has now been failed.
            if ((*currentGoal)->isFailed(this))
            {
                failedGoals.push_back(*currentGoal);

                currentGoal = completedGoals.erase(currentGoal);

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

/** \brief See if the goals has changed since we last checked.
 *  For use with the goal window, to avoid having to update it on every frame.
 */
bool Seat::getHasGoalsChanged()
{
    return hasGoalsChanged;
}

void Seat::resetGoalsChanged()
{
    hasGoalsChanged = false;
}

void Seat::goalsHasChanged()
{
    //Not locking here as this is supposed to be called from a function that already locks.
    hasGoalsChanged = true;
}

std::string Seat::getFormat()
{
    return "color\tfaction\tstartingX\tstartingY\tcolorR\tcolorG\tcolorB";
}

std::ostream& operator<<(std::ostream& os, Seat *s)
{
    os << s->color << "\t" << s->faction << "\t" << s->startingX << "\t"
            << s->startingY << "\t";
    os << s->colourValue.r << "\t" << s->colourValue.g << "\t"
            << s->colourValue.b << "\n";

    return os;
}

std::istream& operator>>(std::istream& is, Seat *s)
{
    is >> s->color >> s->faction >> s->startingX >> s->startingY;
    is >> s->colourValue.r >> s->colourValue.g >> s->colourValue.b;
    s->colourValue.a = 1.0;

    return is;
}

void Seat::loadFromLine(const std::string& line, Seat *s)
{
    std::vector<std::string> elems = Helper::split(line, '\t');

    s->color = Helper::toInt(elems[0]);
    s->faction = elems[1];
    s->startingX = Helper::toInt(elems[2]);
    s->startingY = Helper::toInt(elems[3]);
    s->colourValue.r = Helper::toDouble(elems[4]);
    s->colourValue.g = Helper::toDouble(elems[5]);
    s->colourValue.b = Helper::toDouble(elems[6]);
    s->colourValue.a = 1.0;
}
