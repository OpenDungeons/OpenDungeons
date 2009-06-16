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
#  include <stdlib.h>
#  define sleep(t) _sleep((t) * 1000)
# else
#  include <windows.h>
#  define sleep(t)  Sleep((t) * 1000)
# endif
# ifndef _NEED_SLEEP_ONLY
#  define msleep(t) Sleep(t)
#  define usleep(t) Sleep((t) / 1000)
# endif
#else
# include <unistd.h>
# ifndef _NEED_SLEEP_ONLY
#  define msleep(t) usleep((t) * 1000)
# endif
#endif

#endif /* _SLEEP_H */

