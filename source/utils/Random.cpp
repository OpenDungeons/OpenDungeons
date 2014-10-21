/*!
 * \file   Random.cpp
 * \date   26 April 2011
 * \author andrewbuck, StefanP.MUC
 * \brief  Offers some random number generating functions
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

#include "utils/Random.h"

#include "utils/Helper.h"

#ifdef __MINGW32__
#ifndef mode_t
#include <sys/types.h>
#endif //mode_t
#endif //mingw32
#include <algorithm>
#include <cmath>
#include <ctime>

unsigned long myRandomSeed;
const unsigned long MAX = 32768;

unsigned long randgen()
{
    myRandomSeed = myRandomSeed * 1103515245 + 12345;
    unsigned long returnVal = (unsigned int) (myRandomSeed / 65536) % MAX;

    return returnVal;
}

//! \brief uniformly distributed number [0;1)
double uniform()
{
    return randgen() * 1.0 / static_cast<double>(MAX);
}

//! \brief uniformly distributed number [0;hi)
double uniform(double hi)
{
    return uniform() * hi;
}

//! \brief uniformly distributed number [lo;hi)
double uniform(double lo, double hi)
{
    return uniform() * (hi - lo) + lo;
}

//! \brief random bit
int randint()
{
    return uniform() > 0.5 ? 1 : 0;
}

//! \brief random integer [0;hi]
int randint(int hi)
{
    return static_cast<signed>(uniform() * (hi + 1));
}

//! \brief random integer [lo;hi]
int randint(int lo, int hi)
{
    return static_cast<signed>(uniform() * (hi - lo + 1) + lo);
}

//! \brief random unsigned integer [0;hi]
unsigned int randuint(unsigned int hi)
{
    return static_cast<unsigned int>(uniform() * (hi + 1));
}

//! \brief random unsigned integer [lo;hi]
unsigned int randuint(unsigned int lo, unsigned int hi)
{
    return static_cast<unsigned int>(uniform() * (hi - lo + 1) + lo);
}


namespace Random
{

void initialize()
{
    myRandomSeed = static_cast<unsigned long>(time(0));
}

double Double(double min, double max)
{
    if (min > max)
    {
        std::swap(min, max);
    }

    return uniform(min, max);
}

int Int(int min, int max)
{
    if (min > max)
    {
        std::swap(min, max);
    }

    return randint(min, max);
}

unsigned int Uint(unsigned int min, unsigned int max)
{
    if (min > max)
    {
        std::swap(min, max);
    }

    return randuint(min, max);
}

double gaussianRandomDouble()
{
    return sqrt(-2.0 * log(Double(0.0, 1.0))) * cos(2.0 * PI * Double(0.0, 1.0));
}

} // namespace Random
