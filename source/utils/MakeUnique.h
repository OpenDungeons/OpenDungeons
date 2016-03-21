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

#ifndef MAKEUNIQUE_H
#define MAKEUNIQUE_H

#include <memory>

namespace Utils
{
//If we are in C++14 mode, we can use make_unique from the standard library
//MSVC2013 has make_unique available by default anyway
#if __cplusplus > 201103L || _MSC_VER >= 1800
    using std::make_unique;
#else
    //! \brief Creates a unique_ptr of type T
    template<typename T, typename ...Args>
    std::unique_ptr<T> make_unique( Args&& ...args )
    {
        return std::unique_ptr<T>( new T( std::forward<Args>(args)... ) );
    }
#endif
}

#endif
