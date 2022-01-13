#include "file-system.h"

#define DECALAGE( X, DECAL) ( (X)<<(DECAL))


int is_sysfile_exist() {
    struct stat st;
    int ret = stat(MAIN_DIR, &st);

    if(ret == -1)
        return 0;

    if(S_ISDIR(st.st_mode)) {
        //let's check if .index.bin exist
        char path[strlen(MAIN_DIR) + strlen(INDEX) + 2];
        path[0] = '\0';
        strcat(path, MAIN_DIR);
        strcat(path, "/");
        strcat(path, INDEX);

        ret = stat(path, &st);
        if(ret == 1)
            return 0;
        else {
            if(S_ISREG(st.st_mode))
                    return 1; //exist
        }

    }
    return 0;
} 

int create_sysfile() {
    if(is_sysfile_exist())
        return -2;
    
    if(mkdir(MAIN_DIR, 0700) < 0)
        return -1;
    
    //index and try to set is content to id = 0
    char path[strlen(MAIN_DIR) + strlen(INDEX) + 2];
    path[0] = '\0';
    strcat(path, MAIN_DIR);
    strcat(path, "/");
    strcat(path, INDEX);

    int fd = creat(path, 0700);
    if(fd < 0)
        return -1;

    uint64_t val = 0;
    if(write(fd, &val, sizeof(uint64_t)) < 0)
        return -1;
    else {
        close(fd);
        return 0;
    }
}

int get_taskid_and_update(uint64_t *taskid) {
    if(is_sysfile_exist() == 0)
        return -1;

    char path[strlen(MAIN_DIR) + strlen(INDEX) + 2];
    path[0] = '\0';
    strcat(path, MAIN_DIR);
    strcat(path, "/");
    strcat(path, INDEX);

    int fd = open(path, O_RDONLY);
    if(fd < 0)
        return -1;

    uint64_t id = 0;
    if(read(fd, &id, sizeof(uint64_t)) < 0) {
        close(fd);
        return -1;
    }

    id++;
    close(fd);
    fd = open(path, O_WRONLY | O_TRUNC);
    if(write(fd, &id, sizeof(uint64_t)) < 0) {
        close(fd);
        return -1;
    }

    close(fd);
    *taskid = --id;
    return 0;    
}

