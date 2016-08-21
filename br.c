
/*
 *
 * br (BottleRocket)
 *
 * Control software for the X10 FireCracker wireless computer
 * interface kit.
 *
 * (c) 1999 Ashley Clark (aclark@ghoti.org) and Tymm Twillman (tymm@acm.org)
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

#define VERSION "0.05b3"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <unistd.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <stdlib.h>
#include <ctype.h>
#include <limits.h>
#include <string.h>

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#ifdef HAVE_FEATURES_H
#include <features.h>
#endif

#ifdef HAVE_GETOPT_LONG
#include <getopt.h>
#endif

#include "br.h"
#include "br_cmd.h"
#include "br_cmd_engine.h"

int Verbose = 0;
char *MyName;
void (*saved_br_error_handler)(char *, char *);

void usage()
{
    fprintf(stderr, "BottleRocket version %s\n", VERSION);
    fprintf(stderr, "\n");
    fprintf(stderr, "Usage: %s [<options>][<housecode>(<list>) "
      "<native command> ...]\n\n", MyName);
    fprintf(stderr, "  Options:\n");
#ifdef HAVE_GETOPT_LONG
    fprintf(stderr, "  -v, --verbose\t\t\tadd v's to increase verbosity\n");
    fprintf(stderr, "  -x, --port=PORT\t\tset port to use\n");
    fprintf(stderr, "  -c, --house=[A-P]\t\tuse alternate house code "
      "(default \"A\")\n");
    fprintf(stderr, "  -n, --on=LIST\t\t\tturn on devices in LIST\n");
    fprintf(stderr, "  -f, --off=LIST\t\tturn off devices in LIST\n");
    fprintf(stderr, "  -N, --ON\t\t\tturn on all devices in housecode\n");
    fprintf(stderr, "  -F, --OFF\t\t\tturn off all devices in housecode\n");
    fprintf(stderr, "  -d, --dim=LEVEL[,LIST]\tdim devices in housecode to "
      " relative LEVEL\n");
    fprintf(stderr, "  -B, --lamps_on\t\tturn all lamps in housecode on\n");
    fprintf(stderr, "  -D, --lamps_off\t\tturn all lamps in housecode off\n");
    fprintf(stderr, "  -r, --repeat=NUM\t\trepeat commands NUM times "
      "(0 = ~ forever)\n");
    fprintf(stderr, "  -h, --help\t\t\tthis help\n\n");
#else
    fprintf(stderr, "  -v\tverbose (add v's to increase verbosity)\n");
    fprintf(stderr, "  -x\tset port to use\n");
    fprintf(stderr, "  -c\tuse alternate house code (default \"A\")\n");
    fprintf(stderr, "  -n\tturn on devices\n");
    fprintf(stderr, "  -f\tturn off devices\n");
    fprintf(stderr, "  -N\tturn all devices in housecode on\n");
    fprintf(stderr, "  -F\tturn all devices in housecode off\n");
    fprintf(stderr, "  -d\tdim devices in housecode to relative dimlevel\n");
    fprintf(stderr, "  -B\tturn all lamps in housecode on\n");
    fprintf(stderr, "  -D\tturn all lamps in housecode off\n");
    fprintf(stderr, "  -r\trepeat commands <repeats> times (0 basically "
      "means don't stop)\n");
    fprintf(stderr, "  -h\tthis help\n\n");
#endif
    fprintf(stderr, "<list>\t\tis a comma separated list of devices "
      "(no spaces),\n");
    fprintf(stderr, "\t\teach ranging from 1 to 16\n");     
    fprintf(stderr, "<dimlevel>\tis an integer from %d to %d "
      "(0 means no change)\n", -DIMRANGE, DIMRANGE);
    fprintf(stderr, "<housecode>\tis a letter between A and P\n");
    fprintf(stderr, "<native cmd>\tis one of ON, OFF, DIM, BRIGHT, "
      "ALL_ON, ALL_OFF,\n");
    fprintf(stderr, "\t\tLAMPS_ON or LAMPS_OFF\n\n");
    fprintf(stderr, "For native commands, <list> should only be specified "
      "for ON or OFF.\n\n");

}

void my_br_error_handler(char *where, char *problem)
{
    int tmperrno = errno;
    char *tmpwhere;


    /*
     * Don't print out possibly confusing error info
     *  unless in verbose mode
     */

    tmpwhere = (Verbose ? where:NULL);

    fprintf(stderr, "%s: ", MyName);

    errno = tmperrno;

    (*saved_br_error_handler)(tmpwhere, problem);
}

