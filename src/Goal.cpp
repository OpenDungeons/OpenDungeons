#include "Goal.h"

Goal::Goal(string nName, string nArguments)
{
	name = nName;
	arguments = nArguments;
}

string Goal::getName()
{
	return name;
}

