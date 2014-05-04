#include "SlopeWalk.h"
#include <algorithm>



extern const int mPrecisionDigits = 15;
extern const int Unit = (1 << mPrecisionDigits);


void SlopeWalk::buildSlopes(){
    int ii,jj;
    myArray.sort();
    std::pair<std::array<Vector3i,4>::iterator,std::array<Vector3i,4>::iterator> minMax
	= std::minmax_element(myArray.myArray.begin(), myArray.myArray.end(), [](Vector3i &vv, Vector3i &ww){ return vv.y < ww.y ; });
    int top_index = minMax.second - myArray.myArray.begin() ;
    int down_index = minMax.first - myArray.myArray.begin() ;
    leftSlopes.push_back(0);
    for(ii = top_index; ii != down_index ; ++ii, ii%=4  ){
	leftSlopes.push_back((myArray[ii].x - myArray[ii+1].x) * Unit / (myArray[ii].y - myArray[ii+1].y )); 
	leftVertices.push_back(ii);

    }
    leftSlopes.push_back(0);
    leftVertices.push_back(ii);


    rightSlopes.push_back(0);
    for(ii = top_index; ii != down_index ; --ii, ii%=4  ){
	rightSlopes.push_back((myArray[ii].x - myArray[ii-1].x) * Unit / (myArray[ii].y - myArray[ii-1].y )); 
	rightVertices.push_back(ii);
    }
    rightSlopes.push_back(0);
    rightVertices.push_back(ii);	
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
    if(newy_index < getCurrentLeftVertex().y)
	bb = bb && passLeftVertex();
    if(newy_index < getCurrentRightVertex().y)
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


Vector3i& SlopeWalk::getTopVertex(){
    int ii = *(leftVertices.begin());
    return myArray[ii];
}

Vector3i& SlopeWalk::getBottomVertex(){
    int ii = *(leftVertices.end() - 1);
    return myArray[ii];
}
