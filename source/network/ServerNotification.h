/*
 *  Copyright (C) 2011-2015  OpenDungeons Team
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

enum class ServerNotificationType
{
    // Negociation for multiplayer
    loadLevel, // Tells the client to load the level: + string LevelFilename
    pickNick,
    addPlayers,
    removePlayers,
    startGameMode,
    newMap,
    addClass,
    clientAccepted,
    clientRejected,
    seatConfigurationRefresh,

    chat,
    chatServer,

    turnStarted,

    animatedObjectAddDestination,
    animatedObjectClearDestinations,
    setObjectAnimationState,
    setMoveSpeed,
    entityPickedUp,
    entityDropped,
    entitySlapped,

    playerFighting, // Tells the player he is under attack or attacking
    playerNoMoreFighting, // Tells the player he is no longer under attack or attacking

    addEntity,
    removeEntity,
    creatureRefresh,
    refreshPlayerSeat,
    setEntityOpacity,
    notifyCreatureInfo,
    refreshCreatureVisDebug,

    refreshSeatVisDebug,

    playSpatialSound, // Makes the client play a sound at tile coordinates.
    playCreatureSound, // Play a creature sound at the given position

    refreshTiles,
    refreshVisibleTiles,
    carryEntity,
    releaseCarriedEntity,

    exit
};

ODPacket& operator<<(ODPacket& os, const ServerNotificationType& nt);
ODPacket& operator>>(ODPacket& is, ServerNotificationType& nt);

//! \brief A data structure used to send messages to the clients
class ServerNotification
{
    friend class ODServer;

    public:
        /*! \brief Creates a message to be sent to concernedPlayer. If concernedPlayer is null, the message will be sent to
         *         every connected player.
         */
        ServerNotification(ServerNotificationType type, Player* concernedPlayer);
        virtual ~ServerNotification()
        {}

        ODPacket mPacket;

        static std::string typeString(ServerNotificationType type);

    private:
        ServerNotificationType mType;
        Player *mConcernedPlayer;
};

#endif // SERVERNOTIFICATION_H
