/*
 * br_cmd_engine.c -- Command processing functions for BottleRocket routines
 *  for controlling the X10 FireCracker wireless home automation kit.
 *  (c) 1999 by Tymm Twillman (tymm@acm.org).
 *
 *
 *   This library is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU Lesser General Public
 *   License as published by the Free Software Foundation; either
 *   version 2 of the License, or (at your option) any later version.
 *
 *   This library is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *   Lesser General Public License for more details.
 *
 *   You should have received a copy of the GNU Lesser General Public
 *   License along with this library; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *
 */

#ifdef __cplusplus
extern C {
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#include "br_cmd.h"
#include "br_cmd_engine.h"



int br_default_house = 0;

int br_inverse_cmd(int cmd)
{
    switch (cmd) {
        case ON:
            return OFF;
            break;
        case OFF:
            return ON;
            break;
        case DIM:
            return BRIGHT;
            break;
        case BRIGHT:
            return DIM;
            break;
        case ALL_ON:
            return ALL_OFF;
            break;
        case ALL_OFF:
            return ALL_ON;
            break;
        case ALL_LAMPS_ON:
            return ALL_LAMPS_OFF;
            break;
        case ALL_LAMPS_OFF:
            return ALL_LAMPS_ON;
            break;
    }

    return -1;
}

int br_execute(int fd, br_control_info *cinfo)
{
/*
 * Run through a list of commands and execute them
 */

    register int i;
    register int j;
    register int repeat = cinfo->repeat;
    int inverse = cinfo->inverse;
    char unit;
    int rv;

    if (cinfo == NULL) {
        errno = EINVAL;
        br_error("br_execute", "NULL control info pointer");
        return -1;
    }

    if (cinfo->units == NULL) {
        errno = EINVAL;
        br_error("br_execute", "NULL unit list pointer");
        return -1;
    }

    /* However many times we have to repeat this thing... */

    for (; repeat > 0; repeat--) {

        /* Do for each command in the command list... */

        for (i = 0; i < cinfo->numcmds; i++) {

            /* For each device in the device list for that command */

            if (CMDHASDEVS(cinfo->cmds[i]) && cinfo->units[i]->devs == NULL) {
                errno = EINVAL;
                br_error("br_execute", "NULL device list");
                return -1;
            }

            for (j = 0; j < (CMDHASDEVS(cinfo->cmds[i]) ? cinfo->units[i]->numunits:1); j++) {

                unit = ((char)cinfo->units[i]->houses[j] << 4)
                  | (CMDHASDEVS(cinfo->cmds[i]) ? cinfo->units[i]->devs[j]:0);

                rv = br_cmd(fd, unit,
                  (inverse < 0) ? br_inverse_cmd(cinfo->cmds[i]):cinfo->cmds[i]);
                if (rv < 0)
                    return -1;
            }
        }

        if (inverse) inverse = 0 - inverse;
    }

    return 0;
}

br_unit_list *br_new_unit_list()
{
    br_unit_list *units;

    units = malloc(sizeof(br_unit_list));

    if (units == NULL) {
        br_error("br_new_unit_list", "malloc");
        return NULL;
    }

#ifdef MEM_DEBUG
    printf("br_new_unit_list: Malloced %d bytes at %lx\n",
      sizeof(br_unit_list), (unsigned long)units);
#endif

    units->allocatedunits = 0;
    units->numunits = 0;
    units->devs = NULL;
    units->houses = NULL;

    return units;
}

int br_free_unit_list(br_unit_list *units)
{
    if (units == NULL)
        return 0;

    if (units->devs != NULL) {

#ifdef MEM_DEBUG
        printf("br_free_unit_list: Freeing memory at %lx\n",
          (unsigned long)units->devs);
#endif

        free(units->devs);
    }

    if (units->houses != NULL) {

#ifdef MEM_DEBUG
        printf("br_free_unit_list: Freeing memory at %lx\n",
            (unsigned long)units->houses);
#endif

        free(units->houses);
    }

#ifdef MEM_DEBUG
    printf("br_free_unit_list: Freeing memory at %lx\n",
       (unsigned long)units);
#endif

    free(units);

    return 0;
}

int br_add_unit(br_unit_list *units, int house, int dev)
{
    if (units == NULL) {
        errno = EINVAL;
        br_error("br_add_unit", "NULL unit list");
        return -1;
    }

    if (units->numunits >= units->allocatedunits) {

#ifdef MEM_DEBUG
     printf("br_add_unit: Reallocing memory from %lx...\n",
      (unsigned long)units->devs);
#endif

        units->devs = realloc(units->devs, (units->allocatedunits + UNIT_BLKSIZE) * sizeof(int));

        if (units->devs == NULL) {
            br_error("br_add_unit", "realloc");
            return -1;
        }

#ifdef MEM_DEBUG
    printf("br_add_unit: Realloced %d bytes at %lx\n",
      (units->allocatedunits + UNIT_BLKSIZE) * sizeof(int), (unsigned long)units->devs);

     printf("br_add_unit: Reallocing memory from %lx...\n",
      (unsigned long)units->houses);
#endif

        units->houses = realloc(units->houses, (units->allocatedunits + UNIT_BLKSIZE) * sizeof(int));

        if (units->houses == NULL) {
            br_error("br_add_unit", "realloc");
            return -1;
        }

#ifdef MEM_DEBUG
    printf("br_add_unit: Realloced %d bytes at %lx\n",
      (units->allocatedunits + UNIT_BLKSIZE) * sizeof(int), (unsigned long)units->houses);
#endif

        units->allocatedunits += UNIT_BLKSIZE;

    }

    units->devs[units->numunits] = dev;
    units->houses[units->numunits] = house;

    units->numunits++;

    return 0;
}

int br_del_unit(br_unit_list *units, int house, int dev)
{
    register int i;
    int moveby = 0;


    if (units == NULL) {
        errno = EINVAL;
        br_error("br_del_unit", "NULL unit list");
        return -1;
    }

    for (i = 0; i < (units->numunits - moveby); i++) {
        if (
          ((units->devs && (units->devs[i] == dev))
            || (dev == 0))
          && ((units->houses && (units->houses[i] == house))
            || (house == 0))
        )
        {
            moveby++;
        }

        if (units->devs)
            units->devs[i] = units->devs[i + moveby];

        if (units->houses)
            units->houses[i] = units->houses[i + moveby];
    }

    units->numunits -= moveby;

    if (units->numunits == 0) {

        if (units->devs) {

#ifdef MEM_DEBUG
        printf("br_del_unit: Freeing memory at %lx\n",
          (unsigned long)units->devs);
#endif

            free(units->devs);
            units->devs = NULL;
        }

        if (units->houses) {

#ifdef MEM_DEBUG
        printf("br_del_unit: Freeing memory at %lx\n",
          (unsigned long)units->houses);
#endif

            free(units->houses);
            units->houses = NULL;
        }

        units->numunits = 0;
        units->allocatedunits = 0;
    }

    return 0;
}


br_control_info *br_new_control_info()
{
    br_control_info *cinfo;


    cinfo = malloc(sizeof(br_control_info));

    if (cinfo == NULL) {
        br_error("br_new_control_info", "malloc");
        return NULL;
    }

#ifdef MEM_DEBUG
    printf("br_new_control_info: Malloced %d bytes at %lx\n", sizeof(br_control_info),
      (unsigned long)cinfo);
#endif

    cinfo->inverse = 0;
    cinfo->repeat = 1;
    cinfo->numcmds = 0;
    cinfo->allocatedcmds = 0;
    cinfo->units = NULL;
    cinfo->cmds = NULL;

    return cinfo;
}

int br_free_control_info(br_control_info *cinfo)
{
    if (cinfo) {
        br_free_cmds(cinfo);

#ifdef MEM_DEBUG
        printf("br_free_control_info: Freeing memory at %lx\n",
          (unsigned long)cinfo);
#endif

        free(cinfo);
    }

    return 0;
}

int br_malloc_cmds(br_control_info *cinfo, int numcmds)
{
    if (cinfo == NULL) {
        errno = EINVAL;
        br_error("br_malloc_cmds", "NULL control info pointer");
        return -1;
    }

    cinfo->cmds = malloc(numcmds * sizeof(int));

    if ((cinfo->cmds) == NULL) {
        br_error("br_malloc_cmds", "malloc");
        return -1;
    }

#ifdef MEM_DEBUG
    printf("br_malloc_cmds: Malloced %d bytes at %lx\n",
      numcmds * sizeof(int), (unsigned long)cinfo->cmds);
#endif

    cinfo->units = malloc(numcmds * sizeof(br_unit_list *));

    if ((cinfo->units) == NULL) {
        br_error("br_malloc_cmds", "malloc");
        return -1;
    }

#ifdef MEM_DEBUG
    printf("br_malloc_cmds: Malloced %d bytes at %lx\n",
      numcmds * sizeof(br_unit_list *), (unsigned long)cinfo->units);
#endif

    cinfo->allocatedcmds = numcmds;

    return 0;
}

int br_realloc_cmds(br_control_info *cinfo, int numcmds)
{
    if (cinfo == NULL) {
        errno = EINVAL;
        br_error("br_realloc_cmds", "NULL control info pointer");
        return -1;
    }

#ifdef MEM_DEBUG
    printf("br_realloc_cmds: Reallocing memory from %lx...\n",
      (unsigned long)cinfo->cmds);
#endif

    cinfo->cmds = realloc(cinfo->cmds, numcmds * sizeof(int));

    if ((cinfo->cmds) == NULL) {
        br_error("br_realloc_cmds", "realloc");
        return -1;
    }

#ifdef MEM_DEBUG
    printf("br_realloc_cmds: Realloced %d bytes at %lx\n", numcmds * sizeof(int),
      (unsigned long)cinfo->cmds);

    printf("br_realloc_cmds: Reallocing memory from %lx...\n",
      (unsigned long)cinfo->units);
#endif

    cinfo->units = realloc(cinfo->units, numcmds * sizeof(br_unit_list *));

    if ((cinfo->units) == NULL) {
        br_error("br_realloc_cmds", "realloc");
        return -1;
    }

#ifdef MEM_DEBUG
    printf("br_realloc_cmds: Realloced %d bytes at %lx\n", numcmds * sizeof(br_unit_list *),
      (unsigned long)cinfo->units);

#endif

    cinfo->allocatedcmds = numcmds;

    return 0;
}

int br_free_cmds(br_control_info *cinfo)
{
    register int i;


    if (cinfo == NULL) {
        return 0;
    }

    if (cinfo->cmds) {

#ifdef MEM_DEBUG
        printf("br_free_cmds: Freeing memory at %lx\n",
          (unsigned long)cinfo->cmds);
#endif

       free(cinfo->cmds);
        cinfo->cmds = NULL;
    }

    if (cinfo->units) {
        for (i = 0; i < cinfo->numcmds; i++) {
                br_free_unit_list(cinfo->units[i]);
                cinfo->units[i] = NULL;
        }

#ifdef MEM_DEBUG
        printf("br_free_cmds: Freeing memory at %lx\n",
          (unsigned long)cinfo->units);
#endif

        free(cinfo->units);
        cinfo->units = NULL;
    }

    cinfo->numcmds = 0;
    cinfo->allocatedcmds = 0;

    return 0;
}

int br_add_ul_cmd(br_control_info *cinfo, int cmd, br_unit_list *units)
{
    /*
     * Add a command, plus devices for it to act on and other info, to the
     *  list of commands to be executed
     */

    br_unit_list *tmpunits;


    if (cinfo == NULL) {
        errno = EINVAL;
        br_error("br_add_ul_cmd", "NULL control info pointer");
        return -1;
    }

    if (units == NULL) {
        errno = EINVAL;
        br_error("br_add_ul_cmd", "NULL unit list pointer");
        return -1;
    }

    tmpunits = br_uldup(units);

    if (tmpunits == NULL) {
        br_error("br_add_ul_cmd", "malloc");
        return -1;
    }

    if (cinfo->numcmds >= cinfo->allocatedcmds) {
        if (br_realloc_cmds(cinfo, cinfo->numcmds + CMD_BLKSIZE) < 0) {
            br_free_unit_list(tmpunits);
            return -1;
        }
    }

    cinfo->cmds[cinfo->numcmds] = cmd;
    cinfo->units[cinfo->numcmds] = tmpunits;

    cinfo->numcmds++;

    return 0;
}

int br_add_cmd(br_control_info *cinfo, int cmd, int house, int dev)
{
    br_unit_list *units;
    int tmperrno;


    units = br_new_unit_list();

    if (units == NULL)
        return -1;

    if (br_add_unit(units, house, dev) < 0) {
        tmperrno = errno;
        br_free_unit_list(units);
        errno = tmperrno;
        return -1;
    }

    if (br_add_ul_cmd(cinfo, cmd, units) < 0) {
        tmperrno = errno;
        br_free_unit_list(units);
        errno = tmperrno;
        return -1;
    }

    br_free_unit_list(units);

    return 0;
}

int br_del_cmd(br_control_info *cinfo, int index)
{
    register int i;

    if (cinfo == NULL) {
        errno = EINVAL;
        br_error("br_del_cmd", "NULL control info pointer");
        return -1;
    }

    if (index >= cinfo->numcmds) {
        errno = EINVAL;
        br_error("br_del_cmd", "invalid command index");
        return -1;
    }

    if ((cinfo->numcmds - 1) == 0) {
        if (br_free_cmds(cinfo) < 0)
            return -1;

        return 0;
    }

    br_free_unit_list(cinfo->units[index]);
    cinfo->units[index] = NULL;

    for (i = index; i < cinfo->numcmds - 1; i++) {
        cinfo->cmds[i] = cinfo->cmds[i + 1];
        cinfo->units[i] = cinfo->units[i + 1];
    }

    cinfo->numcmds--;

    return 0;
}

int br_strtoul(char *ulptr, br_unit_list *units, char **endptr)
{
    int house;
    int tmphouse;
    int dev;
    char *my_endptr = NULL;
    char *last_endptr = ulptr;


    house = br_default_house;

    if (units == NULL) {
        errno = EINVAL;
        br_error("br_strtoul", "NULL unit list");
        return -1;
    }

    /* Get rid of any residue */

    if (units->devs)
        free(units->devs);

    if (units->houses)
        free(units->houses);

    units->devs = NULL;
    units->houses = NULL;
    units->allocatedunits = 0;
    units->numunits = 0;

    do {
        while (isspace(*ulptr))
            ulptr++;

        tmphouse = br_strtohc(ulptr, &ulptr);

        if (tmphouse >= 0)
            house = tmphouse;

        last_endptr = ulptr;

        while (isspace(*ulptr))
            ulptr++;

        dev = (int)strtol(ulptr, &my_endptr, 0);

        if ((dev > 16) || (dev < 1))
        {
            errno = EINVAL;
            br_error("br_strtoul", "Bad device number");
            return -1;
        }

        last_endptr = ulptr;

        while (isspace(*my_endptr))
            my_endptr++;

        ulptr = my_endptr;

        if ((*my_endptr != '\0') && (*my_endptr != ',')) {
            *endptr = last_endptr;
            return 0;
        }

        if (br_add_unit(units, house, dev - 1) < 0)
            return -1;
    } while (*ulptr++);

    *endptr = --ulptr;

    return 0;
}

int br_ulcat(br_unit_list *a, br_unit_list *b)
{
    register int i;


    if ((a == NULL) || (b == NULL)) {
        errno = EINVAL;
        br_error("br_ulcat", "NULL unit list");
        return -1;
    }

    for (i = 0; i < b->numunits; i++) {
        if (br_add_unit(a, b->houses[i], b->devs[i]) < 0)
            return -1;
    }

    return 0;
}

br_unit_list *br_uldup(br_unit_list *a)
{
    br_unit_list *units;
    register int i;


    units = br_new_unit_list();

    if (units == NULL)
        return NULL;


    if (a->allocatedunits) {
        units->devs = malloc(sizeof(int) * a->allocatedunits);

        if (units->devs == NULL) {
            br_error("br_uldup", "malloc");
            return NULL;
        }

#ifdef MEM_DEBUG
        printf("br_dldup: Malloced %d bytes at %lx\n",
          sizeof(int) * a->allocatedunits, devs->devs);
#endif

        units->houses = malloc(sizeof(int) * a->allocatedunits);

        if (units->houses == NULL) {
            br_error("br_uldup", "malloc");
            return NULL;
        }

 #ifdef MEM_DEBUG
        printf("br_dldup: Malloced %d bytes at %lx\n",
          sizeof(int) * a->allocatedunits, devs->houses);
#endif

        for (i = 0; i < a->numunits; i++) {
            units->devs[i] = a->devs[i];
            units->houses[i] = a->houses[i];
        }
    }

    units->numunits = a->numunits;
    units->allocatedunits = a->allocatedunits;

    return units;
}

int br_strtohc(char *hcptr, char **endptr)
{
    char *my_endptr = hcptr;
    char c;


    while (isspace(*my_endptr))
        my_endptr++;

    if (!*my_endptr) {
        *endptr = hcptr;
        return -1;
    }

    c = HOUSECODE(*my_endptr);

    if (c < 0) {
        *endptr = hcptr;
        return -1;
    }

    *endptr = ++my_endptr;

    return c;
}

int br_get_ul_device(br_unit_list *units, int index)
{
    if (units == NULL)
        return -1;

    if (index >= units->numunits)
        return -1;

    return units->devs[index];
}

int br_get_ul_house(br_unit_list *units, int index)
{
    if (units == NULL)
        return -1;

    if (index > units->numunits)
        return -1;

    return units->houses[index];
}

int br_get_num_units(br_unit_list *units)
{
    if (units == NULL)
        return 0;

    return units->numunits;
}

int br_get_num_commands(br_control_info *cinfo)
{
    if (cinfo == NULL)
        return 0;

    return cinfo->numcmds;
}

#ifdef __cplusplus
}
#endif
