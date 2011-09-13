#ifndef GOALCLAIMNTILES_H
#define GOALCLAIMNTILES_H

#include "Goal.h"

class GoalClaimNTiles: public Goal
{
    public:
        GoalClaimNTiles(const std::string& nName, const std::string& nArguments,
                const GameMap& gameMap);
        virtual ~GoalClaimNTiles() {}

        // Inherited functions
        bool isMet(Seat *s);
        std::string getDescription();
        std::string getSuccessMessage();
        std::string getFailedMessage();

    private:
        unsigned int numberOfTiles;
};

#endif

