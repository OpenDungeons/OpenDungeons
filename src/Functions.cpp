#include <sstream>
#include <stdio.h>
#include <dirent.h>
using namespace std;

#include <ctype.h>

#include "Globals.h"
#include "Defines.h"
#include "Functions.h"
#include "Creature.h"
#include "MapLight.h"

#if defined(WIN32) || defined(_WIN32)
double const M_PI = 2 * acos(0.0);
#endif

bool readGameMapFromFile(string fileName)
{
	Seat *tempSeat;
	Goal *tempGoal;
	Tile *tempTile;
	Room *tempRoom;
	MapLight *tempLight;
	string tempString, tempString2;
	Creature tempCreature;
	int objectsToLoad;

	// Try to open the input file for reading and throw an error if we can't.
	ifstream baseLevelFile(fileName.c_str(), ifstream::in);
	if( !baseLevelFile.good() )
	{
		cerr << "ERROR: File not found:  " << fileName << "\n\n\n";
		return false;
	}

	// Read in the whole baseLevelFile, strip it of comments and feed it into
	// the stringstream levelFile, to be read by the rest of the function.
	stringstream levelFile;
	while(baseLevelFile.good())
	{
		getline(baseLevelFile, tempString);
		levelFile << stripCommentsFromLine(tempString) << "\n";
	}

	baseLevelFile.close();

	gameMap.clearAll();

	// Read in the version number from the level file
	levelFile >> tempString;
	if(tempString.compare(versionString) != 0)
	{
		cerr << "\n\n\nERROR:  Attempting to load a file produced by a different version of OpenDungeons.\n";
		cerr << "ERROR:  Filename:  " << fileName;
		cerr << "\nERROR:  The file is for OpenDungeons:  " << tempString;
		cerr << "\nERROR:  This version of OpenDungeons:  " << versionString << "\n\n\n";
		exit(1);
	}

	// Read in the name of the next level to load after this one is complete.
	levelFile >> gameMap.nextLevel;

	// Read in the seats from the level file
	levelFile >> objectsToLoad;
	for(int i = 0; i < objectsToLoad; i++)
	{
		tempSeat = new Seat;
		levelFile >> tempSeat;

		gameMap.addEmptySeat(tempSeat);
	}

	// Read in the goals that are shared by all players, the first player to complete all these goals is the winner.
	levelFile >> objectsToLoad;
	for(int i = 0; i < objectsToLoad; i++)
	{
		tempGoal = Goal::instantiateFromStream(levelFile);

		if(tempGoal != NULL)
			gameMap.addGoalForAllSeats(tempGoal);
	}

	// Read in the map tiles from disk
	levelFile >> objectsToLoad;
	gameMap.disableFloodFill();
	for(int i = 0; i < objectsToLoad; i++)
	{
		//NOTE: This code is duplicated in the client side method
		//"addclass" defined in src/Client.cpp and readGameMapFromFile.
		//Changes to this code should be reflected in that code as well
		tempTile = new Tile;
		levelFile >> tempTile;

		gameMap.addTile(tempTile);
	}
	gameMap.enableFloodFill();

	// Loop over all the tiles and force them to examine their
	// neighbors.  This allows them to switch to a mesh with fewer
	// polygons if some are hidden by the neighbors.
	TileMap_t::iterator itr = gameMap.firstTile();
	while(itr != gameMap.lastTile())
	{
		itr->second->setFullness( itr->second->getFullness() );
		itr++;
	}

	// Read in the rooms
	levelFile >> objectsToLoad;
	for(int i = 0; i < objectsToLoad; i++)
	{
		tempRoom = new Room;
		levelFile >> tempRoom;

		gameMap.addRoom(tempRoom);
	}

	// Read in the lights
	levelFile >> objectsToLoad;
	for(int i = 0; i < objectsToLoad; i++)
	{
		tempLight = new MapLight;
		levelFile >> tempLight;

		gameMap.addMapLight(tempLight);
	}

	// Read in the creature class descriptions
	levelFile >> objectsToLoad;
	for(int i = 0; i < objectsToLoad; i++)
	{
		//NOTE: This code is duplicated in the client side method
		//"addclass" defined in src/Client.cpp and writeGameMapToFile.
		//Changes to this code should be reflected in that code as well
		double tempX, tempY, tempZ, tempSightRadius, tempDigRate, tempMoveSpeed;
		int tempHP, tempMana;
		levelFile >> tempString >> tempString2 >> tempX >> tempY >> tempZ;

		levelFile >> tempHP >> tempMana;
		levelFile >> tempSightRadius >> tempDigRate >> tempMoveSpeed;

		Creature *p = new Creature(tempString, tempString2, Ogre::Vector3(tempX, tempY, tempZ), tempHP, tempMana, tempSightRadius, tempDigRate, tempMoveSpeed);
		gameMap.addClassDescription(p);
	}

	// Read in the actual creatures themselves
	levelFile >> objectsToLoad;
	for(int i = 0; i < objectsToLoad; i++)
	{

		//NOTE: This code is duplicated in the client side method
		//"addclass" defined in src/Client.cpp and writeGameMapToFile.
		//Changes to this code should be reflected in that code as well
		Creature *newCreature = new Creature;

		levelFile >> newCreature;

		gameMap.addCreature(newCreature);
	}

	return true;
}

