/*!
 * \file   GameEntity.h
 * \date:  16 September 2011
 * \author StefanP.MUC
 * \brief  Provides the GameEntity class, the base class for all ingame objects
 */

#ifndef GAMEENTITY_H_
#define GAMEENTITY_H_

#include <string>

#include <semaphore.h>

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
        //! \brief Default constructor with default values
        GameEntity( std::string     nName       = "",
                    std::string     nMeshName   = "",
                    int             nColor      = 0,
                    unsigned int    nLevel      = 1
                  ) :
            name        (nName),
            meshName    (nMeshName),
            meshExists  (false),
            color       (nColor),
            level       (nLevel)
        {
            //TODO: find out why this crashes, or replace whole thread system with boost thread
            //sem_init(&nameLockSemaphore, 1, 1);
        }
        virtual ~GameEntity(){}

        // ===== GETTERS =====
        //! \brief Get the name of the object
        inline const std::string&   getName         () const    { return name; }
        //inline const std::string&   getName         () const    { sem_wait(&nameLockSemaphore); const std::string& tempName = name; sem_post(&nameLockSemaphore); return tempName; }

        //! \brief Get the mesh name of the object
        inline const std::string&   getMeshName     () const    { return meshName; }

        //! \brief Get the id of the color that the objects belongs to
        inline int                  getColor        () const    { return color; }

        //! \brief Get the level of the object
        //inline unsigned int         getLevel        () const    { return level; }

        //! \brief Get if the mesh is already existing
        inline bool                 isMeshExisting  () const    { return meshExists; }

        // ===== SETTERS =====
        //! \brief Set the name of the entity
        inline void setName         (const std::string& nName)      { name = nName; }
        //inline void setName         (const std::string& nName)      { sem_wait(&nameLockSemaphore); name = nName; sem_post(&nameLockSemaphore); }

        //! \brief Set the name of the mesh file
        inline void setMeshName     (const std::string& nMeshName)  { meshName = nMeshName; }

        //! \brief Set if the mesh exists
        inline void setColor        (int nColor)                    { color = nColor; }

        //! \brief Set if the mesh exists
        //inline void setLevel        (unsigned int nLevel)           { level = nLevel; }

        //! \brief Set if the mesh exists
        inline void setMeshExisting (bool isExisting)               { meshExists = isExisting; }

        // ===== METHODS =====
        //! \brief Pure virtual function that implements the mesh creation
        virtual void    createMesh      () = 0;

        //! \brief Pure virtual function that implements the mesh deletion
        virtual void    destroyMesh     () = 0;

        //! \brief Pure virtual function that implements code for the removal from the map
        virtual void    deleteYourself  () = 0;

    private:
        //! brief The name of the entity
        std::string     name;

        //! \brief The name of the mesh
        std::string     meshName;

        //! \brief Stores the existence state of the mesh
        bool            meshExists;

        //! \brief The id of the color that the objcts belongs to
        int             color;

        //! \brief The level of the object
        unsigned int    level;

        //mutable sem_t   nameLockSemaphore;
};

#endif /* GAMEENTITY_H_ */

