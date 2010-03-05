#include "Seat.h"

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
	return goals.size();
}

/** \brief A simple accessor function to allow for looping over the goals which must be completed by this seat before it can be declared a winner.
  *
*/
Goal* Seat::getGoal(unsigned int index)
{
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
	return completedGoals[index];
}

/** \brief Loop over the vector of unmet goals and call the isMet() function on each one, if it is met move it to the completedGoals vector.
  *
*/
unsigned int Seat::checkAllGoals()
{
	// Loop over the goals vector and move any goals that have been met to the completed goals vector.
	vector<Goal*>::iterator currentGoal = goals.begin();
	while(currentGoal != goals.end())
	{
		if((*currentGoal)->isMet(this))
		{
			completedGoals.push_back(*currentGoal);
			currentGoal = goals.erase(currentGoal);
		}
		else
		{
			currentGoal++;
		}
	}

	return goals.size();
}

/** \brief Loop over the vector of met goals and call the isUnmet() function on each one, if any of them are no longer satisfied move them back to the goals vector.
  *
*/
unsigned int Seat::checkAllCompletedGoals()
{
	// Loop over the goals vector and move any goals that have been met to the completed goals vector.
	vector<Goal*>::iterator currentGoal = completedGoals.begin();
	while(currentGoal != completedGoals.end())
	{
		if((*currentGoal)->isUnmet(this))
		{
			goals.push_back(*currentGoal);
			currentGoal = completedGoals.erase(currentGoal);
		}
		else
		{
			currentGoal++;
		}
	}

	return completedGoals.size();
}

ostream& operator<<(ostream& os, Seat *s)
{
	os << s->color << "\t" << s->faction << "\t" << s->startingX << "\t" << s->startingY << "\t";
	os << s->colourValue.r << "\t" << s->colourValue.g << "\t" << s->colourValue.b << "\n";

	return os;
}

istream& operator>>(istream& is, Seat *s)
{
	is >> s->color >> s->faction >> s->startingX >> s->startingY;
	is >> s->colourValue.r >> s->colourValue.g >> s->colourValue.b;
	s->colourValue.a = 1.0;

	return is;
}

