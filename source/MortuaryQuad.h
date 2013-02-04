#ifndef MORTUARY_QUAD_H
#define MORTUARY_QUAD_H

#include "Quadtree.h"
#include <vector>

class Creature;

class MortuaryQuad : public CullingQuad{

public:
    MortuaryQuad(  );
    MortuaryQuad( const MortuaryQuad &qd  );
    void mortuaryInsert(Creature* cc);
    void clearMortuary();
    mutable sem_t creaturesInCullingQuadLockSemaphore;


    vector<Creature*>  mortuary;


private:
    





    };




#endif // MORTUARY_QUAD_H
