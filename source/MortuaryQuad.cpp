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

#include "MortuaryQuad.h"

#include "Creature.h"

MortuaryQuad::MortuaryQuad()
{
    sem_init(&mCreaturesInCullingQuadLockSemaphore, 0, 1);
}

MortuaryQuad::MortuaryQuad(const MortuaryQuad &qd):
    mMortuaryQuad(qd.mMortuaryQuad)
{
    sem_init(&mCreaturesInCullingQuadLockSemaphore, 0, 1);

    center = (qd.center);
    mRadius = qd.mRadius;

    if(qd.isLeaf())
    {
        entry = qd.entry;
    }
    else
    {
        nodes = new CullingQuad*[4];
        nodes[0] = new CullingQuad(qd.nodes[UR], this);
        nodes[1] = new CullingQuad(qd.nodes[UL], this);
        nodes[2] = new CullingQuad(qd.nodes[BL], this);
        nodes[3] = new CullingQuad(qd.nodes[BR], this);
        for(int jj = 0; jj < 4; ++jj)
            nodes[jj]->parent = this;
    }
}

void MortuaryQuad ::mortuaryInsert(Creature* cc)
{
    releaseRootSemaphore();
    mMortuaryQuad.push_back(cc);
    holdRootSemaphore();
}

void MortuaryQuad ::clearMortuary()
{
    mMortuaryQuad.clear();
}
