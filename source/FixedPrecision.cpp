#include "FixedPrecision.h"




FixedPrecision::FixedPrecision(){}
FixedPrecision::FixedPrecision(int  ii):value(ii<<precisionDigits){}
FixedPrecision::FixedPrecision(long long  ii):value(ii<<precisionDigits){}


FixedPrecision::FixedPrecision(long long  ii, int dummy ):value(ii){}

FixedPrecision::FixedPrecision(Ogre::Real  rr):value( (1<<precisionDigits) * rr){}




FixedPrecision::operator int()const {return value>>precisionDigits;} ;
FixedPrecision::operator long long ()const{return value>>precisionDigits;} ; 


FixedPrecision  operator+( FixedPrecision const& l, FixedPrecision const& r ){return FixedPrecision(l.value + r.value, 0);}
FixedPrecision  operator-( FixedPrecision const& l, FixedPrecision const& r ){return FixedPrecision(l.value - r.value,0);}
// FixedPrecision  operator*( FixedPrecision const& l, FixedPrecision const& r )
FixedPrecision  operator/( FixedPrecision const& l, FixedPrecision const& r ){return FixedPrecision(l.value<<(FixedPrecision::precisionDigits) / r.value,0);}

FixedPrecision& FixedPrecision::operator++(){ value++; return *this;};
FixedPrecision& FixedPrecision::operator--(){ value--; return *this;};

FixedPrecision& FixedPrecision::operator+=( FixedPrecision const& r ){this->value += r.value; return *this ; }
FixedPrecision& FixedPrecision::operator-=( FixedPrecision const& r ){this->value -= r.value; return *this ; }
// FixedPrecision& operator*=( FixedPrecision const& r ){}
FixedPrecision& FixedPrecision::operator/=( FixedPrecision const& r ){this->value<<(FixedPrecision::precisionDigits) /  r.value; return *this ; }


// nonmember comparison operators
bool operator<( FixedPrecision const& l, FixedPrecision const& r ){return l.value < r.value ; };
bool operator>( FixedPrecision const& l, FixedPrecision const& r ){return l.value > r.value ; };
bool operator<=( FixedPrecision const& l, FixedPrecision const& r ){return l.value <= r.value ; };
bool operator>=( FixedPrecision const& l, FixedPrecision const& r ){return l.value >= r.value ; };



bool operator==( FixedPrecision const& l, FixedPrecision const& r ){return l.value == r.value ; };
bool operator!=( FixedPrecision const& l, FixedPrecision const& r ){return l.value != r.value ; };


FixedPrecision operator&&( FixedPrecision const& l, FixedPrecision const& r ){ return FixedPrecision(l.value && r.value,0); };
FixedPrecision operator||( FixedPrecision const& l, FixedPrecision const& r ){ return FixedPrecision(l.value || r.value,0); };






// FixedPrecision operator<<( FixedPrecision const& l, int  r) { return  l.value<<r ;  };
// FixedPrecision operator>>( FixedPrecision const& l, int  r ){ return  l.value>>r ;  };

 
