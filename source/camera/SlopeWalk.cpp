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

SlopeWalk::SlopeWalk():
    mLeftVertexPassed(false),
    mRightVertexPassed(false)
{
}

void SlopeWalk::buildSlopes()
{
    int ii;
    //A value of how much we are to enlarge our polygon
    //choosen experimentally too large values slows down
    //the culling and may provide some glitches
    const double zoomFactorValue = 1.244364;
    mVertices.sort();
    mVertices.zoom(zoomFactorValue);
    mRightSlopes.clear();
    mLeftSlopes.clear();
    rightVertices.clear();
    leftVertices.clear();

    // find minimal and maximal values of y coordinate at left and right path
    findMinMaxLeft(mVertices.mMyArray);
    findMinMaxRight(mVertices.mMyArray);

    // for each pair of consecutive vertexes on right path create a slope value , which is equal to 'a' as in equation ax + b = 0
    // also put slopes of value 0 at the begging and the end of path

    mRightSlopes.push_back(0);
    for(ii = mTop_right_index; ii != mDown_right_index ; ++ii,ii%=4  )
    {
        mRightSlopes.push_back((mVertices[ii].x - mVertices[ii+1].x) * Unit / (mVertices[ii].y - mVertices[ii+1].y ));
        rightVertices.push_back(ii);

    }
    mRightSlopes.push_back(0);
    rightVertices.push_back(ii);

    // for each pair of consecutive vertexes on left path create a slope value , which is equal to 'a' as in equation ax + b = 0
    // also put slopes of value 0 at the begging and the end of path

    mLeftSlopes.push_back(0);
    for(ii =  mTop_left_index; ii != mDown_left_index ; ii+=3, ii%=4  )
    {
        mLeftSlopes.push_back((mVertices[ii].x - mVertices[ii-1].x) * Unit / (mVertices[ii].y - mVertices[ii-1].y ));
        leftVertices.push_back(ii);
    }
    mLeftSlopes.push_back(0);
    leftVertices.push_back(ii);
}

// reset indexes to the begginging of containers 
void SlopeWalk::prepareWalk()
{
    mLeftSlopeIndex =  mLeftSlopes.begin();
    mRightSlopeIndex = mRightSlopes.begin();
    leftVerticesIndex = leftVertices.begin();
    rightVerticesIndex = rightVertices.begin();

}

// What to do when passing one Vertex down on the Left Path 
bool SlopeWalk::passLeftVertex()
{
    leftVerticesIndex++;
    mLeftSlopeIndex++;
    return leftVerticesIndex == leftVertices.end();
}

// What to do when passing one Vertex down on the Right Path 
bool SlopeWalk::passRightVertex()
{
    rightVerticesIndex++;
    mRightSlopeIndex++;
    return rightVerticesIndex == rightVertices.end();

}

// Check whether we pass a new Vertex on the left or right path, if so notify about it 
bool SlopeWalk::notifyOnMoveDown(long long newy_index)
{

    bool bb = true;
    while(leftVerticesIndex != leftVertices.end() && newy_index < getCurrentLeftVertex().y)
    {
        mLeftVertexPassed = true;
        bb = passLeftVertex();
    }
    bool kk = true;
    while(rightVerticesIndex != rightVertices.end() && newy_index < getCurrentRightVertex().y)
    {
        kk=  passRightVertex();
        mRightVertexPassed = true;
    }
    return bb || kk ;
}

// Get vertex pointed currently  by index on the Left  path
Vector3i SlopeWalk::getCurrentLeftVertex()
{
    int ii = *leftVerticesIndex;
    return mVertices[ii];
}

// Get vertex pointed currently  by index on the Right path
Vector3i SlopeWalk::getCurrentRightVertex()
{
    int ii = *rightVerticesIndex;
    return mVertices[ii];
}

// Get vertex pointed previously  by index on the Left  path
Vector3i SlopeWalk::getPreviousLeftVertex()
{
    int ii = *(leftVerticesIndex - 1);
    return mVertices[ii];
}

// Get vertex pointed previously  by index on the Right path
Vector3i SlopeWalk::getPreviousRightVertex()
{
    int ii = *(rightVerticesIndex -1);
    return mVertices[ii];
}


// get the value of slope currently pointed by index on the left path
long long SlopeWalk::getCurrentXLeft(int64_t yy)
{
    if(mLeftSlopeIndex != mLeftSlopes.begin())
    {
        return getPreviousLeftVertex().x + ((*mLeftSlopeIndex) * (yy - getPreviousLeftVertex().y))/Unit ;
    }
    else
        return getCurrentLeftVertex().x;

}
// get the value of slope currently pointed by index on the right path
long long SlopeWalk::getCurrentXRight(int64_t yy){
    if(mRightSlopeIndex != mRightSlopes.begin())
    {
        return getPreviousRightVertex().x + ((*mRightSlopeIndex) * (yy - getPreviousRightVertex().y))/Unit ;
    }
    else
        return getCurrentRightVertex().x;

}


Vector3i& SlopeWalk::getTopLeftVertex()
{
    return mVertices[ mTop_left_index];
}

Vector3i& SlopeWalk::getBottomLeftVertex()
{
    return mVertices[mDown_left_index];
}

Vector3i& SlopeWalk::getTopRightVertex()
{
    return mVertices[mTop_right_index];
}

Vector3i& SlopeWalk::getBottomRightVertex()
{
    return mVertices[mDown_right_index];
}



void SlopeWalk::printState()
{
    std::cerr << "leftVertices" << std::endl;
    for(auto ii = leftVertices.begin(); ii != leftVertices.end(); ii++)
        std::cerr << mVertices[*ii] << std::endl;
    std::cerr << "rightVertices" << std::endl;
    for(auto ii = rightVertices.begin(); ii != rightVertices.end(); ii++)
        std::cerr << mVertices[*ii] << std::endl;
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
     mTop_left_index = max - aa.begin();
    mDown_left_index = min - aa.begin();

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
    mTop_right_index = max - aa.begin();
    mDown_right_index = min - aa.begin();

}


std::string SlopeWalk::debug()
{

    std::stringstream ss;
    ss << " mTop_left_index " <<  mTop_left_index << " mTop_right_index " << mTop_right_index << " mDown_left_index " << mDown_left_index << " mDown_right_index" << mDown_right_index << std::endl;
    
    for (int ii = 0 ; ii < 4 ; ii++ )
    {
        Vector3i foobar = mVertices[ii];
        ss<< ii << " " <<double(foobar.x)/Unit << " " << double(foobar.y)/Unit <<  std::endl;
    }
 
    ss << "leftVertices" << std::endl;

    for(auto ii : leftVertices)
        ss << ii << " " ;
    ss << std::endl ;
    ss << "rightVertices" << std::endl;

    for(auto ii : rightVertices)
        ss << ii << " " ;    
    ss << std::endl ; 

    ss<< "mRightSlopes " << std::endl;
    for(auto ii : mRightSlopes)
        ss << double(ii)/Unit << std::endl;
    ss << std::endl; 

    ss<< "mLeftSlopes " << std::endl;    
    for(auto ii : mLeftSlopes)
        ss << double(ii)/Unit << std::endl;

    return ss.str();
}