int new_task_sysfile(uint64_t *taskid, struct timing *tm, struct commandline *com) {
    if(tm == NULL || com == NULL)
        return -1;
    if(is_sysfile_exist() == 0)
        return -1;

    //gettind id and update .index
    uint64_t id = 0;
    if(get_taskid_and_update(&id) < 0)
        return -1;
    
    //preparing path
    char tmp[UINT64_STRLEN];
    if(sprintf(tmp, "%ld", id) < 0)
        return -1;
    int len = strlen(MAIN_DIR) + UINT64_STRLEN;
    char path[len + 2];
    path[0] = '\0';
    strcat(path, MAIN_DIR);
    strcat(path, "/");
    strcat(path, tmp);

    if(mkdir(path, 0700) < 0)
        return -1;
    
    //commandline
    char path_commandline[len + strlen(COMMANDLINE) + 2];
    path_commandline[0] = '\0';
    strcat(path_commandline, path);
    strcat(path_commandline, "/");
    strcat(path_commandline, COMMANDLINE);

    //date
    char path_date[len + strlen(COMMANDLINE) + 2];
    path_date[0] = '\0';
    strcat(path_date, path);
    strcat(path_date, "/");
    strcat(path_date, DATE);

    //time-exitcode
    char path_te[len + strlen(TIME_EXITCODE) + 2];
    path_te[0] = '\0';
    strcat(path_te, path);
    strcat(path_te, "/");
    strcat(path_te, TIME_EXITCODE);

    //stdout
    char path_stdout[len + strlen(STDOUT_TASK) + 2];
    path_stdout[0] = '\0';
    strcat(path_stdout, path);
    strcat(path_stdout, "/");
    strcat(path_stdout, STDOUT_TASK);

    //stderr
    char path_stderr[len + strlen(STDERR_TASK) + 2];
    path_stderr[0] = '\0';
    strcat(path_stderr, path);
    strcat(path_stderr, "/");
    strcat(path_stderr, STDERR_TASK);

    //create files - if we find an error from create, we close all fd_NN
    int fd_commandline, fd_date, fd_te, fd_out, fd_err;
    if((fd_commandline = creat(path_commandline, 0700)) < 0) {
        return -1;
    }
    if((fd_date = creat(path_date, 0700)) < 0) {
        close(fd_commandline);
        return -1;
    }
    if((fd_te = creat(path_te, 0700)) < 0) {
        close(fd_commandline);
        close(fd_date);
        return -1;
    }
    if((fd_out = creat(path_stdout, 0700)) < 0) {
        close(fd_commandline);
        close(fd_date);
        close(fd_te);
        return -1;
    }
    if((fd_err = creat(path_stderr, 0700)) < 0) {
        close(fd_commandline);
        close(fd_date);
        close(fd_te);
        close(fd_err);
        return -1;
    }

    //closing fd that we will not used now
    if(close(fd_te) < 0 || close(fd_err) < 0 || close(fd_out) < 0) {
        //try to close, if error in all case we return -1...
        close(fd_commandline);
        close(fd_date);
        return -1;
    }

    //writing commandline and date
    uint32_t nb_com = 0;
    
    uint8_t *com_b = commandline_to_byte_array(&nb_com, com);
    if(com_b == NULL) {
        close(fd_commandline);
        close(fd_date);
        return -1;
    }
    if(write(fd_commandline, com_b, nb_com) < 0) {
        close(fd_commandline);
        close(fd_date);
        return -1;
    }
    free(com_b);
    
    if(write(fd_date, tm, sizeof(uint64_t) + sizeof(uint32_t) + sizeof(uint8_t)) < 0) {
        close(fd_commandline);
        close(fd_date);
        return -1;
    }

    if(close(fd_commandline) < 0 ||  close(fd_date) < 0)
        return -1;
    
    *taskid = id;
    return 0;
}

int rm_task_sysfile(uint64_t taskid) {
    if(is_sysfile_exist() == 0)
        return -1;

    //preparing path
    char tmp[UINT64_STRLEN];
    if(sprintf(tmp, "%ld", taskid) < 0)
        return -1;
    int len = strlen(MAIN_DIR) + UINT64_STRLEN;
    char path[len + 2];
    path[0] = '\0';
    strcat(path, MAIN_DIR);
    strcat(path, "/");
    strcat(path, tmp);

    //dir exist ?
    struct stat st;
    if(stat(path, &st) < 0)
        return -1;

    if(!S_ISDIR(st.st_mode) || (st.st_mode & S_IRWXU) != S_IRWXU)
        return -1;

    //let's walk inside path
    DIR *dirp = opendir(path);
    struct dirent *entry;

    while((entry = readdir(dirp))) {
        if(entry->d_name[0] == '.')
            continue;

        char path_file[len + strlen(entry->d_name) + 2];
        path_file[0] = '\0';
        strcat(path_file, path);
        strcat(path_file, "/");
        strcat(path_file, entry->d_name);

        if(stat(path_file, &st) < 0) {
            closedir(dirp);
            return -1;
        }

        if(!S_ISREG(st.st_mode) || (st.st_mode & S_IRWXU) != S_IRWXU) {
            closedir(dirp);
            return -1;
        }
        else {
            if(unlink(path_file) < 0) {
                closedir(dirp);
                return -1;
            }
        }
    }
    closedir(dirp);

    //delete dir
    if(rmdir(path) < 0) {
        return -1;
    }
    return 0;
}

