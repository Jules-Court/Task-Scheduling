
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
#include "timing-text-io.h"
#include "string-utils.h"
#include "commandline.h"

#include "request_reply.h"


uint8_t *CreateReq_CLIENT_REQUEST_LIST_TASKS(uint16_t *request_length)
{
     //preparing
      uint8_t *opcode = uint16_to_byte_array(CLIENT_REQUEST_LIST_TASKS);
      *request_length= sizeof(uint16_t);
      return opcode;
}


uint8_t *CreateReq_CLIENT_REQUEST_CREATE_TASK(
                            int argc, char * argv[], 
                            char *minutes_str, char *hours_str,  char * daysofweek_str,
                            uint16_t *request_length, int pos_com )
{

      uint8_t *request = NULL;
      uint8_t *opcode = NULL;
      uint8_t *com_byte = NULL;
      uint8_t *minutes = NULL;
      uint8_t *hours = NULL;
      uint8_t *concat_tmp_time = NULL;
      uint8_t *time_tab = NULL;
      uint8_t *code_time  = NULL;

      //position of commandline /!\ switch(getopt) can permute argv
  /*    int pos_com = 0;

printf("rr0\n");

      for(int i = argc-1; i > 0; i--) {
        //case for -c
        if(strcmp(argv[i], "-c") == 0) {
          pos_com = i+1;
          break;
        }

        //case for -m -H -d -p
        if(strcmp(argv[i], "-m") == 0 || strcmp(argv[i], "-H") == 0 || strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "-p") == 0) {
          pos_com = i+2;
          break;
        }
      }
*/
      if(pos_com>argc-1)
         goto error;

      //preparing
      struct timing time_struct;
      if(timing_from_strings(&time_struct, minutes_str, hours_str, daysofweek_str) < 0) goto error;
      
      struct commandline *com = parse_argv(argc-pos_com, argv+pos_com);
      if(com == NULL) 
        goto error;
      uint32_t nb_bytes = 0; //total len
      opcode = uint16_to_byte_array(CLIENT_REQUEST_CREATE_TASK);
      if(opcode == NULL) 
        goto error;

      com_byte = commandline_to_byte_array(&nb_bytes, com);
      if(com_byte == NULL) 
        goto error;
      minutes = uint64_to_byte_array(time_struct.minutes);
      if(minutes == NULL) 
        goto error;
      hours = uint32_to_byte_array(time_struct.hours);
      if(hours == NULL) 
        goto error;

       // ----- CONCAT -----

      //concat of time bytes arrays
      concat_tmp_time = concat_byte_array(minutes, sizeof(uint64_t), hours, sizeof(uint32_t));
      if(concat_tmp_time == NULL) 
        goto error;
      uint32_t nb_bytes_time = sizeof(uint64_t) + sizeof(uint32_t);

      time_tab = concat_byte_array(concat_tmp_time, nb_bytes_time, &time_struct.daysofweek, sizeof(uint8_t));
      if(time_tab == NULL) 
        goto error;
    
      
      nb_bytes_time += sizeof(uint8_t);
      //we keep time_tab !

      
      //concat opcode and time
      code_time = concat_byte_array(opcode, sizeof(uint16_t), time_tab, nb_bytes_time);
      if(code_time == NULL) 
        goto error;
          
      nb_bytes_time += sizeof(uint16_t); //len of code + time
      //we keep code_time !

      //final concat time_code and commandline
      request = concat_byte_array(code_time, nb_bytes_time, com_byte, nb_bytes); //ALLOC OF request
      if(request == NULL) 
            goto error;

      nb_bytes += nb_bytes_time;
      *request_length = nb_bytes;
      //----- -----

    free_commandline(com, 0);
    com = NULL;

    goto cleanup;
    
    error:
        free(request);

    cleanup:
        free(opcode);
        free(time_tab);
        free(concat_tmp_time);
        free(minutes);
        free(hours);


        free(code_time);
        free(com_byte);

        return request;

}





