/*! \file   StackTracePrint.cpp
 *  \author paul424
 *  \date   Sun Jun 22 18:16:35 CEST 2014
 *  \brief  Namespace StackTracePrint containing functions for call
 *  stack printing ( by default after program crash).
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

#if defined WIN32
// TODO : StackTracePrint do not work under windows (headers are different). We should try to find if
// there is something equivalent
#elif defined (__i386__) | defined (__x86_64__)  // Only for supported platforms

#include "utils/StackTracePrint.h"

#include <cxxabi.h>
#include <execinfo.h>
#include <unistd.h>

#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void StackTracePrint::critErrHandler(int sig_num, siginfo_t* info, void* ucontext)
{
// Prevent running a function that isn't supported on certain platforms.
#if not defined (__i386__) & not defined (__x86_64__)
    std::cout << "No error handler supported for this platform" << std::endl;
    return;
#endif

    void* array[50];
    void* caller_address;
    sig_ucontext_t* uc = static_cast<sig_ucontext_t*>(ucontext);

#if defined(__i386__) // gcc specific
    caller_address = reinterpret_cast<void*>(uc->uc_mcontext.eip); // EIP: x86 specific
#elif defined(__x86_64__) // gcc specific
    caller_address = reinterpret_cast<void*>(uc->uc_mcontext.rip); // RIP: x86_64 specific
#else
#error Unsupported architecture. // TODO: Add support for other arch.
#endif
    // void* caller_address = (void*) uc->uc_mcontext.eip; // x86 specific

    std::cerr << "signal " << sig_num
              << " (" << strsignal(sig_num) << "), address is "
              << info->si_addr << " from " << caller_address
              << std::endl << std::endl;

    int size = backtrace(array, 50);

    array[1] = caller_address;

    char** messages = backtrace_symbols(array, size);

    // skip first stack frame (points here)
    for (int i = 1; i < size && messages != nullptr; ++i)
    {
        char* mangled_name = nullptr, *offset_begin = nullptr, *offset_end = nullptr;

        // find parantheses and +address offset surrounding mangled name
        for (char* p = messages[i]; *p; ++p)
        {
            if (*p == '(')
            {
                mangled_name = p;
            }
            else if (*p == '+')
            {
                offset_begin = p;
            }
            else if (*p == ')')
            {
                offset_end = p;
                break;
            }
        }

        // if the line could be processed, attempt to demangle the symbol
        if (mangled_name && offset_begin && offset_end &&
            mangled_name < offset_begin)
        {
            *mangled_name++ = '\0';
            *offset_begin++ = '\0';
            *offset_end++ = '\0';

            int status;
            char * real_name = abi::__cxa_demangle(mangled_name, 0, 0, &status);

            // if demangling is successful, output the demangled function name
            if (status == 0)
            {
                std::cerr << "[bt]: (" << i << ") " << messages[i] << " : "
                          << real_name << "+" << offset_begin << offset_end
                          << std::endl;

            }
            // otherwise, output the mangled function name
            else
            {
                std::cerr << "[bt]: (" << i << ") " << messages[i] << " : "
                          << mangled_name << "+" << offset_begin << offset_end
                          << std::endl;
            }
            free(real_name);
        }
        // otherwise, print the whole line
        else
        {
            std::cerr << "[bt]: (" << i << ") " << messages[i] << std::endl;
        }
    }
    std::cerr << std::endl;

    free(messages);

    exit(EXIT_FAILURE);
}

#endif // supported platforms
