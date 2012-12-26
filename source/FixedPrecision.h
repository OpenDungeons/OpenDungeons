


#ifndef FIXED_PRECISION_H
#define FIXED_PRECISION_H



#include "OgrePrerequisites.h"




 class FixedPrecision{


public:
 
    static const int precisionDigits =  9;
    FixedPrecision();
    FixedPrecision(int);
    FixedPrecision(long long);
    FixedPrecision(long long, int );
    explicit FixedPrecision(Ogre::Real);
    uint32_t value;

    FixedPrecision& operator+=( FixedPrecision const& r );
    FixedPrecision& operator-=( FixedPrecision const& r );
    FixedPrecision& operator*=( FixedPrecision const& r );
    FixedPrecision& operator/=( FixedPrecision const& r );
    FixedPrecision& operator++();
    FixedPrecision& operator--();
    operator int() const;
    operator long long () const; 



};







FixedPrecision operator+( FixedPrecision const& l, FixedPrecision const& r );
FixedPrecision operator-( FixedPrecision const& l, FixedPrecision const& r );
/* FixedPrecision operator*( FixedPrecision const& l, FixedPrecision const& r ); */
FixedPrecision operator/( FixedPrecision const& l, FixedPrecision const& r );

// nonmember comparison operators
bool operator< ( FixedPrecision const& l, FixedPrecision const& r );
bool operator> ( FixedPrecision const& l, FixedPrecision const& r );
bool operator<= ( FixedPrecision const& l, FixedPrecision const& r );
bool operator>= ( FixedPrecision const& l, FixedPrecision const& r );


bool operator==( FixedPrecision const& l, FixedPrecision const& r );
bool operator!=( FixedPrecision const& l, FixedPrecision const& r );


FixedPrecision operator&&( FixedPrecision const& l, FixedPrecision const& r );
FixedPrecision operator||( FixedPrecision const& l, FixedPrecision const& r );




// FixedPrecision operator<<( FixedPrecision const& l, int r );
// FixedPrecision operator>>( FixedPrecision const& l, int r );















#endif // FIXED_PRECISION_H
