#include "CreatureDefinition.h"

CreatureDefinition::CreatureDefinition() :
        creatureJob(nullCreatureJob),
        bedDim1(0),
        bedDim2(0),
        scale(Ogre::Vector3(1, 1, 1)),
        sightRadius(10.0),
        digRate(10.0),
        danceRate(0.35),
        hpPerLevel(0.0),
        manaPerLevel(0.0),
        maxHP(10.0),
        maxMana(10.0),
        coefficientHumans(0.0),
        coefficientCorpars(0.0),
        coefficientUndead(0.0),
        coefficientConstructs(0.0),
        coefficientDenizens(0.0),
        coefficientAltruism(0.0),
        coefficientOrder(0.0),
        coefficientPeace(0.0)
{
}

CreatureDefinition::CreatureJob CreatureDefinition::creatureJobFromString(std::string s)
{
    if (s.compare("BasicWorker") == 0)
        return basicWorker;
    if (s.compare("AdvancedWorker") == 0)
        return advancedWorker;
    if (s.compare("Scout") == 0)
        return scout;
    if (s.compare("WeakFighter") == 0)
        return weakFighter;
    if (s.compare("WeakSpellcaster") == 0)
        return weakSpellcaster;
    if (s.compare("WeakBuilder") == 0)
        return weakBuilder;
    if (s.compare("StrongFighter") == 0)
        return strongFighter;
    if (s.compare("StrongSpellcaster") == 0)
        return strongSpellcaster;
    if (s.compare("StrongBuilder") == 0)
        return strongBuilder;
    if (s.compare("Guard") == 0)
        return guard;
    if (s.compare("SpecialCreature") == 0)
        return specialCreature;
    if (s.compare("Summon") == 0)
        return summon;
    if (s.compare("SuperCreature") == 0)
        return superCreature;

    return nullCreatureJob;
}

std::string CreatureDefinition::creatureJobToString(CreatureJob c)
{
    switch (c)
    {
        case nullCreatureJob:
            return "NullCreatureJob";

        case basicWorker:
            return "BasicWorker";

        case advancedWorker:
            return "AdvancedWorker";

        case scout:
            return "Scout";

        case weakFighter:
            return "WeakFighter";

        case weakSpellcaster:
            return "WeakSpellcaster";

        case weakBuilder:
            return "WeakBuilder";

        case strongFighter:
            return "StrongFighter";

        case strongSpellcaster:
            return "StrongSpellcaster";

        case strongBuilder:
            return "StrongBuilder";

        case guard:
            return "Guard";

        case specialCreature:
            return "SpecialCreature";

        case summon:
            return "Summon";

        case superCreature:
            return "SuperCreature";

        default:
            return "NullCreatureJob";
    }
}

std::string CreatureDefinition::getFormat()
{
    return "# className\tcreatureJob\tmeshName\tbedMeshName\tbedDim1\tbedDim2\tscaleX\tscaleY\tscaleZ\thp/level\tmana/level\tsightRadius\tdigRate\tdanceRate\tmoveSpeed\tcHumans\tcCorpars\tcUndead\tcConstructs\tcDenizens\tcAltruism\tcOrder\tcPeace\n";
}

std::ostream& operator<<(std::ostream& os, CreatureDefinition *c)
{
    //TODO: Need to include maxHP/maxMana in the file format.
    os << c->className << "\t" << CreatureDefinition::creatureJobToString(
            c->creatureJob) << "\t" << c->meshName << "\t";
    os << c->bedMeshName << "\t" << c->bedDim1 << "\t" << c->bedDim2 << "\t";
    os << c->scale.x << "\t" << c->scale.y << "\t" << c->scale.z << "\t";
    os << c->hpPerLevel << "\t" << c->manaPerLevel << "\t";
    os << c->sightRadius << "\t" << c->digRate << "\t" << c->danceRate << "\t"
            << c->moveSpeed << "\t";
    os << c->coefficientHumans << "\t" << c->coefficientCorpars << "\t"
            << c->coefficientUndead << "\t";
    os << c->coefficientConstructs << "\t" << c->coefficientDenizens << "\t";
    os << c->coefficientAltruism << "\t" << c->coefficientOrder << "\t"
            << c->coefficientPeace;
    return os;
}

std::istream& operator>>(std::istream& is, CreatureDefinition *c)
{
    std::string tempString;
    is >> c->className >> tempString;
    c->creatureJob = CreatureDefinition::creatureJobFromString(tempString);
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
std::string CreatureDefinition::getOgreNamePrefix()
{
    return "Creature_";
}

