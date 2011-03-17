#ifndef GOAKILLALLENEMIES_H
#define GOAKILLALLENEMIES_H

#include "Goal.h"

class GoalKillAllEnemies: public Goal
{
    public:
        GoalKillAllEnemies(std::string nName, std::string nArguments);

        // Inherited functions
        bool isMet(Seat *s);
        std::string getDescription();
        std::string getSuccessMessage();
        std::string getFailedMessage();
};

#endif

