#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <string>
#include <iostream>
//#include <fstream>
#include <vector>

#include "Globals.h"
#include "Tile.h"
#include "RenderRequest.h"
#include "ServerNotification.h"

bool readGameMapFromFile(std::string fileName);
void writeGameMapToFile(std::string fileName);

double randomDouble(double min, double max);
int randomInt(int min, int max);
unsigned int randomUint(unsigned int min, unsigned int max);
double gaussianRandomDouble();
void seedRandomNumberGenerator();

void swap(int &a, int &b);
std::string stripCommentsFromLine(std::string line);

void colourizeEntity(Entity *ent, int colour);
std::string colourizeMaterial(std::string materialName, int color);

void queueRenderRequest(RenderRequest *r);
void queueServerNotification(ServerNotification *n);

std::vector<std::string> listAllFiles(std::string directoryName);

void waitOnRenderQueueFlush();

bool startServer();

#endif

