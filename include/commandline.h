#ifndef COMMANDLINE_H
#define COMMANDLINE_H

#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <endian.h>

#include "string-utils.h"

struct commandline {
    uint32_t argc;
    struct string_utils **argv;
};

// argc = length of the task to parse.
// *argv[] = pointer to a tab that contain the task
// EX: echo hello   -   argc = 2 / *argv[2] = {"echo", "hello"};
struct commandline *parse_argv(uint32_t argc, char *argv[]);


//commandline to byte array
//nb is fill with the number of bytes - can be use for write()
uint8_t *commandline_to_byte_array(uint32_t *nb, struct commandline *com);

//produce a commandline from file descriptor (will read in the pipe)
struct commandline *read_commandline(int fd);

//return a char from the string field
char * string_commandline(struct commandline *com);

//free commandline
//clearString is a bool, if true this function will clear the sub attributes from com->argv[i] (string / len)
void free_commandline(struct commandline *com, int clearString);

struct commandline *commandline_from_byte_array(uint8_t *buffer, size_t sz) ;


#endif