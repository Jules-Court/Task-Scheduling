

#include "saturnd.h"


int descripteurrequete=-1;
int descripteurreponse=-1;
int nextTask=0;
int volatile thread_running = 1;


void cbSigInt(int signalNumber)
{
	// On capte SIGTERM et SIGINT et on n'en fait rien : pas d'exit sur réception de ces signaux
	// Seul moyen de fermer saturdn : cassini -q  OU SIGKILL
	
	/*
	if (signalNumber == SIGINT)
		printf("Reception SIGINT \n");
	if (signalNumber == SIGTERM)
		printf("Reception SIGTERM \n");
	if (signalNumber == SIGKILL)
		printf("Reception SIGKILL \n");
	*/
}


int createPipeDir()
{
	char *strNomUtilisateur = NULL;
	char * strPipesDir = NULL;

	// !! Pas de free() sur pass : cf man getpwuid() 
	//  => il peut s'agir d'un buffer statique
    struct passwd *pass = getpwuid(getuid()); //to get the username
    if(pass == NULL) 
		return 0;
    strNomUtilisateur = pass->pw_name;
	

	strPipesDir = calloc(strlen("/tmp//saturnd/pipes")+1+strlen(strNomUtilisateur),sizeof(char));
	
	
	if(sprintf(strPipesDir,"/tmp/%s",strNomUtilisateur) < 0) {
		free(strPipesDir);
		return 0;
	}

	if(mkdir(strPipesDir, 0700) == -1 && errno != EEXIST) {
		free(strPipesDir);
		return 0;
	}
	
	strcat(strPipesDir,"/saturnd");
	
	
	if(mkdir(strPipesDir, 0700) == -1 && errno != EEXIST) {
		free(strPipesDir);
		return 0;
	}
	

	strcat(strPipesDir,"/pipes");

	if(mkdir(strPipesDir, 0700) == -1 && errno != EEXIST) {
		free(strPipesDir);
		return 0;
	}
	
	free(strPipesDir);
	return 1;
}

void PrintHexa(uint8_t *buffer, size_t sz)
{
	printf("++++++++\n");
	for(int i = 0;i<sz;i++)
	{
		printf("%02X, ",buffer[i]);
	}
	printf("-------\n");
}

void WriteResponse(uint8_t *buffer, size_t length)
{
#if __DEBUG__
	PrintHexa(buffer,length);
#endif
	if ( write(descripteurreponse, buffer, length) != length) 
	{ 
		perror("write_reponse");
	}
}

void ExecuteTask(uint64_t taskid)
{
 	pid_t pid = fork();
   	if (pid == 0) 
   	{
	   	// dans le fils
		struct commandline *cmd = get_commandline_task(taskid);
		
		// On reconstitue un argv pour le execvp
		char **argv=calloc(cmd->argc + 1  , sizeof(char *));
		for(int i=0;i<cmd->argc;i++)
		{
			// Allocation d'1 octet de plus pour ajout du \0 terminal
			char *str=calloc(cmd->argv[i]->len +1, sizeof(uint8_t));
			memcpy(str,cmd->argv[i]->string, cmd->argv[i]->len);
			
			argv[i]=str;
		}

		// le execvp ne prenant pas d'argc, les arguments sont traités 
		// jusqu'à atteindre un argv NULL.
		// Nous ajoutons donc cet argv NULL 
		// Cette affectation est redondante avec le calloc, mais ajoutée pour la clarté
		argv[cmd->argc]=NULL;

		// On récupére les descripteurs vers les fichiers de sortie stdout.bin et stderr.bin
		int fichier_sortie_stdout = get_task_output_file( taskid, 'o') ;
		int fichier_sortie_stderr = get_task_output_file( taskid, 'e') ;

		// On fait pointer STDOUT et STDERR vers les fichiers de dump
		dup2(fichier_sortie_stdout, STDOUT_FILENO);
 		dup2(fichier_sortie_stderr, STDERR_FILENO);
		
		// On peut fermer les descripteurs sur les fichiers, ceux-ci (les descripteurs)
		// étant maintenant utilisés par STDOUT et STDERR
		close(fichier_sortie_stdout);
		close(fichier_sortie_stderr);
   
		// argv[0] => nom de la commande a executer (utilisation du PATH courant pour trouver la commande)
		// argv => arguments (comprenant aussi le nom de la commande, par convention) et le argv NULL en fin de tableau
		execvp(argv[0], argv);

		//printf("echec execve %s %d \n", argv[0] , errno );
		//fflush(stdout);
		// en cas d'échec du execvp on sort (on ne parvient ici que dans le cas ou execvp échoue)
		exit(0);
    }
    else if (pid <0 )
    {
		// erreur dans le fork()
		// Pas de traitement particulier
	}
	else
	{
		// Dans le pere
		uint16_t exitcode = 0;
		int status;
		// attente de la fermeture du processus forké
		int retpid = waitpid(pid, &status, 0);
	
		if(retpid==pid)
		{
			// vérification que le forké est bien sorti sur un exit()
			if( WIFEXITED(status) )
			{
				// récupération du exit code
				exitcode = WEXITSTATUS(status) ;
				//printf("exit code %d pour %ld ( %0x ) \n",exitcode, taskid, status );
				// Ajout de la date de fin + exitcode 
				add_time_exitcode_to_task( taskid,  time(NULL),  exitcode); 
			}
   		}
		  
    }
}


