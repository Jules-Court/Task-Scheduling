#include "cassini.h"

#include <sys/time.h>
#include <time.h>

#define __DEBUG__ 0

 
int fd_request=-1;
int fd_reply=-1;


void cleanPipes()
{
	if( fd_request != -1)
		close(fd_request);
	if( fd_reply != -1)
		close(fd_reply);

}


bool openRequestPipe(char **pipes_directory, char **path_request )
{
	if(*pipes_directory == NULL)
		*pipes_directory= getPipesDir();


	if(*pipes_directory == NULL)
	{
		perror("pipes directory");
		return false;
}

	*path_request = getRequestPipePath(*pipes_directory);
	if(*path_request == NULL)
	{
		perror("path_request");
		return false;
	}

	if ( (fd_request = open(*path_request, O_WRONLY  )) < 0)
	{
    	printf (" open error\n ");
		perror("open requete");
		return false;
	}

  return true;
}

bool openReplyPipe(char **pipes_directory, char **path_reply)
{
	if(*pipes_directory == NULL)
		*pipes_directory= getPipesDir();


	if(*pipes_directory == NULL)
	{
		perror("pipes directory");
		return false;
	}


	*path_reply = getResponsePipePath(*pipes_directory);

	if(*path_reply == NULL)
		{
		perror("path_reply");
		return false;
	}


	if ( (fd_reply = open(*path_reply, O_RDONLY)) < 0)
	{
		perror("open reponse");
		return false;
	}
  return true;
}


const char usage_info[] = "\
   usage: cassini [OPTIONS] -l -> list all tasks\n\
      or: cassini [OPTIONS]    -> same\n\
      or: cassini [OPTIONS] -q -> terminate the daemon\n\
      or: cassini [OPTIONS] -c [-m MINUTES] [-H HOURS] [-d DAYSOFWEEK] COMMAND_NAME [ARG_1] ... [ARG_N]\n\
          -> add a new task and print its TASKID\n\
             format & semantics of the \"timing\" fields defined here:\n\
             https://pubs.opengroup.org/onlinepubs/9699919799/utilities/crontab.html\n\
             default value for each field is \"*\"\n\
      or: cassini [OPTIONS] -r TASKID -> remove a task\n\
      or: cassini [OPTIONS] -x TASKID -> get info (time + exit code) on all the past runs of a task\n\
      or: cassini [OPTIONS] -o TASKID -> get the standard output of the last run of a task\n\
      or: cassini [OPTIONS] -e TASKID -> get the standard error\n\
      or: cassini -h -> display this message\n\
\n\
   options:\n\
     -p PIPES_DIR -> look for the pipes in PIPES_DIR (default: /tmp/<USERNAME>/saturnd/pipes)\n\
";

void PrintHexa(uint8_t *buffer, size_t sz)
{
	printf("++++++++\n");
	for(int i = 0;i<sz;i++){
		printf("%02X, ",buffer[i]);

	}
	printf("-------\n");
}
int send_request(int fd, uint8_t *request, uint32_t nb_bytes) {
  if(request == NULL) return 1;
  if(nb_bytes <= 0) return 1; //we never send 0 bytes...
  if(nb_bytes > PIPE_BUF) { //never send a request in multiple parts
    printf("Request is too long...\n");
    return 1;
  }
  #if __DEBUG__
  printf("taille envoyee = %d \n",nb_bytes);
  PrintHexa(request, nb_bytes);
  #endif

  if(write(fd, request, nb_bytes) < 0) {
    return 1;
  }
  return 0;
}


void cbSigInt(int signalNumber)
{
	if (signalNumber != SIGINT)
		printf("Erreur : SIGINT attendu\n");
		
	cleanPipes();
	exit(EXIT_FAILURE);

}


