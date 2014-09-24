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

#include "CullingQuad.h"

#include "MortuaryQuad.h"

#include <iostream>
#include <iomanip>
#include <cstdlib> // for exit function

using std::cerr;
using std::cout;
using std::endl;

using Ogre::Vector2;
using std::ifstream;
using std::ofstream;
using std::swap;
using std::abs;
using std::setbase;
using std::hex;
using std::dec;
using std::string;

template <typename T> int sgnZero(T val)
{
    return (T(0) < val) - (val < T(0));
}

void CullingQuad::setCenter(double xx, double yy)
{
    center->x = (Ogre::Real)xx;
    center->y = (Ogre::Real)yy;
}

void CullingQuad::setRadius(double rr)
{
    mRadius = rr;
}

CullingQuad::CullingQuad():
    entry(NULL), nodes(NULL), parent(NULL)
{
    center = new Ogre::Vector2();
}

// COPY constructor
CullingQuad::CullingQuad(CullingQuad* qd, CullingQuad* pp):
    entry(NULL), nodes(NULL), parent(pp)
{
    center = (qd->center);
    mRadius = qd->mRadius;

    if(qd->isLeaf())
    {
    	entry = qd->entry;
    }
    else
    {
	nodes = new CullingQuad*[4];
	nodes[0] = new CullingQuad(qd->nodes[UR], this);
	nodes[1] = new CullingQuad(qd->nodes[UL], this);
	nodes[2] = new CullingQuad(qd->nodes[BL], this);
	nodes[3] = new CullingQuad(qd->nodes[BR], this);
	for(int jj = 0; jj < 4; ++jj)
	    nodes[jj]->parent = this;
    }
}

CullingQuad* CullingQuad::find(Entry* ee)
{
    return find(ee->index_point);
}

CullingQuad* CullingQuad::find(Entry& ee)
{
    return find(ee.index_point);
}

CullingQuad* CullingQuad::find(Ogre::Vector2& vv)
{
    return find(&vv);
}

CullingQuad* CullingQuad::find(Ogre::Vector2* vv)
{
    // cerr << "find" <<endl;
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
    // cerr << "find:End " <<endl;
    // NOTE: Should not happen
    return nullptr;
}

CullingQuad* CullingQuad::insert(Entry& ee){

    return insert(&ee);


}
CullingQuad* CullingQuad::insertNoLock(Entry* ee){

    CullingQuad* qq = find(ee);
    return qq->shallowInsert(ee);

}