void writeGameMapToFile(string fileName)
{
	ofstream levelFile(fileName.c_str(), ifstream::out);
	Creature *tempCreature;
	Tile *tempTile;

	// Write the identifier string and the version number
	levelFile << versionString << "  # The version of OpenDungeons which created this file (for compatability reasons).\n";

	// write out the name of the next level to load after this one is complete.
	levelFile << gameMap.nextLevel << " # The level to load after this level is complete.\n";

	// Write out the seats to the file
	levelFile << "\n# Seats\n" << gameMap.numEmptySeats()+gameMap.numFilledSeats() << "  # The number of seats to load.\n";
	levelFile << "# " << Seat::getFormat() << "\n";
	for(unsigned int i = 0; i < gameMap.numEmptySeats(); i++)
	{
		levelFile << gameMap.getEmptySeat(i);
	}

	for(unsigned int i = 0; i < gameMap.numFilledSeats(); i++)
	{
		levelFile << gameMap.getFilledSeat(i);
	}

	// Write out the goals shared by all players to the file.
	levelFile << "\n# Goals\n" << gameMap.numGoalsForAllSeats() << "  # The number of goals to load.\n";
	levelFile << "# " << Goal::getFormat() << "\n";
	for(unsigned int i = 0; i < gameMap.numGoalsForAllSeats(); i++)
	{
		levelFile << gameMap.getGoalForAllSeats(i);
	}

	// Write out the tiles to the file
	levelFile << "\n# Tiles\n" << gameMap.numTiles() << "  # The number of tiles to load.\n";
	levelFile << "# " << Tile::getFormat() << "\n";
	TileMap_t::iterator itr = gameMap.firstTile();
	while(itr != gameMap.lastTile())
	{
		//NOTE: This code is duplicated in the client side method
		//"addclass" defined in src/Client.cpp and readGameMapFromFile.
		//Changes to this code should be reflected in that code as well
		tempTile = itr->second;
		levelFile << tempTile->x << "\t" << tempTile->y << "\t";
		levelFile << tempTile->getType() << "\t" << tempTile->getFullness();

		levelFile << endl;

		itr++;
	}

	// Write out the rooms to the file
	levelFile << "\n# Rooms\n" << gameMap.numRooms() << "  # The number of rooms to load.\n";
	levelFile << "# " << Room::getFormat() << "\n";
	for(unsigned int i = 0; i < gameMap.numRooms(); i++)
	{
		levelFile << gameMap.getRoom(i) << endl;
	}

	// Write out the lights to the file.
	levelFile << "\n# Lights\n" << gameMap.numMapLights() << "  # The number of lights to load.\n";
	levelFile << "# " << MapLight::getFormat() << "\n";
	for(unsigned int i = 0; i < gameMap.numMapLights(); i++)
	{
		levelFile << gameMap.getMapLight(i) << endl;
	}

	// Write out the creature descriptions to the file
	levelFile << "\n# Creature classes\n" << gameMap.numClassDescriptions() << "  # The number of creature classes to load.\n";
	levelFile << "# className\tmeshName\tscaleX\tscaleY\tscaleZ\tHP\tmana\tsightRadius\tdigRate\tmoveSpeed\n";
	for(unsigned int i = 0; i < gameMap.numClassDescriptions(); i++)
	{
		//NOTE: This code is duplicated in the client side method
		//"addclass" defined in src/Client.cpp and readGameMapFromFile.
		//Changes to this code should be reflected in that code as well
		tempCreature = gameMap.getClassDescription(i);
		levelFile << tempCreature->className << "\t" << tempCreature->meshName << "\t" << tempCreature->scale.x << "\t" << tempCreature->scale.y << "\t" << tempCreature->scale.z << "\t";
		levelFile << tempCreature->hp << "\t" << tempCreature->mana << "\t";
		levelFile << tempCreature->sightRadius << "\t" << tempCreature->digRate << "\t" << tempCreature->moveSpeed << "\n";
	}

	// Write out the individual creatures to the file
	levelFile << "\n# Creatures\n" << gameMap.numCreatures() << "  # The number of creatures to load.\n";
	levelFile << "# " << Creature::getFormat() << "\n";
	for(unsigned int i = 0; i < gameMap.numCreatures(); i++)
	{
		//NOTE: This code is duplicated in the client side method
		//"addclass" defined in src/Client.cpp and readGameMapFromFile.
		//Changes to this code should be reflected in that code as well
		levelFile << gameMap.getCreature(i) << endl;
	}

	levelFile << endl;

	levelFile.close();
}