int add_time_exitcode_to_task(uint64_t taskid, uint64_t time, uint16_t exitcode) {
    if(is_sysfile_exist() == 0)
        return -1;

    //preparing path
    char tmp[UINT64_STRLEN];
    if(sprintf(tmp, "%ld", taskid) < 0)
        return -1;
    int len = strlen(MAIN_DIR) + UINT64_STRLEN + strlen(TIME_EXITCODE);
    char path[len + 3];
    path[0] = '\0';
    strcat(path, MAIN_DIR);
    strcat(path, "/");
    strcat(path, tmp);
    strcat(path, "/");
    strcat(path, TIME_EXITCODE);

    struct stat st;
    if(stat(path, &st) < 0)
        return -1;

    if(!S_ISREG(st.st_mode) || (st.st_mode & S_IRWXU) != S_IRWXU)
        return -1;
    
    int fd = 0;
    if((fd = open(path, O_WRONLY | O_APPEND)) < 0)
        return -1;
    
    // On persiste en Big Edian sur disque
    time= htobe64(time);
    if(write(fd, &time, sizeof(uint64_t)) < 0) {
        close(fd);
        return -1;
    }
    // On persiste en Big Edian sur disque
    exitcode = htobe16(exitcode);
    if(write(fd, &exitcode, sizeof(uint16_t)) < 0) {
        close(fd);
        return -1;
    }

    if(close(fd) < 0)
        return -1;
    else
        return 0;
}



extern void PrintHexa(uint8_t *buffer, size_t sz);
int check_date_task(struct timing *timing, uint64_t taskid) {
   

    if(timing == NULL)
        return -1;
    if(is_sysfile_exist() == 0)
        return -1;

    time_t actual_time = time(NULL);
    struct tm *tm_struct = localtime(&actual_time);
  
     //minutes
    uint64_t maskCurrentMinute =1;
    maskCurrentMinute = DECALAGE( maskCurrentMinute, tm_struct->tm_min);
    //on decale le bit correspondant à tm_min dans un masque afin de comparer ce masque a timing->minutes
    if((be64toh(timing->minutes) & maskCurrentMinute) == 0)
        return -1;



    //hours
    uint32_t maskCurrentHour =1;
    maskCurrentHour = DECALAGE( maskCurrentHour, tm_struct->tm_hour);
    if((be32toh(timing->hours) & maskCurrentHour) == 0)
        return -1;
    //days
     uint8_t maskCurrentDay =1;
    maskCurrentDay = DECALAGE( maskCurrentDay, tm_struct->tm_wday);
    if(((timing->daysofweek) & maskCurrentDay)== 0)
        return -1;
    
    uint64_t last_execution_time;
  

    if ( get_last_execution_time_for_task( taskid, &last_execution_time) == 0)
    {
        struct tm *lastexecution_tm_struct = localtime((time_t*)&last_execution_time);


 
        

    uint64_t maskLastMinute =1;
    maskLastMinute = DECALAGE( maskLastMinute,lastexecution_tm_struct->tm_min);
  
    uint32_t maskLastHour =1;
    maskLastHour = DECALAGE( maskLastHour, lastexecution_tm_struct->tm_hour);


    uint8_t maskLastDay =1;
    maskLastDay = DECALAGE( maskLastDay, lastexecution_tm_struct->tm_wday);
     
        if( //minutes
       
            (maskCurrentMinute & maskLastMinute)
            && 
            //hours
            (maskCurrentHour & maskLastHour) 
            &&
            //days
            (maskCurrentDay & maskLastDay) 
        )
        {
            // déja exécuté ce jour, cette heure, cette minute
            return -1;
        }
    }

    return 0;
}

