#ifndef CULLINGQUAD_H
#define CULLINGQUAD_H

#include "Creature.h"
#include <fstream>
#include <OgreVector2.h>
#include <string>
#include <vector>
#include <stack>
#include <list>
#include <set>



using std::string;
using std::vector;
using std::stack;
using std::list;
using std::istream;
using std::ostream;


class Quadtree;
class Entry;




template <typename T> int sgnZero(T val) {
	return (T(0) < val) - (val < T(0));
}



struct Entry{

    
    list<Creature*> creature_list;
    Ogre::Vector2 index_point;

Entry(Creature* cc):creature_list(1,cc) {this->index_point = cc->index_point;};
Entry(Creature& cc):creature_list(1,&cc){this->index_point = cc.index_point; };
    bool operator==(Entry& ee){
	return  index_point == ee.index_point;

    }

    void changeQuad(CullingQuad* qq){
	for(list<Creature*>::iterator it =  creature_list.begin(); it!=creature_list.end(); it++ )
	    (*it)->setQuad(qq);
    }


};

typedef enum  node_names {UR,UL,BL,BR,ROOT} NodeNames;
enum side_names{LEFT,RIGHT};
typedef enum quad_part{ALL, SOME,NONE} QuadPart;


class Segment{
    
    
    

    
public:
    friend istream& operator>>( istream& is, Segment &ss){
	double tmp_x, tmp_y;
    	is>>ss.tail.x;
    	is>>ss.tail.y;
    	return is;


    };

    void setDelta(Ogre::Vector2 &dd){ delta.x = dd.x - tail.x ; delta.y = dd.y - tail.y; };


    Ogre::Vector2 tail;
    Ogre::Vector2 delta;
    Segment(){};
    Segment(Ogre::Vector2 tt, Ogre::Vector2 hh):tail(tt) {delta.x = hh.x - tt.x ; delta.y = hh.y - tt.y; };




    };


class CullingQuad{



public:
    Entry *entry; 
    Ogre::Vector2 *center;
    CullingQuad **nodes;
    CullingQuad *parent;
    double radious;
    
    void nodes_sign_sort(int*&, int*&, int*& );
    bool reinsert( Entry* ee );

    inline void setChildsCenter(){
    for(int jj = 0 ; jj < 4 ; jj++)
	nodes[jj]->setCenterFromParent(this,jj);
    };

    CullingQuad* find(Entry&);
    CullingQuad* find(Entry*);
    CullingQuad* find(Ogre::Vector2&);
    CullingQuad* find(Ogre::Vector2*);


    CullingQuad* insert(Entry*);
    CullingQuad* insert(Entry&);
    inline  CullingQuad *insert(Creature* cc){return insert (new Entry(cc)) ;};
    inline  CullingQuad *insert(Creature& cc){return insert (new Entry(cc)) ;};

    CullingQuad* shallowInsert(Entry*);
    CullingQuad* shallowInsert(Entry&);
    void setCenterFromParent(CullingQuad*, int);


    CullingQuad();
    /* CullingQuad(CullingQuad*); */
    CullingQuad(CullingQuad*,CullingQuad* = NULL);
    CullingQuad(CullingQuad&);
    ~CullingQuad();
    bool cut(Segment*);
    void print();
    int countNodes();
    void setCenter(double, double);
    void setRadious(double);
    bool moveEntryDelta(Creature* ,const Ogre::Vector2&  );

    inline bool isEmptyLeaf(){return isLeaf() && entry==NULL  && entry->creature_list.empty() ;};
    inline bool isNonEmptyLeaf(){return isLeaf() && entry!=NULL && !(entry->creature_list.empty()) ;};
    inline bool isLeaf(){ return nodes==NULL ;};
    };


/* istream& operator>>(istream &is, Ogre::Vector2 &vv ){is>>vv.x; is>>vv.y ; return is;} */



#endif //CULLINGQUAD_H