int checkimmutableport(char *port_source)
{
/*
 * Check to see if the user is allowed to specify an alternate serial port
 */

    if (!ISSETID())
        return 0;

    errno = EPERM;
    br_error("checkimmutableport", "You are not authorized to change the X10 port!");
    fprintf(stderr, "\tPort specified %s\n", port_source);

    return -1;
}

int add_dimcmd(br_control_info *cinfo, br_unit_list *units, int dim_level)
{
/*
 * Turn info into stuff usable by the command engine and add it to the
 *  list of things to do.
 */

    register int i;
    int index = 0;
    int dev;
    int house;
    int cmd;


    cmd = ((dim_level < 0) ? DIM:BRIGHT);
    dim_level = (dim_level < 0) ? -dim_level:dim_level;

    if (br_get_num_units(units)) {
        while ((dev = br_get_ul_device(units, index)) != -1) {
            house = br_get_ul_house(units, index);
            if (br_add_cmd(cinfo, ON, house, dev) < 0)
                return -1;
            for (i = 0; i < dim_level; i++) {
                if (br_add_cmd(cinfo, cmd, house, 0) < 0)
                    return -1;
            }
            index++;
        }
    } else {
        for (i = 0; i < dim_level; i++) {
            if (br_add_cmd(cinfo, cmd, br_default_house, 0) < 0)
                return -1;
        }
    }

    return 0;
}

int gethouse(char *house)
{
/*
 * Grab a house code from the command line
 */

    char *end;
    int c;


    c = br_strtohc(house, &end);

    if ((c  < 0) || (*end != '\0')) {
        errno = EINVAL;
        br_error("gethouse", "House code must be in range [A-P]");
        return -1;
    }

    return c;
}

int getunits(char *list, br_unit_list **units)
{
/*
 * Get a list of devices for an operation to be performed on from
 *  the command line
 */
    char *end;


    if (*units == NULL) {
        if ((*units = br_new_unit_list()) == NULL)
            return -1;
    }

    if ((br_strtoul(list, *units, &end) < 0)
      || (*end != '\0'))
    {
        br_free_unit_list(*units);
        errno = EINVAL;
        br_error("getunits", "Devices must be in the range of [1-16], housecodes [A-P]");
        *units = NULL;
        return -1;
    }

    return 0;
}

int getdim(char *list, br_unit_list **units, int *dim)
{
/*
 * Get devices that should be dimmed from the command line, and how
 *  much to dim them
 */    
    
    char *end;


    *dim = strtol(list, &end, 0);

    while (isspace(*end))
        end++;

    if (*end == ',')
        end++;

    if (*end) {
        if (getunits(end, units) < 0)
            return -1;   /* error already printed */
    } else {
        if (*units)
            br_free_unit_list(*units);
        *units = NULL;
    }

    /*
     * May have more dimlevels when I get a chance to play with variable
     *  dimming
     */

    if ((*dim < -DIMRANGE) || (*dim > DIMRANGE))
    {
        if (*units)
            br_free_unit_list(*units);
        *units = NULL;
        errno = EINVAL;
        br_error("getdim", "Invalid dim level");

        return -1;
    }

    return 0;
}

int open_port(br_control_info *cinfo, char *port)
{
/*
 * Open the serial port that a FireCracker device is (we expect) on
 */
    int fd;


    if (Verbose >= 2)
        printf("%s: Opening serial port %s.\n", MyName, port);

    /*
     * Oh, yeah.  Don't need O_RDWR for ioctls.  This is safer.
     */
        
    if ((fd = open(port, O_RDONLY | O_NONBLOCK)) < 0) {
        br_error("open_port", "Unable to open serial port");
        return -1;
    }
    
    /*
     * If we end up with a reserved fd, don't mess with it.  Just to make sure.
     */
    
    if (!SAFE_FILENO(fd)) {
        close(fd);
        errno = EBADF;
        return -1;
    }

    return fd;
}

int close_port(int fd)
{
/*
 * Close the serial port when we're done using it
 */

    if (Verbose >= 2)
        printf("%s: Closing serial port.\n", MyName);

    close(fd);

    return 0;
}

int native_getunits(char *arg, br_unit_list **units)
{
/*
 * Get units to be accessed from the command line in native BottleRocket style
 */

    if (strlen(arg) < 2) {
        errno = EINVAL;
        br_error("native_getunits", "No units specified");
        return -1;
    }

    if (getunits(arg, units) < 0)
        return -1; /* error already printed */

    return 0;
}

