/*
 *  Copyright (C) 2011-2016  OpenDungeons Team
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

#include "camera/CullingVectorManager.h"
#include "utils/VectorInt64.h"

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
private:

    std::deque<int64_t> mRightSlopes;
    std::deque<int64_t> mLeftSlopes;
    std::deque<int64_t>::iterator mRightSlopeIndex;
    std::deque<int64_t>::iterator mLeftSlopeIndex;

    // index numbers in myArray of Vertices
    // belonging to the right path
    std::deque<int32_t> mRightVertices;
    // index numbers in myArray of Vertices
    // belonging to the left path
    std::deque<int32_t> mLeftVertices;
    // point the Vertex currently processed
    // on the right path
    std::deque<int32_t>::iterator mRightVerticesIndex;
    // pint the Vertex currenly processed
    // on the left path
    std::deque<int32_t>::iterator mLeftVerticesIndex;
    bool passLeftVertex();
    bool passRightVertex();
    int mDownRightIndex;
    int mTopRightIndex;
    int mDownLeftIndex;
    int mTopLeftIndex;

public:
    void printState();
    // A place for raw points
    // The values of them will be used heavily
    // in this class
    // DummyArrayClass keeps clockwise order
    // of those points
    CullingVectorManager mVertices;
    bool notifyOnMoveDown(int64_t);
    void prepareWalk();

    int64_t getCurrentXLeft(int64_t yy);
    int64_t getCurrentXRight(int64_t yy);

    VectorInt64 getCurrentLeftVertex();
    VectorInt64 getCurrentRightVertex();
    VectorInt64 getPreviousLeftVertex();
    VectorInt64 getPreviousRightVertex();
    VectorInt64& getTopLeftVertex();
    VectorInt64& getBottomLeftVertex();
    VectorInt64& getTopRightVertex();
    VectorInt64& getBottomRightVertex();
    void findMinMaxLeft(const std::vector<VectorInt64>& );
    void findMinMaxRight(const std::vector<VectorInt64>& );
    std::string debug();
    void buildSlopes();
    void convexHull();

};

#endif // SLOPEWALK_H_
