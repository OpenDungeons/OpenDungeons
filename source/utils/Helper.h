/*!
 * \file   Helper.h
 * \date   10 September 2011
 * \author StefanP.MUC, hwoarangmy, Bertram
 * \brief  Provides helper functions, constants and defines
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

#ifndef HELPER_H_
#define HELPER_H_

#include <string>
#include <iostream>
#include <sstream>
#include <vector>
#include <cstdint>

//! \brief Math constant pi. Always use this one, never M_PI from <cmath> (it's not portable)
const double PI = 3.141592653589793238462643;

//! \brief Helper namespace that contains globals that really don't fit in any better context
namespace Helper
{
    /*! \brief Converts a string to a primitive type T
     *
     *  \param str The string to be converted
     *  \return The converted primitive number
     */
    template<typename T>
    T stringToT(const std::string& str)
    {
        std::istringstream stream(str);
        T t = 0;
        stream >> t;
        return t;
    }

    /*! \brief Checks if a string contains primitive type T
     *
     *  \param str The string to be checked
     *  \return true if string contains type T, else false
     */
    template<typename T>
    bool checkIfT(const std::string& str)
    {
        std::istringstream stream(str);
        T t = 0;
        return (stream >> t);
    }

    //! \brief Split a line based on a delimiter.
    std::vector<std::string> split(const std::string& line, char delimiter);

    int toInt(const std::string& text);
    uint32_t toUInt32(const std::string& text);

    double toDouble(const std::string& text);

    // Needed on MSVC <2012
    // http://social.msdn.microsoft.com/Forums/vstudio/en-US/260e04fc-dd05-4a96-8953-9c6ea1ad62fb/cant-find-stdround-in-cmath?forum=vclanguage
    int round(double d);
    int round(float d);

    //! \brief Returns the filenames of the given directory.
    //! \note The folder parameter given is part of the returned filenames.
    bool fillFilesList(const std::string& path,
                       std::vector<std::string>& listFiles,
                       const std::string& fileExtension);

    //! \brief Returns the file stem (filename alone without the extension) of the given directory.
    bool fillFileStemsList(const std::string& path,
                           std::vector<std::string>& listFiles,
                           const std::string& fileExtension);

    //! \brief opens the file fileName and adds the uncommented lines to the stream.
    //! Returns true is the file could be open and false if an error occurs
    bool readFileWithoutComments(const std::string& fileName, std::stringstream& stream);
}

#endif // HELPER_H_
