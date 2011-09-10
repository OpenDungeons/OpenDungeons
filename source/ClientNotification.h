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
        enum ClientNotificationType
        {
            creaturePickUp, creatureDrop, markTile, exit
        };

        ClientNotificationType type;

        void *p;
        void *p2;
        bool flag;

        static std::deque<ClientNotification*> clientNotificationQueue;
        static sem_t clientNotificationQueueSemaphore;
        static sem_t clientNotificationQueueLockSemaphore;
};

#endif
