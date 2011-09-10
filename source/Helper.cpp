/*!
 * \file   Helper.cpp
 * \date   10 September 2011
 * \author StefanP.MUC
 * \brief  Provides helper functions
 */

#include <iostream>
#include <sstream>

#include "Helper.h"

namespace Helper
{
    /*! \brief Helper function, converts a string to an int
     *
     *  \param str The string to be converted
     *  \return The converted number
     */
    int stringToInt(const std::string& str)
    {
        std::stringstream stream(str);
        int i = 0;
        stream >> i;
        return i;
    }

    /*! \brief Helper function, converts a string to an int
     *
     *  \param str The string to be converted
     *  \return The converted number
     */
    unsigned int stringToUInt(const std::string& str)
    {
        std::stringstream stream(str);
        unsigned int i = 0;
        stream >> i;
        return i;
    }

    /*! \brief Helper function, converts a string to a float
     *
     *  \param str The string to be converted
     *  \return The converted number
     */
    float stringToFloat(const std::string& str)
    {
        std::stringstream stream(str);
        float f = 0;
        stream >> f;
        return f;
    }

    /*! \brief Helper function, converts a string to a double
     *
     *  \param str The string to be converted
     *  \return The converted number
     */
    double stringToDouble(const std::string& str)
    {
        std::stringstream stream(str);
        double d = 0;
        stream >> d;
        return d;
    }

    /*! \brief Helper function, checks if a string contains an int
     *
     *  \param str The string to be checked
     *  \return true if string contains an int, else false
     */
    bool checkIfInt(const std::string& str)
    {
        std::istringstream stream(str);
        int a;
        return (stream >> a);
    }

    /*! \brief Helper function, checks if a string contains a float
     *
     *  \param str The string to be checked
     *  \return true if string contains a float, else false
     */
    bool checkIfFloat(const std::string& str)
    {
        std::istringstream stream(str);
        float f;
        return (stream >> f);
    }
}