string forceLowercase(string s)
{
	string tempString;
	
	for(unsigned int i = 0; i < s.size(); i++)
	{
		tempString += tolower(s[i]);
	}

	return tempString;
}

namespace Random
{
	unsigned long myRandomSeed = 14329;
	unsigned long MAX = 32768;

	// This is a very simple random number generator.  It is nothing fancy
	// but should be good enough for our purposes.  The rand() function
	// behave slightly differently on different systems and was causing
	// bugs so it was replaced with this one.
	unsigned long randgen()
	{
		myRandomSeed = myRandomSeed*1103515245 + 12345; 
		return (unsigned int)(myRandomSeed / 65536) % 32768; 
	}

	// uniformly distributed number [0;1)
	double uniform()
	{
		return randgen() * (1.0 / ((double) MAX));
	}

	// uniformly distributed number [0;hi)
	double uniform(double hi)
	{
		return uniform() * hi;
	}

	// uniformly distributed number [lo;hi)
	double uniform(double lo, double hi)
	{
		return (uniform() * (hi - lo)) + lo;
	}

	// random bit
	signed randint()
	{
		return uniform() > 0.5 ? 1 : 0;
	}

	// random integer [0;hi]
	signed randint(signed hi)
	{
		return static_cast<signed>(uniform() * (hi + 1));
	}

	// random integer [lo;hi]
	signed randint(signed lo, signed hi)
	{
		return static_cast<signed> (lo + (uniform() * ((hi + 1) - lo)));
	}

	// random unsigned integer [0;hi]
	unsigned int randuint(unsigned int hi)
	{
		return static_cast<unsigned int> (uniform() * (hi + 1));
	}

	// random unsigned integer [lo;hi]
	unsigned int randuint(unsigned int lo, unsigned int hi)
	{
		return static_cast<unsigned int> (lo + (uniform() * ((hi + 1) - lo)));
	}
} //end namespace Random

/************************************************
*randomDouble() returns a double between the 
*lower number entered and the higher number 
*entered.  One or both numbers can be negative
************************************************/
double randomDouble(double min, double max)
{
	if(min > max)
	{
		double temp = min;
		min = max;
		max = temp;
	}
	
	return Random::uniform(min, max);
}

int randomInt(int min, int max)
{
	if(min > max)
	{
		int temp = min;
		min = max;
		max = temp;
	}
	
	return Random::randint(min, max);
}

unsigned int randomUint(unsigned int min, unsigned int max)
{
	if(min > max)
	{
		unsigned int temp = min;
		min = max;
		max = temp;
	}
	
	return Random::randuint(min, max);
}

//returns a gaussian distributed random double value in [-1,1]
double gaussianRandomDouble()
{
	double temp1 = randomDouble(0.0, 1.0);
	double temp2 = randomDouble(0.0, 1.0);

	double val1;
	//double val2;

	val1 = sqrt(-2.0*log(temp1)) * cos(2.0*M_PI*temp2);
	// val2 = sqrt(-2.0*log(temp1)) * sin(2.0*M_PI*temp2);
	
	return val1;
}

void seedRandomNumberGenerator()
{
	Random::myRandomSeed = time(0);
}

void swap(int &a, int &b)
{
	int temp = a;
	a = b;
	b = temp;
}

string stripCommentsFromLine(string line)
{
	// Find the first occurrence of the comment symbol on the line and return everything before that character.
	size_t index = line.find('#');
	return line.substr(0, index);
}

void colourizeEntity(Entity *ent, int colour)
{
	// Colorize the the textures
	// Loop over the sub entities in the mesh
	for(unsigned int i = 0; i < ent->getNumSubEntities(); i++)
	{
		SubEntity *tempSubEntity = ent->getSubEntity(i);
		tempSubEntity->setMaterialName(colourizeMaterial(tempSubEntity->getMaterialName(), colour));
	}
}

