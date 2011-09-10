#include "ServerNotification.h"

sem_t ServerNotification::serverNotificationQueueLockSemaphore;
sem_t ServerNotification::serverNotificationQueueSemaphore;
std::deque<ServerNotification*> ServerNotification::serverNotificationQueue;