char* eraseSubString(char* string, char* substring)
{
    if (strlen(string) == 0)
        return string;

    char* newString = NULL;

    int lenSub = strlen(substring);
    int lenString = strlen(string);
    char* index = strstr(string, substring);
    if (index)
    {
        newString = (char*) calloc(lenString+1, sizeof(char));
        // On copie le debut
        int lenDebut = index - string;
        strncpy(newString, string, lenDebut);
        // On copie la fin
        strncpy(newString+ lenDebut, index + lenSub , lenString - lenDebut - lenSub);
        //printf("  apres suppression de %s => %s devient %s \n", substring, string, newString);
        free(string);
    }
    return newString;
}


void SetTimer(int delaiEnSecondes)
{

	struct itimerval valueItimer;

	struct timeval itinterval;
	itinterval.tv_sec=0;
	itinterval.tv_usec=0;

	struct timeval itvalue;
	itvalue.tv_sec= delaiEnSecondes;
	itvalue.tv_usec=0;

	valueItimer.it_interval=itinterval;
	valueItimer.it_value=itvalue;


	setitimer( ITIMER_REAL, &valueItimer, NULL);

}


void cbSigTimer(int signalNumber)
{
  printf("Pipe de requete impossible a ouvrir. Saturnd probablement non lancé\n");
  exit(-1);
}

