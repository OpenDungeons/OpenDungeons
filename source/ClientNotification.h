/*!
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

#ifndef CLIENTNOTIFICATION_H
#define CLIENTNOTIFICATION_H

#include <deque>

#ifdef __MINGW32__
#ifndef mode_t
#include <sys/types.h>
#endif //mode_t
#endif //mingw32
#include <semaphore.h>

/*! \brief A data structure used to pass messages to the clientNotificationProcessor thread.
 *
 */
//TODO:  Make this class a base class and let specific messages be subclasses of this type with each having its own data structure so they don't need the unused fields
class ClientNotification
{
public:
    ClientNotification():
        mType(invalidType),
        mP(0),
        mP2(0),
        mFlag(false)
    {}

    enum ClientNotificationType
    {
        invalidType, creaturePickUp, creatureDrop, markTile, exit
    };

    ClientNotificationType mType;

    void *mP;
    void *mP2;
    bool mFlag;

    static std::deque<ClientNotification*> mClientNotificationQueue;
    static sem_t mClientNotificationQueueSemaphore;
    static sem_t mClientNotificationQueueLockSemaphore;
};

#endif // CLIENTNOTIFICATION_H
