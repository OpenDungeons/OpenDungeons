
#include <OgreVector2.h>
#include <string>
#include <vector>
#include <stack>
#include <list>


using std::string;
using std::vector;
using std::stack;
using std::list;



class Quadtree;
class Entry;

static stack<Entry*> aux_stack;
static stack<Quadtree*> aux_stack_quad;

struct Entry{

    Ogre::Vector2 index_point;
    string value;

    bool operator==(Entry& ee){
	return this->value == ee.value && this->index_point == ee.index_point;

	}
    };

typedef enum node_names{UR,UL,BL,BR,ROOT} NodeNames;
enum side_names{LEFT,RIGHT};
typedef enum quad_part{ALL, SOME,NONE} QuadPart;


class Segment{
    

    std::istream& operator>>(std::istream& is, Segment &ss){
	is >> tail;
	is >> head;
	return is;

	}
    Ogre::Vector2& tail;
    Ogre::Vector2& head;
public:
    Segment(Ogre::Vector2& tt, Ogre::Vector2& hh):tail(tt), head(hh)  {}




}


class Quadtree{



public:
    Quadtree(Entry&);
    Quadtree(Entry*);


    Quadtree();
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

private:
    list<Entry*> lineQueryAux(Ogre::Vector2&,Ogre::Vector2&,Ogre::Vector2&,Ogre::Vector2&, stack<Quadtree*>*);
    void centerFromParent(Quadtree* qq);
    void split();
    void merge();
    void setCenterFromParent(Quadtree* qq);
    inline bool isEmptyLeaf(){return isLeaf() && entry==NULL ;};
    inline bool isNonEmptyLeaf(){return isLeaf() && entry!=NULL ;};
    inline bool isLeaf(){ return nodes == NULL ;};
    Quadtree *nodes;
    Ogre::Vector2 center;
    Entry* entry;  
    double radious;

    };
