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

#ifndef SLOPEWALK_H_
#define SLOPEWALK_H_

#include "camera/DummyArrayClass.h"

#include "utils/Vector3i.h"

#include <deque>
#include <array>


using std::deque;
using std::array;

class SlopeWalk
{

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
    int down_right_index;
    int top_right_index;
    int down_left_index;
    int top_left_index;



public:
    void printState();
    DummyArrayClass myArray;
    bool notifyOnMoveDown(long long);
    void prepareWalk();

    long long getCurrentDxLeft();
    long long getCurrentDxRight();

    Vector3i getCurrentLeftVertex();
    Vector3i getCurrentRightVertex();
    Vector3i& getTopLeftVertex();
    Vector3i& getBottomLeftVertex();
    Vector3i& getTopRightVertex();
    Vector3i& getBottomRightVertex();
    void findMinMaxLeft(std::array<Vector3i,4>& );
    void findMinMaxRight(std::array<Vector3i,4>& );

    void buildSlopes();

};

#endif // SLOPEWALK_H_
