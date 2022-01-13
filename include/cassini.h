#ifndef CASSINI_H
#define CASSINI_H

#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <stdio.h>
#include <errno.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <limits.h>
#include <stdbool.h>
#include <signal.h>


#include "client-request.h"
#include "server-reply.h"
#include "string-utils.h"
#include "commandline.h"
#include "timing-text-io.h"
#include "path.h"
#include "request_reply.h"

#include "file-system.h" //TESTS -- TO REMOVE LATER

int send_request(int fd, uint8_t *request, uint32_t nb_bytes);

#endif // CASSINI
