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

#include "utils/StackTracePrint.h"

#include "utils/Helper.h"
#include "utils/LogManager.h"

#include <SFML/System.hpp>

#include <cxxabi.h>
#include <execinfo.h>
#include <unistd.h>
#include <signal.h>
#include <ucontext.h>

#include <iostream>
#include <cstring>
#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>

//! \brief This structure mirrors the one found in /usr/include/asm/ucontext.h
//! \note: Its members should stay same named.
typedef struct _sig_ucontext
{
    unsigned long     uc_flags;
    struct ucontext*  uc_link;
    stack_t           uc_stack;
    struct sigcontext uc_mcontext;
    sigset_t          uc_sigmask;
} sig_ucontext_t;

class StackTracePrintPrivateData
{
public:
    StackTracePrintPrivateData()
    {
        //Init the error hanlder used to get a full stacktrace when crashing
        struct sigaction sigact;
        std::memset(&sigact, 0, sizeof(sigact));
        sigact.sa_sigaction = critErrHandler;
        sigact.sa_flags = SA_RESTART | SA_SIGINFO;
        if (sigaction(SIGSEGV, &sigact, nullptr) != 0)
        {
            std::cerr << "error setting signal handler for: "
                << SIGSEGV << strsignal(SIGSEGV) << std::endl;
            exit(EXIT_FAILURE);
        }
    }
    
    static void critErrHandler(int sig_num, siginfo_t* info, void* ucontext);
};

static sf::Mutex gMutex;

StackTracePrintPrivateData* StackTracePrint::mPrivateData = nullptr;

StackTracePrint::StackTracePrint(const std::string& crashFilePath)
{
    sf::Lock lock(gMutex);

    if(mPrivateData != nullptr)
        throw std::exception();

    mPrivateData = new StackTracePrintPrivateData;
}

StackTracePrint::~StackTracePrint()
{
    if(mPrivateData != nullptr)
    {
        delete mPrivateData;
        mPrivateData = nullptr;
    }
}

void StackTracePrintPrivateData::critErrHandler(int sig_num, siginfo_t* info, void* ucontext)
{
// Prevent running a function that isn't supported on certain platforms.
#if not defined (__i386__) & not defined (__x86_64__)
    std::cout << "No error handler supported for this platform" << std::endl;
    return;
#endif
    // We print the callstack in a stringstream. Then, we will print it to the crashStream
    // and, then, to the log manager
    std::stringstream crashStream;

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

    crashStream << "signal " << sig_num
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
                crashStream << "[bt]: (" << i << ") " << messages[i] << " : "
                          << real_name << "+" << offset_begin << offset_end
                          << std::endl;

            }
            // otherwise, output the mangled function name
            else
            {
                crashStream << "[bt]: (" << i << ") " << messages[i] << " : "
                          << mangled_name << "+" << offset_begin << offset_end
                          << std::endl;
            }
            free(real_name);
        }
        // otherwise, print the whole line
        else
        {
            crashStream << "[bt]: (" << i << ") " << messages[i] << std::endl;
        }
    }

    crashStream.flush();
    // We try to export to the logs
    LogManager* logMgr = LogManager::getSingletonPtr();
    if(logMgr != nullptr)
    {
        while(crashStream.good())
        {
            std::string log;

            if(!Helper::readNextLineNotEmpty(crashStream, log))
                break;

            logMgr->logMessage(LogMessageLevel::CRITICAL, __FILE__, __LINE__, log);
        }
    }

    // Set the stream at beginning
    crashStream.clear();
    crashStream.seekg(0, crashStream.beg);
    // We export everything to the crash file
    while(crashStream.good())
    {
        std::string log;

        if(!Helper::readNextLineNotEmpty(crashStream, log))
            break;

        std::cerr << log << std::endl;
    }
    std::cerr.flush();

    free(messages);

    exit(EXIT_FAILURE);
}
