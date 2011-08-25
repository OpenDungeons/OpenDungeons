#ifndef SEAT_H
#define SEAT_H

#include <OgreColourValue.h>
#include <semaphore.h>
#include <string>
#include <vector>
#include <ostream>
#include <istream>

class Goal;

class Seat
{
    public:
        // Constructors
        Seat();

        // Public functions
        void addGoal(Goal *g);
        unsigned int numGoals();
        Goal* getGoal(unsigned int index);
        void clearGoals();
        void clearCompletedGoals();

        unsigned int checkAllGoals();
        unsigned int checkAllCompletedGoals();

        unsigned int numCompletedGoals();
        Goal* getCompletedGoal(unsigned int index);

        unsigned int numFailedGoals();
        Goal* getFailedGoal(unsigned int index);

        unsigned int getNumClaimedTiles();
        void setNumClaimedTiles(const unsigned int& num);
        inline const unsigned int& rawGetNumClaimedTiles() const{return numClaimedTiles;}
        void rawSetNumClaimedTiles(const unsigned int& num){numClaimedTiles = num;}
        void incrementNumClaimedTiles();
        bool getHasGoalsChanged();
        void resetGoalsChanged();
        inline int getColor() const {return color;}
        inline int getGold() const {return gold;}
        inline int getMana() const {return mana;}
        inline int getManaDelta() const {return manaDelta;}
        

        // Public data members
        int color; /**< \brief The color index of the players sitting in this seat. */
        std::string faction; /**< \brief The name of the faction that this seat is playing as. */
        int startingX; /**< \brief The starting camera location (in tile coordinates) of this seat. */
        int startingY; /**< \brief The starting camera location (in tile coordinates) of this seat. */
        Ogre::ColourValue colourValue; /**< \brief The actual color that this color index translates into. */
        double mana; /**< \brief The amount of 'keeper mana' the player has. */
        double manaDelta; /**< \brief The amount of 'keeper mana' the player gains/loses per turn, updated in GameMap::doTurn(). */
        double hp; /**< \brief The amount of 'keeper HP' the player has. */
        int gold; /**< \brief The total amount of gold coins in the keeper's treasury and in the dungeon heart. */
        int goldMined; /**< \brief The total amount of gold coins mined by workers under this seat's control. */
        int numCreaturesControlled;
        double factionHumans;
        double factionCorpars;
        double factionUndead;
        double factionConstructs;
        double factionDenizens;
        double alignmentAltruism;
        double alignmentOrder;
        double alignmentPeace;

        

        static std::string getFormat();
        friend std::ostream& operator<<(std::ostream& os, Seat *s);
        friend std::istream& operator>>(std::istream& is, Seat *s);

    private:
        void goalsHasChanged();
        
        std::vector<Goal*> goals; /**< \brief The currently unmet goals for this seat, the first Seat to empty this wins. */
        sem_t goalsLockSemaphore;

        std::vector<Goal*> completedGoals; /**< \brief The met goals for this seat. */
        sem_t completedGoalsLockSemaphore;

        std::vector<Goal*> failedGoals; /**< \brief The unmet goals for this seat which cannot possibly be met in the future. */
        sem_t failedGoalsLockSemaphore;

        unsigned int numClaimedTiles; /**< \brief How many tiles have been claimed by this seat, updated in GameMap::doTurn(). */

        bool hasGoalsChanged;

        mutable sem_t numClaimedTilesLockSemaphore;
};

#endif

