/*
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

#define BOOST_TEST_MODULE Random
#include "BoostTestTargetConfig.h"

#include "gamemap/Pathfinding.h"

struct Point
{
    int x;
    int y;
    int getX() const
    { return x; }
    int getY() const
    { return y; }
};

BOOST_AUTO_TEST_CASE(test_Pathfinding)
{
    Point a{9,1};
    Point b{1,9};
    BOOST_CHECK(Pathfinding::squaredDistanceTile(a, b) == 128);
    // There might be round errors. We check that the difference is small enough
    BOOST_CHECK((Pathfinding::distanceTile(a, b) - std::sqrt(128.0f)) < 0.0001f);
    BOOST_CHECK(Pathfinding::squaredDistance(9,1,1,9) == 128);
}
