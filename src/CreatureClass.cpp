#include "CreatureClass.h"

CreatureClass::CreatureClass()
{
	creatureJob = nullCreatureJob;
	sightRadius = 10.0;
	digRate = 0.0;
	danceRate = 0.0;
	hpPerLevel = 0.0;
	manaPerLevel = 0.0;
	moveSpeed = 1.0;
	maxHP = 10.0;
	maxMana = 10.0;
}

CreatureClass::CreatureJob CreatureClass::creatureJobFromString(std::string s)
{
	if(s.compare("BasicWorker") == 0)          return basicWorker; 
	if(s.compare("AdvancedWorker") == 0)       return advancedWorker; 
	if(s.compare("Scout") == 0)                return scout; 
	if(s.compare("WeakFighter") == 0)          return weakFighter; 
	if(s.compare("WeakSpellcaster") == 0)      return weakSpellcaster; 
	if(s.compare("WeakBuilder") == 0)          return weakBuilder; 
	if(s.compare("StrongFighter") == 0)        return strongFighter; 
	if(s.compare("StrongSpellcaster") == 0)    return strongSpellcaster; 
	if(s.compare("StrongBuilder") == 0)        return strongBuilder; 
	if(s.compare("Guard") == 0)                return guard; 
	if(s.compare("SpecialCreature") == 0)      return specialCreature; 
	if(s.compare("Summon") == 0)               return summon; 
	if(s.compare("SuperCreature") == 0)        return superCreature;

	return nullCreatureJob;
}

std::string CreatureClass::creatureJobToString(CreatureJob c)
{
	switch(c)
	{
		case nullCreatureJob:    return "NullCreatureJob";    break;
		case basicWorker:        return "BasicWorker";        break;
		case advancedWorker:     return "AdvancedWorker";     break;
		case scout:              return "Scout";              break;
		case weakFighter:        return "WeakFighter";        break;
		case weakSpellcaster:    return "WeakSpellcaster";    break;
		case weakBuilder:        return "WeakBuilder";        break;
		case strongFighter:      return "StrongFighter";      break;
		case strongSpellcaster:  return "StrongSpellcaster";  break;
		case strongBuilder:      return "StrongBuilder";      break;
		case guard:              return "Guard";              break;
		case specialCreature:    return "SpecialCreature";    break;
		case summon:             return "Summon";             break;
		case superCreature:      return "SuperCreature";      break;

		default:                 return "NullCreatureJob";    break;
	}
}

bool CreatureClass::isWorker()
{
	return (creatureJob == basicWorker || creatureJob == advancedWorker);
}

std::string CreatureClass::getFormat()
{
	return "# className\tcreatureJob\tmeshName\tbedMeshName\tbedDim1\tbedDim2\tscaleX\tscaleY\tscaleZ\thp/level\tmana/level\tsightRadius\tdigRate\tdanceRate\tmoveSpeed\tcHumans\tcCorpars\tcUndead\tcConstructs\tcDenizens\tcAltruism\tcOrder\tcPeace\n";
}

std::ostream& operator<<(std::ostream& os, CreatureClass *c)
{
	//TODO: Need to include maxHP/maxMana in the file format.
	os << c->className << "\t" << CreatureClass::creatureJobToString(c->creatureJob) << "\t" << c->meshName << "\t";
	os << c->bedMeshName << "\t" << c->bedDim1 << "\t" << c->bedDim2 << "\t";
	os << c->scale.x << "\t" << c->scale.y << "\t" << c->scale.z << "\t";
	os << c->hpPerLevel << "\t" << c->manaPerLevel << "\t";
	os << c->sightRadius << "\t" << c->digRate << "\t" << c->danceRate << "\t" << c->moveSpeed << "\t";
	os << c->coefficientHumans << "\t" << c->coefficientCorpars << "\t" << c->coefficientUndead << "\t";
	os << c->coefficientConstructs << "\t" << c->coefficientDenizens << "\t";
	os << c->coefficientAltruism << "\t" << c->coefficientOrder << "\t" << c->coefficientPeace;
	return os;
}

std::istream& operator>>(std::istream& is, CreatureClass *c)
{
	std::string tempString;
	is >> c->className >> tempString;
	c->creatureJob = CreatureClass::creatureJobFromString(tempString);
	is >> c->meshName;
	is >> c->bedMeshName >> c->bedDim1 >> c->bedDim2;
	is >> c->scale.x >> c->scale.y >> c->scale.z;
	is >> c->hpPerLevel >> c->manaPerLevel;
	is >> c->sightRadius >> c->digRate >> c->danceRate >> c->moveSpeed;
	is >> c->coefficientHumans >> c->coefficientCorpars >> c->coefficientUndead;
	is >> c->coefficientConstructs >> c->coefficientDenizens;
	is >> c->coefficientAltruism >> c->coefficientOrder >> c->coefficientPeace;

	return is;
}

/** \brief Conform: AttackableObject - Returns the prefix used in the OGRE identifier for this object.
 *
*/
std::string CreatureClass::getOgreNamePrefix()
{
	return "Creature_";
}

