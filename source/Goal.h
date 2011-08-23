#ifndef GOAL_H
#define GOAL_H

#include <string>
#include <vector>
#include <ostream>
#include <istream>

class Seat;
class GameMap;

class Goal
{
    public:
        // Constructors
        Goal(const std::string& nName, const std::string& nArguments, const GameMap& gameMap);

        // Functions which must be overridden by child classes
        virtual bool isMet(Seat *s) = 0;
        virtual std::string getDescription() = 0;
        virtual std::string getSuccessMessage() = 0;
        virtual std::string getFailedMessage() = 0;

        // Functions which can be overridden (but do not have to be) by child classes
        virtual void doSuccessAction();
        virtual bool isVisible();
        virtual bool isUnmet(Seat *s);
        virtual bool isFailed(Seat *s);

        // Functions which cannot be overridden by child classes
        const std::string& getName() const{return name;}

        void addSuccessSubGoal(Goal *g);
        unsigned numSuccessSubGoals();
        Goal* getSuccessSubGoal(int index);

        void addFailureSubGoal(Goal *g);
        unsigned numFailureSubGoals();
        Goal* getFailureSubGoal(int index);

        static std::string getFormat();
        friend std::ostream& operator<<(std::ostream& os, Goal *g);
        static Goal* instantiateFromStream(std::istream& is, const GameMap& gameMap);

    protected:
        const GameMap& gameMap;
        std::string name;
        std::string arguments;
        std::vector<Goal*> successSubGoals;
        std::vector<Goal*> failureSubGoals;
};
/*
#include "Seat.h"

#include "GoalKillAllEnemies.h"
#include "GoalProtectCreature.h"
#include "GoalClaimNTiles.h"
#include "GoalMineNGold.h"
#include "GoalProtectDungeonTemple.h"
*/

#endif

