/*
 *
 * BottleRocket command output functions.  x10_br_out, br_error, BitWait,
 *  Delay, HoldLoops and br_error_handler are accessable to programs.
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

#ifdef __cplusplus
extern C {
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <unistd.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <string.h>

#ifdef HAVE_ERRNO_H
#include <errno.h>
#else
extern int errno;
#endif

#ifdef HAVE_SYS_TERMIOS_H
#include <sys/termios.h>
#endif

#ifdef HAVE_TERMIOS_H
#include <termios.h>
#endif

#include "br_cmd.h"
#include "br_translate.h"


#ifndef TIOCM_FOR_0
#define TIOCM_FOR_0 TIOCM_DTR
#endif

#ifndef TIOCM_FOR_1
#define TIOCM_FOR_1 TIOCM_RTS
#endif

/*
 * These values should be good for pretty much everyone, but you can
 *   try increasing them if you have problems.  Values are in uSec;
 *   PreCmdDelay is the amount of time to hold output lines in
 *   "clock" position before a command, PostCmdDelay is how long
 *   to stay in "clock" position after a command, and InterBitDelay
 *   is how long each bit/clock wiggled out the serial port should
 *   last.
 */

int br_pre_cmd_delay = 350000;   /* empirically found... */
int br_post_cmd_delay = 350000;
int br_inter_bit_delay = 1400;

int br_verbose = 0;

const char *br_cmd_list[] = {
    "ON",
    "OFF",
    "DIM",
    "BRIGHT",
    "ALL OFF",
    "ALL ON",
    "ALL LAMPS OFF",
    "ALL LAMPS ON",
    "PAUSE"
};


/*
 * Can externally set the error handler in case an app wants to handle
 *  the output for itself
 *
 */

static void br_int_err_handler(char *, char *);

void (*br_error_handler)(char *, char *) = br_int_err_handler;

void br_error(char *where, char *problem)
{
    int tmperrno;


    tmperrno = errno;

    if (br_error_handler)
        (*br_error_handler)(where, problem);

    errno = tmperrno;
}

static void br_int_err_handler(char *where, char *problem)
{
    int tmperrno = errno;


    fprintf(stderr, "error: ");

    if (tmperrno)
        fprintf(stderr, "[%s] ", strerror(tmperrno));

    if (problem)
        fprintf(stderr, "%s ", problem);

    if (where)
        fprintf(stderr, "in %s", where);

    if (!where && !problem && !tmperrno)
        fprintf(stderr, "(unknown error)");

    fprintf(stderr, "\n");
}

static int usec_sleep(long usecs)
{
    /*
     * Sleep for a little while.  Using select() so we don't busy-
     *  wait for this long.
     */

    struct timeval sleeptime;

    sleeptime.tv_sec = usecs / 1000000;
    sleeptime.tv_usec = usecs % 1000000;

    if (select(0, NULL, NULL, NULL, &sleeptime) < 0) {
        br_error("usec_sleep", "select");
        return -1;
    }

    return 0;
}

static int usec_delay(long usecs)
{
    /*
     * busy-wait for short delays
     */

    struct timeval endtime;
    struct timeval currtime;

    /*
     * This way of doing things stolen from Firecracker, by Chris Yokum.
     *   Much better.  What was I thinking before?
     */

    if (gettimeofday(&endtime, NULL) < 0) {
        br_error("usec_delay", "gettimeofday");
        return -1;
    }

    endtime.tv_usec += usecs;

    if (endtime.tv_usec >= 1000000) {
        endtime.tv_sec++;
        endtime.tv_usec -= 1000000;
    }

    do {
        if (gettimeofday(&currtime, NULL) < 0) {
            br_error("usec_delay", "gettimeofday");
            return -1;
        }
    } while (timercmp(&endtime, &currtime, >));

    return 0;
}

static int bits_out(const int fd, const int bits)
{
    /*
     * Send out one command bit; set RTS or DTR (but only one) depending on
     *  value.
     *
     * This now assumes that both RTS and DTR are high; it just clears the one
     *  that we don't want set for this bit.
     */

    int out;


    out = (bits) ? TIOCM_FOR_0:TIOCM_FOR_1;

    /* Set RTS, DTR to desired settings */

    if (ioctl(fd, TIOCMBIC, &out) < 0) {
        br_error("bits_out", "ioctl");
        return -1;
    }

    if (usec_delay(br_inter_bit_delay) < 0)
        return -1;

    return 0;
}

static int clock_out(const int fd)
{
    /*
     * Send out a "clock pulse" -- both RTS and DTR set; used before/after
     *  command (long pulse) and between command bits (short)
     */

    int out = TIOCM_FOR_0 | TIOCM_FOR_1;


    if (ioctl(fd, TIOCMBIS, &out) < 0) {
        br_error("clock_out", "ioctl");
        return -1;
    }

    if (usec_delay(br_inter_bit_delay) < 0)
        return -1;
    
    return 0;
}


