#ifndef _BR_H
#define _BR_H

/*
 * br.h -- Macros/definitions for br.c (BottleRocket program for
 *         controlling X10 FireCracker wireless home automation
 *         kits)
 *
 * (c) 1999 Tymm Twillman (tymm@acm.org)
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License
 *   as published by the Free Software Foundation; either version 2
 *   of the License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_ISSETUGID

/*
 * Thanks to Warner Losh for info on how to do this better
 */

#define ISSETID() (issetugid())
#else
#define ISSETID() (getuid() != geteuid() || getgid() != getegid())
#endif

#define SAFE_FILENO(fd) ((fd != STDIN_FILENO) && (fd != STDOUT_FILENO) \
                        && (fd != STDERR_FILENO))



#endif /* #ifdef _BR_H */
