//TODO: ideally we shouldn't need this file. Reasons:
// - The stream operators should be replaced by a proper XML reader class that creates
// the Definition objects through its ctor (that's what ctors are for).
// - The name strings should ideally be read from a file, too (XML? Script? but not hardcoded,
// who knows what creature types will there be in the future - shouldn't need recompiling
// for plain content additions/adjustments or translations).
// - CreatureDefintion is ... a plain Defintion. That's what header files are for.

#include "CreatureDefinition.h"

CreatureDefinition::CreatureJob CreatureDefinition::creatureJobFromString(std::string s)
{
    if (s.compare("BasicWorker") == 0)
        return basicWorker;
    else if (s.compare("AdvancedWorker") == 0)
        return advancedWorker;
    else if (s.compare("Scout") == 0)
        return scout;
    else if (s.compare("WeakFighter") == 0)
        return weakFighter;
    else if (s.compare("WeakSpellcaster") == 0)
        return weakSpellcaster;
    else if (s.compare("WeakBuilder") == 0)
        return weakBuilder;
    else if (s.compare("StrongFighter") == 0)
        return strongFighter;
    else if (s.compare("StrongSpellcaster") == 0)
        return strongSpellcaster;
    else if (s.compare("StrongBuilder") == 0)
        return strongBuilder;
    else if (s.compare("Guard") == 0)
        return guard;
    else if (s.compare("SpecialCreature") == 0)
        return specialCreature;
    else if (s.compare("Summon") == 0)
        return summon;
    else if (s.compare("SuperCreature") == 0)
        return superCreature;
    else
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

