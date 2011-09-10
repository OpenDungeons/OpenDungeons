/*!
 * \file   Helper.h
 * \date   10 September 2011
 * \author StefanP.MUC
 * \brief  Provides helper functions, constants and defines
 */

#ifndef HELPER_H_
#define HELPER_H_

#include <string>
#include <iostream>
#include <sstream>

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
}

#endif /* HELPER_H_ */
