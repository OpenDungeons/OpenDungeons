#include "ServerNotification.h"

sem_t ServerNotification::mServerNotificationQueueLockSemaphore;
std::deque<ServerNotification*> ServerNotification::serverNotificationQueue;
