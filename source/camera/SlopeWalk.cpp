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

#include "camera/SlopeWalk.h"
#include "utils/LogManager.h"
#include "utils/Helper.h"

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include <boost/geometry/geometries/register/point.hpp>
#include <boost/geometry/geometries/adapted/boost_tuple.hpp>

#include <algorithm>
#include <iostream>
#include <sstream>

BOOST_GEOMETRY_REGISTER_POINT_2D(VectorInt64, int64_t, boost::geometry::cs::cartesian, x, y)
//! Values used to know whether to show and/or hide a mesh
typedef VectorInt64 Point;
typedef boost::geometry::model::polygon<Point> Polygon;

template <typename Point>
void get_coordinates(Point const& p)
{
    OD_LOG_DBG( "get__coordinates : " + Helper::toString(double(boost::geometry::get<0>(p))/VectorInt64::UNIT) + ", " + Helper::toString( double(boost::geometry::get<1>(p))/VectorInt64::UNIT )) ;
}

void SlopeWalk::buildSlopes()
{
    //A value of how much we are to enlarge our polygon
    //chosen experimentally too large values slows down
    //the culling and may provide some glitches
    mVertices.sort();
    mRightSlopes.clear();
    mLeftSlopes.clear();
    mRightVertices.clear();
    mLeftVertices.clear();

    // find minimal and maximal values of y coordinate at left and right path
    findMinMaxLeft(mVertices.mMyArray);
    findMinMaxRight(mVertices.mMyArray);

    // for each pair of consecutive vertexes on right path create a slope value , which is equal to 'a' as in equation ax + b = 0
    // also put slopes of value 0 at the begging and the end of path

    mRightSlopes.push_back(0);
    for(int ii = mTopRightIndex; ii != mDownRightIndex ; ++ii, ii %= mVertices.mMyArray.size())
    {
        int64_t divisor = mVertices[ii].y - mVertices[ii+1].y;
        mRightSlopes.push_back((mVertices[ii].x - mVertices[ii+1].x) * VectorInt64::UNIT / divisor);
        mRightVertices.push_back(ii);

    }
    mRightSlopes.push_back(0);
    mRightVertices.push_back(mDownRightIndex);

    // for each pair of consecutive vertexes on left path create a slope value , which is equal to 'a' as in equation ax + b = 0
    // also put slopes of value 0 at the begging and the end of path

    mLeftSlopes.push_back(0);
    for(int ii =  mTopLeftIndex; ii != mDownLeftIndex ; ii+=mVertices.mMyArray.size() - 1, ii%=mVertices.mMyArray.size())
    {
        int64_t divisor = mVertices[ii].y - mVertices[ii-1].y;
        mLeftSlopes.push_back((mVertices[ii].x - mVertices[ii-1].x) * VectorInt64::UNIT / divisor);
        mLeftVertices.push_back(ii);
    }
    mLeftSlopes.push_back(0);
    mLeftVertices.push_back(mDownLeftIndex);
}

// reset indexes to the begginging of containers
void SlopeWalk::prepareWalk()
{
    mLeftSlopeIndex =  mLeftSlopes.begin();
    mRightSlopeIndex = mRightSlopes.begin();
    mLeftVerticesIndex = mLeftVertices.begin();
    mRightVerticesIndex = mRightVertices.begin();
}

// What to do when passing one Vertex down on the Left Path
bool SlopeWalk::passLeftVertex()
{
    mLeftVerticesIndex++;
    mLeftSlopeIndex++;
    return mLeftVerticesIndex == mLeftVertices.end();
}

// What to do when passing one Vertex down on the Right Path
bool SlopeWalk::passRightVertex()
{
    mRightVerticesIndex++;
    mRightSlopeIndex++;
    return mRightVerticesIndex == mRightVertices.end();
}

// Check whether we pass a new Vertex on the left or right path, if so notify about it
bool SlopeWalk::notifyOnMoveDown(int64_t newyIndex)
{
    bool bb = true;
    while(mLeftVerticesIndex != mLeftVertices.end() && newyIndex < getCurrentLeftVertex().y)
    {
         bb = passLeftVertex();
    }
    bool kk = true;
    while(mRightVerticesIndex != mRightVertices.end() && newyIndex < getCurrentRightVertex().y)
    {
        kk=  passRightVertex();
     }
    return bb || kk ;
}

// Get vertex pointed currently  by index on the Left  path
VectorInt64 SlopeWalk::getCurrentLeftVertex()
{
    int ii = *mLeftVerticesIndex;
    return mVertices[ii];
}

// Get vertex pointed currently  by index on the Right path
VectorInt64 SlopeWalk::getCurrentRightVertex()
{
    int ii = *mRightVerticesIndex;
    return mVertices[ii];
}

// Get vertex pointed previously  by index on the Left  path
VectorInt64 SlopeWalk::getPreviousLeftVertex()
{
    int ii = *(mLeftVerticesIndex - 1);
    return mVertices[ii];
}

// Get vertex pointed previously  by index on the Right path
VectorInt64 SlopeWalk::getPreviousRightVertex()
{
    int ii = *(mRightVerticesIndex -1);
    return mVertices[ii];
}