int check_date_forall(uint64_t *taskid) {
    if(is_sysfile_exist() == 0)
        return -1;

    struct stat st;
    DIR *dirp = opendir(MAIN_DIR);
    struct dirent *entry;

    int len_dir = strlen(MAIN_DIR);

    while((entry = readdir(dirp))) {
        //hiden file ?
        if(entry->d_name[0] == '.')
            continue;

        //preparing dir path and check mode
        int len = len_dir + UINT64_STRLEN + strlen(DATE) + 3;
        char path[len];
        path[0] = '\0';
        strcat(path, MAIN_DIR);
        strcat(path, "/");
        strcat(path, entry->d_name);

        if(stat(path, &st) < 0)
        return -1;
        if(!S_ISDIR(st.st_mode) || (st.st_mode & S_IRWXU) != S_IRWXU) {
            closedir(dirp);
            return -1;
        }

        //completing path
        strcat(path, "/");
        strcat(path, DATE);
        //printf("%s\n", path); //TO REMOVE

        //can be used ?
        if(stat(path, &st) < 0)
            continue; //let's check the others too, maybe a fill was deleted ?
        if(!S_ISREG(st.st_mode) || (st.st_mode & S_IRWXU) != S_IRWXU)
            continue; //let's check the others too

        //getting date for checking
        struct timing timing;
        int fd;
        if((fd = open(path, O_RDONLY)) < 0) {
            closedir(dirp);
            return -1;
        }

        if(read(fd, &timing, sizeof(uint64_t) + sizeof(uint32_t) + sizeof(uint8_t)) < 0) {
            closedir(dirp);
            close(fd);
            return -1;
        }
        close(fd);

        //to exec ?
        uint64_t currentTaskid = strtoul(entry->d_name, NULL, 10);
        if(check_date_task(&timing, currentTaskid) == 0) {
            //we find a date ! let's close files and return taskid an an integer
            *taskid = currentTaskid;
            closedir(dirp);
            return 0;
        }

    } //end of while

    closedir(dirp);
    return -1;
}

struct commandline *get_commandline_task(uint64_t taskid) {
    if(is_sysfile_exist() == 0)
        return NULL;
    
    int len = strlen(MAIN_DIR) + UINT64_STRLEN + strlen(COMMANDLINE);
    char tmp[UINT64_STRLEN];
    tmp[0] = '\0';
    if(sprintf(tmp, "%ld", taskid) < 0)
        return NULL;

    char path[len + 3];
    path[0] = '\0';
    strcat(path, MAIN_DIR);
    strcat(path, "/");
    strcat(path, tmp);
    strcat(path, "/");
    strcat(path, COMMANDLINE);

    //exist ?
    struct stat st;
    if(stat(path, &st) < 0)
        return NULL;
    if(!S_ISREG(st.st_mode) || (st.st_mode & S_IRWXU) != S_IRWXU)
        return NULL;

    int fd;
    if((fd = open(path, O_RDONLY)) < 0)
        return NULL;

    struct commandline *com = read_commandline(fd);
    close(fd);
    return com;
}

uint8_t *get_time_exitcode_task(uint32_t *final_len, uint64_t taskid) {
    if(is_sysfile_exist() == 0)
        return NULL;
    
    //build path
    int len = strlen(MAIN_DIR) + UINT64_STRLEN + strlen(TIME_EXITCODE);
    char tmp[UINT64_STRLEN];
    tmp[0] = '\0';
    if(sprintf(tmp, "%ld", taskid) < 0)
        return NULL;

    char path[len + 3];
    path[0] = '\0';
    strcat(path, MAIN_DIR);
    strcat(path, "/");
    strcat(path, tmp);
    strcat(path, "/");
    strcat(path, TIME_EXITCODE);

    //exist ?
    struct stat st;
    if(stat(path, &st) < 0)
        return NULL;
    if(!S_ISREG(st.st_mode) || (st.st_mode & S_IRWXU) != S_IRWXU)
        return NULL;

    //len
    uint32_t len_te = sizeof(uint64_t) + sizeof(uint16_t);
    uint32_t nb_runs = st.st_size / len_te;
    uint32_t len_header = sizeof(uint16_t) + sizeof(uint32_t);

    //byte array
    uint8_t *byte_array = malloc(len_header + st.st_size);
    if(byte_array == NULL)
        return NULL;

    uint16_t code = htobe16(SERVER_REPLY_OK);
    uint32_t nb_runs_endian = htobe32(nb_runs);
    memcpy(byte_array, &code, sizeof(uint16_t));
    memcpy(byte_array+sizeof(uint16_t), &nb_runs_endian, sizeof(uint32_t));
    
    *final_len = len_header;
    //nb_runs = 0 ?
    if(nb_runs == 0) {
        return byte_array; //"empty" but not an error
    }

    //let's read
    int fd;
    if((fd = open(path, O_RDONLY)) < 0)
        return NULL;

    // On lit le reste du fichier (liste [time , exitcode] ) directement dans le buffer puisque cette liste
    // a ete persistee en Big Endian
    int nb = read(fd, byte_array+len_header, st.st_size);
    close(fd);
    if(nb != st.st_size || nb < 0) {
        free(byte_array);
        return NULL;
    }

    *final_len += st.st_size;
    return byte_array;
}

