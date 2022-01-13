
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h> // for open
#include <unistd.h> // for close
#include "file-system.h"
#include "cassini.h"

int requestPipe(int fd_request,int fd_reply);