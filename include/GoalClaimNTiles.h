#ifndef GOALCLAIMNTILES_H
#define GOALCLAIMNTILES_H

#include "Goal.h"

class GoalClaimNTiles: public Goal
{
    public:
                GoalClaimNTiles(const std::string& nName,
                        const std::string& nArguments);

        // Inherited functions
        bool isMet(Seat *s);
        string getDescription();
        string getSuccessMessage();
        string getFailedMessage();

    private:
        unsigned int numberOfTiles;
};

#endif

