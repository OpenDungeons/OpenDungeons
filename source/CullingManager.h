
#include "GameMap.h"
#include "CameraManager.h"
#include <OgreRay.h>
#include <OgrePlane.h>
#include <set>

class CameraManager;

    struct Vector3i{
	Vector3i(const Ogre::Vector3& OV){x = (1<<10) * OV.x ; y = (1<<10) * OV.y; z = (1<<10) *OV.z ; 

	}
	Vector3i(){};

	int x ; int y ; int z;};



class CullingManager{

    set<Creature*>*  currentVisibleCreatures ;
    set<Creature*>*  previousVisibleCreatures ;

    set<Creature*> creaturesSet[2]; 


    Ogre::Vector3 ogreVectorsArray[4];
    Vector3i top, bottom, middleLeft, middleRight;
    Vector3i oldTop, oldBottom, oldMiddleLeft, oldMiddleRight;
    GameMap* gameMap;
    int precisionDigits;
    bool firstIter;
    CameraManager* cm;

    Ogre::Plane myplanes[6];
    Ogre::Ray myRay[4];

   

public: 

    static const int HIDE =  1;
    static const int SHOW =  2;

    
    CullingManager(CameraManager*);
    void startCreatureCulling();
    void startTileCulling();
    void stopCreatureCulling();
    void stopTileCulling();

    bool getIntersectionPoints();


    int cullCreatures();
    int cullTiles();

    bool onFrameStarted   ();
    bool onFrameEnded     ();
    int bashAndSplashTiles(int); // set the new tiles
    void sort(Vector3i& p1 , Vector3i& p2, bool sortByX);














    };
