#ifndef TRAPCANNON_H
#define TRAPCANNON_H

#include <vector>
#include <string>

#include "Trap.h"
#include "Tile.h"
#include "AttackableObject.h"

class TrapCannon : public Trap
{
	public:
		TrapCannon();

		void addCoveredTile(Tile* t, double nHP = defaultTileHP);
		void removeCoveredTile(Tile* t);
		Tile* getCoveredTile(int index);
		std::vector<Tile*> getCoveredTiles();
		unsigned int numCoveredTiles();
		void clearCoveredTiles();

		// Methods inherited from AttackableObject.
		//TODO:  Sort these into the proper places in the rest of the file.
		double getHP(Tile *tile);
		double getDefense();
		void takeDamage(double damage, Tile *tileTakingDamage);
		void recieveExp(double experience);
		bool isMobile();
		int getLevel();
		int getColor();
		string getName();
		AttackableObject::AttackableObjectType getAttackableObjectType();
};

#endif

