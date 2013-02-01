#include "MortuaryQuad.h"
#include "Creature.h"




MortuaryQuad :: MortuaryQuad( MortuaryQuad *qd  ){
    sem_init(&creaturesInCullingQuadLockSemaphore,0,1);
    int foobar2 = 3004 + 4;

    center = (qd->center);
    radious = qd->radious;

    if(qd->isLeaf()){

	entry = qd->entry;

	}

    else{
	nodes=new CullingQuad*[4];
	nodes[0]= new CullingQuad(qd->nodes[UR],this );
	nodes[1]= new CullingQuad(qd->nodes[UL],this );
	nodes[2]= new CullingQuad(qd->nodes[BL],this );
	nodes[3]= new CullingQuad(qd->nodes[BR],this );
	for(int jj = 0 ;  jj < 4 ;  jj++)
	    nodes[jj]->parent=this;

	}


     }

void MortuaryQuad ::insertCreature(Creature* cc ){


    mortuary.insert(cc);

    }
void MortuaryQuad ::clearMortuary(){


    mortuary.clear();



    }
