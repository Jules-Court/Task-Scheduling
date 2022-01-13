
#ifndef FILE_SYSTEM_H
#define FILE_SYSTEM_H

#include "timing.h"
#include "string-utils.h"
#include "commandline.h"
#include "server-reply.h"

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <endian.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <dirent.h>
#include <time.h>


//MACRO
#define MAIN_DIR ".sysfile" //rep for system file
#define INDEX ".index.bin" //contain the next task id to create
#define UINT64_STRLEN 20

#define COMMANDLINE "commandline.bin" //contain the commandline to exec
#define DATE "date.bin" //contain the dates exec
#define TIME_EXITCODE "time-exitcode.bin" //contain every dates and exitcode
#define STDOUT_TASK "stdout.bin" //contain the last stdout
#define STDERR_TASK "stderr.bin" //contain the last stderr

//functions
int is_sysfile_exist(); //return 0 if it don't or 1
int create_sysfile(); //return 0 if it was created or -2 if it don't (exist) or -1 if an error occured

int get_taskid_and_update(uint64_t *taskid); //set the next id possible in taskid and increment index. return0 on succes and -1 on failure

int new_task_sysfile(/* out */ uint64_t *taskid, /* in */ struct timing *tm, /* in */  struct commandline *com); //create a new task and set it's id on taskid. in case of succes return 0, or -1 in failure
int rm_task_sysfile(uint64_t taskid); //return 0on succes, or -1

int add_time_exitcode_to_task(uint64_t taskid, uint64_t time, uint16_t exitcode); //adding a new "line" description with time and exitcode
/***  to write on STDOUT_TASK or STDERR_TASK, we must used use dup functions before exec and set O_TRUNC flag ***/

int check_date_task(struct timing *time, uint64_t taskid); //return 0 if the time is matching or -1 if it don't
int check_date_forall(uint64_t *taskid); //set taskid (>=0) if this task is to be executed, return -1 on error and 0 on succes.
                                         //(if 0 is returnd we can reuse this function (maybe others task to execute), and wait if -1 is returned)

struct commandline *get_commandline_task(uint64_t taskid); //return NULL if an error occurred or the struct
uint8_t *get_time_exitcode_task(uint32_t *final_len, uint64_t taskid); //return NULL if an error occurred or the pointer. set to final_len the bytes nbrs of uint8_t*
struct string_utils *get_string_stdout_stderr(uint64_t taskid, char flag); //return NULL if an error occurred, else return the struct. flago possibles: 'o' -> stdout | 'e' -> stderr
int count_task_dir(); //return the number of tasks in .sysfile
uint8_t *get_tasks_list(uint32_t *final_len); //return NULL if an error occured, else the content on uint8_t*. set the bytes nbrs on final_len


// get last taskid execution time
int get_last_execution_time_for_task(uint64_t taskid, /* out */ uint64_t *time) ;

//int write_task_output(uint64_t taskid, char *string, char flag) ;

int get_nbruns(uint64_t taskid) ;
int get_task_output_file(uint64_t taskid, char flag) ;
#endif