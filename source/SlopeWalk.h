#ifndef SLOPEWALK_H_
#define SLOPEWALK_H_

#include "Vector3i.h"
#include "DummyArrayClass.h"
#include <deque>



using std::deque;

class SlopeWalk{

    friend class CullingManager;

private:

    std::deque<long long> rightSlopes;
    std::deque<long long> leftSlopes;
    std::deque<long long>::iterator rightSlopeIndex;
    std::deque<long long>::iterator leftSlopeIndex;



    std::deque<int> rightVertices;
    std::deque<int> leftVertices;
    std::deque<int>::iterator rightVerticesIndex;
    std::deque<int>::iterator leftVerticesIndex;
    bool passLeftVertex();
    bool passRightVertex();


public:

    DummyArrayClass myArray;   
    bool notifyOnMoveDown(long long);
    void prepareWalk();

    long long getCurrentDxLeft();
    long long getCurrentDxRight();

    Vector3i getCurrentLeftVertex();
    Vector3i getCurrentRightVertex();
    Vector3i& getTopVertex();
    Vector3i& getBottomVertex();

    void buildSlopes();

};

#endif /* SLOPEWALK_H_ */
