#include <ctype.h>

#include "Globals.h"
#include "Functions.h"
#include "Creature.h"

#if defined(WIN32) || defined(_WIN32)
double const M_PI = 2 * acos(0.0);
#endif


void readGameMapFromFile(string fileName)
{
	ifstream levelFile(fileName.c_str(), ifstream::in);
	if( !levelFile.good() )
	{
		cerr << "ERROR: File not found:  " << fileName << "\n\n\n";
		exit(1);
	}

	gameMap.clearAll();

	Tile *tempTile;
	Room *tempRoom;
	string tempString, tempString2;
	Creature tempCreature;
	int objectsToLoad;

	levelFile >> objectsToLoad;

	// Read in the map tiles from disk
	for(int i = 0; i < objectsToLoad; i++)
	{
		
		//NOTE: This code is duplicated in the client side method
		//"addclass" defined in src/Client.cpp and readGameMapFromFile.
		//Changes to this code should be reflected in that code as well
		tempTile = new Tile;
		levelFile >> tempTile;

		gameMap.addTile(tempTile);
	}

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

	levelFile.close();
}

void writeGameMapToFile(string fileName)
{
	ofstream levelFile(fileName.c_str(), ifstream::out);
	Creature *tempCreature;
	Tile *tempTile;

	// Write out the tiles to the file
	levelFile << gameMap.numTiles() << endl;
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
	levelFile << "\n" << gameMap.numRooms() << "\n";
	for(unsigned int i = 0; i < gameMap.numRooms(); i++)
	{
		levelFile << gameMap.getRoom(i) << endl;
	}

	// Write out the creature descriptions to the file
	levelFile << "\n" << gameMap.numClassDescriptions() << "\n";
	for(unsigned int i = 0; i < gameMap.numClassDescriptions(); i++)
	{
		//NOTE: This code is duplicated in the client side method
		//"addclass" defined in src/Client.cpp and readGameMapFromFile.
		//Changes to this code should be reflected in that code as well
		tempCreature = gameMap.getClassDescription(i);
		levelFile << tempCreature->className << "\t" << tempCreature->meshName << "\t" << tempCreature->scale.x << "\t" << tempCreature->scale.y << "\t" << tempCreature->scale.z << "\t";
		levelFile << tempCreature->hp << "\t" << tempCreature->mana << "\t";
		levelFile << tempCreature->sightRadius << "\t" << tempCreature->digRate << "\n";
	}

	// Write out the individual creatures to the file
	levelFile << "\n" << gameMap.numCreatures() << "\n";
	for(unsigned int i = 0; i < gameMap.numCreatures(); i++)
	{
		//NOTE: This code is duplicated in the client side method
		//"addclass" defined in src/Client.cpp and readGameMapFromFile.
		//Changes to this code should be reflected in that code as well
		levelFile << gameMap.getCreature(i);
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

/************************************************
*randomDouble() returns a double between the 
*lower number entered and the higher number 
*entered.  One or both numbers can be negative
************************************************/
double randomDouble(double min,double max)
{
	if(min > max)
	{
	double temp = min;
	min = max;
	max = temp;
	}
	
double r = -1*((double)rand()/(double)(RAND_MAX+1));
double value = min;
value += r*(max-min);

return value;
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
	srand(time(0));
}

void swap(int &a, int &b)
{
	int temp = a;
	a = b;
	b = temp;
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

							// Check to see if the current pixel matches the target colour
							if(*blue == 255 && *green == 0 && *red == 255)
							{
								if(colour < playerColourValues.size())
								{
									*blue = (uint8)(playerColourValues[colour].b * 255);
									*green = (uint8)(playerColourValues[colour].g * 255);
									*red = (uint8)(playerColourValues[colour].r * 255);
									*alpha = (uint8)(playerColourValues[colour].a * 255);
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

