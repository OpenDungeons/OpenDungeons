#ifndef TRAP_H
#define TRAP_H

#include <vector>
#include <string>

#include "Tile.h"
#include "Seat.h"
#include "AttackableObject.h"

class Trap : public AttackableObject
{
	public:
		enum TrapType {nullTrapType = 0, cannon};

		Trap();
		static Trap* createTrap(TrapType nType, const std::vector<Tile*> &nCoveredTiles);
		//static Trap* createTrapFromStream(istream &is);
		//virtual void absorbTrap(Trap *t);

		//void createMeshes();
		//void destroyMeshes();
		//void deleteYourself();

		TrapType getType();
		static string getMeshNameFromTrapType(TrapType t);
		static TrapType getTrapTypeFromMeshName(string s);

		static int costPerTile(TrapType t);

		Seat *controllingSeat;
		string name, meshName;

		// Functions which can be overridden by child classes.
		virtual void doUpkeep(Trap *r);
		
		virtual void addCoveredTile(Tile* t, double nHP = defaultTileHP);
		virtual void removeCoveredTile(Tile* t);
		virtual Tile* getCoveredTile(int index);
		std::vector<Tile*> getCoveredTiles();
		virtual unsigned int numCoveredTiles();
		virtual void clearCoveredTiles();

	protected:
		const static double defaultTileHP = 10.0;

		std::vector<Tile*> coveredTiles;
		std::map<Tile*,double> tileHP;
		TrapType type;
		bool meshExists;
};

#include "TrapCannon.h"

#endif

