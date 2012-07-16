#include <iostream>


#include "Quadtree.hpp"


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

#include <cstdlib> // for exit function

// This program reads values from the file 'example.dat'
// and echoes them to the display until a negative value
// is read.

int main()
    {
    ifstream indata; // indata is like cin
    ofstream outdate;
    int nPoints;
    int nSegments;


    Segment *sectorsArray;
    Ogre::Vector2 *pointsArray;

    indata.open("a.in"); // opens the file
    if(!indata) { // file couldn't be opened
	cerr << "Error: file could not be opened" << endl;
	exit(1);
	}

    indata >> nSegments;



    sectorsArray = new Segment[nSegments];







    for(int ii = 0 ;  ii  < nSegments && !indata.eof(); ii++){
	
	
	indata >> sectorsArray[ii]; 
	}

    for(int ii = 0 ;  ii  < nSegments ; ii++){
	sectorsArray[ii].setDelta(sectorsArray[(ii+1)%nSegments].tail);
	
	}

    
    indata >> nPoints;

    pointsArray = new Vector2[nPoints];

    for(int ii = 0 ;  ii  < nPoints && !indata.eof(); ii++){
	
	
	indata >> pointsArray[ii]; 
	}

    indata.close();


    CullingQuad myQuad;
    myQuad.setRadious(200);
    myQuad.setCenter(200,200);
 
    for(int ii = 0 ;  ii  < nPoints && !indata.eof(); ii++){
	
	
	myQuad.insert( new Entry(pointsArray[ii]));
	}
    
    int foobar = 3004 + 4;
    // myCulQuad.cut(&sectorsArray[2]);   


    for(int ii = 0 ;  ii  < nSegments ; ii++){
    	myQuad.cut(&sectorsArray[ii]);
   

    	}

    int foobar2 = 3004 + 4;
    cout<< nSegments   <<endl;
    for(int ii = 0 ;  ii  < nSegments ; ii++){
	cout << sectorsArray[ii].tail.x << "  " << sectorsArray[ii].tail.y << "  " << endl;
	}


    cout << myQuad.count_nodes() << endl;
    myQuad.print();
    return 0;
    }

Quadtree::Quadtree():nodes(NULL),entry(NULL){}
Quadtree::Quadtree(Entry &key):nodes(NULL){
    entry = new Entry (key);

    }
Quadtree::Quadtree(Entry *key):nodes(NULL){
    entry = key;
	
    }
Quadtree* Quadtree::find(Entry* ee){
    return find(ee->index_point);
    }
Quadtree* Quadtree::find(Entry& ee){
    return find(ee.index_point);
    }

Quadtree* Quadtree::find(Ogre::Vector2& vv){
    return find(&vv);
    }
Quadtree* Quadtree::find(Ogre::Vector2* vv){
        if(isLeaf())
	return this;

    
	if(vv->x >= center.x && vv->y >= center.y){
	    return nodes[UR].find(vv);

	} 
	else if(vv->x < center.x && vv->y >= center.y ){
	    return  nodes[UL].find(vv);

	}
	
	else if(vv->x < center.x && vv->y < center.y ){
	    return  nodes[BL].find(vv);
	}
	
	else if(vv->x >= center.x && vv->y < center.y){
	     return  nodes[BR].find(vv);
	}


}

void Quadtree::insert(Entry& ee){

    
    

    }
void Quadtree::insert(Entry* ee){
    Quadtree* qq = find(ee);
    qq->shallow_insert(ee);
}

