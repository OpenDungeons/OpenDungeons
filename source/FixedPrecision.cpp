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

#include "FixedPrecision.h"

FixedPrecision::FixedPrecision()
{}

FixedPrecision::FixedPrecision(int ii):
    mValue(ii << mPrecisionDigits)
{}

FixedPrecision::FixedPrecision(long long  ii):
    mValue(ii << mPrecisionDigits)
{}

FixedPrecision::FixedPrecision(long long ii, int dummy):
    mValue(ii)
{}

FixedPrecision::FixedPrecision(Ogre::Real rr):
    mValue((1 << mPrecisionDigits) * rr)
{}

FixedPrecision::operator int() const
{
    return mValue >> mPrecisionDigits;
}

FixedPrecision::operator long long() const
{
    return mValue >> mPrecisionDigits;
}

FixedPrecision operator+(FixedPrecision const& l, FixedPrecision const& r)
{
    return FixedPrecision(l.mValue + r.mValue, 0);
}

FixedPrecision  operator-(FixedPrecision const& l, FixedPrecision const& r)
{
    return FixedPrecision(l.mValue - r.mValue, 0);
}

// FixedPrecision  operator*(FixedPrecision const& l, FixedPrecision const& r)

FixedPrecision operator/(FixedPrecision const& l, FixedPrecision const& r)
{
    return FixedPrecision(l.mValue << (FixedPrecision::mPrecisionDigits) / r.mValue, 0);
}

FixedPrecision& FixedPrecision::operator++()
{
    mValue++;
    return *this;
}

FixedPrecision& FixedPrecision::operator--()
{
    mValue--;
    return *this;
}

FixedPrecision& FixedPrecision::operator+=(FixedPrecision const& r)
{
    this->mValue += r.mValue;
    return *this;
}

FixedPrecision& FixedPrecision::operator-=(FixedPrecision const& r)
{
    this->mValue -= r.mValue;
    return *this;
}

// FixedPrecision& operator*=(FixedPrecision const& r)
// {}

FixedPrecision& FixedPrecision::operator/=(FixedPrecision const& r)
{
    this->mValue << (FixedPrecision::mPrecisionDigits) / r.mValue;
    return *this;
}

// nonmember comparison operators
bool operator<(FixedPrecision const& l, FixedPrecision const& r)
{
    return l.mValue < r.mValue;
}

bool operator>(FixedPrecision const& l, FixedPrecision const& r)
{
    return l.mValue > r.mValue;
}

bool operator<=(FixedPrecision const& l, FixedPrecision const& r)
{
    return l.mValue <= r.mValue;
}

bool operator>=(FixedPrecision const& l, FixedPrecision const& r)
{
    return l.mValue >= r.mValue;
}

bool operator==(FixedPrecision const& l, FixedPrecision const& r)
{
    return l.mValue == r.mValue;
}

bool operator!=(FixedPrecision const& l, FixedPrecision const& r)
{
    return l.mValue != r.mValue;
}

FixedPrecision operator&&(FixedPrecision const& l, FixedPrecision const& r)
{
    return FixedPrecision(l.mValue && r.mValue, 0);
}

FixedPrecision operator||(FixedPrecision const& l, FixedPrecision const& r)
{
    return FixedPrecision(l.mValue || r.mValue, 0);
}

// FixedPrecision operator<<(FixedPrecision const& l, int r)
// { return l.mValue << r; }

// FixedPrecision operator>>(FixedPrecision const& l, int r)
// { return l.mValue >> r; }
