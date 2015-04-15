/*
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

#if defined __MINGW32__

#include "utils/StackTraceWinMinGW.h"

#include <SFML/System.hpp>

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

static sf::Mutex gMutex;

StackTraceWinMinGW* StackTraceWinMinGW::mInstance = nullptr;

StackTraceWinMinGW::StackTraceWinMinGW(const std::string& crashFilePath) :
    mExceptionFilterId(nullptr),
    mCrashFilePath(crashFilePath)
{
    sf::Lock lock(gMutex);

    if(mInstance != nullptr)
        throw std::exception();

    mInstance = this;
    mExceptionFilterId = SetUnhandledExceptionFilter(exceptionFilter);
}

StackTraceWinMinGW::~StackTraceWinMinGW()
{
    if (mExceptionFilterId != nullptr)
    {
        SetUnhandledExceptionFilter(mExceptionFilterId);
        mExceptionFilterId = nullptr;
    }
}

void StackTraceWinMinGW::bfdSectionCallback(bfd* abfd, asection* sec, void* voidData)
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

int StackTraceWinMinGW::bfdInitCtx(std::ofstream& crashFile, struct BfdCtx* bfdCtx, const char * procName)
{
    bfdCtx->handle = nullptr;
    bfdCtx->symbol = nullptr;

    bfd* bfdHandle = bfd_openr(procName, 0);
    if(bfdHandle == nullptr)
    {
        crashFile << "Failed to open bfd from " << procName << std::endl;
        return 1;
    }

    int r1 = bfd_check_format(bfdHandle, bfd_object);
    int r2 = bfd_check_format_matches(bfdHandle, bfd_object, nullptr);
    int r3 = bfd_get_file_flags(bfdHandle) & HAS_SYMS;

    if (!(r1 && r2 && r3))
    {
        bfd_close(bfdHandle);
        crashFile << "Failed to init bfd from " << procName << std::endl;
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
            crashFile << "Failed to read symbols from " << procName << std::endl;
            return 1;
        }
    }

    bfdCtx->handle = bfdHandle;
    bfdCtx->symbol = static_cast<asymbol **>(symbolTable);

    return 0;
}

void StackTraceWinMinGW::bfdCloseCtx(struct BfdCtx* bfdCtx)
{
    if(bfdCtx)
    {
        if(bfdCtx->symbol)
            free(bfdCtx->symbol);

        if(bfdCtx->handle)
            bfd_close(bfdCtx->handle);
    }
}

LONG WINAPI StackTraceWinMinGW::exceptionFilter(LPEXCEPTION_POINTERS info)
{
    std::ofstream crashFile(mInstance->mCrashFilePath);

    if (!SymInitialize(GetCurrentProcess(), 0, TRUE))
    {
        crashFile << "Failed to init symbol context" << std::endl;
        return 0;
    }

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
                if(mInstance->bfdInitCtx(crashFile, &newSet.bfdCtx, processName) == 0)
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

        crashFile << frame.AddrPC.Offset << ", " << processName << ", " << data.file << ", " << data.line << ", " << data.func << std::endl;
    }

    crashFile.flush();
    crashFile.close();

    for(struct BfdSet& tmpSet : listSets)
        mInstance->bfdCloseCtx(&tmpSet.bfdCtx);

    listSets.clear();

    SymCleanup(GetCurrentProcess());

    return 0;
}

#endif // defined __MINGW32__
