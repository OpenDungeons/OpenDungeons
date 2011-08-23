#ifndef GOAKILLALLENEMIES_H
#define GOAKILLALLENEMIES_H

#include "Goal.h"

class GoalKillAllEnemies: public Goal
{
    public:
        GoalKillAllEnemies(const std::string& nName,
                const std::string& nArguments, const GameMap& gameMap);

        // Inherited functions
        bool isMet(Seat *s);
        std::string getDescription();
        std::string getSuccessMessage();
        std::string getFailedMessage();
};

#endif

