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

#include "network/ODPacket.h"

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
            loadLevel, // Tells the client to load the level: + string LevelFilename
            pickNick,
            yourSeat,
            addPlayer,
            newMap,
            turnsPerSecond,
            addTile,
            addMapLight,
            removeMapLight,
            addClass,
            clientAccepted,

            chat,
            chatServer,

            markTiles,
            buildRoom,
            removeRoomTile,
            buildTrap,
            removeTrapTile,
            turnStarted,
            setTurnsPerSecond,

            tileFullnessChange,

            animatedObjectAddDestination,
            animatedObjectClearDestinations,
            setObjectAnimationState,
            setMoveSpeed,
            entityPickedUp,
            entityDropped,

            playerFighting, // Tells the player he is under attack or attacking
            playerNoMoreFighting, // Tells the player he is no longer under attack or attacking

            addCreature,
            removeCreature,
            creatureRefresh,
            tileClaimed,
            refreshPlayerSeat,
            addRenderedMovableEntity,
            removeRenderedMovableEntity,
            notifyCreatureInfo,

            playSpatialSound, // Makes the client play a sound at tile coordinates.
            playCreatureSound, // Play a sound at the creature position

            refreshTiles,

            exit
        };

        ServerNotification(ServerNotificationType type,
            Player* concernedPlayer);
        virtual ~ServerNotification()
        {}

        ODPacket mPacket;

        static std::string typeString(ServerNotificationType type);
        friend ODPacket& operator<<(ODPacket& os, const ServerNotification::ServerNotificationType& nt);
        friend ODPacket& operator>>(ODPacket& is, ServerNotification::ServerNotificationType& nt);

    private:
        ServerNotificationType mType;
        Player *mConcernedPlayer;
};

#endif // SERVERNOTIFICATION_H