struct string_utils *get_string_stdout_stderr(uint64_t taskid, char flag) {
    if(is_sysfile_exist() == 0)
        return NULL;
    
    if(flag != 'o' && flag != 'e')
        return NULL;

    int len;
    if(flag == 'o')
        len = strlen(MAIN_DIR) + UINT64_STRLEN + strlen(STDOUT_TASK);
    else
        len = strlen(MAIN_DIR) + UINT64_STRLEN + strlen(STDERR_TASK);
    
    char tmp[UINT64_STRLEN];
    tmp[0] = '\0';
    if(sprintf(tmp, "%ld", taskid) < 0)
        return NULL;

    char path[len + 3];
    path[0] = '\0';
    strcat(path, MAIN_DIR);
    strcat(path, "/");
    strcat(path, tmp);
    strcat(path, "/");
    if(flag == 'o')
        strcat(path, STDOUT_TASK);
    else
        strcat(path, STDERR_TASK);

    //exist ?
    struct stat st;
    if(stat(path, &st) < 0)
        return NULL;
    if(!S_ISREG(st.st_mode) || (st.st_mode & S_IRWXU) != S_IRWXU)
        return NULL;

    //prep fd
    int fd;
    if((fd = open(path, O_RDONLY)) < 0)
        return NULL;

    //prep string
    struct string_utils *su = malloc(sizeof(struct string_utils));
    if(su == NULL) {
        close(fd);
        return NULL;
    }

    su->len = st.st_size;
    su->string = malloc(sizeof(char) * st.st_size +1);
    if(su->string == NULL) {
        close(fd);
        free(su);
        return NULL;
    }
    su->string[sizeof(char) * st.st_size] = '\0';

    //read and set string
    if(read(fd, su->string, sizeof(char) * st.st_size) < 0) {
        close(fd);
        free_string_utils(su);
        return NULL;
    }
    
    close(fd);
    return su;
}

int count_task_dir() {
    if(is_sysfile_exist() == 0)
        return -1;
    
    int count = 0;
    struct stat st;
    DIR *dirp = opendir(MAIN_DIR);
    struct dirent *entry;

    while((entry = readdir(dirp))) {
        if(entry->d_name[0] == '.')
            continue;

        int len = strlen(MAIN_DIR) + UINT64_STRLEN + 2;
        char path[len];
        path[0] = '\0';
        strcat(path, MAIN_DIR);
        strcat(path, "/");
        strcat(path, entry->d_name);

        if(stat(path, &st) < 0)
        return -1;
        if(!S_ISDIR(st.st_mode) || (st.st_mode & S_IRWXU) != S_IRWXU) {
            closedir(dirp);
            return -1;
        }
        count++;
    }
    closedir(dirp);
    return count;
}


