/*!
 * \file   ActiveEntity.h
 * \date
 * \author
 * \brief  Pure virtual base class to make an object active
 */

#ifndef ACTIVEENTITY_H
#define ACTIVEENTITY_H

/*! \class ActiveEntity ActiveEntity.h
 *  \brief Holds everything common to active objects (that do something on their own)
 *
 * Functions and properties that are needed by all active objects should be placed in here.
 * An object is active if it is doing something on it's own (like Rooms and Traps).
 */
class ActiveEntity
{
    public:
        virtual ~ActiveEntity() {}

        //! \brief defines what happens on each turn with this object
        virtual bool doUpkeep() = 0;
};

#endif
