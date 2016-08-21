#ifndef X10_BR_CMD_H
#define X10_BR_CMD_H

/*
 *
 * Header file for BottleRocket command output functions
 *  (c) 1999 Tymm Twillman (tymm@acm.org)
 *
 * This is for interfacing with the X10 Dynamite wireless transmitter for X10
 * home automation hardware.
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

#define DIMRANGE 12
#define ISDIMCMD(cmd) ((cmd == DIM) || (cmd == BRIGHT))
#define CMDHASDEVS(cmd) ((cmd == ON) || (cmd == OFF))  /* command need device to work on? */
#define HOUSENAME(house) (((house < 0) || (house > 15)) ? \
          '?':"ABCDEFGHIJKLMNOP"[(int)house])
#define DEVNAME(dev) (((dev < 0) || (dev > 16)) ? 0 : dev + 1)

/* ugly or what? */
#define HOUSECODE(val) (HOUSENAME(toupper(val) - 'A') != '?' ? (toupper(val) - 'A'):-1)



/*
 * Commands allowed for x10_br_out
 */

#define ON 0
#define OFF 1
#define DIM 2
#define BRIGHT 3
#define ALL_OFF 4
#define ALL_ON 5
#define ALL_LAMPS_OFF 6
#define ALL_LAMPS_ON 7
#define PAUSE 8

extern const char *br_cmd_list[];

int br_cmd(int /* file desc */, unsigned char /* address */, int /* cmd */);
void br_error(char * /* where */, char * /* problem */);

/*
 * Shouldn't need to mess with timing things, but just in case you want to...
 */

extern int br_pre_cmd_delay;
extern int br_post_cmd_delay;
extern int br_inter_bit_delay;

/*
 * How verbose should we be?
 */

extern int br_verbose;

/*
 * In case an application wants to handle the errors for itself, it can
 *  change this to point to its own error handler.
 */

extern void (*br_error_handler)(char * /* where */, char * /*problem */);

#endif
