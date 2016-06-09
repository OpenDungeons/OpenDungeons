/*
 *  Copyright (C) 2011-2016  OpenDungeons Team
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

#ifndef CULLINGVECTORMANAGER_H_
#define CULLINGVECTORMANAGER_H_

#include "utils/VectorInt64.h"

#include <array>
#include <vector>

/*! \brief The DummyArrayClass class is a wrapper 
 *  around the std::array mainly to overload the 
 *  operator [].        
 */


class CullingVectorManager
{
public:
    VectorInt64& operator[](int ii) ;
    void sort();
    void sortByPhi(std::vector<VectorInt64> &ll, VectorInt64& cc);
    void zoom(double);
    std::vector<VectorInt64>  mMyArray;
};

#endif // CULLINGVECTORMANAGER_H_
