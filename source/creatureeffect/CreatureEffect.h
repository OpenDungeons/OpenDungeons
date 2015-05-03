/*
 *  Copyright (C) 2011-2015  OpenDungeons Team
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef CREATUREEFFECT_H
#define CREATUREEFFECT_H

#include <istream>

class Creature;

enum class CreatureEffectType
{
    unknown,
    heal,
    explode
};

std::ostream& operator<<(std::ostream& os, const CreatureEffectType& type);
std::istream& operator>>(std::istream& is, CreatureEffectType& type);

class CreatureEffect
{
public:
    // If particleEffectName is not empty, the corresponding particle effect will be
    // sent when the effect is added to the creature for nbTurnsEffect turns.
    CreatureEffect(uint32_t nbTurnsEffect, const std::string& particleEffectScript) :
        mNbTurnsEffect(nbTurnsEffect),
        mParticleEffectScript(particleEffectScript)
    {}

    virtual ~CreatureEffect()
    {}

    virtual CreatureEffectType getCreatureEffectType() const
    {return CreatureEffectType::unknown; }

    inline uint32_t getNbTurnsEffect() const
    { return mNbTurnsEffect; }

    inline const std::string& getParticleEffectScript() const
    { return mParticleEffectScript; }

    //! This function will be called during the creature upkeep
    bool upkeepEffect(Creature& creature);

    //! This function will be called during the creature upkeep before
    //! the effect is released and deleted
    virtual void releaseEffect(Creature& creature)
    {}

    static void write(const CreatureEffect& effect, std::ostream& os);
    //! loads a CreatureEffect from the given stream. Note that the stream might contain
    //! more data than just the CreatureEffect. Thus, we should not use optional data
    //! by reading the stream and expecting an EOF. If the data may vary, we should
    //! write how many data there is and read them accordingly
    static CreatureEffect* load(std::istream& is);

protected:
    //! Writes the CreatureEffect to the stream
    virtual void exportToStream(std::ostream& os) const;
    virtual void importFromStream(std::istream& is);

    //! This function will be called during the creature upkeep and should apply
    //! the wanted effect
    virtual void applyEffect(Creature& creature) = 0;
    CreatureEffect() :
        mNbTurnsEffect(0)
    {}

private:
    uint32_t mNbTurnsEffect;
    std::string mParticleEffectScript;
};

#endif // CREATUREEFFECT_H
