

#include "Vector3i.h"

Vector3i& Vector3i::operator+(Vector3i& vv){ this->x+=vv.x; this->y+=vv.y; this->z+=vv.z; return *this;}
Vector3i& Vector3i::operator/(int ii){ this->x/=ii; this->y/=ii; this->z/=ii; return *this;}


ostream& operator<<(ostream& ss,Vector3i& vv){ ss << vv.x << " " << vv.y <<" " << vv.z << " " ; return ss;   };

// Vector3i::Vector3i(const Ogre::Vector3& OV)
// {
//     x = (int64_t)((1 << mPrecisionDigits) * OV.x);
//     y = (int64_t)((1 << mPrecisionDigits) * OV.y);
//     z = (int64_t)((1 << mPrecisionDigits) * OV.z);
// }
