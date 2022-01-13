#ifndef REQUEST_REPLY_H
#define REQUEST_REPLY_H



uint8_t *CreateReq_CLIENT_REQUEST_LIST_TASKS(uint16_t *request_length);

uint8_t *CreateReq_CLIENT_REQUEST_CREATE_TASK(
                            int argc, char * argv[], 
                            char *minutes_str, char *hours_str,  char * daysofweek_str,
                            uint16_t *request_length, int pos_com );


int print_reply_RM(int fd);
int print_reply_SO_SE(int fd);
int print_reply_TM(int fd);

int print_reply_CR(int fd);
int print_reply_TX(int fd);
int print_reply_LS(int fd);


#endif