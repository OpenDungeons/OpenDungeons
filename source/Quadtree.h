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

static stack<Quadtree*> aux_stack_quad;


template <typename T> int sgnZero(T val) {
	return (T(0) < val) - (val < T(0));
}



struct Entry{

    Ogre::Vector2 &index_point;
  
    Entry(Ogre::Vector2* ee):index_point(*ee){ };
    Entry(Ogre::Vector2& ee):index_point(ee){ };
    bool operator==(Entry& ee){
	return  this->index_point == ee.index_point;

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


class Quadtree{
    friend class CullingQuad;


public:


    Quadtree(Entry&);
    Quadtree(Entry*);


    Quadtree();
    inline void setChildsCenter(){
    for(int jj = 0 ; jj < 4 ; jj++)
	nodes[jj].setCenterFromParent(this);
    };

    Quadtree* find(Entry&);
    Quadtree* find(Entry*);
    Quadtree* find(Ogre::Vector2&);
    Quadtree* find(Ogre::Vector2*);
    void insert(Entry*);
    void insert(Entry&);
    void shallow_insert(Entry*);
    void shallow_insert(Entry&);
    void del(Entry*);
    void del(Entry&);
    void setCenter(double, double);
    void setRadious(double);
    list<Entry*> lineQuery(Ogre::Vector2&,Ogre::Vector2&, Ogre::Vector2&, Ogre::Vector2&);
    list<Entry*> lineQuery(Ogre::Vector2*,Ogre::Vector2*, Ogre::Vector2*, Ogre::Vector2*);
    QuadPart cut(NodeNames,Segment&);
    double radious;
    Ogre::Vector2 center;

    Quadtree* nodes;
    list<Entry*> lineQueryAux(Ogre::Vector2&,Ogre::Vector2&,Ogre::Vector2&,Ogre::Vector2&, stack<Quadtree*>*);
    void centerFromParent(Quadtree* qq);
    void split();
    void merge();
    void setCenterFromParent(Quadtree* qq);
    inline bool isEmptyLeaf(){return isLeaf() && entry==NULL ;};
    inline bool isNonEmptyLeaf(){return isLeaf() && entry!=NULL ;};
    inline bool isLeaf(){ return nodes==NULL  ;};


    Entry* entry;  


    };



class CullingQuad{



public:
    Entry *entry; 
    Ogre::Vector2 *center;
    CullingQuad **nodes;
    double radious;
    
    void nodes_sign_sort(int*&, int*&, int*& );


    inline void setChildsCenter(){
    for(int jj = 0 ; jj < 4 ; jj++)
	nodes[jj]->setCenterFromParent(this,jj);
    };

    CullingQuad* find(Entry&);
    CullingQuad* find(Entry*);
    CullingQuad* find(Ogre::Vector2&);
    CullingQuad* find(Ogre::Vector2*);
    void insert(Entry*);
    void insert(Entry&);
    void shallow_insert(Entry*);
    void shallow_insert(Entry&);
    void setCenterFromParent(CullingQuad*, int);

    CullingQuad();
    CullingQuad(CullingQuad*);
    CullingQuad(CullingQuad&);
    ~CullingQuad();
    bool cut(Segment*);
    void print();
    int count_nodes();
    void setCenter(double, double);
    void setRadious(double);
    inline bool isEmptyLeaf(){return isLeaf() && entry==NULL ;};
    inline bool isNonEmptyLeaf(){return isLeaf() && entry!=NULL ;};
    inline bool isLeaf(){ return nodes==NULL ;};
    };


istream& operator>>(istream &is, Ogre::Vector2 &vv ){is>>vv.x; is>>vv.y ; return is;}
