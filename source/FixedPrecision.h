/*
 *  Created on: 24. feb. 2011
 *  Author: oln
 *
 *  Copyright (C) 2011-2014  OpenDungeons Team
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _FIXED_PRECISION_H_
#define _FIXED_PRECISION_H_

#include "OgrePrerequisites.h"

class FixedPrecision
{

public:

    static const int precisionDigits =  9;
    FixedPrecision();
    FixedPrecision(int);
    FixedPrecision(long long);
    FixedPrecision(long long, int);
    explicit FixedPrecision(Ogre::Real);
    uint32_t value;

    FixedPrecision& operator+=(FixedPrecision const& r);
    FixedPrecision& operator-=(FixedPrecision const& r);
    FixedPrecision& operator*=(FixedPrecision const& r);
    FixedPrecision& operator/=(FixedPrecision const& r);
    FixedPrecision& operator++();
    FixedPrecision& operator--();
    operator int() const;
    operator long long () const;
};

FixedPrecision operator+(FixedPrecision const& l, FixedPrecision const& r);
FixedPrecision operator-(FixedPrecision const& l, FixedPrecision const& r);
/* FixedPrecision operator*( FixedPrecision const& l, FixedPrecision const& r); */
FixedPrecision operator/(FixedPrecision const& l, FixedPrecision const& r);

// nonmember comparison operators
bool operator< (FixedPrecision const& l, FixedPrecision const& r);
bool operator> (FixedPrecision const& l, FixedPrecision const& r);
bool operator<= (FixedPrecision const& l, FixedPrecision const& r);
bool operator>= (FixedPrecision const& l, FixedPrecision const& r);

bool operator==(FixedPrecision const& l, FixedPrecision const& r);
bool operator!=(FixedPrecision const& l, FixedPrecision const& r);

FixedPrecision operator&&(FixedPrecision const& l, FixedPrecision const& r);
FixedPrecision operator||(FixedPrecision const& l, FixedPrecision const& r);

// FixedPrecision operator<<(FixedPrecision const& l, int r);
// FixedPrecision operator>>(FixedPrecision const& l, int r);

#endif // _FIXED_PRECISION_H_
