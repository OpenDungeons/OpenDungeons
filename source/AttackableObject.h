#ifndef ATTACKABLEOBJECT_H
#define ATTACKABLEOBJECT_H

#include <vector>
#include <string>
#include <cassert>

class GameMap;
class Tile;

/*! \brief An abstract class representing an interface for creatures, traps, doors, etc to present to the AI subsystem for combat calculations.
 *
 * Because this class is abstract it cannot be instantiated directly.  Instead,
 * other classes like Creature should subclass this class and implement all of
 * the pure virtual functions this class provides.  This forces all entities in
 * the game which are destroyable to present a common interface to AI
 * calculation routines, allowing the AI code to be written once and allow it to
 * handle multiple types of objects in combat (i.e. creatures attacking rooms
 * and traps attacking creatures).
 */
class AttackableObject
{
    public:
        enum AttackableObjectType
        {
            creature, room, trap, door
        };

        AttackableObject();

        inline virtual void setGameMap(GameMap* gameMap)
        {
            this->gameMap = gameMap;
            assert(gameMap != NULL);
        }

        //! \brief Returns a list of the tiles that this object is in/covering.  For creatures and other small objects
        //! this will be a single tile, for larger objects like rooms this will be 1 or more tiles.
        virtual std::vector<Tile*> getCoveredTiles() = 0;

        //! \brief Returns the HP associated with the given tile of the object, it is up to the object how they want to treat the tile/HP relationship.
        virtual double getHP(Tile *tile) = 0;

        //! \brief Returns defense rating for the object, i.e. how much less than inflicted damage should it recieve.
        virtual double getDefense() const = 0;

        //! \brief Subtracts the given number of hitpoints from the object, the tile specifies where
        //! the enemy inflicted the damage and the object can use this accordingly.
        virtual void takeDamage(double damage, Tile *tileTakingDamage) = 0;

        //! \brief Adds the given number experience points to the object, does not necessarily check to see if the object's level should be increased.
        virtual void recieveExp(double experience) = 0;

        //! \brief Returns whether or not the object is capable of moving including.  AI calculations can use this to optimize pathfinding, etc.
        virtual bool isMobile() const = 0;

        //! \brief Returns the current level that the object has attained based on its accumulated experience points, mostly used for creatures/traps.
        virtual int getLevel() const = 0;

        //! \brief Returns the color of the seat which controls the given object, used for determining whether a unit is an ally or enemy.
        virtual int getColor() const = 0;

        //! \brief Returns the name of the object.
        virtual const std::string& getName() const = 0;

        //! \brief Returns the type of the object, i.e. creature, room, trap, etc, for AI calculations to use in threat assessments.
        virtual AttackableObjectType getAttackableObjectType() const = 0;

        static std::vector<AttackableObject*> removeDeadObjects(
                const std::vector<AttackableObject*> &objects);
    protected:
        //TODO: We should change this to be set by the constructor.
        GameMap* gameMap;
};

#endif

