#include "ClientNotification.h"

sem_t ClientNotification::clientNotificationQueueLockSemaphore;
sem_t ClientNotification::clientNotificationQueueSemaphore;
std::deque<ClientNotification*> ClientNotification::clientNotificationQueue;