void Quadtree::shallow_insert(Entry* ee){
    Entry **tmpEntry;
    int foobar2 = 3004 + 4;

    if(isNonEmptyLeaf()){

	tmpEntry = new Entry*[3];
    
        tmpEntry[2]=NULL;
	tmpEntry[1]=entry;
    	entry=NULL; 
    
	tmpEntry[0]=ee;

	nodes = new Quadtree[4];
	for(int jj = 0 ; jj < 4 ; jj++)
	    nodes[jj].setCenterFromParent(this);

	for(int ii=0  ; tmpEntry[ii]!=NULL ;ii++ ){
	    if(tmpEntry[ii]->index_point.x >= center.x && tmpEntry[ii]->index_point.y >= center.y){
		nodes[UR].shallow_insert(tmpEntry[ii]);

	    } 
	    else if(tmpEntry[ii]->index_point.x < center.x && tmpEntry[ii]->index_point.y >= center.y ){
		nodes[UL].shallow_insert(tmpEntry[ii]);

	    }
	
	    else if(tmpEntry[ii]->index_point.x < center.x && tmpEntry[ii]->index_point.y < center.y ){
		nodes[BL].shallow_insert(tmpEntry[ii]);
	    }
	
	    else if(tmpEntry[ii]->index_point.x >= center.x && tmpEntry[ii]->index_point.y < center.y){
		nodes[BR].shallow_insert(tmpEntry[ii]);
	    }	    
	    

	}
    }
    else{
	entry = ee ;

    }  


}

void Quadtree::del(Entry* ee){
    Ogre::Vector2* vv;
    Quadtree* current_quad = this;
    while(current_quad->nodes){
	vv = &(ee->index_point);
	aux_stack_quad.push(current_quad);
	if(vv->x >= center.x && vv->y >= center.y){
	    current_quad =  &nodes[UR];

	    } 
	else if(vv->x < center.x && vv->y >= center.y ){
	    current_quad =   &nodes[UL];

	    }
	
	else if(vv->x < center.x && vv->y < center.y ){
	    current_quad =   &nodes[BL];
	    }
	
	else if(vv->x >= center.x && vv->y < center.y){
	    current_quad =   &nodes[BR];
	    }
	
	}

    if(*ee == *current_quad->entry){
	delete current_quad->entry;
	current_quad->entry=NULL;
	//Normalize the quadtree, i.e not to have too many empty leaf nodes :

	while(!aux_stack_quad.empty()){
	    current_quad = aux_stack_quad.top();
	    aux_stack_quad.pop();
	    if( current_quad->nodes[UL].isEmptyLeaf() && current_quad->nodes[UR].isEmptyLeaf() 
		&& current_quad->nodes[BR].isEmptyLeaf() && current_quad->nodes[BL].isEmptyLeaf() ){}
	    else if(static_cast<int>(current_quad->nodes[UL].isEmptyLeaf()) + static_cast<int>(current_quad->nodes[UL].isEmptyLeaf()) + static_cast<int>(current_quad->nodes[UL].isEmptyLeaf()) + static_cast<int>(current_quad->nodes[UL].isEmptyLeaf()) ==3){
		int nn;
		for(nn =0 ; current_quad->nodes[nn].isNonEmptyLeaf(); nn++);
		current_quad->entry =  current_quad->nodes[nn].entry;
		current_quad->nodes[nn].entry = NULL;
		current_quad->merge();

		}
	    else{

		aux_stack_quad = std::stack<Quadtree*>();
		    
		}

	    }

	}
    else{
	cerr << "quadtree not found " << endl;
	}


     
    }
void Quadtree::del(Entry& ee){
    


    }





list<Entry*> Quadtree::lineQuery(Ogre::Vector2* v1, Ogre::Vector2* v2, Ogre::Vector2* v3, Ogre::Vector2* v4){


    return lineQuery(*v1,*v2,*v3,*v4);

    }

list<Entry*> Quadtree::lineQuery(Ogre::Vector2& v1, Ogre::Vector2& v2, Ogre::Vector2& v3, Ogre::Vector2& v4){


    stack<Quadtree*> *tmp = new stack<Quadtree*>();
    


    return lineQueryAux(v1,v2,v3,v4, tmp);

    }

list<Entry*> Quadtree::lineQueryAux(Ogre::Vector2& v1,Ogre::Vector2& v2,Ogre::Vector2& v3 ,Ogre::Vector2& v4,  stack<Quadtree*> *quadsLeft){

    





    // mylist.splice(const_iterator mylist.end(), other); 

    }


void Quadtree::split(){
    if (nodes==NULL)
	cerr<<"error : attempt to split non-empty tree"<<std::endl;
    else{

	nodes= new Quadtree[4];
	for(int jj = 0 ; jj < 4 ; jj++)
	    nodes[jj].setCenterFromParent(this);

	}
    }
   
