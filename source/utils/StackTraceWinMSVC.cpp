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

#include <windows.h>
#include <excpt.h>
#include <imagehlp.h>
#include <psapi.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>
#include <fstream>
#include <vector>

class StackTracePrintPrivateData
{
public:
    StackTracePrintPrivateData(const std::string& crashFilePath) :
        mCrashFilePath(crashFilePath)
    {
        mInstance = this;
        mExceptionFilterId = SetUnhandledExceptionFilter(exceptionFilter);
    }

    virtual ~StackTracePrintPrivateData()
    {
        if (mExceptionFilterId != nullptr)
        {
            SetUnhandledExceptionFilter(mExceptionFilterId);
            mExceptionFilterId = nullptr;
        }
    }

    static LONG WINAPI exceptionFilter(LPEXCEPTION_POINTERS info);
    LPTOP_LEVEL_EXCEPTION_FILTER mExceptionFilterId;

    std::string mCrashFilePath;

    static StackTracePrintPrivateData* mInstance;
};

StackTracePrintPrivateData* StackTracePrintPrivateData::mInstance = nullptr;

static sf::Mutex gMutex;

StackTracePrintPrivateData* StackTracePrint::mPrivateData = nullptr;

StackTracePrint::StackTracePrint(const std::string& crashFilePath)
{
    sf::Lock lock(gMutex);

    if(mPrivateData != nullptr)
        throw std::exception();

    mPrivateData = new StackTracePrintPrivateData(crashFilePath);
}

StackTracePrint::~StackTracePrint()
{
    if(mPrivateData != nullptr)
    {
        delete mPrivateData;
        mPrivateData = nullptr;
    }
}

LONG WINAPI StackTracePrintPrivateData::exceptionFilter(LPEXCEPTION_POINTERS info)
{
    // We print the callstack in a stringstream. Then, we will print it to the crashStream
    // and, then, to the log manager
    std::stringstream crashStream;

    if (!SymInitialize(GetCurrentProcess(), 0, TRUE))
        return 0;

    int depth = 128;
    IMAGEHLP_LINE line = { 0 };

    PCONTEXT context = info->ContextRecord;

    char procname[MAX_PATH];
    GetModuleFileNameA(nullptr, procname, sizeof procname);

	STACKFRAME frame;
    memset(&frame,0,sizeof(frame));

    frame.AddrPC.Offset = context->Eip;
    frame.AddrPC.Mode = AddrModeFlat;
    frame.AddrStack.Offset = context->Esp;
    frame.AddrStack.Mode = AddrModeFlat;
    frame.AddrFrame.Offset = context->Ebp;
    frame.AddrFrame.Mode = AddrModeFlat;

    HANDLE process = GetCurrentProcess();
    HANDLE thread = GetCurrentThread();

    char symbol_buffer[sizeof(IMAGEHLP_SYMBOL) + 255];
    DWORD symbolOffset = 0;

    line.SizeOfStruct = sizeof line;

    std::string symbolFile;
    std::string symbolFunction;
    std::string processName;
    char processNameRaw[MAX_PATH];

    while(StackWalk(IMAGE_FILE_MACHINE_I386, process, thread, &frame, context,
        0, SymFunctionTableAccess, SymGetModuleBase, 0))
    {
        --depth;
        if (depth < 0)
            break;

        if (frame.AddrReturn.Offset == 0)
            break;

        if (frame.AddrPC.Offset == 0)
        {
            crashStream << frame.AddrPC.Offset << ", " << procname << std::endl;
            continue;
        }

        IMAGEHLP_SYMBOL *symbol = reinterpret_cast<IMAGEHLP_SYMBOL*>(symbol_buffer);
        symbol->SizeOfStruct = (sizeof *symbol) + 255;
        symbol->MaxNameLength = 254;

        DWORD moduleBase = SymGetModuleBase(process, frame.AddrPC.Offset);
        if ((moduleBase != 0) && GetModuleFileNameA(reinterpret_cast<HINSTANCE>(moduleBase), processNameRaw, MAX_PATH))
            processName = processNameRaw;
        else
            processName = "[unknown module]";

        DWORD dummy = 0;
        if (SymGetSymFromAddr(process, frame.AddrPC.Offset, &dummy, symbol))
            symbolFile = symbol->Name;
        else
            symbolFile = "[unknown function]";

        std::vector<char> und_name(1024);
        UnDecorateSymbolName(symbol->Name, &und_name[0], 1024, UNDNAME_COMPLETE);
        symbolFunction = std::string(&und_name[0], strlen(&und_name[0]));

        if (!SymGetLineFromAddr(process, frame.AddrPC.Offset, &symbolOffset, &line))
        {
            line.FileName = "[unknown file]";
            line.LineNumber = 0;
        }

        crashStream << frame.AddrPC.Offset << ", " << processName << ", " << line.FileName << ", " << symbolFile << ", " << line.LineNumber << std::endl;
    }

    crashStream.flush();
    // We try to export to the logs
    LogManager* logMgr = LogManager::getSingletonPtr();
    if (logMgr != nullptr)
    {
        while (crashStream.good())
        {
            std::string log;

            if (!Helper::readNextLineNotEmpty(crashStream, log))
                break;

            logMgr->logMessage(LogMessageLevel::CRITICAL, __FILE__, __LINE__, log);
        }
    }

    // Set the stream at beginning
    crashStream.clear();
    crashStream.seekg(0, crashStream.beg);
    // We export everything to the crash file
    std::ofstream crashFile(mInstance->mCrashFilePath);
    while (crashStream.good())
    {
        std::string log;

        if (!Helper::readNextLineNotEmpty(crashStream, log))
            break;

        crashFile << log << std::endl;
    }
    crashFile.flush();
    crashFile.close();

    SymCleanup(process);

    return 0;
}

