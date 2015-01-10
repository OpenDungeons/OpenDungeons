/*!
 * \file   Random.cpp
 * \date   26 April 2011
 * \author andrewbuck, StefanP.MUC
 * \brief  Offers some random number generating functions
 *
 *  Copyright (C) 2011-2015  OpenDungeons Team
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

#include <algorithm>
#include <cmath>
#include <ctime>

unsigned long myRandomSeed;
const unsigned long MAX = 32768;

static unsigned long randgen()
{
    myRandomSeed = myRandomSeed * 1103515245 + 12345;
    //TODO: What is the purpose of the cast?
    unsigned long returnVal = static_cast<unsigned int>(myRandomSeed / 65536) % MAX;

    return returnVal;
}

//! \brief uniformly distributed number [0;1)
static double uniform()
{
    return randgen() * 1.0 / static_cast<double>(MAX);
}

//! \brief uniformly distributed number [lo;hi)
static double uniform(double lo, double hi)
{
    return uniform() * (hi - lo) + lo;
}

//! \brief random integer [lo;hi]
static int randint(int lo, int hi)
{
    return static_cast<int>(uniform() * (hi - lo + 1) + lo);
}

//! \brief random unsigned integer [lo;hi]
static unsigned int randuint(unsigned int lo, unsigned int hi)
{
    return static_cast<unsigned int>(uniform() * (hi - lo + 1) + lo);
}


namespace Random
{

void initialize()
{
    myRandomSeed = static_cast<unsigned long>(std::time(0));
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
    return std::sqrt(-2.0 * log(Double(0.0, 1.0))) * cos(2.0 * PI * Double(0.0, 1.0));
}

} // namespace Random