uint8_t *get_tasks_list(uint32_t *final_len) {
    int count = count_task_dir(); //already check with is_sysfile_exist()
    if(count < 0)
        return NULL;

    uint8_t *tmp = NULL;
    uint32_t len_tmp = 0;

    struct stat st;
    DIR *dirp = opendir(MAIN_DIR);
    struct dirent *entry;


    int len_dir = strlen(MAIN_DIR);

    while((entry = readdir(dirp))) {
        //hiden file ?
        if(entry->d_name[0] == '.')
            continue;

        //--- getting datas ---
        //taskid
        uint64_t taskid = strtoul(entry->d_name, NULL, 10);

        //timing
        int len = len_dir + UINT64_STRLEN + strlen(DATE) + 3;
        char path[len];
        path[0] = '\0';
        strcat(path, MAIN_DIR);
        strcat(path, "/");
        strcat(path, entry->d_name);
        strcat(path, "/");
        strcat(path, DATE);

        if(stat(path, &st) < 0) {
            count--;
            continue;
        }
        if(!S_ISREG(st.st_mode) || (st.st_mode & S_IRWXU) != S_IRWXU) {
            count--;
            continue;
        }
        //fill timing
        struct timing tm;
        int fd;
        if((fd = open(path, O_RDONLY)) < 0) {
            count--;
            continue;
        }
        if(read(fd, &tm, sizeof(uint64_t) + sizeof(uint32_t) + sizeof(uint8_t)) < 0) {
            close(fd);
            count--;
            continue;
        }
        close(fd);

        //commandline
        struct commandline *com = get_commandline_task(taskid);
        if(com == NULL) {
            count--;
            continue;
        }
        //--- getting datas ---

        //--- concat bytes array ---
        //taskid
        uint32_t len_taskid = sizeof(uint64_t);
        uint8_t *taskid_tab = uint64_to_byte_array(taskid);
        if(taskid_tab == NULL) {
            count --;
            free_commandline(com, 1);
            continue;
        }

        //timing
        uint8_t *time_tab = malloc(sizeof(uint64_t) + sizeof(uint32_t) + sizeof(uint8_t));
        if(time_tab == NULL) {
            count--;
            free(taskid_tab);
            free_commandline(com, 1);
            continue;
        }
        uint32_t len_time = sizeof(uint64_t);
        memcpy(time_tab, &tm.minutes, len_time);
        memcpy(time_tab+len_time, &tm.hours, sizeof(uint32_t));
        len_time += sizeof(uint32_t);
        memcpy(time_tab+len_time, &tm.daysofweek, sizeof(uint8_t));
        len_time += sizeof(uint8_t);

        //commandline
        uint32_t len_com = 0;
        uint8_t *com_tab = commandline_to_byte_array(&len_com, com);
        if(com_tab == NULL) {
            count--;
            free(taskid_tab);
            free_commandline(com, 1);
            free(time_tab);
            free(com_tab);
            continue;
        }

        //concat
        uint8_t *tmp_tabs = malloc(sizeof(uint8_t)*(len_taskid + len_time + len_com));
        if(tmp_tabs == NULL) {
            count--;
            free(taskid_tab);
            free_commandline(com, 1);
            free(time_tab);
            free(com_tab);
            continue;
        }

        memcpy(tmp_tabs, taskid_tab, len_taskid);
        memcpy(tmp_tabs+len_taskid, time_tab, len_time);
        memcpy(tmp_tabs+(len_taskid+len_time), com_tab, len_com);
        uint32_t len_tmps = len_taskid + len_com + len_time;

        //init ?
        if(tmp == NULL) {
            tmp = tmp_tabs;
            len_tmp = len_tmps;
        }
        else {
            uint8_t *concat_all = malloc(sizeof(uint8_t)*(len_tmp + len_taskid + len_com + len_time));
            if(concat_all == NULL) {
                count--;
                free(taskid_tab);
                free_commandline(com, 1);
                free(time_tab);
                free(com_tab);
                free(tmp_tabs);
                continue;
            }

            memcpy(concat_all, tmp, len_tmp);
            memcpy(concat_all+len_tmp, tmp_tabs, len_tmps);
            len_tmp += len_tmps;
            free(tmp);
            tmp = concat_all;
            free(tmp_tabs);

        }
        //--- concat bytes array ---

        //free
        free(taskid_tab);
        free_commandline(com, 1);
        free(time_tab);
        free(com_tab);
    }
    closedir(dirp);

    //finally...
    uint32_t len_header = sizeof(uint16_t) + sizeof(uint32_t);
    uint8_t *header = malloc(sizeof(uint8_t) * (len_header + len_tmp));
    if(header == NULL) {
        if(tmp != NULL)
            free(tmp);
        return NULL;
    }
    
    uint16_t code = htobe16(SERVER_REPLY_OK);
    uint32_t nb_tasks_endian = htobe32(count);
    
    memcpy(header, &code, sizeof(uint16_t));
    memcpy(header+sizeof(uint16_t), &nb_tasks_endian, sizeof(uint32_t));

    if(tmp != NULL) {
        memcpy(header+len_header, tmp, len_tmp);
        free(tmp);
        *final_len = len_header + len_tmp;
    }
    else
        *final_len = len_header;

    return header;
}



