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

void Seat::completeGoal(unsigned int index)
{
	Goal *tempGoal = goals[index];
	goals.erase(goals.begin() + index);
	completedGoals.push_back(tempGoal);
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

