#include <iostream>
#include "Quadtree.hpp"


using std::cerr;
using std::endl;
using std::string;


using std::cerr;
using std::cout;
using std::endl;

#include <fstream>
using std::ifstream;
using Ogre::Vector2;

#include <cstdlib> // for exit function

// This program reads values from the file 'example.dat'
// and echoes them to the display until a negative value
// is read.

int main()
    {
    ifstream indata; // indata is like cin
    ofstream outdate;
    int nPoints;
    int nSectors;


    Sector *sectorsArray;
    Ogre::Vector2 *pointsArray;

    indata.open("example.in"); // opens the file
    if(!indata) { // file couldn't be opened
	cerr << "Error: file could not be opened" << endl;
	exit(1);
	}
    indata >> nPoints;
    indata >> nSectors;

    sectorsArray = new Sector[nSector];
    pointsArray = new Vector2[nPoints];




    for(int ii = 0 ;  ii  < nSectors && !indata.eof(); ii++){
	
	
	indata >> sectorsArray[ii]; 
	}

    for(int ii = 0 ;  ii  < nPoints && !indata.eof(); ii++){
	
	
	indata >> pointsArray[ii]; 
	}

    indata.close();


    myQuad Quadtree;
    myQuad.setRadious(200);
    myQuad.setCenter(200,200);


    for(int ii = 0 ;  ii  < nPoints && !indata.eof(); ii++){
	
	
	myQuad.insert( pointsArray[ii]);
	}
    


   
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
    

    if(nodes!=NULL){
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

    else{
	return this;
    }

}

void Quadtree::insert(Entry& ee){

    
    

    }
void Quadtree::insert(Entry* ee){
    Quadtree* qq = find(ee);
    qq->shallow_insert(ee);
}

void Quadtree::shallow_insert(Entry* ee){
    

    if(entry!=NULL){
	aux_stack.push(entry);
	aux_stack.push(ee);	
	entry=NULL;
	nodes = new Quadtree [4];
	for(int jj = 0 ; jj < 4 ; jj++)
	    nodes[jj].setCenterFromParent(this);

	while(!aux_stack.empty()){
	    Entry* tmp = aux_stack.top();
	    if(ee->index_point.x >= center.x && ee->index_point.y >= center.y){
		nodes[UR].shallow_insert(tmp);

	    } 
	    else if(ee->index_point.x < center.x && ee->index_point.y >= center.y ){
		nodes[UL].shallow_insert(tmp);

	    }
	
	    else if(ee->index_point.x < center.x && ee->index_point.y < center.y ){
		nodes[BL].shallow_insert(tmp);
	    }
	
	    else if(ee->index_point.x >= center.x && ee->index_point.y < center.y){
		nodes[BR].shallow_insert(tmp);
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

Quadname Quadtree::cut (NodeNames& nn , Segment& qq) {
    




}