int native_getcmd(char *arg)
{
/*
 * Convert a native BottleRocket command to the appropriate token
 */

    if (!strcasecmp(arg, "ON"))
        return ON;

    if (!strcasecmp(arg, "OFF"))
        return OFF;

    if (!strcasecmp(arg, "DIM"))
        return DIM;

    if (!strcasecmp(arg, "BRIGHT"))
        return BRIGHT;

    if (!strcasecmp(arg, "ALL_ON"))
        return ALL_ON;

    if (!strcasecmp(arg, "ALL_OFF"))
        return ALL_OFF;

    if (!strcasecmp(arg, "LAMPS_ON"))
        return ALL_LAMPS_ON;

    if (!strcasecmp(arg, "LAMPS_OFF"))
        return ALL_LAMPS_OFF;

    errno = EINVAL;
    br_error("native_getcmd", "Native br commands are ON, OFF, DIM,\n\t"
      "BRIGHT, ALL_ON, ALL_OFF, LAMPS_ON or LAMPS_OFF");
    errno = EINVAL;

    return -1;
}

int native_cmdline(br_control_info *cinfo, int argc, char *argv[], int optind)
{
/*
 * Get arguments from the command line in native BottleRocket style
 */

    int cmd;
    int i;
    int house;
    br_unit_list *units = NULL;


    if (argc - optind < 2) {
        usage();
        errno = EINVAL;
        return -1;
    }

    /*
     * Two arguments at a time; device and command
     */

    for (i = optind; i < argc - 1; i += 2) {
        cmd = native_getcmd(argv[i + 1]);
        
        switch(cmd) {
            case ON:
                if (native_getunits(argv[i], &units) < 0)
                    return -1;
                if (br_add_ul_cmd(cinfo, ON, units) < 0)
                    return -1;
                break;
        
            case OFF:
                if (native_getunits(argv[i], &units) < 0)
                    return -1;
                if (br_add_ul_cmd(cinfo, OFF, units) < 0)
                    return -1;
                break;
        
            case DIM:
                if ((house = gethouse(argv[i])) < 0)
                    return -1;
                if (br_add_cmd(cinfo, DIM, house, 0) < 0)
                    return -1;
                break;
        
            case BRIGHT:
                if ((house = gethouse(argv[i])) < 0)
                    return -1;
                if (br_add_cmd(cinfo, BRIGHT, house, 0) < 0)
                    return -1;
                break;
    
            case ALL_ON:
                if ((house = gethouse(argv[i])) < 0)
                    return -1;
                if (br_add_cmd(cinfo, ALL_ON, house, 0) < 0)
                    return -1;
                break;
        
            case ALL_OFF:
                if ((house = gethouse(argv[i])) < 0)
                    return -1;
                if (br_add_cmd(cinfo, ALL_OFF, house, 0) < 0)
                    return -1;
                break;
        
            case ALL_LAMPS_ON:
                if ((house = gethouse(argv[i])) < 0)
                    return -1;
                if (br_add_cmd(cinfo, ALL_LAMPS_ON, house, 0) < 0)
                    return -1;
                break;
        
            case ALL_LAMPS_OFF:
                if ((house = gethouse(argv[i])) < 0)
                    return -1;
                if (br_add_cmd(cinfo, ALL_LAMPS_OFF, house, 0) < 0)
                    return -1;
                break;

            default:
                errno = EINVAL;
                return -1;
        }
    }

    if (i != argc) {
        usage();
        errno = EINVAL;
        return -1;
    }

    if (units)
        br_free_unit_list(units);

    return 0;
}        