string colourizeMaterial(string materialName, int colour)
{
	string tempString;
	stringstream tempSS;
	HardwarePixelBufferSharedPtr tempPixBuf;
	PixelBox tempPixelBox;
	Technique *tempTechnique;
	Pass *tempPass;
	TextureUnitState *tempTextureUnitState;
	TexturePtr tempTexture;
	uint8 *pixelData;

	tempSS.str(tempString);
	tempSS << "Color_" << colour << "_" << materialName;
	MaterialPtr tempMaterial = MaterialPtr(Ogre::MaterialManager::getSingleton().getByName(tempSS.str()));

	cout << "\n\nCloning material:  " << tempSS.str();

	// If this texture has not been copied and colourized yet then do so
	if(tempMaterial.isNull())
	{
		cout << "   Material does not exist, creating a new one.";
		//MaterialPtr newMaterial = MaterialPtr(Ogre::MaterialManager::getSingleton().create(tempSS.str(), "manualMaterialsGroup"));
		MaterialPtr newMaterial = MaterialPtr(Ogre::MaterialManager::getSingleton().getByName(materialName))->clone(tempSS.str());

		// Loop over the techniques for the new material
		for(unsigned int j = 0; j < newMaterial->getNumTechniques(); j++)
		{
			tempTechnique = newMaterial->getTechnique(j);
			// Loop over the passes for the current technique
			for(unsigned int k = 0; k < tempTechnique->getNumPasses(); k++)
			{
				tempPass = tempTechnique->getPass(k);
				// Loop over the TextureUnitStates for the current pass
				for(unsigned int l = 0; l < tempPass->getNumTextureUnitStates(); l++)
				{
					// Get a pointer to the actual pixel data for this texture
					tempTextureUnitState = tempPass->getTextureUnitState(l);
					tempTexture = tempTextureUnitState->_getTexturePtr();
					tempPixBuf = tempTexture->getBuffer();
					tempPixBuf->lock(HardwareBuffer::HBL_NORMAL);
					tempPixelBox = tempPixBuf->getCurrentLock();
					pixelData = static_cast<uint8*>(tempPixelBox.data);

					// Loop over the pixels themselves and change the bright pink ones to the given colour
					for(unsigned int x = 0; x < tempTexture->getWidth(); x++)
					{
						for(unsigned int y = 0; y < tempTexture->getHeight(); y++)
						{
							uint8 *blue = pixelData++;
							uint8 *green = pixelData++;
							uint8 *red = pixelData++;
							uint8 *alpha = pixelData++;

							int deltaR = *red - 235, deltaG = *green - 20, deltaB = *blue - 235;
							int totalDistance = fabs((double)deltaR) + fabs((double)deltaG) + fabs((double)deltaB);
							int factor = 10;
							// Check to see if the current pixel matches the target colour
							if(totalDistance <= 65)
							{
								//if(colour < playerColourValues.size())
								Player *tempPlayer = gameMap.getPlayerByColour(colour);
								if(tempPlayer != NULL)
								{
									int newR = tempPlayer->seat->colourValue.r*255 + factor*deltaR;
									int newG = tempPlayer->seat->colourValue.g*255 + factor*deltaG;
									int newB = tempPlayer->seat->colourValue.b*255 + factor*deltaB;
									newR = (newR < 0) ? 0 : newR;
									newR = (newR > 255) ? 255 : newR;
									newG = (newG < 0) ? 0 : newG;
									newG = (newG > 255) ? 255 : newG;
									newB = (newB < 0) ? 0 : newB;
									newB = (newB > 255) ? 255 : newB;
									*blue = (uint8)newB;
									*green = (uint8)newG;
									*red = (uint8)newR;
									*alpha = (uint8)(tempPlayer->seat->colourValue.a * 255);
								}
							}
						}
					}

					tempPixBuf->unlock();
				}
			}
		}
	}

	return tempSS.str();

}

void queueRenderRequest(RenderRequest *r)
{
	sem_wait(&renderQueueSemaphore);
	renderQueue.push_back(r);
	sem_post(&renderQueueSemaphore);
}


vector<string> listAllFiles(string directoryName)
{
    vector<string> tempVector;
    DIR *d;
    struct dirent *dir;
    d = opendir(directoryName.c_str());
    if (d)
    {
        while ((dir = readdir(d)) != NULL)
        {
            tempVector.push_back(dir->d_name);
        }

        closedir(d);
    }
    return tempVector;
}