// get the value of slope currently pointed by index on the left path
int64_t SlopeWalk::getCurrentXLeft(int64_t yy)
{
    if(mLeftSlopeIndex != mLeftSlopes.begin())
    {
        return getPreviousLeftVertex().x + ((*mLeftSlopeIndex) * (yy - getPreviousLeftVertex().y))/VectorInt64::UNIT ;
    }
    else
        return getCurrentLeftVertex().x;
}

// get the value of slope currently pointed by index on the right path
int64_t SlopeWalk::getCurrentXRight(int64_t yy)
{
    if(mRightSlopeIndex != mRightSlopes.begin())
    {
        return getPreviousRightVertex().x + ((*mRightSlopeIndex) * (yy - getPreviousRightVertex().y))/VectorInt64::UNIT ;
    }
    else
        return getCurrentRightVertex().x;
}


VectorInt64& SlopeWalk::getTopLeftVertex()
{
    return mVertices[ mTopLeftIndex];
}

VectorInt64& SlopeWalk::getBottomLeftVertex()
{
    return mVertices[mDownLeftIndex];
}

VectorInt64& SlopeWalk::getTopRightVertex()
{
    return mVertices[mTopRightIndex];
}

VectorInt64& SlopeWalk::getBottomRightVertex()
{
    return mVertices[mDownRightIndex];
}



void SlopeWalk::printState()
{
    std::cerr << "mLeftVertices" << std::endl;
    for(auto ii = mLeftVertices.begin(); ii != mLeftVertices.end(); ++ii)
        std::cerr << mVertices[*ii] << std::endl;
    std::cerr << "mRightVertices" << std::endl;
    for(auto ii = mRightVertices.begin(); ii != mRightVertices.end(); ++ii)
        std::cerr << mVertices[*ii] << std::endl;
}

void SlopeWalk::findMinMaxLeft(const std::vector<VectorInt64> &aa)
{

    auto min = aa.begin();
    auto max = aa.begin();
    for(auto ii = aa.begin(); ii !=aa.end(); ++ii )
    {
        if(ii->y < min->y)
            min = ii;
        if(ii->y >= max->y)
            max = ii;

    }
     mTopLeftIndex = max - aa.begin();
     mDownLeftIndex = min - aa.begin();
}

void SlopeWalk::findMinMaxRight(const std::vector<VectorInt64> &aa)
{
    auto min = aa.begin();
    auto max = aa.begin();
    for(auto ii = aa.begin(); ii !=aa.end(); ++ii )
    {
        if(ii->y <= min->y)
            min = ii;
        if(ii->y > max->y)
            max = ii;
    }
    mTopRightIndex = max - aa.begin();
    mDownRightIndex = min - aa.begin();
}


std::string SlopeWalk::debug()
{

    std::stringstream ss;
    ss << " mTopLeftIndex " <<  mTopLeftIndex << " mTopRightIndex " << mTopRightIndex << " mDownLeftIndex " << mDownLeftIndex << " mDownRightIndex" << mDownRightIndex << std::endl;

    for(unsigned int ii = 0; ii < mVertices.mMyArray.size(); ++ii)
    {
        VectorInt64 foobar = mVertices[ii];
        ss << ii << " " << double(foobar.x) / VectorInt64::UNIT << " " << double(foobar.y) / VectorInt64::UNIT <<  std::endl;
    }

    ss << "mLeftVertices" << std::endl;

    for(auto ii : mLeftVertices)
        ss << ii << " ";
    ss << std::endl ;
    ss << "mRightVertices" << std::endl;

    for(auto ii : mRightVertices)
        ss << ii << " ";
    ss << std::endl ;

    ss<< "mRightSlopes " << std::endl;
    for(auto ii : mRightSlopes)
        ss << double(ii) / VectorInt64::UNIT << std::endl;
    ss << std::endl;

    ss<< "mLeftSlopes " << std::endl;
    for(auto ii : mLeftSlopes)
        ss << double(ii) / VectorInt64::UNIT << std::endl;

    return ss.str();
}

void SlopeWalk::convexHull()
{
    const double zoomFactorValue = 1.0565;
    mVertices.zoom(zoomFactorValue);
    Polygon polygon;
    boost::geometry::append(polygon, mVertices.mMyArray);

    uint32_t nb;
    nb = boost::geometry::num_points(polygon);
    if(nb != mVertices.mMyArray.size())
    {
        OD_LOG_ERR("Unexpected number of polygons=" + Helper::toString(nb));
        return;
    }

    boost::geometry::correct(polygon);

    // After correction, the polygon should be looped so we expect the size to
    // have one more point
    nb = boost::geometry::num_points(polygon) - 1;
    if(nb != mVertices.mMyArray.size())
    {
        OD_LOG_ERR("Unexpected number of polygons=" + Helper::toString(nb));
        return;
    }

    Polygon hull;
    mVertices.mMyArray.clear();

    boost::geometry::convex_hull(polygon, hull);
    boost::geometry::for_each_point(boost::geometry::exterior_ring(hull), [this](Point &p){this->mVertices.mMyArray.push_back(p);});
    mVertices.mMyArray.pop_back();
}