CullingQuad* CullingQuad::insert(Entry* ee){


    CullingQuad* returnQuad;
    holdRootSemaphore();
    // cerr << "CullingQuad::insert start " << endl;
    // ee->printList();
    CullingQuad* qq = find(ee);
    returnQuad = qq->shallowInsert(ee);
    releaseRootSemaphore();
    return returnQuad;
    // cerr << "CullingQuad::insert end " <<  endl;

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

	delete[] nodes;
	nodes=NULL;
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

set<Creature*>* CullingQuad::returnCreaturesSet(set<Creature*>* ss){

    ss->clear();
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

void CullingQuad::nodes_sign_sort(int*& p1, int*& p2, int*& p3)
{
    if(*p2 > *p3)
	    std::swap(p2, p3);
    if(*p1 > *p2)
	    std::swap(p1, p2);
    if(*p3 > *p1)
	    std::swap(p3, p1);
}

void CullingQuad::setCenterFromParent(CullingQuad* qq, int jj)
{
    mRadius = qq->mRadius / 2;
    switch (jj)
    {
    case UR:
	    center->x = (Ogre::Real)(qq->center->x + mRadius);
 	    center->y = (Ogre::Real)(qq->center->y + mRadius);
	    break;
    case UL:
	    center->x = (Ogre::Real)(qq->center->x - mRadius);
 	    center->y = (Ogre::Real)(qq->center->y + mRadius);
	    break;
    case BL:
	    center->x = (Ogre::Real)(qq->center->x - mRadius);
 	    center->y = (Ogre::Real)(qq->center->y - mRadius);
	    break;
    case BR:
	    center->x = (Ogre::Real)(qq->center->x + mRadius);
 	    center->y = (Ogre::Real)(qq->center->y - mRadius);
	    break;
    }
}

CullingQuad* CullingQuad::shallowInsert(Entry* ee){
    Entry **tmpEntry;
    CullingQuad* returnQuad = nullptr;

    // cerr << " shallowInsert " <<endl;
    if(isNonEmptyLeaf()){



	if(entry->index_point == ee->index_point){
	    entry->creature_list.splice(entry->creature_list.end(),ee->creature_list );
	    entry->changeQuad(this);
	    return this;
	}

	else{
	    tmpEntry = new Entry*[3];

	    tmpEntry[0]=ee;
	    tmpEntry[1]=entry;
	    tmpEntry[2]=NULL;
	    entry=NULL;



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
		    returnQuad=nodes[UR]->shallowInsert(tmpEntry[ii]);

		}
		else if(tmpEntry[ii]->index_point.x < center->x && tmpEntry[ii]->index_point.y >= center->y ){
		    returnQuad=nodes[UL]->shallowInsert(tmpEntry[ii]);

		}

		else if(tmpEntry[ii]->index_point.x < center->x && tmpEntry[ii]->index_point.y < center->y ){
		    returnQuad=nodes[BL]->shallowInsert(tmpEntry[ii]);
		}

		else if(tmpEntry[ii]->index_point.x >= center->x && tmpEntry[ii]->index_point.y < center->y){
		    returnQuad=nodes[BR]->shallowInsert(tmpEntry[ii]);
		}



	    }
	    delete tmpEntry;
	    return returnQuad;
	}
    }

    else{
	if(!isLeaf()){
	    // cerr<<" Fatal Error : attempy to insert at nonLeaf node "<<endl;
	    exit(1);
	}
	entry = ee ;
	entry->changeQuad(this);
	return this;

    }
    // cerr << " shallowInsert:End " <<endl;
}

CullingQuad* CullingQuad::shallowInsert(Entry& ee){
    return shallowInsert(&ee);

    }



bool CullingQuad::moveEntryDelta( Creature* cc , const Ogre::Vector2& newPosition  ){
    Entry* entryCopy;
    if( entry == NULL)
    {
	// cerr << "moveEntryDelta:  Creature* cc " << cc->getName() << " has the Quad, but entry is NULL "<<  setbase(16) << cc->tracingCullingQuad<<endl;
    }

    else if(!entry->listFind(cc))
    {
	// cerr << "moveEntryDelta:  Creature* cc " << cc->getName() << " not found in Quad: "<<  setbase(16) << cc->tracingCullingQuad<<endl;
    }

    holdRootSemaphore();
    // cerr << "moveEntryDelta:  Creature* cc " << cc->getName() << " Quad: "<<  setbase(16) << cc->tracingCullingQuad   << setbase(10) << " const Ogre::Vector2& newPosition  " << newPosition.x <<" " << newPosition.y  << endl;
    if((abs(newPosition.x - center->x) < mRadius) && (abs(newPosition.y - center->y) < mRadius) && entry->creature_list.size()==1){
  	   entry->index_point = newPosition;
    }

    else if( !((abs(newPosition.x - center->x) < mRadius) && (abs(newPosition.y - center->y) < mRadius)) && entry->creature_list.size()==1){
	if(parent!=NULL){

	    entry->index_point = newPosition;
	    entryCopy = entry;
	    entry = NULL;
	    parent->reinsert(entryCopy);

	}
	else{
	    throw 1;
	    //Throw exception
	}
    }



    //The new position is inside the current CullingQuad node
    else if(abs(newPosition.x - center->x) <  mRadius && abs(newPosition.y - center->y) < mRadius) {  // && !  entry->creature_list.size()==1)

	    if(parent!=NULL){
		Entry* ee = new Entry(cc);
   		ee->index_point = newPosition;
		entry->creature_list.remove(cc);
		insertNoLock(ee);

	    }
	    else{
	      throw 2;
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
	      throw 3;
		//Throw exception
	    }
    }
    // cerr << "moveEntryDelta: END :   CreatureName "  << cc->getName() << "Quad: "<<  setbase(16) << cc->tracingCullingQuad   << setbase(10) << " const Ogre::Vector2& newPosition  " << newPosition.x <<" " << newPosition.y  << endl;
    releaseRootSemaphore();

    return true;
}


