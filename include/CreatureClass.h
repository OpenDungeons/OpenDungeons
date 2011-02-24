#ifndef CREATURECLASS_H
#define CREATURECLASS_H

#include <string>
#include <iostream>
#include "Ogre.h"

class CreatureClass;

#include "AnimatedObject.h"

class CreatureClass : public AnimatedObject
{
	public:
		enum CreatureJob { nullCreatureJob = 0, basicWorker = 1, advancedWorker, scout, weakFighter, weakSpellcaster, weakBuilder,
		                   strongFighter, strongSpellcaster, strongBuilder, guard, specialCreature, summon, superCreature };

		// Constructors and operators
		CreatureClass();

		static CreatureJob creatureJobFromString(std::string s);
		static std::string creatureJobToString(CreatureJob c);

		bool isWorker();
		string getOgreNamePrefix();

		// Class properties
		//NOTE: Anything added to this class must be included in the '=' operator for the Creature class.
		CreatureJob creatureJob;
		std::string className;
		std::string meshName;
		std::string bedMeshName;
		int bedDim1, bedDim2;
		Ogre::Vector3 scale;
		double sightRadius;		// The inner radius where the creature sees everything
		double digRate;			// Fullness removed per turn of digging
		double danceRate;		// How much the danced upon tile's color changes per turn of dancing
		double hpPerLevel;
		double manaPerLevel;
		double maxHP, maxMana;

		// Probability coefficients to determine how likely a creature is to come through the portal.
		double coefficientHumans;
		double coefficientCorpars;
		double coefficientUndead;
		double coefficientConstructs;
		double coefficientDenizens;
		double coefficientAltruism;
		double coefficientOrder;
		double coefficientPeace;

		static std::string getFormat();

		std::string getName() {return name;}
		std::string name;

		friend ostream& operator<<(ostream& os, CreatureClass *c);

		friend istream& operator>>(istream& is, CreatureClass *c);
};

#endif

