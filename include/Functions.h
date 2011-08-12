#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <string>
#include <OgreEntity.h>

class RenderRequest;
class ServerNotification;

bool readGameMapFromFile(const std::string& fileName);
void writeGameMapToFile(const std::string& fileName);

void queueServerNotification(ServerNotification *n);

bool startServer();

#endif
