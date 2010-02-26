#include "Seat.h"

void Seat::addGoal(Goal *g)
{
	goals.push_back(g);
}

unsigned int Seat::numGoals()
{
	return goals.size();
}

Goal* Seat::getGoal(unsigned int index)
{
	return goals[index];
}

unsigned int Seat::numCompletedGoals()
{
	return completedGoals.size();
}

Goal* Seat::getCompletedGoal(unsigned int index)
{
	return completedGoals[index];
}

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

