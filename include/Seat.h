#ifndef SEAT_H
#define SEAT_H

#include <iostream>
#include <vector>

#include <Ogre.h>
#include <semaphore.h>

#include "Goal.h"

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

		// Public data members
		int color;		/**< \brief The color index of the players sitting in this seat. */
		string faction;		/**< \brief The name of the faction that this seat is playing as. */
		int startingX;		/**< \brief The starting camera location (in tile coordinates) of this seat. */
		int startingY;		/**< \brief The starting camera location (in tile coordinates) of this seat. */
		Ogre::ColourValue colourValue;		/**< \brief The actual color that this color index translates into. */
		double mana;		/**< \brief The amount of 'keeper mana' the player has. */
		double manaDelta;	/**< \brief The amount of 'keeper mana' the player gains/loses per turn, updated in GameMap::doTurn(). */
		double hp;		/**< \brief The amount of 'keeper HP' the player has. */
		int gold;		/**< \brief The total amount of gold coins in the keeper's treasury and in the dungeon heart. */
		int goldMined;		/**< \brief The total amount of gold coins mined by workers under this seat's control. */
		unsigned int numClaimedTiles;	/**< \brief How many tiles have been claimed by this seat, updated in GameMap::doTurn(). */
		int numCreaturesControlled;
		double factionHumans;
		double factionCorpars;
		double factionUndead;
		double factionConstructs;
		double factionDenizens;
		double alignmentAltruism;
		double alignmentOrder;
		double alignmentPeace;

		static string getFormat();
		friend ostream& operator<<(ostream& os, Seat *s);
		friend istream& operator>>(istream& is, Seat *s);

	private:
		std::vector<Goal*> goals;		/**< \brief The currently unmet goals for this seat, the first Seat to empty this wins. */
		sem_t goalsLockSemaphore;
		std::vector<Goal*> completedGoals;	/**< \brief The met goals for this seat. */
		sem_t completedGoalsLockSemaphore;
		std::vector<Goal*> failedGoals;	/**< \brief The unmet goals for this seat which cannot possibly be met in the future. */
		sem_t failedGoalsLockSemaphore;
};

#endif

