#ifndef TRAP_H
#define TRAP_H

#include <string>
#include <vector>
#include <istream>
#include <ostream>
#include <map>

class Tile;
class Seat;
class GameMap;
class Player;

#include "ActiveEntity.h"
#include "AttackableEntity.h"
#include "GameEntity.h"

/** \brief Defines a trap
 *  
 */
class Trap: public GameEntity, public AttackableEntity, public ActiveEntity
{
 /* TODO: Trap and room share a lot of things, so we might want to make a shared
 *  base-class, like "Building" or something.
 */
    public:
        enum TrapType
        {
            nullTrapType = 0, cannon, boulder
        };

        Trap();
        virtual ~Trap() {}

        static Trap
        * createTrap(TrapType nType, const std::vector<Tile*> &nCoveredTiles,
                Seat *nControllingSeat, void* params = NULL);
        static Trap* buildTrap(GameMap* gameMap, Trap::TrapType nType,
                               const std::vector< Tile* >& coveredTiles,
                               Player* player, bool inEditor = false, void* params = NULL);
        static Trap* createTrapFromStream(std::istream &is, GameMap* gameMap);
        //virtual void absorbTrap(Trap *t);

        void createMesh();
        void destroyMesh();
        void deleteYourself();

        inline const TrapType& getType() const{return type;}
        static std::string getMeshNameFromTrapType(TrapType t);
        static TrapType getTrapTypeFromMeshName(std::string s);

        static int costPerTile(TrapType t);

        inline const std::string& getMeshName() const{return meshName;}

        Seat *controllingSeat;

        // Functions which can be overridden by child classes.
        virtual bool doUpkeep();
        virtual bool doUpkeep(Trap *t);
        
        virtual std::vector<AttackableEntity*> aimEnemy();
        virtual void damage(std::vector<AttackableEntity*>);

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
        double getDefense() const;
        void takeDamage(double damage, Tile *tileTakingDamage);
        void recieveExp(double experience);
        bool isMobile() const;
        int getLevel() const;
        AttackableEntity::AttackableObjectType getAttackableObjectType() const;

    protected:
        int reloadTime;
        int reloadTimeCounter;
        double minDamage, maxDamage;
        const static double defaultTileHP;// = 10.0;

        std::string meshName;
        std::vector<Tile*> coveredTiles;
        std::map<Tile*, double> tileHP;
        TrapType type;
        double exp;
};

#endif

