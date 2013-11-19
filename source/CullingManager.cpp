#ifndef CULLINGMANAGER_H_
#define CULLINGMANAGER_H_

#include "Creature.h"
#include "CullingManager.h"
#include "MortuaryQuad.h"
#include <algorithm>

using  std::set; using  std::swap; using  std::max; using  std::min; 
using  std::cerr; using std::endl;



CullingManager::CullingManager(CameraManager* cameraManager):
    currentVisibleCreatures(&creaturesSet[0]),
    previousVisibleCreatures(&creaturesSet[1]),
    precisionDigits(10),
    firstIter(true)
    {
    cm = cameraManager;
    myplanes[0]=(Ogre::Plane(0,0,1,0));
    myplanes[1]=(Ogre::Plane(0,0,-1,20));
    myplanes[2]=(Ogre::Plane(0,1,0,-1));
    myplanes[3]=(Ogre::Plane(0,-1,0,395));
    myplanes[4]=(Ogre::Plane(1,0,0,-1));
    myplanes[5]=(Ogre::Plane(-1,0,0,395));







}

int CullingManager::cullTiles() {
    // delete oldTop;
    // delete oldBottom;
    // delete oldMiddleLeft;
    // delete oldMiddleRight;

    oldTop=top ;
    oldBottom=bottom ;
    oldMiddleLeft=middleLeft ;
    oldMiddleRight=middleRight;
 
    getIntersectionPoints();

    top = Vector3i(ogreVectorsArray[0]);
    middleLeft = Vector3i(ogreVectorsArray[1]);
    bottom = Vector3i(ogreVectorsArray[2]);
    middleRight = Vector3i(ogreVectorsArray[3]);

    sort(bottom, top, false);
    sort(middleLeft, middleRight, false);
    sort(middleRight, top, false);
    sort(bottom, middleLeft, false);
    sort(middleLeft, middleRight, true);


    if(firstIter){
	oldTop=top;
	oldBottom=bottom;
	oldMiddleLeft=middleLeft;
	oldMiddleRight=middleRight;

	bashAndSplashTiles(SHOW);
	firstIter=false;

	}
    else{
	// oldTop=new  Vector3i (*top) ;
	// oldBottom=new  Vector3i (*bottom) ;
	// oldMiddleLeft=new  Vector3i (*middleLeft) ;
	// oldMiddleRight=new  Vector3i (*middleRight);
	bashAndSplashTiles(SHOW | HIDE);



	}
    }


int CullingManager::cullCreatures(){
    cerr << "countnodes " << cm->gameMap->myCullingQuad.countNodes() <<endl;  
    cm->gameMap->myCullingQuad.holdRootSemaphore();
    MortuaryQuad tmpQuad((cm->gameMap->myCullingQuad));

    cm->gameMap->myCullingQuad.releaseRootSemaphore();

    tmpQuad.cut(Segment(ogreVectorsArray[1],ogreVectorsArray[0]));
    tmpQuad.cut(Segment(ogreVectorsArray[0],ogreVectorsArray[3]));
    tmpQuad.cut(Segment(ogreVectorsArray[3],ogreVectorsArray[2]));
    tmpQuad.cut(Segment(ogreVectorsArray[2],ogreVectorsArray[1]));

    cerr << "tmpQuad.countNodes() " << tmpQuad.countNodes() <<endl;  


    swap(currentVisibleCreatures, previousVisibleCreatures);
    currentVisibleCreatures = tmpQuad.returnCreaturesSet(currentVisibleCreatures);
    cerr << "currentVisibleCreatures  " << currentVisibleCreatures->size() <<endl;

    std::set<Creature*> intersection; 
    std::set<Creature*> ascendingCreatures;
    std::set<Creature*> descendingCreatures;    
    std::set_intersection(previousVisibleCreatures->begin(), previousVisibleCreatures->end(), currentVisibleCreatures->begin(), currentVisibleCreatures->end(), std::inserter(intersection, intersection.end()));  
    std::set_difference(currentVisibleCreatures->begin(), currentVisibleCreatures->end(), intersection.begin(), intersection.end(),    std::inserter(ascendingCreatures, ascendingCreatures.end())); 
    std::set_difference(previousVisibleCreatures->begin(), previousVisibleCreatures->end(), intersection.begin(), intersection.end(),    std::inserter(descendingCreatures, descendingCreatures.end())); 


    std::set_difference(previousVisibleCreatures->begin(), previousVisibleCreatures->end(), intersection.begin(), intersection.end(),    std::inserter(descendingCreatures, descendingCreatures.end())); 
    

    for( std::vector<Creature*>:: iterator it = tmpQuad.mortuary.begin(); it != tmpQuad.mortuary.end() ; it++  ){
	descendingCreatures.erase(*it) ; 


	}

    cerr << "ascendingCreatures  " << ascendingCreatures.size() <<endl;
    cerr << "descendingCreatures " << descendingCreatures.size()<<endl;

    // sort the new tiles to form the proper diamod


    for(std::set<Creature*> :: iterator it = ascendingCreatures.begin() ;  it != ascendingCreatures.end() ; it++   )
    	(*it)->show();

    for(std::set<Creature*> :: iterator it = descendingCreatures.begin() ;  it != descendingCreatures.end() ; it++   )
    	(*it)->hide();

    return 1;


    }

