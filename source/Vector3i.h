#ifndef VECTOR3I_H_
#define VECTOR3I_H_

#include <iostream>
#include <OgreVector3.h>

extern const int mPrecisionDigits ;
extern const int Unit ;

// A custom vector used commonly between the CameraManager and the CullingManager classes


using std::ostream;

struct Vector3i
{
    Vector3i():
        x(0),
        y(0),
        z(0)
    {}
    Vector3i(int64_t tmp_x, int64_t tmp_y , int64_t tmp_z   ):x(tmp_x*Unit) ,y(tmp_y*Unit) ,z(tmp_z*Unit){}  

    int64_t x;
    int64_t y;
    int64_t z;
    Vector3i& operator+(Vector3i& vv);
    Vector3i& operator/(int ii);
    Vector3i(const Ogre::Vector3& OV);



};

ostream& operator<<(ostream& ss,Vector3i& vv)  ;

#endif /* VECTOR3I_H_ */
