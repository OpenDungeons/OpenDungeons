#ifndef TRAP_H
#define TRAP_H

#include <string>
#include <vector>
#include <istream>
#include <ostream>
#include <map>

class Tile;
class Seat;

#include "AttackableObject.h"
#include "ActiveObject.h"

class Trap: public AttackableObject, public ActiveObject
{
    public:
        enum TrapType
        {
            nullTrapType = 0, cannon
        };

        Trap();
        static Trap
        * createTrap(TrapType nType, const std::vector<Tile*> &nCoveredTiles,
                Seat *nControllingSeat);
        static Trap* createTrapFromStream(std::istream &is);
        //virtual void absorbTrap(Trap *t);

        void createMeshes();
        void destroyMeshes();
        void deleteYourself();

        TrapType getType();
        static std::string getMeshNameFromTrapType(TrapType t);
        static TrapType getTrapTypeFromMeshName(std::string s);

        static int costPerTile(TrapType t);

        std::string getName();
        std::string getMeshName();

        Seat *controllingSeat;

        // Functions which can be overridden by child classes.
        virtual bool doUpkeep();
        virtual bool doUpkeep(Trap *t);

        virtual void addCoveredTile(Tile* t, double nHP = defaultTileHP);
        virtual void removeCoveredTile(Tile* t);
        virtual Tile* getCoveredTile(int index);
        std::vector<Tile*> getCoveredTiles();virtual unsigned int numCoveredTiles();
        virtual void clearCoveredTiles();

        static std::string getFormat();
        friend std::istream& operator>>(std::istream& is, Trap *t);
        friend std::ostream& operator<<(std::ostream& os, Trap *t);

        // Methods inherited from AttackableObject.
        //TODO:  Sort these into the proper places in the rest of the file.
        double getHP(Tile *tile);
        double getDefense();
        void takeDamage(double damage, Tile *tileTakingDamage);
        void recieveExp(double experience);
        bool isMobile();
        int getLevel();
        int getColor();
        AttackableObject::AttackableObjectType getAttackableObjectType();

    protected:
        const static double defaultTileHP;// = 10.0;

        std::string name, meshName;
        std::vector<Tile*> coveredTiles;
        std::map<Tile*, double> tileHP;
        TrapType type;
        bool meshExists;
        double exp;
};

#include "TrapCannon.h"

#endif

