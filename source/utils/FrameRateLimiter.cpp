/*!
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

#include "FrameRateLimiter.h"

#include <boost/thread/thread_only.hpp>

namespace
{
    const sf::Int64 divisor = boost::micro::den;
    const sf::Int64 minDifference = divisor / 1000;

    template<typename T>
    T fromFrameRate(T frameRate)
    {
        return divisor / frameRate;
    }

    //Since we are just inverting to get the framerate,
    //we can use the same fuction to invert back.
#define toFrameRate fromFrameRate
}

FrameRateLimiter::FrameRateLimiter(sf::Int64 frameRate)
  : mMinFrameTime(fromFrameRate(frameRate))
{
    setFrameRate(frameRate);
}


void FrameRateLimiter::sleepIfEarly()
{
    sf::Int64 timeSinceLast = mClock.getElapsedTime().asMicroseconds();
    if(mMinFrameTime - timeSinceLast > minDifference)
    {
        boost::this_thread::sleep_for(boost::chrono::microseconds(mMinFrameTime - timeSinceLast));
    }
    mClock.restart();
}

unsigned int FrameRateLimiter::getFrameRate()
{
    return toFrameRate(mMinFrameTime);
}

void FrameRateLimiter::setFrameRate(unsigned int frameRate)
{
    mMinFrameTime = fromFrameRate(frameRate);
}