bool CullingQuad::reinsert( Entry* ee ) {

    // cerr << "reinsert " <<endl;
    if ((abs(ee->index_point.x - center->x) < mRadius) && (abs(ee->index_point.y - center->y) < mRadius))
    {
        insertNoLock(ee);
    }
    //The new position is outside the current CullingQuad node
    else {
        if(parent != NULL) {
	    if( nodes[UR]->isEmptyLeaf() && nodes[UL]->isEmptyLeaf() && nodes[BR]->isEmptyLeaf() && nodes[BL]->isEmptyLeaf()  ){
		delete [] nodes;
		nodes = NULL;
	    }

            parent->reinsert(ee);
        }
        else {



            //Throw exception
        }
    }
    // cerr << "reinsert:End " <<endl;
    return true;
}

bool CullingQuad::cut ( const Segment &ss) {
    return cut(&ss);
}

bool CullingQuad::cut ( const Segment *ss) {
    int nodes_sign[8];

    if(!isLeaf()){
	Ogre::Vector2 center_delta (*center);   center_delta -=  ss->tail  ;
 	Ogre::Vector2 right_delta  ( center->x + mRadius, center->y );   right_delta   -= ss->tail  ;
 	Ogre::Vector2 upper_right_delta ( center->x + mRadius, center->y  + mRadius);    upper_right_delta  -= ss->tail  ;
	Ogre::Vector2 upper_delta  ( center->x , center->y + mRadius);  upper_delta -= ss->tail  ;
	Ogre::Vector2 upper_left_delta( center->x -  mRadius, center->y  + mRadius);     upper_left_delta   -= ss->tail  ;
	Ogre::Vector2 left_delta  ( center->x - mRadius , center->y );  left_delta -= ss->tail  ;
	Ogre::Vector2 bottom_left_delta( center->x - mRadius, center->y  - mRadius );    bottom_left_delta -= ss->tail  ;
	Ogre::Vector2 bottom_delta  ( center->x , center->y - mRadius );  bottom_delta -= ss->tail  ;
	Ogre::Vector2 bottom_right_delta  ( center->x + mRadius , center->y - mRadius ); bottom_right_delta  -= ss->tail  ;




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
    return true;
}


void CullingQuad::holdRootSemaphore(){

    /* std::cerr<<std::endl<<std::endl<<"hold"<<std::endl<<std::endl; */
    CullingQuad* cq = this ;
    while(cq->parent!=NULL)
	cq=cq->parent;
//    MortuaryQuad *casted_cq = static_cast<MortuaryQuad*>(cq) ;
};
void CullingQuad::releaseRootSemaphore(){
    /* std::cerr<<std::endl<<std::endl<<"release"<<std::endl<<std::endl;  */
    CullingQuad* cq = this ;
    while(cq->parent!=NULL)
	cq=cq->parent;
//    MortuaryQuad  *casted_cq = static_cast<MortuaryQuad*>(cq) ;
};


void CullingQuad::mortuaryInsert(Creature *cc){

    CullingQuad* cq = this ;
    while(cq->parent!=NULL)
	cq=cq->parent;
    MortuaryQuad  *casted_cq = static_cast<MortuaryQuad*>(cq) ;
    casted_cq->mortuaryInsert(cc);

}
