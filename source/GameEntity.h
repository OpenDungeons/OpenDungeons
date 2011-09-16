/*!
 * \file   GameEntity.h
 * \date:  16 September 2011
 * \author StefanP.MUC
 * \brief  Provides the GameEntity class, the base class for all ingame objects
 */

#ifndef GAMEENTITY_H_
#define GAMEENTITY_H_

#include <string>

/*! \brief This class holds elements that are common to every object placed in the game
 *
 */
class GameEntity
{
    public:
        //! \brief Default constructor with default values
        GameEntity( std::string nName       = "",
                    std::string nMeshName   = "",
                    int         nColor      = 0
                  ) :
            name        (nName),
            meshName    (nMeshName),
            color       (nColor)
        {}

        //! \brief  Get the name of the object
        const std::string&  getName     () const    { return name; }

        //! \brief  Get the mesh name of the object
        const std::string&  getMeshName () const    { return meshName; }

        //! \brief Get the id of the color that the objects belongs to
        int                 getColor    () const    { return color; }

        //! \brief Pure virtual function that implments the mesh creation
        virtual void createMesh() = 0;

        //! \brief Pure virtual function that implments the mesh deletion
        virtual void deleteMesh() = 0;

        //! \brief Pure virtual function that implements code for the removal from the map
        virtual void deleteYourself() = 0;

    private:
        //! brief The name of the entity
        std::string name;

        //! \brief The name of the mesh
        std::string meshName;

        //! \brief The id of the color that the objcts belongs to
        int         color;
};

#endif /* GAMEENTITY_H_ */
