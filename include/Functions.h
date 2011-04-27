#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <string>
#include <OgreEntity.h>

class RenderRequest;
class ServerNotification;

bool readGameMapFromFile(const std::string& fileName);
void writeGameMapToFile(const std::string& fileName);

void colourizeEntity(Ogre::Entity *ent, int colour);
std::string colourizeMaterial(const std::string& materialName, int color);

void queueServerNotification(ServerNotification *n);

bool startServer();

#endif