int print_reply_RM(int fd) {


 uint16_t reptype = 0;

  if(read(fd, &reptype, sizeof(uint16_t)) < 0)
  {
      perror("print_reply_RM");
       return 1;
 }
      
 reptype = be16toh(reptype);
      
      
   if(reptype == SERVER_REPLY_ERROR) { //ER ?

        uint16_t errcode = 0;
       if(read(fd, &errcode, sizeof(uint16_t)) < 0) {
            perror("print_reply_RM");
            return 1;
        }
        errcode = be16toh(errcode);

        if(errcode != SERVER_REPLY_ERROR_NOT_FOUND)
        {
            perror("print_reply_RM ERROR CODE incorrect");
            return 1;
        }


       // printf("%d\n", errcode);
        return 1;
   }
   else
   if(reptype == SERVER_REPLY_OK) 
   {
     return 0;

   }


    else
   {
        perror("Bad reply");
          return 1;
   }
       //OK ?

// TODO

// OK ou ERROR
return 0;
// TODO

// OK 

// ER
// ERRCODE doit être NF

}

int print_reply_SO_SE(int fd) {

 
 uint16_t reptype = 0;

  if(read(fd, &reptype, sizeof(uint16_t)) < 0)
  {
      perror("print_reply_LS");
       return 1;
 }
   
 reptype = be16toh(reptype);
    
      
   if(reptype == SERVER_REPLY_ERROR) { //ER ?

        uint16_t errcode = 0;
       if(read(fd, &errcode, sizeof(uint16_t)) < 0) {
            perror("print_reply_SO_SE");
            return 1;
        }
     
        errcode = be16toh(errcode);


        if(errcode != SERVER_REPLY_ERROR_NOT_FOUND && errcode != SERVER_REPLY_ERROR_NEVER_RUN)
        {
            perror("print_reply_SO_SE ERROR CODE incorrect");
            return 1;
        }


        printf("%d\n", errcode);
        return 1;
   }
   else
   if(reptype == SERVER_REPLY_OK) 
   {
        uint32_t string_length = 0;

        if(read(fd, &string_length, sizeof(uint32_t)) < 0)
        {
            perror("print_SO_SE_String_length");
            return 1;
        }

       string_length= be32toh(string_length);
        char * string = calloc(string_length+1,sizeof(char));

        if(read(fd, string, string_length) < 0)
        { 
            perror("print_SO_SE_String");
            return 1;
        }

         //printf("%s",string);
         free(string);

   }


    else
   {
        perror("Bad reply");
          return 1;
   }
       //OK ?

// TODO

// OK ou ERROR
return 0;
}


int print_reply_TM(int fd) {
 uint16_t reptype = 0;

      if(read(fd, &reptype, sizeof(uint16_t)) < 0)
        return 1;
      

      reptype = be16toh(reptype);
      if(reptype != SERVER_REPLY_OK) { //OK ?
        perror("Bad reply");
        return 1;
      }

return 0;
// TODO
// OK 
}


int print_reply_CR(int fd) {

       uint16_t reptype = 0;

      if(read(fd, &reptype, sizeof(uint16_t)) < 0)
        return 1;
      

      reptype = be16toh(reptype);
      if(reptype != SERVER_REPLY_OK) { //OK ?
        perror("Bad reply");
        return 1;
      }

      uint64_t taskid = 0;
      if(read(fd, &taskid, sizeof(uint64_t)) < 0) 
        return 1; //taskid is decalared upper - can be used to send and get reply

      taskid = be64toh(taskid);
      
      printf("%ld\n", taskid);

  return 0;
}



