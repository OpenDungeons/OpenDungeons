/*
 * sleep.h  1.0 02/03/10
 *
 * Defines cross-platform sleep, usleep, etc.
 *
 * By Wu Yongwei
 *
 * All code here is considered in the public domain though I do wish my
 * name could be retained if anyone uses them. :-)
 *
 */

#ifndef _SLEEP_H
#define _SLEEP_H

#ifdef _WIN32
# if defined(_NEED_SLEEP_ONLY) && (defined(_MSC_VER) || defined(__MINGW32__))
#  include <cstdlib>
#  define OD_SLEEP(t) _sleep((t) * 1000)
# else
#  include <windows.h>
#  define OD_SLEEP(t)  Sleep((t) * 1000)
# endif
# ifndef _NEED_SLEEP_ONLY
#  define OD_MSLEEP(t) Sleep(t)
#  define OD_USLEEP(t) Sleep((t) / 1000)
# endif
#else
# include <unistd.h>
# ifndef _NEED_SLEEP_ONLY
#  define OD_MSLEEP(t) usleep((t) * 1000)
# endif
#endif

#endif /* _SLEEP_H */