int main(int argc, char **argv)
{
    char *port_source = "at compile time";
    char *tmp_port;
    char *port = X10_PORTNAME;
    int opt;
    int house = 0;
    int repeat;
    int dimlevel = 0;
    int fd;
    br_control_info *cinfo = NULL;
    br_unit_list *units = NULL;
    
#ifdef HAVE_GETOPT_LONG
    int opt_index;
    static struct option long_options[] = {
        {"help",       no_argument,            0, 'h'},
        {"port",       required_argument,      0, 'x'},
        {"repeat",     required_argument,      0, 'r'},
        {"on",         required_argument,      0, 'n'},
        {"off",        required_argument,      0, 'f'},
        {"ON",         no_argument,            0, 'N'},
        {"OFF",        no_argument,            0, 'F'},
        {"dim",        required_argument,      0, 'd'},
        {"lamps_on",   no_argument,            0, 'B'},
        {"lamps_off",  no_argument,            0, 'D'},
        {"inverse",    no_argument,            0, 'i'},
        {"house",      required_argument,      0, 'c'},
        {"verbose",    no_argument,            0, 'v'},
        {"pause",      no_argument,            0, 'p'},
        {0, 0, 0, 0}
    };
#endif

#define OPT_STRING     "x:hvr:ic:n:Nf:Fd:BDp"

    /*
     * Jimmy in the local error handler that hides the
     *  function where the error occurred (to keep from
     *  being confusing) and also prints the program name
     */

    saved_br_error_handler = br_error_handler;
    br_error_handler = my_br_error_handler;

    MyName = argv[0];

    /*
     * This is where we store all of the info to be passed to the
     *  command engine.
     */

    if ((cinfo = br_new_control_info()) == NULL)
        exit(errno);

    if ((tmp_port = getenv("X10_PORTNAME"))) {
        port_source = "in the environment variable X10_PORTNAME";

        if (checkimmutableport(port_source)) {
            exit(errno);
        } else {
            port = tmp_port;
        }
    }
    
#ifdef HAVE_GETOPT_LONG
    while ((opt = getopt_long(argc, argv, OPT_STRING, long_options,
      &opt_index)) != -1)
    {
#else
    while ((opt = getopt(argc, argv, OPT_STRING)) != -1) {
#endif        
        switch (opt) {
            case 'x':                                  /* Set the port */
                port_source = "on the command line";
                if (checkimmutableport(port_source) < 0)
                    exit(errno);
                port = optarg;
                break;
            case 'r':                                /* Repeat */
                repeat = atoi(optarg);
                if (!repeat && !isdigit(*optarg)) {
                    errno = EINVAL;
                    br_error(NULL, "Invalid repeat value");
                    exit(errno);
                }
                cinfo->repeat = (repeat ? repeat:INT_MAX);
                break;
            case 'v':                                /* Verbose */
                if (Verbose++ == 10)
                    fprintf(stderr, "\nGet a LIFE already.  "
                      "I've got enough v's, thanks.\n\n");
                if (Verbose >= 4)
                    br_verbose = Verbose - 3;
                break;
            case 'i':
                cinfo->inverse++;    /* no this isn't documented.
                                      *   your free gift for reading the source.
                                      */
                break;
            case 'c':                                /* Set housecode */
                if ((br_default_house = gethouse(optarg)) < 0)
                    exit(errno);
		house = br_default_house;
                break;
            case 'n':                                /* Device(s) on */
                if (getunits(optarg, &units) < 0)
                    exit(errno);
                if (br_add_ul_cmd(cinfo, ON, units) < 0)
                    exit(errno);
                break;
            case 'N':                                /* All on */
                if (br_add_cmd(cinfo, ALL_ON, house, 0) < 0)
                    exit(errno);
                break;
            case 'f':                                /* Device(s) off */
                if (getunits(optarg, &units) < 0)
                    exit(errno);
                if (br_add_ul_cmd(cinfo, OFF, units) < 0)
                    exit(errno);
                break;
            case 'F':                                /* All off */
                if (br_add_cmd(cinfo, ALL_OFF, house, 0) < 0)
                    exit(errno);
                break;
            case 'd':                                /* Dim (/bright) */
                if (getdim(optarg, &units, &dimlevel) < 0)
                    exit(errno);
                if (add_dimcmd(cinfo, units, dimlevel) < 0)
                    exit(errno);
                break;
            case 'B':                                /* All lamps on */
                if (br_add_cmd(cinfo, ALL_LAMPS_ON, house, 0) < 0)
                    exit(errno);
                break;
            case 'D':                                /* All lamps off */
                if (br_add_cmd(cinfo, ALL_LAMPS_OFF, house, 0) < 0)
                    exit(errno);
                break;
            case 'p':
                if (br_add_cmd(cinfo, PAUSE, 0, 0) < 0)
                    exit(errno);
                break;
            case 'h':                                /* Help */
                usage();
                exit(0);
            default:                                 /* Someone messed up. */
                usage();
                exit(EINVAL);
        }
    }

    if (argc > optind) {
        /*
         * Must be using the native BottleRocket command line...
         */
    
        if (native_cmdline(cinfo, argc, argv, optind) < 0)
            exit(errno);
    }

    if (!br_get_num_commands(cinfo)) {
        usage();
        exit(EINVAL);
    }

    if ((fd = open_port(cinfo, port)) < 0)
        exit(errno);

    if (Verbose >= 2)
        printf("%s: Executing %d commands\n", MyName,
          br_get_num_commands(cinfo));

    if (br_execute(fd, cinfo) < 0)
        exit(errno);
            
    if (close_port(fd) < 0)
        exit(errno);

    if (Verbose >= 3)
        printf("%s: Cleaning up...\n", MyName);

    br_free_unit_list(units);
    br_free_control_info(cinfo);

    return 0;
}
