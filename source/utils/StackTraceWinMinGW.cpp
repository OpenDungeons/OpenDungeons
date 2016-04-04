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

#define PACKAGE "OpenDungeons"

#include <windows.h>
#include <excpt.h>
#include <imagehlp.h>
#include <bfd.h>
#include <psapi.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>
#include <cxxabi.h>
#include <fstream>
#include <sstream>
#include <vector>

struct BfdCtx
{
    bfd* handle;
    asymbol** symbol;
};

struct BfdSet
{
    std::string name;
    struct BfdCtx bfdCtx;
};

struct BfdSearchData
{
    asymbol** symbol;
    bfd_vma offset;
    std::string file;
    std::string func;
    unsigned int line;
};

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
    static void bfdSectionCallback(bfd* abfd, asection* sec, void* voidData);

    int bfdInitCtx(std::ostream& crashStream, struct BfdCtx* bfdCtx, const char * procName);
    void bfdCloseCtx(struct BfdCtx* bfdCtx);

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

void StackTracePrintPrivateData::bfdSectionCallback(bfd* abfd, asection* sec, void* voidData)
{
	struct BfdSearchData *data = static_cast<struct BfdSearchData*>(voidData);

	if (!data->func.empty())
		return;

	if (!(bfd_get_section_flags(abfd, sec) & SEC_ALLOC))
		return;

	bfd_vma vma = bfd_get_section_vma(abfd, sec);
	if (data->offset < vma || vma + bfd_get_section_size(sec) <= data->offset)
		return;

    const char* file = nullptr;
    const char* func = nullptr;
    unsigned int line = 0;
	bfd_find_nearest_line(abfd, sec, data->symbol, data->offset - vma, &file, &func, &line);
	if(file != nullptr)
        data->file = file;
	if(func != nullptr)
        data->func = func;
    data->line = line;
}

int StackTracePrintPrivateData::bfdInitCtx(std::ostream& crashStream, struct BfdCtx* bfdCtx, const char * procName)
{
    bfdCtx->handle = nullptr;
    bfdCtx->symbol = nullptr;

    bfd* bfdHandle = bfd_openr(procName, 0);
    if(bfdHandle == nullptr)
    {
        crashStream << "Failed to open bfd from " << procName << std::endl;
        return 1;
    }

    int r1 = bfd_check_format(bfdHandle, bfd_object);
    int r2 = bfd_check_format_matches(bfdHandle, bfd_object, nullptr);
    int r3 = bfd_get_file_flags(bfdHandle) & HAS_SYMS;

    if (!(r1 && r2 && r3))
    {
        bfd_close(bfdHandle);
        crashStream << "Failed to init bfd from " << procName << std::endl;
        return 1;
    }

    void* symbolTable = nullptr;

    unsigned dummy = 0;
    if (bfd_read_minisymbols(bfdHandle, FALSE, &symbolTable, &dummy) == 0)
    {
        if (bfd_read_minisymbols(bfdHandle, TRUE, &symbolTable, &dummy) < 0)
        {
            free(symbolTable);
            bfd_close(bfdHandle);
            crashStream << "Failed to read symbols from " << procName << std::endl;
            return 1;
        }
    }

    bfdCtx->handle = bfdHandle;
    bfdCtx->symbol = static_cast<asymbol **>(symbolTable);

    return 0;
}

void StackTracePrintPrivateData::bfdCloseCtx(struct BfdCtx* bfdCtx)
{
    if(bfdCtx)
    {
        if(bfdCtx->symbol)
            free(bfdCtx->symbol);

        if(bfdCtx->handle)
            bfd_close(bfdCtx->handle);
    }
}

LONG WINAPI StackTracePrintPrivateData::exceptionFilter(LPEXCEPTION_POINTERS info)
{
    // We print the callstack in a stringstream. Then, we will print it to the crashStream
    // and, then, to the log manager
    std::stringstream crashStream;

    if (!SymInitialize(GetCurrentProcess(), 0, TRUE))
        return 0;

    bfd_init();

    std::vector<struct BfdSet> listSets;
    int depth = 128;

    PCONTEXT context = info->ContextRecord;

    char procname[MAX_PATH];
    GetModuleFileNameA(nullptr, procname, sizeof procname);

    struct BfdCtx* bfdCtx = nullptr;

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
    char processNameRaw[MAX_PATH];

    while(StackWalk(IMAGE_FILE_MACHINE_I386, process, thread, &frame, context,
        0, SymFunctionTableAccess, SymGetModuleBase, 0))
    {
        --depth;
        if (depth < 0)
            break;

        IMAGEHLP_SYMBOL *symbol = reinterpret_cast<IMAGEHLP_SYMBOL*>(symbol_buffer);
        symbol->SizeOfStruct = (sizeof *symbol) + 255;
        symbol->MaxNameLength = 254;

        DWORD moduleBase = SymGetModuleBase(process, frame.AddrPC.Offset);

        const char * processName = "[unknown module]";
        if ((moduleBase != 0) && GetModuleFileNameA(reinterpret_cast<HINSTANCE>(moduleBase), processNameRaw, MAX_PATH))
        {
            processName = processNameRaw;
            for(struct BfdSet& setSearch : listSets)
            {
                if(setSearch.name.compare(processName) == 0)
                    bfdCtx = &setSearch.bfdCtx;
            }

            if(bfdCtx == nullptr)
            {
                listSets.push_back(BfdSet());
                struct BfdSet& newSet = listSets.back();
                newSet.name = processName;
                if(mInstance->bfdInitCtx(crashStream, &newSet.bfdCtx, processName) == 0)
                {
                    bfdCtx = &newSet.bfdCtx;
                }
            }
        }

        struct BfdSearchData data;
        if(bfdCtx != nullptr)
        {
            data.symbol = bfdCtx->symbol;
            data.offset = frame.AddrPC.Offset;
            data.line = 0;

            bfd_map_over_sections(bfdCtx->handle, &bfdSectionCallback, &data);
        }

        if (data.file.empty())
        {
            DWORD dummy = 0;
            if (SymGetSymFromAddr(process, frame.AddrPC.Offset, &dummy, symbol))
                data.file = symbol->Name;
            else
                data.file = "[unknown file]";
        }

        int status;
        char* demangled = abi::__cxa_demangle(data.func.c_str(), nullptr, nullptr, &status);
        if((status == 0) && (demangled != nullptr))
            data.func = demangled;

        if(demangled != nullptr)
            free(demangled);

        crashStream << frame.AddrPC.Offset << ", " << processName << ", " << data.file << ", " << data.line << ", " << data.func << std::endl;
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
    std::ofstream crashFile(mInstance->mCrashFilePath);
    while(crashStream.good())
    {
        std::string log;

        if(!Helper::readNextLineNotEmpty(crashStream, log))
            break;

        crashFile << log << std::endl;
    }
    crashFile.flush();
    crashFile.close();

    // We release the resources
    for(struct BfdSet& tmpSet : listSets)
        mInstance->bfdCloseCtx(&tmpSet.bfdCtx);

    listSets.clear();

    SymCleanup(GetCurrentProcess());

    return 0;
}
