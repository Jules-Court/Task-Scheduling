#include "pipe.h"

/* Ebauche qui a été remplacé qui devait servir a la gestion des requêtes pour saturnd */


int requestPipe(int fd_request, int fd_reply)
{

    if ((fd_reply = open("run/pipes/saturnd-reply-pipe", O_WRONLY)) == -1)
    {
        printf("Error with opening the reply pipe\n");
        return -1;
    }
    if ((fd_request = open("run/pipes/saturnd-request-pipe", O_RDONLY)) == -1)
    {
        printf("Error with opening the request pipe\n");
        return -1;
    }

    uint16_t opcode;
    uint64_t taskid;

    while (1)
    {
        uint16_t operation = read(fd_request, &opcode, sizeof(opcode));

        switch (operation)
        {

        case CLIENT_REQUEST_LIST_TASKS:
        { // -l // SEUL OK POSSIBLE //FONCTION
            uint32_t nb = 0;
            uint8_t *tab = get_tasks_list(&nb);
            if (tab == NULL)
            {
                printf("Error tab == null");
                close(fd_reply);
                close(fd_request);
                return -1;
            }
            if (write(fd_reply, tab, nb) < 0)
            {
                close(fd_reply);
                close(fd_request);
                return -1;
            }
        }
        break;
        case CLIENT_REQUEST_CREATE_TASK:
        { //-c //SEUL OK POSSIBLE // FONCTIONNE
            struct timing *time = malloc(sizeof(time));
            if (read(fd_request, &time, sizeof(uint64_t) + sizeof(uint32_t) + sizeof(uint8_t)) < 0)
            {
                printf("Error to read request_pipe");
                free(time);
                return -1;
            }
            if (time == NULL)
            {
                printf("Error time == null");
                free(time);
                return -1;
            }

            if (new_task_sysfile(&taskid, time, read_commandline(fd_request)) < 0)
            {

                free(time);
                printf("Error in creation of the task");
                close(fd_reply);
                close(fd_request);
                return -1;
            }

            uint16_t code = htobe16(SERVER_REPLY_OK);

            if (write(fd_reply, &code, sizeof(uint16_t)) < 0)
            {
                close(fd_reply);
                close(fd_request);
                free(time);
                return -1;
            }
            free(time);
        }
        break;
        case CLIENT_REQUEST_REMOVE_TASK:
        { //-r //OK ET ERROR POSSIBLE AVEC JUSTE NF  // FONCTIONNE

            uint16_t code = htobe16(SERVER_REPLY_OK);
            uint64_t reptype;
            if (read(fd_request, &reptype, sizeof(reptype)) < 0)
            {
                printf("Error reading");
                break;
            }
            reptype = be16toh(reptype);
            if (rm_task_sysfile(reptype) == -1)
            {
                code = htobe16(SERVER_REPLY_ERROR);
                if (write(fd_reply, &code, sizeof(uint16_t)) < 0)
                {
                    printf("Error reading");
                }
                code = be16toh(SERVER_REPLY_ERROR_NOT_FOUND);
                if (write(fd_reply, &code, sizeof(uint16_t)) < 0)
                {
                    printf("Error writing in pipe");
                }
            }
            if (write(fd_reply, &code, sizeof(uint16_t)) < 0)
            {
                printf("Error reading");
            }
        }
        break;
        case CLIENT_REQUEST_GET_TIMES_AND_EXITCODES:
        { //-x // OK ET ERROR POSSIBLE AVEC JUSTE NF // FONCTIONNE
            uint16_t code = htobe16(SERVER_REPLY_OK);
            uint32_t len;
            uint64_t task_user;
            if (read(fd_request, &task_user, sizeof(uint64_t)) < 0)
            {
                printf("Error to read request_pipe");
                break;
            }

            task_user = be64toh(task_user);
            uint8_t *time_exitcode = get_time_exitcode_task(&len, task_user);
            if (time_exitcode == NULL)
            {
                code = htobe16(SERVER_REPLY_ERROR);
                if (write(fd_reply, &code, sizeof(uint16_t)) < 0)
                {
                    printf("Error writing");
                    break;
                }
                code = htobe16(SERVER_REPLY_ERROR_NOT_FOUND);
                if (write(fd_reply, &code, sizeof(uint16_t)) < 0)
                {
                    printf("Error writing");
                    break;
                }
            }
            uint8_t *op_code = uint16_to_byte_array(code);
            uint8_t *request = concat_byte_array(op_code, sizeof(uint16_t), time_exitcode, sizeof(uint8_t));
            if (write(fd_reply, request, sizeof(uint16_t) + sizeof(uint8_t)) < 0)
            {
                printf("Error writing");
                break;
            }
        }
        break;

        case CLIENT_REQUEST_TERMINATE:
        { //-q // SEUL OK POSSIBLE //FONCTIONNE
            uint16_t code = htobe16(SERVER_REPLY_OK);
            if (write(fd_reply, &code, sizeof(code)) < 0)
            {
                close(fd_reply);
                close(fd_request);
                return -1;
            }
            close(fd_reply);
            close(fd_request);
        }
        break;

        case CLIENT_REQUEST_GET_STDOUT:
        { //-o //OK ET ERROR POSSIBLE NF SI PAS DE TASKID, NR SI AS FAITES AU - 1 FOIS // FONCTIONNE
            uint16_t code = htobe16(SERVER_REPLY_OK);
            uint64_t task_user;
            if (read(fd_request, &task_user, sizeof(uint64_t)) == -1)
            {
                printf("Error");
                break;
            }
            task_user = be64toh(task_user);

            struct string_utils *stdout = malloc(sizeof(struct string_utils));
            if (stdout == NULL)
            {
                printf("stderr = null");
            }

            stdout = get_string_stdout_stderr(task_user, 'o');
            ;
            if (stdout == NULL)
            {
                code = htobe16(SERVER_REPLY_ERROR);
                if (write(fd_reply, &code, sizeof(uint16_t)) < 0)
                {
                    printf("Error writing");
                    break;
                }
                // verifier si la tache a été execute avec check_date_forall

                if (task_user < 0 || task_user > taskid)
                {
                    code = htobe16(SERVER_REPLY_ERROR_NOT_FOUND);
                }
                else if (check_date_forall(&task_user) == -1)
                {
                    code = htobe16(SERVER_REPLY_ERROR_NEVER_RUN);
                }
            }

            char *output = stdout->string;
            uint8_t *output_to_uint = string_to_byte_array(output);

            uint8_t *op_code = uint16_to_byte_array(code);
            uint8_t *size_output = uint32_to_byte_array(stdout->len);

            uint8_t *concat_code_size = concat_byte_array(op_code, sizeof(uint16_t), size_output, sizeof(uint32_t));
            uint8_t *request = concat_byte_array(concat_code_size, sizeof(uint16_t) + sizeof(uint32_t), output_to_uint, sizeof(uint8_t));
            if (write(fd_reply, request, sizeof(uint16_t) + sizeof(uint32_t) + sizeof(uint8_t)) < 0)
            {
                printf("Error writing");
                break;
            }
        }
        break;

        case CLIENT_REQUEST_GET_STDERR:
        { //-e //OK ET ERROR POSSIBLE NF SI PAS DE TASKID, NR SI AS FAITES AU - 1 FOIS // // FONCTIONNE
            uint16_t code = htobe16(SERVER_REPLY_OK);
            uint64_t task_user;
            if (read(fd_request, &task_user, sizeof(uint64_t)) == -1)
            {
                printf("Error reading");
                break;
            }
            task_user = be64toh(task_user);

            struct string_utils *stderr = malloc(sizeof(struct string_utils));
            if (stderr == NULL)
            {
                printf("stderr = null");
            }

            stderr = get_string_stdout_stderr(task_user, 'e');
            if (stderr == NULL)
            {
                code = be16toh(SERVER_REPLY_ERROR);
                if (write(fd_reply, &code, sizeof(uint16_t)) < 0)
                {
                    printf("Error writing");

                    break;
                }
                // verifier si la tache a été execute avec check_date_forall ou check_date_task?

                if (task_user < 0 || task_user > taskid)
                {
                    code = htobe16(SERVER_REPLY_ERROR_NOT_FOUND);
                }
                else if (check_date_forall(&task_user) == -1)
                {
                    code = htobe16(SERVER_REPLY_ERROR_NEVER_RUN);
                }
            }
            char *output = stderr->string;
            uint8_t *output_to_uint = string_to_byte_array(output);

            uint8_t *op_code = uint16_to_byte_array(code);
            uint8_t *size_output = uint32_to_byte_array(stderr->len);

            uint8_t *concat_code_size = concat_byte_array(op_code, sizeof(uint16_t), size_output, sizeof(uint32_t));
            uint8_t *request = concat_byte_array(concat_code_size, sizeof(uint16_t) + sizeof(uint32_t), output_to_uint, sizeof(uint8_t));

            if (write(fd_reply, request, sizeof(uint16_t) + sizeof(uint32_t) + sizeof(uint8_t)) < 0)
            {
                printf("Error writing");
                break;
            }
        }
        break;

        default:
            break;
        }

        close(fd_reply);
        close(fd_request);
    }
    return 0;
}
