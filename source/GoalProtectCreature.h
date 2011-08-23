#ifndef GOALPROTECTCREATURE_H
#define GOALPROTECTCREATURE_H

#include <string>

#include "Goal.h"

class GoalProtectCreature: public Goal
{
    public:
        GoalProtectCreature(const std::string& nName,
                const std::string& nArguments, const GameMap& gameMap);

        // Inherited functions
        bool isMet(Seat *s);
        bool isUnmet(Seat *s);
        bool isFailed(Seat *s);
        std::string getDescription();
        std::string getSuccessMessage();
        std::string getFailedMessage();

    private:
        std::string creatureName;
};

#endif

