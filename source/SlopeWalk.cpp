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

#include "SlopeWalk.h"

#include <algorithm>
#include <iostream>

extern const int mPrecisionDigits = 20;
extern const int Unit = (1 << mPrecisionDigits);
using std::cerr;
using std::endl;
using std::cout;


void SlopeWalk::buildSlopes(){
    int ii;
    myArray.sort();
    rightSlopes.clear();
    leftSlopes.clear();
    rightVertices.clear();
    leftVertices.clear();
    // std::pair<std::array<Vector3i,4>::iterator,std::array<Vector3i,4>::iterator> minMaxL
    // 	= std::minmax_element(myArray.myArray.begin(), myArray.myArray.end(), [](Vector3i &vv, Vector3i &ww){ return vv.y < ww.y ; });
    // std::pair<std::array<Vector3i,4>::reverse_iterator,std::array<Vector3i,4>::reverse_iterator> minMaxR
    // 	= std::minmax_element(myArray.myArray.rbegin(), myArray.myArray.rend(), [](Vector3i &vv, Vector3i &ww){ return vv.y < ww.y ; });


    // top_left_index = minMaxL.second - myArray.myArray.begin() ;
    // down_left_index = minMaxL.first - myArray.myArray.begin() ;
    findMinMaxLeft(myArray.mMyArray);
    findMinMaxRight(myArray.mMyArray);

    cout << top_left_index  << endl;
    cout << down_left_index << endl;
    // for (auto jj = myArray.myArray.begin(); jj != myArray.myArray.end(); jj++){
    // 	cout << *jj << endl;
    // }
    // top_right_index = myArray.myArray.size() - (minMaxR.second - myArray.myArray.rbegin())  - 1;
    // down_right_index = myArray.myArray.size() - (minMaxR.first - myArray.myArray.rbegin())  - 1;

    cout << top_right_index  << endl;
    cout << down_right_index << endl;



    rightSlopes.push_back(0);
    for(ii = top_right_index; ii != down_right_index ; ++ii, ii%=4  ){
	rightSlopes.push_back((myArray[ii].x - myArray[ii+1].x) * Unit / (myArray[ii].y - myArray[ii+1].y ));
	rightVertices.push_back(ii);

    }
    rightSlopes.push_back(0);
    rightVertices.push_back(ii);


    leftSlopes.push_back(0);
    for(ii = top_left_index; ii != down_left_index ; ii+=3, ii%=4  ){
	leftSlopes.push_back((myArray[ii].x - myArray[ii-1].x) * Unit / (myArray[ii].y - myArray[ii-1].y ));
	leftVertices.push_back(ii);
    }
    leftSlopes.push_back(0);
    leftVertices.push_back(ii);
}


void SlopeWalk::prepareWalk(){
    leftSlopeIndex =  leftSlopes.begin();
    rightSlopeIndex = rightSlopes.begin();
    leftVerticesIndex = leftVertices.begin();
    rightVerticesIndex = rightVertices.begin();

}


bool SlopeWalk::passLeftVertex(){
    leftVerticesIndex++;
    leftSlopeIndex++;
    return leftVerticesIndex == leftVertices.end();
}

bool SlopeWalk::passRightVertex(){
    rightVerticesIndex++;
    rightSlopeIndex++;
    return rightVerticesIndex == rightVertices.end();

}

bool SlopeWalk::notifyOnMoveDown(long long newy_index){

    bool bb = true;
    if(leftVerticesIndex != leftVertices.end() && newy_index < getCurrentLeftVertex().y)
	bb = bb && passLeftVertex();
    if(rightVerticesIndex != rightVertices.end() && newy_index < getCurrentRightVertex().y)
	bb=  bb && passRightVertex();
    return bb ;
}


Vector3i SlopeWalk::getCurrentLeftVertex(){
    int ii = *leftVerticesIndex;
    return myArray[ii];


}
Vector3i SlopeWalk::getCurrentRightVertex(){
    int ii = *rightVerticesIndex;
    return myArray[ii];

}

long long SlopeWalk::getCurrentDxLeft(){
    return *leftSlopeIndex;

}


long long SlopeWalk::getCurrentDxRight(){
    return *rightSlopeIndex;

}


Vector3i& SlopeWalk::getTopLeftVertex(){
    return myArray[top_left_index];
}

Vector3i& SlopeWalk::getBottomLeftVertex(){
    return myArray[down_left_index];
}

Vector3i& SlopeWalk::getTopRightVertex(){
    return myArray[top_right_index];
}

Vector3i& SlopeWalk::getBottomRightVertex(){
    return myArray[down_right_index];
}



void SlopeWalk::printState(){
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
    for(auto ii = aa.begin(); ii !=aa.end(); ii++ ){
	if(ii->y <= min->y)
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
    for(auto ii = aa.begin(); ii !=aa.end(); ii++ ){
	if(ii->y < min->y)
	    min = ii;
	if(ii->y > max->y)
	    max = ii;

    }
    top_right_index = max - aa.begin();
    down_right_index = min - aa.begin();

}
