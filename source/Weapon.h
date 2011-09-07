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
        Weapon(const std::string& name, const double& damage, const double& range,
                const double& defense, Creature* parent, const std::string& handString);

        void createMesh();
        void destroyMesh();
        void deleteYourself();

        static std::string getFormat();
        friend std::ostream& operator<<(std::ostream& os, Weapon *w);
        friend std::istream& operator>>(std::istream& is, Weapon *w);

        inline const double&   getDamage() const                   { return damage; }
        inline void            setDamage(const double& nDamage)    { damage = nDamage; }

        inline const double&   getDefense() const                  { return defense; }
        inline void            setDefense(const double& nDefense)  { defense = nDefense; }

        inline const std::string&  getName() const                 { return name; }
        inline void                setName(std::string nName)      { name = nName; }

        inline const std::string&  getHandString() const           { return handString; }
        inline void                setHandString(std::string nHandString)
                                                                   { handString = nHandString; }

        inline const std::string&  getMeshName() const             { return meshName; }
        inline void                setMeshName(std::string nMeshName)
                                                                   { meshName = nMeshName; }

        inline Creature*       getParentCreature() const           { return parentCreature; }
        inline void            setParentCreature(Creature* nParent){ parentCreature = nParent; }

        inline const double&   getRange() const                    { return range; }
        inline void            setRange(const double& nRange)      { range = nRange; }

    private:
        std::string name;
        std::string meshName;
        std::string handString;
        double      damage;
        double      range;
        double      defense;
        Creature*   parentCreature;
        bool        meshExists;
};

#endif