int get_last_execution_time_for_task(uint64_t taskid, uint64_t *time) 
{
    uint32_t buflen;
    uint8_t *buffer = get_time_exitcode_task(&buflen,  taskid);
    if( buffer == NULL)
        return -1;
    

    //len
    uint32_t len_te = sizeof(uint64_t) + sizeof(uint16_t);
     uint32_t len_header = sizeof(uint16_t) + sizeof(uint32_t);
 
    uint32_t nb_runs = be32toh( *((uint32_t*)(buffer + sizeof(uint16_t)))) ;
   
    if( nb_runs ==0)
    {
        free(buffer);
        return -1;
    }
    if( buflen != ( len_header + len_te*nb_runs) )
    {
        free(buffer);
        return -1;
    }
   
     uint8_t *bufcourant= buffer;
    bufcourant += len_header;

    // go to last run
    bufcourant += ( nb_runs-1) *(len_te);
 
    // get last run time

    uint64_t lastExecTime = *((uint64_t*)bufcourant) ;
    uint64_t lastBEExecTime = be64toh( lastExecTime );
    *time=  lastBEExecTime;
    free(buffer);
 
    return 0;
}


int get_nbruns(uint64_t taskid) 
{
    uint32_t buflen;
    uint8_t *buffer = get_time_exitcode_task(&buflen,  taskid);
    if( buffer == NULL)
        return -1;
    

    //len
  //  uint32_t len_te = sizeof(uint64_t) + sizeof(uint16_t);
   // uint32_t len_header = sizeof(uint16_t) + sizeof(uint32_t);
 
    uint32_t nb_runs = be32toh( *((uint32_t*)(buffer + sizeof(uint16_t)))) ;
   
   
    free(buffer);
 
    return nb_runs;
}



int get_task_output_file(uint64_t taskid, char flag) 
{
    if(is_sysfile_exist() == 0)
        return -1;
    
    if(flag != 'o' && flag != 'e')
        return -1;

    int len;
    if(flag == 'o')
        len = strlen(MAIN_DIR) + UINT64_STRLEN + strlen(STDOUT_TASK);
    else
        len = strlen(MAIN_DIR) + UINT64_STRLEN + strlen(STDERR_TASK);
    
    char tmp[UINT64_STRLEN];
    tmp[0] = '\0';
    if(sprintf(tmp, "%ld", taskid) < 0)
        return -1;

    char path[len + 3];
    path[0] = '\0';
    strcat(path, MAIN_DIR);
    strcat(path, "/");
    strcat(path, tmp);
    strcat(path, "/");
    if(flag == 'o')
        strcat(path, STDOUT_TASK);
    else
        strcat(path, STDERR_TASK);

    //exist ?
    struct stat st;
    if(stat(path, &st) < 0)
        return -1;
    if(!S_ISREG(st.st_mode) || (st.st_mode & S_IRWXU) != S_IRWXU)
        return -1;

    //prep fd
    int fd = 0;
    if((fd = open(path, O_WRONLY | O_TRUNC)) < 0)
        return -1;
    
    return fd;
}

