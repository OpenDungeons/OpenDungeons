/*
 *  Copyright (C) 2011-2014  OpenDungeons Team
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SERVERNOTIFICATION_H
#define SERVERNOTIFICATION_H

#include <deque>
#include <string>
#include <OgreVector3.h>

#ifdef __MINGW32__
#ifndef mode_t
#include <sys/types.h>
#endif //mode_t
#endif //mingw32

class Tile;
class Creature;
class MovableGameEntity;
class Player;

//! \brief A data structure used to pass messages to the serverNotificationProcessor thread.
//TODO:  Make this class a base class and let specific messages be subclasses of this type with each having its own data structure so they don't need the unused fields
class ServerNotification
{
    public:
        enum ServerNotificationType
        {
            turnStarted,
            setTurnsPerSecond,

            tileFullnessChange,

            addMapLight,
            removeMapLight,

            animatedObjectAddDestination,
            animatedObjectClearDestinations,
            setObjectAnimationState,
            creaturePickUp,
            creatureDrop,

            exit
        };

        //TODO:  Employ some void pointers on this to make this data structure smaller
        ServerNotificationType type;
        void *p;
        bool b;
        std::string str;
        Ogre::Vector3 vec;
        double doub;
        Tile *tile;
        Creature *cre;
        MovableGameEntity *ani;
        Player *player;
        long int turnNumber;

        static std::deque<ServerNotification*> serverNotificationQueue;
};

#endif // SERVERNOTIFICATION_H
