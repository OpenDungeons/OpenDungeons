#ifndef GOAL_H
#define GOAL_H

#include <string>
using namespace std;

class Goal
{
	public:
		Goal(string nName, string nArguments);

		virtual bool isMet() = 0;
		virtual bool isVisible() = 0;
		virtual string getSuccessMessage() = 0;
		virtual string getDescription() = 0;
		string getName();

	private:
		string arguments;
		string name;
};

#endif

