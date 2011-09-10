/*!
 * \file   Helper.h
 * \date   10 September 2011
 * \author StefanP.MUC
 * \brief  Provides helper functions, constats and defines
 */

#ifndef HELPER_H_
#define HELPER_H_

#include <string>

const double PI = 3.141592653589793238462643;

namespace Helper
{
    int          stringToInt     (const std::string& str);
    unsigned int stringToUInt    (const std::string& str);
    float        stringToFloat   (const std::string& str);
    double       stringToDouble  (const std::string& str);
    bool         checkIfInt      (const std::string& str);
    bool         checkIfFloat    (const std::string& str);
}

#endif /* HELPER_H_ */
