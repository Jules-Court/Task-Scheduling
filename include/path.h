#ifndef PATH_H
#define PATH_H

#include <stdlib.h>
#include <unistd.h>
#include <pwd.h>
#include <string.h>
#include <stdio.h>

#define REQUEST_PIPE "saturnd-request-pipe"
#define REPLY_PIPE "saturnd-reply-pipe"


char* getPipesDir(void);
char* getRequestPipePath(char *PipeDir);
char* getResponsePipePath(char *PipeDir);

#endif