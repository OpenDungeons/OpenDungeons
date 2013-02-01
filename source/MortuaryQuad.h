#ifndef MORTUARY_QUAD_H
#define MORTUARY_QUAD_H

#include "Quadtree.h"


class Creature;

class MortuaryQuad : CullingQuad{

public:

    MortuaryQuad( MortuaryQuad *qd  );
    void insertCreature(Creature* cc);
    void clearMortuary();

private:
    

    set<Creature*>  mortuary;



    };




#endif // MORTUARY_QUAD_H
