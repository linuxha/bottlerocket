#ifndef _CMD_HANDLING_H
#define _CMD_HANDLING_H

#define CMD_BLKSIZE 64   /* How many commands should we allocate space for at a time? */
#define UNIT_BLKSIZE 5    /*  How many units in a command allocated at a time */

typedef struct {
    int numunits;
    int allocatedunits;
    int *devs;
    int *houses;
} br_unit_list;

typedef struct {
    int inverse;
    int repeat;
    int numcmds;
    int allocatedcmds;
    br_unit_list **units;
    int *cmds;
} br_control_info;

int br_inverse_cmd(int /* command */);
int br_set_fd(br_control_info *, int /* file descriptor */);
int br_execute(int fd, br_control_info *);
br_unit_list *br_new_unit_list();
int br_free_unit_list(br_unit_list *);
int br_add_unit(br_unit_list *, int /* house */, int /* house */);
int br_del_unit(br_unit_list *, int /* house */, int /* device */);
int br_malloc_cmds(br_control_info *, int /* number of commands */);
int br_realloc_cmds(br_control_info *, int /* number of commands */);
int br_free_cmds(br_control_info *);
int br_add_ul_cmd(br_control_info *, int /* command */,
               br_unit_list * /* units */);
int br_add_cmd(br_control_info *, int /* command */, int /* house */,
               int /* device */);
int br_del_cmd(br_control_info *, int /* command index */);
br_control_info *br_new_control_info();
int br_free_control_info(br_control_info *);
int br_strtoul(char * /* dlptr */, br_unit_list * /* units */, char ** /* endptr */);
int br_ulcat(br_unit_list * /* units a */, br_unit_list * /* units b */);
br_unit_list *br_uldup(br_unit_list * /* units */);
int br_strtohc(char * /* hcptr */, char ** /* endptr */);
int br_get_num_commands(br_control_info *);
int br_get_ul_device(br_unit_list * /* units */, int /* index */);
int br_get_ul_house(br_unit_list * /* units */, int /* index */);
int br_get_num_units(br_unit_list * /* units */);

extern int br_default_house;

#endif