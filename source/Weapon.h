#ifndef WEAPON_H
#define WEAPON_H

#include <string>
#include <istream>
#include <ostream>

#include "GameEntity.h"

class Creature;

class Weapon : public GameEntity
{
    public:
        Weapon( const std::string&  name        = "",
                double              damage      = 0.0,
                double              range       = 0.0,
                double              defense     = 0.0,
                Creature*           parent      = 0,
                const std::string&  handString  = ""
               ) :
            GameEntity      (name, "", 0),
            handString      (handString),
            damage          (damage),
            range           (range),
            defense         (defense),
            parentCreature  (parent)
        {}

        virtual ~Weapon() {};

        void createMesh();

        static std::string getFormat();
        friend std::ostream& operator<<(std::ostream& os, Weapon *w);
        friend std::istream& operator>>(std::istream& is, Weapon *w);

        inline double           getDamage() const                   { return damage; }
        inline void             setDamage(const double nDamage)     { damage = nDamage; }

        inline double           getDefense() const                  { return defense; }
        inline void             setDefense(const double nDefense)   { defense = nDefense; }

        inline const std::string&  getHandString() const           { return handString; }
        inline void                setHandString(const std::string& nHandString)
                                                                   { handString = nHandString; }

        inline Creature*        getParentCreature() const           { return parentCreature; }
        inline void             setParentCreature(Creature* nParent){ parentCreature = nParent; }

        inline double           getRange() const                    { return range; }
        inline void             setRange(const double nRange)       { range = nRange; }

        //TODO: implment these in a good way
        bool doUpkeep(){ return true; }
        void recieveExp(double experience){}
        void takeDamage(double damage, Tile *tileTakingDamage) {}
        double getHP(Tile *tile) {return 0;}
        std::vector<Tile*> getCoveredTiles() { return std::vector<Tile*>() ;}

    private:
        std::string handString;
        double      damage;
        double      range;
        double      defense;
        Creature*   parentCreature;
};

#endif