void *cbCheckTime(void *arg)
{
	while(1)
	{
		uint64_t taskid_to_execute; 
		if(thread_running == 0) break;
		while( check_date_forall(&taskid_to_execute) != -1)
		{
			ExecuteTask(taskid_to_execute);
		}
		sleep(20);
	}
	pthread_exit(0);
}



int main(){
	int errorCode = EXIT_SUCCESS;

	char* pathrequest =  NULL;
	char* pathresponse = NULL;
	char * pipes_directory = NULL;
	char * buffer = calloc(MAXCHAINE,sizeof(char));

	if(fork() != 0){
		exit(0);
	}
	
	setsid();
	
	// Handler sur SIGINT (CTRL-C) pour fermer les pipes
	if (signal(SIGINT, cbSigInt) == SIG_ERR)
		perror("Erreur sur appel callback SIGINT");

	if (signal(SIGTERM, cbSigInt) == SIG_ERR)
		perror("Erreur sur appel callback SIGTERM");


	if( is_sysfile_exist() == 0)
		create_sysfile(); 


	int n;


	pthread_t threadCheckTime;
	int result = pthread_create(&threadCheckTime, NULL, cbCheckTime, NULL);

	if(result!=0){
		perror("Erreur creation thread");
		exit(1);
	}

	pipes_directory = getPipesDir();
	if(pipes_directory == NULL)
		goto error;
	
	if ( createPipeDir() == 0)
		goto error;

	pathrequest = getRequestPipePath(pipes_directory);
	if(pathrequest == NULL)
		goto error;

	mkfifo(pathrequest, 0600);



	pathresponse = getResponsePipePath(pipes_directory);
	if(pathresponse == NULL)
		goto error;
	
	
	mkfifo(pathresponse, 0600);


	if ( (descripteurrequete = open(pathrequest, O_RDONLY )) < 0){
		perror("open requete");
		goto error;
	}

	if ( (descripteurreponse = open(pathresponse, O_WRONLY )) < 0 && errno != ENXIO){
		perror("open reponse");
		goto error;
	}

	const int szHEADER =sizeof(uint16_t);
	const int szTIMING =sizeof(uint8_t ) +sizeof(uint32_t )+sizeof(uint64_t );
	const int szTASKID =sizeof(uint64_t);


   struct pollfd pfd;
   pfd.fd= descripteurrequete;
   pfd.events = POLLIN;
 	int retPoll =0;


	while(1)
	{
		retPoll = poll(&pfd,1,-1);
  	
		if(retPoll >0 && ( pfd.revents & POLLHUP) != 0 )
    	{
			// Suite à la réception d'une commande de cassini, celui-ci s'est fermé
			// le pipe descripteurrequete a donc été close() par cassini
			// Il reste donc en POLLHUP et poll() retourne immédiatement en POLLHUP
			// La boucle infinie s'execute donc en permanence (appel de poll) et le temps
			// machine explose

			close(descripteurrequete);
			if ( (descripteurrequete = open(pathrequest, O_RDONLY )) < 0)
			{
				perror("open requete");
				goto error;
			}
		}
		else if(retPoll >0 && ( pfd.revents & POLLIN) != 0 )
    	{
			n = read(descripteurrequete, buffer, MAXCHAINE) ;

			if(n == -1)
			{
				// Pipe vide -- normalement impossible puisque POLLIN
				//printf("(pipe empty)\n"); 
				sleep(1);
				continue;
			}
			else
			{ 
				if( n>= szHEADER )
				{
					/*
					0x4c53 ('LS') : LIST -- lister toutes les tâches
					0x4352 ('CR') : CREATE -- créér une nouvelle tâche
					0x524d ('RM') : REMOVE -- supprimer une tâche
					0x5458 ('TX') : TIMES_EXITCODES -- lister l'heure d'exécution et la valeur de retour
					de toutes les exécutions précédentes de la tâche
					0x534f ('SO') : STDOUT -- afficher la sortie standard de la dernière exécution de la tâche
					0x5345 ('SE') : STDERR -- afficher la sortie erreur standard de la dernière exécution de la tâche
					0x4b49 ('TM') : TERMINATE -- terminer le démon
					*/

					if( strncmp(buffer,"LS",szHEADER)==0)
					{
						uint32_t szBufRetour;
						uint8_t * bufretour = get_tasks_list(&szBufRetour);

						WriteResponse(bufretour,szBufRetour);
						free( bufretour);

					}
					else if( strncmp(buffer,"CR",szHEADER)==0)
					{

						struct timing *time=( struct timing *) (buffer+szHEADER);
						struct commandline *com = 
						commandline_from_byte_array( (uint8_t *)
									( buffer+szHEADER +szTIMING), 
									n-szHEADER -szTIMING) ;

#if __DEBUG__
						printf("taille lue = %d \n",n);
						printf(" longueur SZTIMING %d \n",szTIMING);
						PrintHexa((uint8_t *)time,szTIMING);
#endif
						uint64_t taskid;

						int ret =  new_task_sysfile(   /* out */ &taskid, 
														/* in */ time,
														/* in */ com); 
			
						if(ret != 0)
						{
							//erreur de creation mais rien de prevu dans le protocole
						}

						else {
							// réponse : OK + TASKID
							uint64_t taskidInBE = htobe64(taskid);
							size_t szBufRetour = szHEADER + szTASKID;
							
							uint8_t *  bufretour= calloc(szBufRetour,sizeof(uint8_t));
						
							memcpy(bufretour,"OK",szHEADER);
							memcpy(bufretour+szHEADER, (uint8_t *)(&taskidInBE), szTASKID);
							
							WriteResponse(bufretour,szBufRetour);
							free( bufretour);

						}
						free_commandline(com, 1);
					}

					
					else if( strncmp(buffer,"RM",szHEADER)==0)
					{
						uint64_t taskid = *((uint64_t*)(buffer + szHEADER));
						taskid = be64toh(taskid);


						int ret = rm_task_sysfile(taskid);
						if(ret != 0)
						{
							WriteResponse(((uint8_t *)"ERNF"),szHEADER * 2);
						}
						else
						{
							WriteResponse(((uint8_t *)"OK"),szHEADER);
						}

					}
					else if( strncmp(buffer,"TX",szHEADER)==0)
					{

						uint64_t taskid = *((uint64_t*)(buffer + szHEADER));
						taskid = be64toh(taskid);

						uint32_t szBufRetour;
						uint8_t * bufretour = get_time_exitcode_task(&szBufRetour, taskid);

						
						if(bufretour == NULL){
							WriteResponse(((uint8_t *)"ERNF"),szHEADER * 2);
						}
						else
						{
							WriteResponse(bufretour,szBufRetour);
						}

						free( bufretour);



					}
					else if( strncmp(buffer,"SO",szHEADER)==0 ||  strncmp(buffer,"SE",szHEADER)==0)
					{
						char flag='o';
						if( strncmp(buffer,"SE",szHEADER)==0)
							flag='e';

						uint64_t taskid = *((uint64_t*)(buffer + szHEADER));
						taskid = be64toh(taskid);

						struct string_utils *strstdout = get_string_stdout_stderr(taskid,flag);

						
						if(strstdout == NULL){
							// inexistant
							WriteResponse(((uint8_t *)"ERNF"),szHEADER * 2);

						}
						else if (get_nbruns(taskid) == 0)
						{	
							// jamais execute
							WriteResponse(((uint8_t *)"ERNR"),szHEADER * 2);
						}
						else
						{
							const int szUINT32 = sizeof(uint32_t);

							// réponse : OK + STRING
							
							size_t szBufRetour = szHEADER + szUINT32 + strstdout->len;
							
							uint8_t *bufretour= calloc(szBufRetour,sizeof(uint8_t));
						
							memcpy(bufretour,"OK",szHEADER);
							uint32_t lenBE = htobe32(strstdout->len);
		
							memcpy(bufretour+szHEADER, &lenBE, szUINT32);

							memcpy(bufretour+szHEADER+szUINT32, (uint8_t *)(strstdout->string), strstdout->len);
		
							WriteResponse(bufretour,szBufRetour);
							free( bufretour);

						}

						free_string_utils( strstdout);

					}
					else if( strncmp(buffer,"TM",szHEADER)==0)
					{
						// Réponse à TERMINATE
						// Seule une réponse OK est possible :
						// REPTYPE='OK' <uint16>
						WriteResponse(((uint8_t *)"OK"),szHEADER);
						thread_running = 0;
						pthread_join(threadCheckTime, NULL);
						goto cleanup;
					}
					else
					{
						// Erreur

					}
				}

				
			}
	
			memset(buffer,0,MAXCHAINE);
			
		}
	}
	
	

	
	goto cleanup;
	
	error:
		errorCode =  EXIT_FAILURE;
		
	cleanup:
		close(descripteurrequete);
		close(descripteurreponse);	
		free(pipes_directory);
		free(pathrequest);
		free(pathresponse);
		free(buffer);
	
	return errorCode;
	
	
		
}
	