/*! \brief Auxilary function, according to mode flags : SHOW and HIDE will try to show or hide a tile in single pass
 *  In each call there are two processes : one which traces the old camera view ( the one which would hide old Tiles ) , and one 
 *  which traces the new camera view ( the one which would show new Tiles). Mode parameter allows to only activate one of those processes, or activate both or none :)
 *  TODO : IMPLEMENT THE FLOOR AND CEIL MATH FUNCTIONS FOR FRACTURE VALUES, SO THAT THE EDGEING TILES ( IN CAMERA VIEW ) ARE NOT CULLED AWAY 
 */

int CullingManager::bashAndSplashTiles(int mode)
{
    bool bash,splash;

    int xxLeftOld = oldTop.x;
    int xxRightOld= oldTop.x;

    int dxLeftOld1 = (int)(oldMiddleLeft.x - oldTop.x)* (1<<precisionDigits) / (int)(oldTop.y - oldMiddleLeft.y);
    int dxRightOld1 = (int)(oldMiddleRight.x - oldTop.x)* (1<<precisionDigits) / (int)(oldTop.y - oldMiddleRight.y) ;

    int dxLeftOld2 = (int)(oldBottom.x - oldMiddleLeft.x)* (1<<precisionDigits)/ (int)(oldMiddleLeft.y - oldBottom.y );
    int dxRightOld2 =(int)(oldBottom.x - oldMiddleRight.x)* (1<<precisionDigits)/ (int)(oldMiddleRight.y - oldBottom.y) ;


    int xxLeft = top.x;
    int xxRight= top.x;

    int  dxLeft1 = (int)(middleLeft.x - top.x) * (1<<precisionDigits) / (int)(top.y - middleLeft.y);
    int  dxRight1 = (int)(middleRight.x - top.x)* (1<<precisionDigits) / (int)(top.y - middleRight.y) ;

    int  dxLeft2 = (int)(bottom.x - middleLeft.x)* (1<<precisionDigits)/ (int)(middleLeft.y - bottom.y );
    int  dxRight2 =(int)(bottom.x - middleRight.x)* (1<<precisionDigits)/ (int)(middleRight.y - bottom.y) ;


      

    int bb = min(bottom.y,oldBottom.y);

       
    for (int yy =  ((max(top.y,oldTop.y)>>precisionDigits)+1 )<<precisionDigits ; yy >= bb ; yy-=(1<<precisionDigits)) {





	// if(yy == top.y)splashY=!splashY;

	// if(yy == oldTop.y)bashY=!bashY;




	    if ( yy > middleLeft.y && yy<=top.y) {
	      xxLeft += dxLeft1;
	    }
	    else if( yy <= middleLeft.y && yy>bottom.y){
	      xxLeft += dxLeft2;

	    }
	    if ( yy > middleRight.y && yy<=top.y ) {
	      xxRight += dxRight1;
	    }
	    else if( yy <=  middleRight.y && yy>bottom.y ){
	      xxRight += dxRight2;

	    }


	    if ( yy > oldMiddleLeft.y && yy<=oldTop.y) {
		xxLeftOld += dxLeftOld1;
	    }
	    else if( yy <=  oldMiddleLeft.y && yy>oldBottom.y) {
		xxLeftOld += dxLeftOld2;

	    }
	    if ( yy > oldMiddleRight.y && yy<=oldTop.y) {
		xxRightOld += dxRightOld1;
	    }
	    else if( yy <=  oldMiddleRight.y && yy>oldBottom.y){
		xxRightOld += dxRightOld2;
	    }

	int rr =  max(xxRight,xxRightOld);
	for (int xx =  ((min(xxLeft,xxLeftOld)>>precisionDigits) -1)<<precisionDigits  ; xx <= rr ; xx+= (1<<precisionDigits)) {


	    // if(xx <=(int)xxLeft ) splashX=!splashX;
	    // if(xx <=(int)xxLeftOld  && xx >(int)xxRightOld) ) bashX=true;

    

	    splash = (xx >=(int)xxLeft  && xx <=(int)xxRight && (yy >= (int)bottom.y) && yy <= (int)top.y)  ;
	   
	    bash   = (xx >=(int)xxLeftOld  && xx <=(int)xxRightOld && (yy >= (int)oldBottom.y) && yy <= (int)oldTop.y) ;
	    
	    // cerr<< " x" <<  xx  << " y" << yy << " " <<bash<<splash << endl;		    
	      if(bash && splash && (mode & HIDE) && (mode & SHOW) ){}
	      else if (bash && (mode & HIDE))	      cm->gameMap->getTile(xx>>precisionDigits,yy>>precisionDigits)->hide();
	      else if (splash && (mode & SHOW))	      cm->gameMap->getTile(xx>>precisionDigits,yy>>precisionDigits)->show();




	    // if(xx >(int)xxRightOld) bashX=!bashX;
	    // if(xx >(int)xxRight)splashX=!splashX;
	}

	// if(yy < (int)oldBottom.y)bashY=!bashY;
	// if(yy < (int)bottom.y)splashY=!splashY;
    }      
    return 1;        
}



