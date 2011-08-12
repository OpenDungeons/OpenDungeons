#ifndef WEAPON_H
#define WEAPON_H

#include <string>
#include <istream>
#include <ostream>

class Creature;

class Weapon
{
    public:
        Weapon();

        std::string name, meshName;
        double damage, range, defense;
        Creature *parentCreature;
        std::string handString;

        void createMesh();
        void destroyMesh();
        void deleteYourself();

        static std::string getFormat();
        friend std::ostream& operator<<(std::ostream& os, Weapon *w);
        friend std::istream& operator>>(std::istream& is, Weapon *w);

    private:
        bool meshExists;
};

#endif

