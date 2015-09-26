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


/* Currently I try to impment the most general polygon rasterizing algorithm , that is : 
 * 1. We choose the max and min points due to their Y value . 
 * 2. We sort all the points by the angle value in it's polar representation ( that is going , by visiting them clockwise due to the center of the polygon ) 
 * 3. Now we have two paths from which we can walk from top to down vertices : call them left and right . 
 * 4. Between each following pair of vertices we can establish a >> slope << , which is just the the "a" in the eq. of linear form y = ax + b 
 * 5. Now start drawing our polygon Row by Row From top to bottom . Each Row has given the most left and rightmost tile due to use of both paths prepared before -- Left path for tracing the most Leftmost Tile , Right path teh most Rightmost Tile in each  
*/




#ifndef SLOPEWALK_H_
#define SLOPEWALK_H_

#include "camera/DummyArrayClass.h"

#include "utils/Vector3i.h"

#include <deque>
#include <array>
#include <string>

using std::deque;
using std::array;
using std::string;

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
    bool leftVertexPassed;
    bool rightVertexPassed;


public:
    SlopeWalk();
    void printState();
    // A place for raw points
    DummyArrayClass myArray;
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
    string debug();
    void buildSlopes();

};

#endif // SLOPEWALK_H_
