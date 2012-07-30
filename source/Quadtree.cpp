#include <iostream>
#include <cstdlib>

#include "Quadtree.h"


using std::cerr;
using std::endl;
using std::string;



using std::cerr;
using std::cout;
using std::endl;



using Ogre::Vector2;
using std::ifstream;
using std::ofstream;
using std::swap;
using std::abs;


#include <cstdlib> // for exit function

// This program reads values from the file 'example.dat'
// and echoes them to the display until a negative value
// is read.

// int main()
//     {
//     ifstream indata; // indata is like cin
//     ofstream outdate;
//     int nPoints;
//     int nSegments;


//     Segment *sectorsArray;
//     Ogre::Vector2 *pointsArray;

//     indata.open("a.in"); // opens the file
//     if(!indata) { // file couldn't be opened
// 	cerr << "Error: file could not be opened" << endl;
// 	exit(1);
// 	}

//     indata >> nSegments;



//     sectorsArray = new Segment[nSegments];







//     for(int ii = 0 ;  ii  < nSegments && !indata.eof(); ii++){
	
	
// 	indata >> sectorsArray[ii]; 
// 	}

//     for(int ii = 0 ;  ii  < nSegments ; ii++){
// 	sectorsArray[ii].setDelta(sectorsArray[(ii+1)%nSegments].tail);
	
// 	}

    
//     indata >> nPoints;

//     pointsArray = new Vector2[nPoints];

//     for(int ii = 0 ;  ii  < nPoints && !indata.eof(); ii++){
	
	
// 	indata >> pointsArray[ii]; 
// 	}

//     indata.close();


//     CullingQuad myQuad;
//     myQuad.setRadious(200);
//     myQuad.setCenter(200,200);
 
//     for(int ii = 0 ;  ii  < nPoints && !indata.eof(); ii++){
	
	
// 	myQuad.insert( new Entry(pointsArray[ii]));
// 	}
    
//     int foobar = 3004 + 4;
//     // myCulQuad.cut(&sectorsArray[2]);   


//     for(int ii = 0 ;  ii  < nSegments ; ii++){
//     	myQuad.cut(&sectorsArray[ii]);
   

//     	}

//     int foobar2 = 3004 + 4;
//     cout<< nSegments   <<endl;
//     for(int ii = 0 ;  ii  < nSegments ; ii++){
// 	cout << sectorsArray[ii].tail.x << "  " << sectorsArray[ii].tail.y << "  " << endl;
// 	}


//     cout << myQuad.countNodes() << endl;
//     myQuad.print();
//     return 0;
//     }



void CullingQuad::setCenter(double xx, double yy){
    center->x = xx;
    center->y = yy;


}

void CullingQuad::setRadious(double rr){
    radious = rr;

}

CullingQuad::CullingQuad():nodes(NULL),entry(NULL), parent(NULL){

    center = new Ogre::Vector2();


}



// COPY constructor
CullingQuad::CullingQuad(CullingQuad* qt,CullingQuad* pp ):parent(pp){

    int foobar2 = 3004 + 4;

    if(qt->isLeaf()){

	nodes = NULL;
	entry = qt->entry;
	center = (qt->center);
	radious = qt->radious;
	
	return;
       }

    else{
	nodes=new CullingQuad*[4];
	for(int jj = 0 ;  jj < 4 ;  jj++)
	    nodes[jj]->parent=this;
	entry = NULL;
	nodes[0]= new CullingQuad(qt->nodes[UR],this );
	nodes[1]= new CullingQuad(qt->nodes[UL],this );
	nodes[2]= new CullingQuad(qt->nodes[BL],this );
	nodes[3]= new CullingQuad(qt->nodes[BR],this );
	center = (qt->center);
	radious = qt->radious;

	}
}


CullingQuad* CullingQuad::find(Entry* ee){
    return find(ee->index_point);
    }
CullingQuad* CullingQuad::find(Entry& ee){
    return find(ee.index_point);
    }



CullingQuad* CullingQuad::find(Ogre::Vector2& vv){
    return find(&vv);
    }
CullingQuad* CullingQuad::find(Ogre::Vector2* vv){
        if(isLeaf())
	return this;

    
	if(vv->x >= center->x && vv->y >= center->y){
	    return nodes[UR]->find(vv);

	} 
	else if(vv->x < center->x && vv->y >= center->y ){
	    return  nodes[UL]->find(vv);

	}
	
	else if(vv->x < center->x && vv->y < center->y ){
	    return  nodes[BL]->find(vv);
	}
	
	else if(vv->x >= center->x && vv->y < center->y){
	     return  nodes[BR]->find(vv);
	}


}

CullingQuad* CullingQuad::insert(Entry& ee){

    return insert(&ee);
    

    }
CullingQuad* CullingQuad::insert(Entry* ee){
    CullingQuad* qq = find(ee);
    return qq->shallowInsert(ee);
}




CullingQuad::~CullingQuad(){

    if(nodes!=NULL){
	if(nodes[0]!=NULL)
	    delete nodes[0];
	if(nodes[1]!=NULL)
	    delete nodes[1];
	if(nodes[2]!=NULL)
	    delete nodes[2];
	if(nodes[3]!=NULL)
	    delete nodes[3];
	}

    }