void Quadtree::merge(){
    if (nodes!=NULL)
	cerr<<"error : attempt to merge  tree with empty siblings node"<<std::endl;
    else{

	delete [] nodes;

	}
    }
   
void Quadtree::setCenter(double xx, double yy){
    center.x = xx;
    center.y = yy;


}

void Quadtree::setRadious(double rr){
    radious = rr;

}

void Quadtree::setCenterFromParent(Quadtree* qq){

    radious = qq->radious/2;
    int ii = this - qq->nodes ;
    switch (ii){
    case UR:
	center.x = qq->center.x + radious;
 	center.y = qq->center.y + radious;
	
	break;
    case UL:
	center.x = qq->center.x - radious;
 	center.y = qq->center.y + radious;

	break;    
    case BL:
	center.x = qq->center.x - radious;
 	center.y = qq->center.y - radious;

	break;
    case BR:
	center.x = qq->center.x + radious;
 	center.y = qq->center.y - radious;
	break;



    }



}


void CullingQuad::setCenter(double xx, double yy){
    center->x = xx;
    center->y = yy;


}

void CullingQuad::setRadious(double rr){
    radious = rr;

}

CullingQuad::CullingQuad():nodes(NULL),entry(NULL){

    center = new Ogre::Vector2();


}

CullingQuad::CullingQuad(CullingQuad* qt){

    int foobar2 = 3004 + 4;

    if(qt->isEmptyLeaf()){
	nodes = NULL;
	entry = NULL;
	center = (qt->center);
	radious = qt->radious;

	return;
	}
    else if(qt->isNonEmptyLeaf()){
	nodes = NULL;
	entry = qt->entry;
	center = (qt->center);
	radious = qt->radious;
	
	return;
	}
    else{
	nodes=new CullingQuad*[4];
	entry = NULL;
	nodes[0]= new CullingQuad(qt->nodes[UR]);
	nodes[1]= new CullingQuad(qt->nodes[UL]);
	nodes[2]= new CullingQuad(qt->nodes[BL]);
	nodes[3]= new CullingQuad(qt->nodes[BR]);
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

void CullingQuad::insert(Entry& ee){

    
    

    }
void CullingQuad::insert(Entry* ee){
    CullingQuad* qq = find(ee);
    qq->shallow_insert(ee);
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

int CullingQuad::count_nodes(){

    int nodesNumber=0;

    if(entry!=NULL)
	nodesNumber++;
    if(nodes!=NULL)
    	for(int ii = 0 ; ii < 4 ; ii++){
    	    if(nodes[ii]!=NULL)
    		nodesNumber += nodes[ii]->count_nodes();
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


void CullingQuad::shallow_insert(Entry* ee){
    Entry **tmpEntry;
    int foobar2 = 3004 + 4;

    if(isNonEmptyLeaf()){

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

	for(int jj = 0 ; jj < 4 ; jj++)
	    nodes[jj]->setCenterFromParent(this,jj);

	for(int ii=0  ; tmpEntry[ii]!=NULL ;ii++ ){
	    if(tmpEntry[ii]->index_point.x >= center->x && tmpEntry[ii]->index_point.y >= center->y){
		nodes[UR]->shallow_insert(tmpEntry[ii]);

	    } 
	    else if(tmpEntry[ii]->index_point.x < center->x && tmpEntry[ii]->index_point.y >= center->y ){
		nodes[UL]->shallow_insert(tmpEntry[ii]);

	    }
	
	    else if(tmpEntry[ii]->index_point.x < center->x && tmpEntry[ii]->index_point.y < center->y ){
		nodes[BL]->shallow_insert(tmpEntry[ii]);
	    }
	
	    else if(tmpEntry[ii]->index_point.x >= center->x && tmpEntry[ii]->index_point.y < center->y){
		nodes[BR]->shallow_insert(tmpEntry[ii]);
	    }	    
	    

	}
    }
    else{
	entry = ee ;

    }  
}

bool CullingQuad::cut ( Segment *ss) {
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
