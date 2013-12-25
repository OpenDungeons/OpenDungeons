/*!
 * \file   Building.h
 * \date:  22 March 2011
 * \author StefanP.MUC
 * \brief  Provides common methods and members for buildable objects, like rooms and traps
 */

#ifndef BUILDING_H_
#define BUILDING_H_

#include "GameEntity.h"
#include "Seat.h"


/*! \class GameEntity GameEntity.h
 *  \brief This class holds elements that are common to every object placed in the game
 *
 * Functions and properties that are common to every buildable object like rooms and traps
 * should be placed into this classand initialised with a good default value in the default
 * constructor. Member variables are private and only accessed through getters and setters.
 */
class Building : public GameEntity
{
    public:
        //! \brief Default constructor with default values
        Building() :
            controllingSeat (0)
        { }

        virtual ~Building(){}

        // ===== GETTERS =====
        inline Seat*    getControllingSeat ()   const   { return controllingSeat; }

        // ===== SETTERS =====
        inline void setControllingSeat  (Seat* nCS) { controllingSeat = nCS; setColor(nCS->getColor()); }
    private:
        //! \brief The Seat controlling this building
        Seat*                   controllingSeat;
};

#endif /* BUILDING_H_ */