int print_reply_LS(int fd) {

  struct timing *time;
  struct commandline *com;
  //char *str;
  
  uint32_t nb_tasks = 0;
  uint64_t task_id = 0;


  uint16_t reptype = 0;

  if(read(fd, &reptype, sizeof(uint16_t)) < 0) {
    perror("print_reply_LS");
    return 1;
  }
      
  reptype = be16toh(reptype);
      
  if(reptype != SERVER_REPLY_OK) { //OK ?
      perror("Bad reply");
      return 1;
  }



  if(read(fd, &nb_tasks, sizeof(uint32_t)) < 0) {
    perror("print_reply_LS");
    return 1;
  }
  nb_tasks = be32toh(nb_tasks);


  for(int i = 0; i < nb_tasks; i++) {
    //task_id
    if(read(fd, &task_id, sizeof(uint64_t)) < 0) {
      perror("print_reply_LS");
      return 1;
    }
    task_id = be64toh(task_id);
    printf("%ld: ", task_id);


    //time
    time = malloc(sizeof(struct timing));
    if(time == NULL) {
      perror("print_reply_LS");
      return 1;
    }

    if(read(fd, time, sizeof(uint64_t) + sizeof(uint32_t) + sizeof(uint8_t)) < 0) {
      perror("print_reply_LS");
      free(time);
      return 1;
    }

    char *str_time = malloc(TIMING_TEXT_MIN_BUFFERSIZE);
    if(str_time == NULL) {
      perror("print_reply_LS");
      return 1; 
    }
    str_time[0] = '\0';

    time->minutes = be64toh(time->minutes);
    time->hours = be32toh(time->hours);
    timing_string_from_timing(str_time, time);
    printf("%s ", str_time);
    free(str_time);
    free(time);

    //commandlines
    com = read_commandline(fd);
    if(com == NULL) return 1;
    char *str_com = string_commandline(com);
    if(str_com == NULL) {
      perror("print_reply_LS");
      return 1;
    }
    printf("%s\n", str_com);
    free(str_com);
    free_commandline(com, 1);
  }

  return 0;
}

int print_reply_TX(int fd) {
  struct tm *tmp;
  time_t time_exec;
  uint64_t nb_sec = 0;
  uint32_t nb_runs = 0;
  uint16_t exit_code = 0;
  char str_time[24];
  str_time[23] = '\0';


  uint16_t reptype = 0;

  if(read(fd, &reptype, sizeof(uint16_t)) < 0)
  {
      perror("print_reply_LS");
       return 1;
  }
      
  reptype = be16toh(reptype);
      
      
  if(reptype == SERVER_REPLY_ERROR) { //ER ?

    uint16_t errcode = 0;
    if(read(fd, &errcode, sizeof(uint16_t)) < 0) {
        perror("print_reply_TX");
        return 1;
      }
    errcode = be16toh(errcode);

    if(errcode != SERVER_REPLY_ERROR_NOT_FOUND) {
        perror("print_reply_TX ERROR CODE incorrect");
        return 1;
    }
    printf("%d\n", errcode);
    return 1;
  }
  else
  if(reptype == SERVER_REPLY_OK) { //OK ?

    if(read(fd, &nb_runs, sizeof(uint32_t)) < 0) {
        perror("print_reply_TX");
        return 1;
    }
    nb_runs = be32toh(nb_runs);

    for(int i = 0; i < nb_runs; i++) {
        if(read(fd, &nb_sec, sizeof(uint64_t)) < 0) {
          perror("print_reply_TX");
          return 1;
        }
        //time format
        nb_sec = be64toh(nb_sec);
        time_exec = (time_t)nb_sec;
        tmp = localtime(&time_exec);
        if(strftime(str_time, 24, "%Y-%m-%d %H:%M:%S", tmp) == 0) {
          perror("print_reply_TX");
          return 1;
        }
        printf("%s ", str_time);

        //exit_code
        if(read(fd, &exit_code, sizeof(uint16_t)) < 0) {
          perror("print_ls_TX");
          return 1;
        }
        exit_code = be16toh(exit_code);
        printf("%d\n", exit_code);


    }
  }
  else {
      perror("Bad reply");
      return 1;
  }
  return 0;
}