void CullingQuad::print(){
    if(entry!=NULL)
	cout << entry->index_point.x << " " << entry->index_point.y << endl;
    if(nodes!=NULL)
    	for(int ii = 0 ; ii < 4 ; ii++){
    	    if(nodes[ii]!=NULL)
    		nodes[ii]->print();
    	}

}

set<Creature*>* CullingQuad::returnCreaturesSet(){
    set<Creature*>* ss = new set<Creature*>() ;

    if(entry!=NULL){
	for(list<Creature*>::iterator it = entry->creature_list.begin()  ; it != entry->creature_list.end()   ; it++   ){
	    ss->insert(*it);
	}
    }

    if(nodes!=NULL)
    	for(int ii = 0 ; ii < 4 ; ii++){
    	    if(nodes[ii]!=NULL)
    		nodes[ii]->returnCreaturesSetAux(ss);
    	}
    return ss;
}

void CullingQuad::returnCreaturesSetAux(set<Creature*> *ss){

    if(entry!=NULL){
	for(list<Creature*>::iterator it = entry->creature_list.begin()  ; it != entry->creature_list.end()   ; it++   ){
	    ss->insert(*it);
	}	
    }
	
    if(nodes!=NULL)
    	for(int ii = 0 ; ii < 4 ; ii++){
    	    if(nodes[ii]!=NULL)
    		nodes[ii]->returnCreaturesSetAux(ss);
    	}

}



int CullingQuad::countNodes(){

    int nodesNumber=0;

    if(entry!=NULL)
	nodesNumber++;
    if(nodes!=NULL)
    	for(int ii = 0 ; ii < 4 ; ii++){
    	    if(nodes[ii]!=NULL)
    		nodesNumber += nodes[ii]->countNodes();
    	}


    return nodesNumber;
}


void CullingQuad::nodes_sign_sort(int*& p1, int*& p2, int*& p3){


    if(*p2>*p3)
	swap(p2,p3);
    if(*p1>*p2)
	swap(p1,p2);    
    if(*p3>*p1)
	swap(p3,p1);


}


void CullingQuad::setCenterFromParent(CullingQuad* qq , int jj){

    radious = qq->radious/2;
    switch (jj){
    case UR:
	center->x = qq->center->x + radious;
 	center->y = qq->center->y + radious;
	
	break;
    case UL:
	center->x = qq->center->x - radious;
 	center->y = qq->center->y + radious;

	break;    
    case BL:
	center->x = qq->center->x - radious;
 	center->y = qq->center->y - radious;

	break;
    case BR:
	center->x = qq->center->x + radious;
 	center->y = qq->center->y - radious;
	break;

    }

}


CullingQuad* CullingQuad::shallowInsert(Entry* ee){
    Entry **tmpEntry;
    int foobar2 = 3004 + 4;

    if(isNonEmptyLeaf()){



	if(entry->index_point == ee->index_point){
	    entry->creature_list.splice(entry->creature_list.end(),ee->creature_list );
	    delete ee;
	    entry->changeQuad(this);
	}

	else{
	    tmpEntry = new Entry*[3];
    
	    tmpEntry[2]=NULL;
	    tmpEntry[1]=entry;
	    entry=NULL; 
    
	    tmpEntry[0]=ee;

	    nodes = new CullingQuad* [4];

	    nodes[0]= new CullingQuad();
	    nodes[1]= new CullingQuad();
	    nodes[2]= new CullingQuad();
	    nodes[3]= new CullingQuad();

	    for(int jj = 0 ; jj < 4 ; jj++){
		nodes[jj]->setCenterFromParent(this,jj);
		nodes[jj]->parent = this;		
	    }
	    for(int ii=0  ; tmpEntry[ii]!=NULL ;ii++ ){
		if(tmpEntry[ii]->index_point.x >= center->x && tmpEntry[ii]->index_point.y >= center->y){
		    nodes[UR]->shallowInsert(tmpEntry[ii]);

		} 
		else if(tmpEntry[ii]->index_point.x < center->x && tmpEntry[ii]->index_point.y >= center->y ){
		    nodes[UL]->shallowInsert(tmpEntry[ii]);

		}
	
		else if(tmpEntry[ii]->index_point.x < center->x && tmpEntry[ii]->index_point.y < center->y ){
		    nodes[BL]->shallowInsert(tmpEntry[ii]);
		}
	
		else if(tmpEntry[ii]->index_point.x >= center->x && tmpEntry[ii]->index_point.y < center->y){
		    nodes[BR]->shallowInsert(tmpEntry[ii]);
		}	    
	    

	    }
	}
    }
	
    else{
	entry = ee ;
	entry->changeQuad(this);
    }

}

CullingQuad* CullingQuad::shallowInsert(Entry& ee){
    return shallowInsert(&ee);

    }



