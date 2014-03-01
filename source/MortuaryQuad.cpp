#include "MortuaryQuad.h"
#include "Creature.h"

MortuaryQuad::MortuaryQuad()
{
    sem_init(&creaturesInCullingQuadLockSemaphore,0,1);
}

MortuaryQuad::MortuaryQuad(const MortuaryQuad &qd):
    mortuary(qd.mortuary)
{
    sem_init(&creaturesInCullingQuadLockSemaphore, 0, 1);

    center = (qd.center);
    mRadius = qd.mRadius;

    if(qd.isLeaf()) {
        entry = qd.entry;
    }
    else {
        nodes = new CullingQuad*[4];
        nodes[0] = new CullingQuad(qd.nodes[UR], this);
        nodes[1] = new CullingQuad(qd.nodes[UL], this);
        nodes[2] = new CullingQuad(qd.nodes[BL], this);
        nodes[3] = new CullingQuad(qd.nodes[BR], this);
        for(int jj = 0; jj < 4; ++jj)
            nodes[jj]->parent = this;
    }
}

void MortuaryQuad ::mortuaryInsert(Creature* cc ){

    releaseRootSemaphore();
    mortuary.push_back(cc);
    holdRootSemaphore();


    }
void MortuaryQuad ::clearMortuary(){


    mortuary.clear();



    }

