#include "commandline.h"

struct commandline *parse_argv(uint32_t argc, char *argv[]) {
    if(argc <= 0) return NULL;
    if(argv == NULL) return NULL;
    
    struct commandline *com = malloc(sizeof(struct commandline));
    if(com == NULL) return NULL;    
    

    com->argv = malloc(sizeof(struct string_utils *) * argc);
    if(com->argv == NULL) return NULL;
    com->argc = argc;

    for(int i = 0; i < argc; i++) {
        com->argv[i] = malloc(sizeof(struct string_utils));
        if(com->argv[i] == NULL) return NULL;

        com->argv[i]->len = strlen(argv[i]);
        com->argv[i]->string = argv[i];
    }

    return com;
}

uint8_t *commandline_to_byte_array(uint32_t *nb, struct commandline *com) {
    if(com == NULL) return NULL;

    uint32_t len = sizeof(uint32_t); //argc

    for(int i = 0; i < com->argc; i++) {
        len += sizeof(uint32_t); //com[i]->len
        len += com->argv[i]->len; //string
    }

    uint8_t *tmp;
    uint8_t * tab = malloc(len);
    if(tab == NULL) return NULL;

    //fill the tab with argc
    tmp = uint32_to_byte_array(com->argc);
    for(int i = 0; i < sizeof(uint32_t); i++) {
        tab[i] = tmp[i];
    }

    //now, let's fill with argv
    free(tmp);
    int ind = 0;
    int i = sizeof(uint32_t);
    while(i < len) {
        //argv[i]->len
        tmp = uint32_to_byte_array(com->argv[ind]->len);
        for(int j = 0; j < sizeof(uint32_t); j++) {
            tab[i] = tmp[j];
            i++; //because we iterate with i for tab, and this part fill j bytes
        }

        //argv[i]->string
        free(tmp);
        tmp = string_to_byte_array(com->argv[ind]->string);
        for(int j = 0; j < com->argv[ind]->len; j++) {
            tab[i] = tmp[j];
            i++; //because we iterate with i for tab, and this part fill j bytes
        }
        ind++;
        free(tmp);
    }
    *nb = len;
    return tab;
}

struct commandline *read_commandline(int fd) {
    uint32_t argc = 0;
    uint32_t argc_tmp = 0;
    uint8_t *tmp;

    if(read(fd, &argc, sizeof(uint32_t)) < 0) {
        perror("read_commandline");
        return NULL;
    }
    argc = be32toh(argc);

    struct commandline *com = malloc(sizeof(struct commandline));
    if(com == NULL) return NULL;

    com->argv = malloc(sizeof(struct string_utils *) * argc);
    if(com->argv == NULL) return NULL;

    com->argc = argc;

    for(int i = 0; i < argc; i++) {
        //len of string i
        if(read(fd, &argc_tmp, sizeof(uint32_t)) < 0) {
            perror("read_commandline");
            free(com->argv);
            free(com);
            return NULL;
        }
        com->argv[i] = malloc(sizeof(struct string_utils));
        if(com->argv[i] == NULL) {
            perror("read_commandline");
            return NULL;
        }
        
        argc_tmp = be32toh(argc_tmp);
        com->argv[i]->len = argc_tmp;
        
        //let's get that string
        tmp = malloc(argc_tmp);
        if(tmp == NULL) {
            perror("read_commandline");
            return NULL;
        }
        
        if(read(fd, tmp, argc_tmp) < 0) {
            perror("read_commandline");
            free(com->argv);
            free(com);
            return NULL;
        }

        char *str_tmp = byte_array_to_string(tmp,  argc_tmp);
        if(str_tmp == NULL) return NULL;

        com->argv[i]->string = str_tmp;
        str_tmp = NULL;
        free(tmp);
    }
    return com;
}

char * string_commandline(struct commandline *com) {
    uint32_t len_str = com->argc;//com->argc; //com->argc - 1 space + 1 for \0

    for(int i = 0; i < com->argc; i++) {
        len_str += com->argv[i]->len;
    }
    char *str = malloc(len_str);
    if(str == NULL) return NULL;
    str[0] = '\0';

    for(int i = 0; i < com->argc; i++) {
        strcat(str, com->argv[i]->string);
        if(i < com->argc-1) strcat(str, " ");
    }
    return str;
}


void free_commandline(struct commandline *com, int clearString) {
    if(com == NULL) return;

    for(int i = 0; i < com->argc; i++) {
        if(clearString){
            free_string_utils(com->argv[i]);
        }
        else {
            free(com->argv[i]);
            com->argv[i] = NULL;
        }
    }
    free(com->argv);
    com->argv = NULL;
    free(com);
    com = NULL;
}


struct commandline *commandline_from_byte_array(uint8_t *buffer, size_t sz) {
    uint32_t argc = 0;
    uint32_t argc_tmp = 0;
    uint8_t *tmp;
    const int szUINT32 = sizeof(uint32_t);


    argc=*((uint32_t*)buffer);
    buffer+=szUINT32;

    argc = be32toh(argc);

    struct commandline *com = malloc(sizeof(struct commandline));
    if(com == NULL) return NULL;

    com->argv = malloc(sizeof(struct string_utils *) * argc);
    if(com->argv == NULL) return NULL;

    com->argc = argc;

    for(int i = 0; i < argc; i++) {
        //len of string i
        argc_tmp=*((uint32_t*)buffer);
        buffer+=szUINT32;

        com->argv[i] = malloc(sizeof(struct string_utils));
        if(com->argv[i] == NULL) {
            perror("read_commandline");
            return NULL;
        }
        
        argc_tmp = be32toh(argc_tmp);
        com->argv[i]->len = argc_tmp;
        
        //let's get that string
        tmp = malloc(argc_tmp);
        if(tmp == NULL) {
            perror("read_commandline");
            return NULL;
        }
        memcpy(tmp,buffer, argc_tmp);
        buffer+=argc_tmp;

        char *str_tmp = byte_array_to_string(tmp,  argc_tmp);
        if(str_tmp == NULL) return NULL;

        com->argv[i]->string = str_tmp;
        str_tmp = NULL;
        free(tmp);
    }
    return com;
}