//MAIN
int main(int argc, char * argv[]) {
    
  errno = 0;
  // Handler sur SIGINT (CTRL-C) pour fermer les pipes
	if (signal(SIGINT, cbSigInt) == SIG_ERR)
		printf("\nErreur sur appel callback SIGINT\n");
	
  char* path_reply = NULL;
  char* path_request  = NULL;

  char * minutes_str = "*";
  char * hours_str = "*";
  char * daysofweek_str = "*";
  char * pipes_directory = NULL;
  
  //free only if malloc -> in case, not at the end
  uint8_t *request = NULL; //will be allocate with the bytes array - concat to send request
  uint8_t *opcode = NULL;
  uint8_t *task = NULL;
  uint16_t request_length =0;
	
	
	
  uint16_t operation = CLIENT_REQUEST_LIST_TASKS;
  uint64_t taskid;
  
  int opt;
  char * strtoull_endp;
  bool pipes_modified = false;
 
  int count_com = 0; //to find if the user try to use 2 or more commands (ex: -x -c -e)
  while((opt = getopt(argc, argv, "+hlcqr:x:p:o:e:")) != -1 && count_com==0) 
  {
    switch (opt) 
    {
      case 'l':
        if(count_com > 0) goto error;
        operation = CLIENT_REQUEST_LIST_TASKS;
        count_com++;
        break;
      case 'c':
        if(count_com > 0) goto error;
        operation = CLIENT_REQUEST_CREATE_TASK;
        count_com++;
        int optTime;
        
        bool timeNotParsed = true;
        char *optionsTime = "+m:H:d:";
        char *currentOptionTime= (char *) calloc(strlen(optionsTime)+1,sizeof(char));
        strcpy(currentOptionTime, optionsTime);
        while((optTime = getopt(argc, argv, currentOptionTime)) != -1 && timeNotParsed && strlen(currentOptionTime)!=0) 
        {
          switch (optTime) 
          {
            case '?':        
              // option inconnue
              timeNotParsed = false;
              printf("%s", usage_info);
              return 0;                          
              break;
            case 'm':
              minutes_str = optarg;
              currentOptionTime= eraseSubString(currentOptionTime, "m:");
              if(minutes_str == NULL || minutes_str[0] == '-') goto error;
              break;
            case 'H':
              hours_str = optarg;
              currentOptionTime= eraseSubString(currentOptionTime, "H:");
              if(hours_str == NULL || hours_str[0] == '-') goto error;
              break;
            case 'd':
              daysofweek_str = optarg;
              currentOptionTime= eraseSubString(currentOptionTime, "d:");
              if(daysofweek_str == NULL || daysofweek_str[0] == '-') goto error;
              break;
          }
        }
        free( currentOptionTime);
        break;
      case 'p':
        
        pipes_directory = strdup(optarg);
        if (pipes_directory == NULL) goto error;
        pipes_modified=true;
        break;

      case 'q':
        if(count_com > 0) goto error;
        operation = CLIENT_REQUEST_TERMINATE;
        count_com++;
        break;
      case 'r':
        if(count_com > 0) goto error;
        operation = CLIENT_REQUEST_REMOVE_TASK;
        taskid = strtoull(optarg, &strtoull_endp, 10);
        if (strtoull_endp == optarg || strtoull_endp[0] != '\0') goto error;
        count_com++;
        break;
      case 'x':
        if(count_com > 0) goto error;
        operation = CLIENT_REQUEST_GET_TIMES_AND_EXITCODES;
        taskid = strtoull(optarg, &strtoull_endp, 10);
        if (strtoull_endp == optarg || strtoull_endp[0] != '\0') goto error;
        count_com++;
        break;
      case 'o':
        if(count_com > 0) goto error;
        operation = CLIENT_REQUEST_GET_STDOUT;
        taskid = strtoull(optarg, &strtoull_endp, 10);
        if (strtoull_endp == optarg || strtoull_endp[0] != '\0') goto error;
        count_com++;
        break;
      case 'e':
        if(count_com > 0) goto error;
        operation = CLIENT_REQUEST_GET_STDERR;
        taskid = strtoull(optarg, &strtoull_endp, 10);
        if (strtoull_endp == optarg || strtoull_endp[0] != '\0') goto error;
        count_com++;
        break;
      case 'h':
        printf("%s", usage_info);
        return 0;
      case '?':
        fprintf(stderr, "%s", usage_info);
        goto error;
    }
  }


  // --------
  // | TODO |
  // --------

  // si l'on est dans un cas ou il y a plus d'un argument inconnu 
  // ou pas d'option pipe on affiche l'aide
  // cas que l'on veut accepter:
  //    cassini
  //    cassini -p toto 
  //    cassini <commandes correctes>
  // on veut refuser:
  //    cassini erreur
  //    cassini -erreur

  if( argc>1 && count_com==0 && pipes_modified==false)
  {
    printf("%s", usage_info);
    return 0;
  }
 	

	if (signal(SIGALRM , cbSigTimer) == SIG_ERR)
		printf("\nErreur sur appel callback SIGINT\n");

	// Rechercher prochaines taches et delai avant lancement
	SetTimer(1);


	if( openRequestPipe(	&pipes_directory, &path_request ) == false)
		goto error;

  // On désactive le timer sur le open pour éviter qu'il ne se déclenche par la suite
  SetTimer(0);



  //usable: request, reply, fd_request, fd_reply, pipes_directory, path_request, path_reply
  //switch for request - reply
  switch(operation) {
    case CLIENT_REQUEST_LIST_TASKS:// -l
      request = CreateReq_CLIENT_REQUEST_LIST_TASKS(&request_length);
      if(request == NULL) 
        goto error;

      //send
      if(send_request(fd_request, request, request_length) < 0)
        goto error;
      
      // - get reply
      if( openReplyPipe(	&pipes_directory,  &path_reply) == false)      
        goto error;
      if( print_reply_LS(fd_reply) !=0 )
        goto error;
      break;


    case CLIENT_REQUEST_CREATE_TASK: 
    { 
      //-c
      request = CreateReq_CLIENT_REQUEST_CREATE_TASK(argc, argv, 
                                        minutes_str, hours_str, daysofweek_str, 
                                        &request_length, optind);

       if(request == NULL) 
        goto error;
      //send
      if(send_request(fd_request, request, request_length) < 0) 
        goto error;

      //get reply
      if( openReplyPipe(	&pipes_directory,  &path_reply) == false)      
        goto error;

       if( print_reply_CR(fd_reply) !=0 )
        goto error;
      break;


    } break;
    case CLIENT_REQUEST_REMOVE_TASK:
    { 
      //-r
      task=uint64_to_byte_array(taskid);
      if(task == NULL) 
        goto error;
      opcode = uint16_to_byte_array(CLIENT_REQUEST_REMOVE_TASK);  
      if(opcode == NULL) 
        goto error;

      request = concat_byte_array(opcode, sizeof(uint16_t), task, sizeof(uint64_t)); //ALLOC OF request
      if(request == NULL) 
        goto error;
      
      // - send request
      if(send_request(fd_request, request, sizeof(uint16_t)+sizeof(uint64_t)) < 0) 
        goto error;
      
      // - get reply
      if( openReplyPipe(	&pipes_directory,  &path_reply) == false)      
        goto error;

      if( print_reply_RM(fd_reply) !=0 )
        goto error;

     } 
     break;
    case CLIENT_REQUEST_GET_TIMES_AND_EXITCODES: 
    {
      //-x
      task=uint64_to_byte_array(taskid);
      if(task == NULL) 
        goto error;

      opcode = uint16_to_byte_array(CLIENT_REQUEST_GET_TIMES_AND_EXITCODES);
      if(opcode == NULL) 
        goto error;

      request = concat_byte_array(opcode, sizeof(uint16_t), task, sizeof(uint64_t)); //ALLOC OF request
      if(request == NULL) 
        goto error;
      
      // - send request

      if(send_request(fd_request, request, sizeof(uint16_t)+sizeof(uint64_t)) < 0) 
        goto error;
      
      // - get reply
      if( openReplyPipe(	&pipes_directory,  &path_reply) == false)      
        goto error;

      if( print_reply_TX(fd_reply) !=0 )
        goto error;
      break;
      


    }break;
    case CLIENT_REQUEST_TERMINATE:{//-q
    //preparing request
      opcode = uint16_to_byte_array(CLIENT_REQUEST_TERMINATE);
      if(opcode == NULL) 
        goto error;

      //send
      if(send_request(fd_request, opcode, sizeof(uint16_t)) < 0)
        goto error;
      
      // - get reply
      if( openReplyPipe(	&pipes_directory,  &path_reply) == false)      
        goto error;

      if( print_reply_TM(fd_reply) !=0 )
        goto error;

    }break;
    case CLIENT_REQUEST_GET_STDOUT:{//-o
      task=uint64_to_byte_array(taskid);
      if(task == NULL) 
        goto error;

      opcode = uint16_to_byte_array(CLIENT_REQUEST_GET_STDOUT);
      if(opcode == NULL) 
        goto error;

      request = concat_byte_array(opcode, sizeof(uint16_t), task, sizeof(uint64_t)); //ALLOC OF request
      if(request == NULL) 
        goto error;
      
      // - send request
      if(send_request(fd_request, request, sizeof(uint16_t)+sizeof(uint64_t)) < 0)
        goto error;
      
         
      // - get reply
      if( openReplyPipe(	&pipes_directory,  &path_reply) == false)      
        goto error;

      if( print_reply_SO_SE(fd_reply) !=0 )
        goto error;


     } break;

    case CLIENT_REQUEST_GET_STDERR:{//-e
      task=uint64_to_byte_array(taskid);
      if(task == NULL) 
        goto error;
      opcode = uint16_to_byte_array(CLIENT_REQUEST_GET_STDERR);
      if(opcode == NULL) 
        goto error;

      request = concat_byte_array(opcode, sizeof(uint16_t), task, sizeof(uint64_t)); //ALLOC OF request
      if(request == NULL) 
        goto error;

      // - send request
      if(send_request(fd_request, request, sizeof(uint16_t)+sizeof(uint64_t)) < 0) 
        goto error;
      

      // - get reply
      if( openReplyPipe(	&pipes_directory,  &path_reply) == false)      
        goto error;

      


    }break;
    default:
      goto error;
  }

	


	int errorCode = EXIT_SUCCESS;
	goto cleanup;
		
	error:
		errorCode = EXIT_FAILURE;
		int codeErrno= errno;
	
	cleanup:
 		cleanPipes();

		free(request);
		free(task);
		free(opcode);

		free(pipes_directory);
		free(path_request);
		free(path_reply);
		


	if(errorCode == EXIT_FAILURE && codeErrno!=0)
		perror("main");
	
	return errorCode ;




}