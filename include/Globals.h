#ifndef GLOBALS_H
#define GLOBALS_H

#include <semaphore.h>

#include "Tile.h"
#include "Player.h"
#include "GameMap.h"
#include "RenderRequest.h"
#include "ServerNotification.h"

extern GameMap gameMap;
extern SceneManager* mSceneMgr;
extern string MOTD;
extern int MAX_FRAMES_PER_SECOND;
extern vector<Player*> players;
extern Player *me;
extern double turnsPerSecond;
extern long int turnNumber;
extern deque<RenderRequest*> renderQueue;
extern sem_t renderQueueSemaphore;
extern deque<ServerNotification*> serverNotificationQueue;
extern sem_t serverNotificationQueueSemaphore;

#endif

