#ifndef FUNCTIONS_H
#define FUNCTIONS_H

class ServerNotification;
class GameMap;

void queueServerNotification(ServerNotification *n);

bool startServer(GameMap& gameMap);

#endif
