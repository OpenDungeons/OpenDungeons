/*
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

#ifndef DUMMYARRAYCLASS_H_
#define DUMMYARRAYCLASS_H_

#include "utils/Vector3i.h"

#include <array>
#include <vector>

class DummyArrayClass
{
public:

    bool clockwise(int ii , int jj);
    bool anticlockwise(int ii , int jj);
    Vector3i* getClockwiseNext();
    Vector3i* getAntiClockwiseNext();
    Vector3i& operator[](int ii) ;
    void sort();
    void sortByPhi(std::vector<Vector3i> &ll, Vector3i& cc);
    void zoom(double);
    std::array<Vector3i, 4>  mMyArray;
};

#endif // DUMMYARRAYCLASS_H_
