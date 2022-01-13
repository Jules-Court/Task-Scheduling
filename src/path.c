#include "path.h"

char * getPipesDir(void){

    struct passwd *pass = getpwuid(getuid()); //to get the username
    if(pass == NULL) 
		goto error;
		
    char *username = (char*)pass->pw_name;
	if(username == NULL)
		goto error;
	
	char *build_default_path = "/tmp/%s/saturnd/pipes";
	int len = strlen(build_default_path) + strlen(username) -2 /* pour le %s*/ + 1 /* pour le 0 terminal*/;
	char * strPipesDir = calloc(len,sizeof(char));
	if( strPipesDir == NULL)
		goto error;
	
	if(sprintf(strPipesDir,build_default_path,username) < 0)
		goto error;


	return strPipesDir;
	
	error:
		return NULL;
	
};



char* getRequestPipePath(char *PipeDir){
	char * pathPipe =NULL;
	
	if(PipeDir == NULL)
		return NULL;
	
	pathPipe = calloc(strlen(PipeDir) +strlen(REQUEST_PIPE) + 2, sizeof(char));
	if(pathPipe == NULL)
		return NULL;
	
	if(sprintf(pathPipe,"%s/%s",PipeDir,REQUEST_PIPE) < 0)
		return NULL;
	
	return pathPipe;
	
	
}
char* getResponsePipePath(char *PipeDir){

	if(PipeDir == NULL)
		return NULL;
	
	char * pathPipe = calloc(strlen(PipeDir) +strlen(REPLY_PIPE) + 2, sizeof(char));
		if(pathPipe == NULL)
		return NULL;
	
	if(sprintf(pathPipe,"%s/%s",PipeDir,REPLY_PIPE) < 0)
		return NULL;
	
	return pathPipe;
	
}