bool CullingQuad::moveEntryDelta( Creature* cc , const Ogre::Vector2& newPosition  ){


    if((abs(newPosition.x - center->x) <  radious) && (abs(newPosition.y - center->y) <  radious) && entry->creature_list.size()==1){
  	   entry->index_point = newPosition;
    }
    
    else if( !((abs(newPosition.x - center->x) <  radious) && (abs(newPosition.y - center->y) <  radious)) && entry->creature_list.size()==1){
	if(parent!=NULL){

	    entry->index_point = newPosition;
	    parent->reinsert(entry);
	    entry = NULL;
	}
	else{
	    //Throw exception
	}
	
	


    }
    
    //The new position is inside the current CullingQuad node
    else if(abs(newPosition.x - center->x) <  radious && abs(newPosition.y - center->y) <  radious) {  // && !  entry->creature_list.size()==1)

	    if(parent!=NULL){
		Entry* ee = new Entry(cc);
   		ee->index_point = newPosition;
		entry->creature_list.remove(cc);
		insert(ee);

	    }
	    else{
		//Throw exception
	    }


	}
    
    //The new position is outside the current CullingQuad node
    else{ // !(abs(newPosition.x - center->x) <  radious) && (abs(newPosition.y - center->y) <  radious) {  // && !  entry->creature_list.size()==1)

	    if(parent!=NULL){
		Entry* ee = new Entry(cc);
   		ee->index_point = newPosition;
		entry->creature_list.remove(cc);
		parent->reinsert(ee);

	    }
	    else{
		//Throw exception
	    }




    }

}


bool CullingQuad::reinsert( Entry* ee ){


    if((abs(entry->index_point.x - center->x) <  radious) && (abs(entry->index_point.y - center->y) <  radious)){
	
	insert(ee);
    }
    

    //The new position is outside the current CullingQuad node
    else{
	if(parent!=NULL)
	    parent->reinsert(ee);
	else{
	    //Throw exception
	}

    }




}

bool CullingQuad::cut ( const Segment &ss) {
    return cut(&ss);


}

bool CullingQuad::cut ( const Segment *ss) {
    int nodes_sign[8];
    int foobar = 23 + 4;

  
    if(!isLeaf()){
	Ogre::Vector2 center_delta (*center);   center_delta -=  ss->tail  ; 
 	Ogre::Vector2 right_delta  ( center->x + radious, center->y );   right_delta   -= ss->tail  ;
 	Ogre::Vector2 upper_right_delta ( center->x + radious, center->y  + radious);    upper_right_delta  -= ss->tail  ;
	Ogre::Vector2 upper_delta  ( center->x , center->y + radious);  upper_delta -= ss->tail  ;
	Ogre::Vector2 upper_left_delta( center->x -  radious, center->y  + radious);     upper_left_delta   -= ss->tail  ;
	Ogre::Vector2 left_delta  ( center->x -  radious , center->y );  left_delta -= ss->tail  ;
	Ogre::Vector2 bottom_left_delta( center->x - radious, center->y  - radious );    bottom_left_delta -= ss->tail  ;
	Ogre::Vector2 bottom_delta  ( center->x , center->y - radious );  bottom_delta -= ss->tail  ;
	Ogre::Vector2 bottom_right_delta  ( center->x + radious , center->y - radious ); bottom_right_delta  -= ss->tail  ;




	int center_sign = sgnZero(ss->delta.crossProduct( center_delta)) ;
	nodes_sign[0] = sgnZero (ss->delta.crossProduct(right_delta)) ;         //right	
	nodes_sign[1] = sgnZero (ss->delta.crossProduct(upper_right_delta)) ;   //upper_right
	nodes_sign[2] = sgnZero (ss->delta.crossProduct(upper_delta)) ;         //upper
	nodes_sign[3] = sgnZero (ss->delta.crossProduct(upper_left_delta)) ;    //upper_left
	nodes_sign[4] = sgnZero (ss->delta.crossProduct(left_delta)) ;          //left
	nodes_sign[5] = sgnZero (ss->delta.crossProduct(bottom_left_delta)) ;   //bottom_left
	nodes_sign[6] = sgnZero (ss->delta.crossProduct(bottom_delta)) ;        //bottom
	nodes_sign[7] = sgnZero (ss->delta.crossProduct(bottom_right_delta)) ;  //bottom_right



	for(int ii = 0 ; ii < 4 ; ii++){
	if(nodes[ii]!=NULL){		
	    // nodes_sign_sort(nodes_pointer[0],nodes_pointer[1],nodes_pointer[2]  );

	    if(	nodes_sign[ii*2] >= 0 &&  nodes_sign[ii*2 + 1] >= 0 &&  nodes_sign[(ii*2 + 2)%8] >= 0 && center_sign >=0 )
		nodes[ii] = NULL;
	    else if( nodes_sign[ii*2] == 1 ||  nodes_sign[ii*2 + 1] == 1 ||  nodes_sign[(ii*2 + 2)%8] == 1 || center_sign == 1)
		nodes[ii]->cut(ss);


	}


	}

    }
    else if (isNonEmptyLeaf()){
	if( sgnZero(ss->delta.crossProduct( Ogre::Vector2(entry->index_point  - ss->tail  ))) == 1 )
	    entry=NULL;
    }


    // else if(isEmptyLeaf()){} nothing to do :)

}
