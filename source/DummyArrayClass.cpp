#include "DummyArrayClass.h"
#include <vector>
#include <algorithm>

using std::vector;

void DummyArrayClass::sort(){
    vector<Vector3i> ll[4];
    Vector3i cc = { 0 , 0 , 0};
    cc = (cc + myArray[0] + myArray[1] + myArray[2] + myArray[3]) / 4;

    for(int ii = 0 ; ii < 4 ; ii++){
	if(myArray[ii].x > cc.x && myArray[ii].y > cc.y ){
	    ll[0].push_back( myArray[ii]);		
	}
	if(myArray[ii].x > cc.x && myArray[ii].y <= cc.y ){
	    ll[1].push_back( myArray[ii]);		
	}
	if(myArray[ii].x <= cc.x && myArray[ii].y <= cc.y ){
	    ll[2].push_back( myArray[ii]);		
	}
	if(myArray[ii].x <= cc.x && myArray[ii].y > cc.y ){
	    ll[3].push_back( myArray[ii]);		
	}
    }
    int kk = 0 ; 
    for(int ii = 0 ; ii < 4 ; ii++){
	sortByPhi(ll[ii],cc);
	   
	for( auto jj = ll[ii].begin(); jj < ll[ii].end(); jj++){
	    myArray[kk]=*jj;
	    kk++;
	}

    }
}

void DummyArrayClass::sortByPhi(vector<Vector3i> &ll, Vector3i& cc){
    if(ll.size() > 1){
	std::sort(ll.begin(), ll.end(),
		  [&cc](Vector3i vv, Vector3i ww  ){ return (double)(vv.x - cc.x)/(double)(vv.y - cc.y) < (double)(ww.x - cc.x) /(double)(ww.y - cc.y) ;});	    
    }
}


Vector3i& DummyArrayClass::operator[](int ii){
    while(ii < 0 )
	ii += 4;
    return myArray[ii%4];
};
