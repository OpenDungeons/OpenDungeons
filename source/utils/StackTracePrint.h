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

#ifndef STACKTRACEPRINT_H
#define STACKTRACEPRINT_H

#include <string>

class StackTracePrintPrivateData;

//! Class that allows to catch uncaught segfaults and to log the corresponding callstack
//! This class is a singleton and will throw an exception if created more than once
//! This class will hook exception when created and release the hook when released. Thus,
//! it should be instancied as long as segfaults need to be detected.
class StackTracePrint
{
public:
    StackTracePrint(const std::string& crashFilePath);
    virtual ~StackTracePrint();

private:
    //! This class is a singleton and will throw an exception if created more than once
    static StackTracePrintPrivateData* mPrivateData;
};

#endif // STACKTRACEPRINT_H
