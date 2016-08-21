#ifndef BR_TRANSLATE_H
#define BR_TRANSLATE_H

/*

Translation tables for encoding addresses/commands for the X10 FireCracker
  home automation kit

(c) 1999 Tymm Twillman (tymm@acm.org).  Free Software.  LGPL applies.
  No warranties expressed or implied.

*/


/*
 * (for error checking)
 */

#define MAX_CMD     8
#define MAX_housecode  15
#define MAX_DEVICE 15
/*
 * Used to create letter housecode part of a device address -- could use some 
 *  bit magic but this is less of a pain and easier to read
 */

static char housecode_table[] = {
  /* A */ 0x06, /* B */ 0x07, /* C */ 0x04, /* D */ 0x05,
  /* E */ 0x08, /* F */ 0x09, /* G */ 0x0a, /* H */ 0x0b,
  /* I */ 0x0e, /* J */ 0x0f, /* K */ 0x0c, /* L */ 0x0d,
  /* M */ 0x00, /* N */ 0x01, /* O */ 0x02, /* P */ 0x03
};

/*
 * For number part of device address
 */

static char device_table[][2] = {
	/*   1-4 */ {0x00, 0x00}, {0x00, 0x10}, {0x00, 0x08}, {0x00, 0x18},
	/*   5-8 */ {0x00, 0x40}, {0x00, 0x50}, {0x00, 0x48}, {0x00, 0x58},
	/*  9-12 */ {0x04, 0x00}, {0x04, 0x10}, {0x04, 0x08}, {0x04, 0x18},
	/* 13-16 */ {0x04, 0x40}, {0x04, 0x50}, {0x04, 0x48}, {0x04, 0x58}
};

/*
 * For encoding the command
 */

static char cmd_table[] = {
    /* off */       0x00, /* on */       0x20,
    /* dim */       0x98, /* bright */   0x88,
    /* all off */   0x80, /* all on */   0x91, 
    /* lamps off */ 0x84, /* lamps on */ 0x94,
    /* (pause; shouldn't ever use this entry, "on" seems safest) */ 0x20
};

#endif