int br_cmd(int fd, unsigned char unit, int cmd)
{

    /*
     * Put together the commands to send out.  The basic start and end of
     *  each command is the same; just fill in the little bits in the middle
     *
     */

    unsigned char cmd_seq[5] = { 0xd5, 0xaa, 0x00, 0x00, 0xad };

    register int i;
    register int j;
    unsigned char byte;
    int out;
    int housecode;
    int serial_state;
    int device;
#ifdef USE_CLOCAL
    struct termios termios;
    struct termios tmp_termios;
#endif


    if (cmd > MAX_CMD || cmd < 0)
        return -1;

    /*
     * Make sure to set the numeric part of the device address to 0
     *  for dim/bright (they only work per housecode)
     */
    
    if ((cmd == DIM) || (cmd == BRIGHT))
        unit &= 0xf0;

    if (br_verbose >= 2) {
        if (cmd == PAUSE) {
            printf("Pausing 1 second\n");
        } else if (ISDIMCMD(cmd)) {
            printf("Sending command %s to %c\n",
              br_cmd_list[cmd], 'A' + ((unit & 0xf0) >> 4));
        } else {
            printf("Sending command %s to %c%d\n",
              br_cmd_list[cmd], 'A' + ((unit & 0xf0) >> 4),
              (unit & 0x0f) + 1);
        }
    }

    if (cmd == PAUSE) {
        return usec_sleep(1000000);
    }

#ifdef USE_CLOCAL
    if (tcgetattr(fd, &termios) < 0) {
        br_error("br_cmd", "tcgetattr");
        return -1;
    }

    tmp_termios = termios;

    tmp_termios.c_cflag |= CLOCAL;

    if (tcsetattr(fd, TCSANOW, &tmp_termios) < 0) {
        br_error("br_cmd", "tcsetattr");
        return -1;
    }
#endif

    /*
     * Save current state of bits we don't want to touch in serial
     *  register
     */
    
    if (ioctl(fd, TIOCMGET, &serial_state) < 0) {
        br_error("br_cmd", "ioctl");
        return -1;
    }
    
    /*
     * Just keep state of lines to be mucked with
     */

    serial_state &= (TIOCM_FOR_0 | TIOCM_FOR_1);
    
     /* Figure out which ones we're going to want to clear
     *  when finished (they'll both be high after the last
     *  clock_out)
     */

    serial_state ^= (TIOCM_FOR_0 | TIOCM_FOR_1);

    housecode = unit >> 4;
    device = unit & 0x0f;

    /*
     * Slap together the variable part of a command
     */

    cmd_seq[2] |= housecode_table[housecode] << 4 | device_table[device][0];
    cmd_seq[3] |= device_table[device][1] | cmd_table[cmd];

    /*
     * Set lines to clock and wait, to make sure receiver is ready
     */

    if (clock_out(fd) < 0)
        return -1;

    if (usec_sleep(br_pre_cmd_delay) < 0)
        return -1;

    if (br_verbose == 5) {
        printf("              -------HEAD------ -----COMMAND----- --FOOT--\n");
        printf("sending bits: ");
    } else if (br_verbose == 4) {
        printf("Sending bytes: ");
    }

    for (j = 0; j < 5; j++) {
        byte = cmd_seq[j];

        if (br_verbose == 4)
            printf("%02x", (unsigned int)byte);

        /*
         * Roll out the bits, following each one by a "clock".
         */

        for (i = 0; i < 8; i++) {
            out = (byte & 0x80) ? 1:0;
            byte <<= 1;

            if (br_verbose == 5)
                printf("%d", out);

            if ((bits_out(fd, out) < 0) || (clock_out(fd) < 0))
                return -1;
        }

        if ((br_verbose == 4) || (br_verbose == 5))
            printf(" ");
    }

    if ((br_verbose == 4) || (br_verbose == 5)) {
        printf("\n");
        fflush(stdout);
    }

    /*
     * Close with a clock pulse and wait a bit to allow command to complete
     */

    if (clock_out(fd) < 0)
        return -1;

    if (usec_sleep(br_post_cmd_delay) < 0)
        return -1;

    /*
     * Restore the serial lines to how we found them
     */

    if (ioctl(fd, TIOCMBIC, &serial_state) < 0) {
       br_error("x10_br_out", "ioctl");
       return -1;
    }

#ifdef USE_CLOCAL
    if (tcsetattr(fd, TCSANOW, &termios) < 0) {
        br_error("br_cmd", "tcsetattr");
        return -1;
    }
#endif

    return 0;
}

#ifdef __cplusplus
}
#endif
