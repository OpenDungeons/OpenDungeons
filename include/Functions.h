#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <string>
#include <vector>
#include <OgreEntity.h>
#include <OgreLogManager.h>

class RenderRequest;
class ServerNotification;

bool readGameMapFromFile(const std::string& fileName);
void writeGameMapToFile(const std::string& fileName);

double randomDouble(double min, double max);
int randomInt(int min, int max);
unsigned randomUint(unsigned min, unsigned max);
double gaussianRandomDouble();
void seedRandomNumberGenerator();

void swap(int &a, int &b);
std::string stripCommentsFromLine(std::string line);

void colourizeEntity(Ogre::Entity *ent, int colour);
std::string colourizeMaterial(const std::string& materialName, int color);

void queueServerNotification(ServerNotification *n);

std::vector<std::string> listAllFiles(const std::string& directoryName);

//void waitOnRenderQueueFlush();

bool startServer();

#endif

