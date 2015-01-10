/*! \file   StackTracePrint.h
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

#ifndef __STACKTRACE_H__
#define __STACKTRACE_H__

#include <execinfo.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ucontext.h>
#include <unistd.h>

#include <cxxabi.h>
#include <iostream>

namespace StackTracePrint
{

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

//! \brief Handles critical error stack trace printing.
void critErrHandler(int sig_num, siginfo_t* info, void* ucontext);

}

#endif // __STACKTRACE_H__
