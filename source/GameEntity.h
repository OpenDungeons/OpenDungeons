/*!
 * \file   GameEntity.h
 * \date:  16 September 2011
 * \author StefanP.MUC
 * \brief  Provides the GameEntity class, the base class for all ingame objects
 */

/* TODO list:
 * - GameMap pointer should not be part of every object, since there is only one GameMap, at least let it set by ctor
 * - complete the constructor
 * - add semaphores if/where needed
 * - default mesh handle code instead of pure virtuals that do almost the same in every subclass
 * - static removeDeadObjects should not be in here (maybe in GameMap?)
 */

#ifndef GAMEENTITY_H_
#define GAMEENTITY_H_

#include <cassert>
#include <string>
#include <vector>

class GameMap;
class Tile;

/*! \class GameEntity GameEntity.h
 *  \brief This class holds elements that are common to every object placed in the game
 *
 * Functions and properties that are common to every object should be placed into this class
 * and initialised with a good default value in the default constructor.
 * Member variables are private and only accessed through getters and setters.
 */
class GameEntity
{
    public:
        enum ObjectType
        {
            unknown, creature, room, trap, weapon, roomobject, missileobject, tile
        };

        //! \brief Default constructor with default values
        GameEntity( std::string     nName       = "",
                    std::string     nMeshName   = "",
                    int             nColor      = 0
                  ) :
            name        (nName),
            meshName    (nMeshName),
            meshExists  (false),
            color       (nColor),
            active      (true),
            attackable  (true),
            objectType  (unknown),
            gameMap     (0)
        { }
        virtual ~GameEntity(){}

        // ===== GETTERS =====
        //! \brief Get the name of the object
        inline const std::string&   getName         () const    { return name; }

        //! \brief Get the mesh name of the object
        inline const std::string&   getMeshName     () const    { return meshName; }

        //! \brief Get the id of the color that the objects belongs to
        inline int                  getColor        () const    { return color; }

        //! \brief Get if the mesh is already existing
        inline bool                 isMeshExisting  () const    { return meshExists; }

        //! \brief Get if the object is active (doing sth. on its own) or not
        inline bool                 isActive        () const    { return active; }

        //! \brief Get if the object can be attacked or not
        inline bool                 isAttackable    () const    { return attackable; }

        //! \brief Get the type of this object
        inline ObjectType           getObjectType   () const    { return objectType; }

        //! \brief Pointer to the GameMap
        inline GameMap*             getGameMap      () const    { return gameMap; }

        // ===== SETTERS =====
        //! \brief Set the name of the entity
        inline void setName         (const std::string& nName)      { name = nName; }

        //! \brief Set the name of the mesh file
        inline void setMeshName     (const std::string& nMeshName)  { meshName = nMeshName; }

        //! \brief Set if the mesh exists
        inline void setColor        (int nColor)                    { color = nColor; }

        //! \brief Set if the mesh exists
        inline void setMeshExisting (bool isExisting)               { meshExists = isExisting; }

        //! \brief Set the type of the object. Should be done in all final derived constructors
        inline void setObjectType   (ObjectType nType)              { objectType = nType; }

        //! \brief set a pointer the MameMap
        inline virtual void         setGameMap                      (GameMap* gameMap)
        {
            this->gameMap = gameMap;
            assert(gameMap != 0);
        }

        // ===== METHODS =====
        //! \brief Function that implements the mesh creation
        virtual void    createMesh      ();

        //! \brief Function that implements the mesh deletion
        virtual void    destroyMesh     ();

        //! \brief Function that implements code for the removal from the map
        virtual void    deleteYourself  ();

        //! \brief defines what happens on each turn with this object
        virtual bool    doUpkeep        () = 0;

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

        static std::vector<GameEntity*> removeDeadObjects(const std::vector<GameEntity*> &objects)
        {
            std::vector<GameEntity*> ret;
            for(unsigned int i = 0, size = objects.size(); i < size; ++i)
            {
                if (objects[i]->getHP(NULL) > 0.0)
                    ret.push_back(objects[i]);
            }

            return ret;
        }

    private:
        //! brief The name of the entity
        std::string     name;

        //! \brief The name of the mesh
        std::string     meshName;

        //! \brief Stores the existence state of the mesh
        bool            meshExists;

        //! \brief The id of the color that the objcts belongs to
        int             color;

        //! \brief A flag saying whether the object is active (doing something on its own) or not
        bool            active;

        //! \brief A flag saying whether the object can be attacked or not
        bool            attackable;

        //! \brief What kind of object is it
        ObjectType      objectType;

        //! \brief Pointer to the GameMap object.
        GameMap*        gameMap;
};

#endif /* GAMEENTITY_H_ */

