#include "ServerNotification.h"

sem_t ServerNotification::mServerNotificationQueueLockSemaphore;
sem_t ServerNotification::mServerNotificationQueueSemaphore;
std::deque<ServerNotification*> ServerNotification::serverNotificationQueue;
