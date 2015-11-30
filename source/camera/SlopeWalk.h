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

#ifndef SLOPEWALK_H_
#define SLOPEWALK_H_

#include "camera/DummyArrayClass.h"
#include "utils/Vector3i.h"

#include <deque>
#include <array>
#include <string>



/*! \brief The Class SlopeWalk keeps data required to rasterize
 *   one polygon made of game tiles. To do that it remembers two
 *  sides of polygon ; and on left and right side we keep a list 
 *  of Vertexes and Slopes -- the tangens of an angle between two 
 *  polygons edges ( remember the school equation y = ax + b
 *  We add also a bunch of auxilary functions to be able to 
 *  actually trace the position of some abstract point travelling
 *  top - down along polygon's edges.
 */

class SlopeWalk
{

    friend class CullingManager;

private:

    std::deque<long long> mRightSlopes;
    std::deque<long long> mLeftSlopes;
    std::deque<long long>::iterator mRightSlopeIndex;
    std::deque<long long>::iterator mLeftSlopeIndex;


    // index numbers in myArray of Vertices 
    // belonging to the right path
    std::deque<int> rightVertices;
    // index numbers in myArray of Vertices
    // belonging to the left path
    std::deque<int> leftVertices;
    // point the Vertex currently processed 
    // on the right path
    std::deque<int>::iterator rightVerticesIndex;
    // pint the Vertex currenly processed
    // on the left path
    std::deque<int>::iterator leftVerticesIndex;
    bool passLeftVertex();
    bool passRightVertex();
    int mDown_right_index;
    int mTop_right_index;
    int mDown_left_index;
    int mTop_left_index;
    bool mLeftVertexPassed;
    bool mRightVertexPassed;


public:
    SlopeWalk();
    void printState();
    // A place for raw points
    // The values of them will be used heavily 
    // in this class
    // DummyArrayClass keeps clockwise order 
    // of those points
    DummyArrayClass mVertices;
    bool notifyOnMoveDown(long long);
    void prepareWalk();

    long long getCurrentXLeft(int64_t yy);
    long long getCurrentXRight(int64_t yy);

    Vector3i getCurrentLeftVertex();
    Vector3i getCurrentRightVertex();
    Vector3i getPreviousLeftVertex();
    Vector3i getPreviousRightVertex();
    Vector3i& getTopLeftVertex();
    Vector3i& getBottomLeftVertex();
    Vector3i& getTopRightVertex();
    Vector3i& getBottomRightVertex();
    void findMinMaxLeft(std::array<Vector3i,4>& );
    void findMinMaxRight(std::array<Vector3i,4>& );
    std::string debug();
    void buildSlopes();

};

#endif // SLOPEWALK_H_
