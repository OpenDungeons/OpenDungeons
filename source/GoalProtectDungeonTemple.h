#ifndef GOALPROTECTDUNGEONTEMPLE_H
#define GOALPROTECTDUNGEONTEMPLE_H

#include "Goal.h"

class GoalProtectDungeonTemple: public Goal
{
    public:
        // Constructors
        GoalProtectDungeonTemple(const std::string& nName, const std::string& nArguments,
                const GameMap& gameMap);
        virtual ~GoalProtectDungeonTemple() {}

        // Inherited functions
        bool isMet(Seat *s);
        bool isUnmet(Seat *s);
        bool isFailed(Seat *s);
        std::string getDescription();
        std::string getSuccessMessage();
        std::string getFailedMessage();
};

#endif

