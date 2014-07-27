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

#include "ODPacket.h"

#include <deque>
#include <string>
#include <OgreVector3.h>

class Tile;
class Creature;
class MovableGameEntity;
class Player;

//! \brief A data structure used to pass messages to the serverNotificationProcessor thread.
//TODO:  Make this class a base class and let specific messages be subclasses of this type with each having its own data structure so they don't need the unused fields
class ServerNotification
{
    friend class ODServer;

    public:
        enum ServerNotificationType
        {
            // Negociation for multiplayer
            pickNick,
            yourSeat,
            addPlayer,
            newMap,
            turnsPerSecond,
            addTile,
            addMapLight,
            removeMapLight,
            addClass,

            chat,

            markTiles,
            buildRoom,
            buildTrap,
            turnStarted,
            setTurnsPerSecond,

            tileFullnessChange,

            animatedObjectAddDestination,
            animatedObjectClearDestinations,
            setObjectAnimationState,
            creaturePickedUp,
            creatureDropped,

            addCreature,
            tileClaimed,
            refreshPlayerSeat,
            addCreatureBed,
            removeCreatureBed,

            createTreasuryIndicator,
            destroyTreasuryIndicator,

            exit
        };

        ServerNotification(ServerNotificationType type,
            Player* concernedPlayer);
        virtual ~ServerNotification()
        {}

        ODPacket packet;

        static std::string typeString(ServerNotificationType type);

    private:
        ServerNotificationType mType;
        Player *mConcernedPlayer;
};

#endif // SERVERNOTIFICATION_H