bool CullingManager::getIntersectionPoints() {

    const Ogre::Vector3*  myvector    = cm->getCamera()->getWorldSpaceCorners();
    Ogre::Vector3* pp ;



    myRay[0]= Ogre::Ray (myvector[0],  myvector[4] - myvector[0] );
    myRay[1]= Ogre::Ray (myvector[1],  myvector[5] - myvector[1] );
    myRay[2]= Ogre::Ray (myvector[2],  myvector[6] - myvector[2] );
    myRay[3]= Ogre::Ray (myvector[3],  myvector[7] - myvector[3] );

    std::pair<bool, Ogre::Real> intersectionResult;


    //  for(int ii = 0 ; ii <4 ; ii++){
    //  	     intersectionResult =  myRay[ii]->intersects(*myplanes[0]) ;
    //  	     rr++;
    //  	     if(intersectionResult.first)
    //  		 pp= myRay[ii]->getPoint(intersectionResult.second);
    // }



    for(int ii = 0 ; ii <4 ; ii++){
	int rr = 0;
	do
	    {
		intersectionResult =  myRay[ii].intersects(myplanes[rr]) ;
		rr++;
		if(intersectionResult.first)
		    ogreVectorsArray[ii]= (myRay[ii].getPoint(intersectionResult.second));


	    }
	while(!( (intersectionResult.first && ogreVectorsArray[ii].x >= 0.0 && ogreVectorsArray[ii].x <=396.0 && ogreVectorsArray[ii].y >= 0.0 && ogreVectorsArray[ii].y <=396.0    ) )   && rr < 6);
	if(intersectionResult.first){


	}
	else{
	    cerr<< "I didn't find the intersection point for " << ii <<"th ray"<<endl;
	    exit(1);
	}
    }

    cerr<<endl << "intersection points" << endl;
    cerr << ogreVectorsArray[1].x << " " << ogreVectorsArray[1].y << " " << ogreVectorsArray[1].z <<endl;
    cerr << ogreVectorsArray[2].x << " " << ogreVectorsArray[2].y << " " << ogreVectorsArray[2].z <<endl;
    cerr << ogreVectorsArray[3].x << " " << ogreVectorsArray[3].y << " " << ogreVectorsArray[3].z <<endl;
    cerr << ogreVectorsArray[0].x << " " << ogreVectorsArray[0].y << " " << ogreVectorsArray[0].z <<endl;

    return true;
}


bool CullingManager::onFrameStarted   (){
    

    cullTiles();
    cullCreatures();
}

bool CullingManager::onFrameEnded     (){

}

/*! \brief Sort two Vector3i p1 and p2  to satisfy p1 <= p2 according to  
 * the value of X or Y coordiante, which depends on sortByX param . 
 */
void CullingManager::sort(Vector3i& p1 , Vector3i& p2, bool sortByX) {

    if (sortByX) {

        if (p1.x > p2.x) {
	  swap(p1,p2);
        }

    }

    else {

        if (p1.y > p2.y) {
	  swap(p1,p2);
        }


    }
}




#endif /* CULLINGMANAGER_H_ */
