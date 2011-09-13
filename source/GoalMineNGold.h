#ifndef GOALMINENGOLD_H
#define GOALMINENGOLD_H

#include "Goal.h"

class GoalMineNGold: public Goal
{
    public:
        GoalMineNGold(const std::string& nName, const std::string& nArguments,
                const GameMap& gameMap);
        virtual ~GoalMineNGold() {}

        // Inherited functions
        bool isMet(Seat *s);
        std::string getDescription();
        std::string getSuccessMessage();
        std::string getFailedMessage();

    private:
        int goldToMine;
};

#endif

