#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <string>
#include <iostream>
#include <fstream>
#include <vector>
using namespace std;

#include "Globals.h"
#include "Tile.h"
#include "RenderRequest.h"
#include "ServerNotification.h"

bool readGameMapFromFile(string fileName);
void writeGameMapToFile(string fileName);

double randomDouble(double min, double max);
int randomInt(int min, int max);
unsigned int randomUint(unsigned int min, unsigned int max);
double gaussianRandomDouble();
void seedRandomNumberGenerator();

void swap(int &a, int &b);
string stripCommentsFromLine(string line);

void colourizeEntity(Entity *ent, int colour);
string colourizeMaterial(string materialName, int color);

void queueRenderRequest(RenderRequest *r);
void queueServerNotification(ServerNotification *n);

vector<string> listAllFiles(string directoryName);

void waitOnRenderQueueFlush();

#endif

