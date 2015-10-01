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

#include "camera/SlopeWalk.h"

#include <algorithm>
#include <iostream>
#include <sstream>

using std::cerr;
using std::endl;
using std::cout;
using std::stringstream;



SlopeWalk::SlopeWalk():
    leftVertexPassed(false),
    rightVertexPassed(false)
{
    


}


void SlopeWalk::buildSlopes()
{
    int ii;
    myArray.sort();
    myArray.zoom(1.2532);
    rightSlopes.clear();
    leftSlopes.clear();
    rightVertices.clear();
    leftVertices.clear();

    // find minimal and maximal values of y coordinate at left and right path
    findMinMaxLeft(myArray.mMyArray);
    findMinMaxRight(myArray.mMyArray);

    // for each pair of consecutive vertexes on right path create a slope value , which is equal to 'a' as in equation ax + b = 0
    // also put slopes of value 0 at the begging and the end of path

    rightSlopes.push_back(0);
    for(ii = top_right_index; ii != down_right_index ; ++ii,ii%=4  )
    {
        rightSlopes.push_back((myArray[ii].x - myArray[ii+1].x) * Unit / (myArray[ii].y - myArray[ii+1].y ));
        rightVertices.push_back(ii);

    }
    rightSlopes.push_back(0);
    rightVertices.push_back(ii);

    // for each pair of consecutive vertexes on left path create a slope value , which is equal to 'a' as in equation ax + b = 0
    // also put slopes of value 0 at the begging and the end of path

    leftSlopes.push_back(0);
    for(ii = top_left_index; ii != down_left_index ; ii+=3, ii%=4  )
    {
        leftSlopes.push_back((myArray[ii].x - myArray[ii-1].x) * Unit / (myArray[ii].y - myArray[ii-1].y ));
        leftVertices.push_back(ii);
    }
    leftSlopes.push_back(0);
    leftVertices.push_back(ii);
}

// reset indexes to the begginging of containers 
void SlopeWalk::prepareWalk()
{
    leftSlopeIndex =  leftSlopes.begin();
    rightSlopeIndex = rightSlopes.begin();
    leftVerticesIndex = leftVertices.begin();
    rightVerticesIndex = rightVertices.begin();

}

// What to do when passing one Vertex down on the Left Path 
bool SlopeWalk::passLeftVertex()
{
    leftVerticesIndex++;
    leftSlopeIndex++;
    return leftVerticesIndex == leftVertices.end();
}

// What to do when passing one Vertex down on the Right Path 
bool SlopeWalk::passRightVertex()
{
    rightVerticesIndex++;
    rightSlopeIndex++;
    return rightVerticesIndex == rightVertices.end();

}

// Check whether we pass a new Vertex on the left or right path, if so notify about it 
bool SlopeWalk::notifyOnMoveDown(long long newy_index)
{

    bool bb = true;
    while(leftVerticesIndex != leftVertices.end() && newy_index < getCurrentLeftVertex().y)
    {
        leftVertexPassed = true;
        bb = passLeftVertex();
    }
    bool kk = true;
    while(rightVerticesIndex != rightVertices.end() && newy_index < getCurrentRightVertex().y)
    {
        kk=  passRightVertex();
        rightVertexPassed = true;
    }
    return bb || kk ;
}

// Get vertex pointed currently  by index on the Left  path
Vector3i SlopeWalk::getCurrentLeftVertex()
{
    int ii = *leftVerticesIndex;
    return myArray[ii];
}

// Get vertex pointed currently  by index on the Right path
Vector3i SlopeWalk::getCurrentRightVertex()
{
    int ii = *rightVerticesIndex;
    return myArray[ii];
}

// Get vertex pointed previously  by index on the Left  path
Vector3i SlopeWalk::getPreviousLeftVertex()
{
    int ii = *(leftVerticesIndex - 1);
    return myArray[ii];
}

// Get vertex pointed previously  by index on the Right path
Vector3i SlopeWalk::getPreviousRightVertex()
{
    int ii = *(rightVerticesIndex -1);
    return myArray[ii];
}


// get the value of slope currently pointed by index on the left path
long long SlopeWalk::getCurrentXLeft(int64_t yy)
{
    if(leftSlopeIndex != leftSlopes.begin())
    {
        return getPreviousLeftVertex().x + ((*leftSlopeIndex) * (yy - getPreviousLeftVertex().y))/Unit ;
    }
    else
        return getCurrentLeftVertex().x;

}
// get the value of slope currently pointed by index on the right path
long long SlopeWalk::getCurrentXRight(int64_t yy){
    if(rightSlopeIndex != rightSlopes.begin())
    {
        return getPreviousRightVertex().x + ((*rightSlopeIndex) * (yy - getPreviousRightVertex().y))/Unit ;
    }
    else
        return getCurrentRightVertex().x;

}


Vector3i& SlopeWalk::getTopLeftVertex()
{
    return myArray[top_left_index];
}

Vector3i& SlopeWalk::getBottomLeftVertex()
{
    return myArray[down_left_index];
}

Vector3i& SlopeWalk::getTopRightVertex()
{
    return myArray[top_right_index];
}

Vector3i& SlopeWalk::getBottomRightVertex()
{
    return myArray[down_right_index];
}



void SlopeWalk::printState()
{
    cerr << "leftVertices" << endl;
    for(auto ii = leftVertices.begin(); ii != leftVertices.end(); ii++)
        cerr << myArray[*ii] << endl;
    cerr << "rightVertices" << endl;
    for(auto ii = rightVertices.begin(); ii != rightVertices.end(); ii++)
        cerr << myArray[*ii] << endl;
}

void SlopeWalk::findMinMaxLeft(std::array<Vector3i,4> &aa)
{

    auto min = aa.begin();
    auto max = aa.begin();
    for(auto ii = aa.begin(); ii !=aa.end(); ii++ )
    {
        if(ii->y < min->y)
            min = ii;
        if(ii->y >= max->y)
            max = ii;

    }
    top_left_index = max - aa.begin();
    down_left_index = min - aa.begin();

}

void SlopeWalk::findMinMaxRight(std::array<Vector3i,4> &aa)
{

    auto min = aa.begin();
    auto max = aa.begin();
    for(auto ii = aa.begin(); ii !=aa.end(); ii++ )
    {
        if(ii->y <= min->y)
            min = ii;
        if(ii->y > max->y)
            max = ii;

    }
    top_right_index = max - aa.begin();
    down_right_index = min - aa.begin();

}


string SlopeWalk::debug()
{

    stringstream ss;
    ss << "top_left_index " << top_left_index << " top_right_index " << top_right_index << " down_left_index " << down_left_index << " down_right_index" << down_right_index << endl;
    
    for (int ii = 0 ; ii < 4 ; ii++ )
    {
        Vector3i foobar = myArray[ii];
        ss<< ii << " " <<double(foobar.x)/Unit << " " << double(foobar.y)/Unit <<  endl;
    }
 
    ss << "leftVertices" << endl;

    for(auto ii : leftVertices)
        ss << ii << " " ;
    ss << endl ;
    ss << "rightVertices" << endl;

    for(auto ii : rightVertices)
        ss << ii << " " ;    
    ss << endl ; 

    ss<< "rightSlopes " << endl;
    for(auto ii : rightSlopes)
        ss << double(ii)/Unit << endl;
    ss << endl; 

    ss<< "leftSlopes " << endl;    
    for(auto ii : leftSlopes)
        ss << double(ii)/Unit << endl;

    return ss.str();
}
