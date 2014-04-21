/*
 *  Copyright (C) 2011-2014  OpenDungeons Team
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

#ifndef MORTUARY_QUAD_H
#define MORTUARY_QUAD_H

#include "CullingQuad.h"

#include <vector>

class Creature;

//! \brief A class used to update the list of creature present on a culling quad,
//! hence permitting to cull those along the way.
//! FIXME: Is this true. What about rooms, traps and lights if this is true?
class MortuaryQuad : public CullingQuad
{

friend class Quadtree;
friend class CullingManager;
friend class CullingQuad;

public:
    MortuaryQuad();
    MortuaryQuad(const MortuaryQuad &qd);

    void mortuaryInsert(Creature* cc);

    void clearMortuary();

private:
    std::vector<Creature*> mMortuaryQuad;
    mutable sem_t mCreaturesInCullingQuadLockSemaphore;
};

#endif // MORTUARY_QUAD_H
