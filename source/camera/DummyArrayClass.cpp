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

#include "camera/DummyArrayClass.h"

#include <algorithm>

void DummyArrayClass::sort()
{
    std::vector<Vector3i> ll[4];
    Vector3i cc(0, 0, 0);

    cc = (cc + mMyArray[0] + mMyArray[1] + mMyArray[2] + mMyArray[3]) / 4;

    for(int ii = 0; ii < 4; ++ii)
    {
        if(mMyArray[ii].x > cc.x && mMyArray[ii].y > cc.y)
        {
            ll[0].push_back(mMyArray[ii]);
        }
        if(mMyArray[ii].x > cc.x && mMyArray[ii].y <= cc.y)
        {
            ll[1].push_back(mMyArray[ii]);
        }
        if(mMyArray[ii].x <= cc.x && mMyArray[ii].y <= cc.y)
        {
            ll[2].push_back(mMyArray[ii]);
        }
        if(mMyArray[ii].x <= cc.x && mMyArray[ii].y > cc.y)
        {
            ll[3].push_back(mMyArray[ii]);
        }
    }

    int kk = 0;
    for(int ii = 0; ii < 4; ++ii)
    {
        sortByPhi(ll[ii], cc);

        for(std::vector<Vector3i>::const_iterator jj = ll[ii].begin(); jj != ll[ii].end(); ++jj)
        {
            mMyArray[kk] = *jj;
            ++kk;
        }
    }
}

void DummyArrayClass::sortByPhi(std::vector<Vector3i> &ll, Vector3i& cc)
{
    if (ll.size() < 2)
        return;

    std::sort(ll.begin(), ll.end(),
        [&cc](Vector3i vv, Vector3i ww)
        {
            return (double)(vv.y - cc.y) / (double)(vv.x - cc.x) > (double)(ww.y - cc.y) / (double)(ww.x - cc.x);
        }
    );
}


Vector3i& DummyArrayClass::operator[](int ii)
{
    while(ii < 0)
        ii += 4;

    return mMyArray[ii % 4];
}
