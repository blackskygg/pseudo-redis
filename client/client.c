#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

/* some limits */
#define BUF_SIZE 1024 * 1024
#define MAX_RPLY_SIZE 1024*1024
#define NAME_LEN 16

/* the reply types
 * NOTE : the arr protocal is simple : (type|len|val) denotes an element
 */
#define RPLY_COMMAND 0x00
#define RPLY_OK 0x01
#define RPLY_NIL 0x02
#define RPLY_FAIL 0x03
#define RPLY_INT 0x04
#define RPLY_FLOAT 0x05
#define RPLY_STRING 0x06
#define RPLY_ARR 0x07
#define RPLY_TYPE 0x08
/* types for bsses in the arr reply */
#define STR_NORMAL 0x00
#define STR_NIL 0x01
#define STR_END 0x02
#define STR_NOT_QUOTED 0x04
#define STR_NI 0x08
/* type identifiers */
#define STRING 0
#define INTEGER 1
#define SET 2
#define HASH 3
#define LIST 4
#define NONE 5

/* reply structure */
struct reply {
        uint8_t reply_type;
        uint32_t len;
        uint8_t data[];
}__attribute__((__packed__));
typedef struct reply reply_t;

/* raw request structure */
struct request {
        char command[16];
        uint32_t client_id; /* allocated on connection, by the server */
        uint32_t len;  /* length of data */
        uint8_t  data[]; /* the request string */
}__attribute__((__packed__));
typedef struct request request_t;


/* global variables */
int id; /* client id */
int fd; /* used to communicate with the server */
char buf[BUF_SIZE];

/* type->string map */
char *type_str[] = {"string",
                    "integer",
                    "set",
                    "hash",
                    "list",
                    "none"};

int connect_server(ushort port, char *host)
{
        struct hostent* info;
        struct sockaddr_in addr;

        info = gethostbyname(host);

        addr.sin_family = info->h_addrtype;
        addr.sin_port = htons(port);
        memcpy((void *) &addr.sin_addr, info->h_addr_list[0], info->h_length);

        fd = socket(AF_INET, SOCK_STREAM, 0);
        return connect(fd, (struct sockaddr *)&addr, sizeof addr);
}

/* send a binary string */
int send_request(char name[NAME_LEN], char *str, size_t len)
{
        request_t *request = malloc(sizeof(request_t) + len);

        if(!request)
                return -1;

        memcpy(request->command, name, NAME_LEN);
        memcpy(request->data, str, len);
        request->len = len;
        request->client_id = id;
        write(fd, request, sizeof(request_t) + len);
}

void print_str(uint8_t *data, size_t len, bool quote)
{
        if(quote)
                putchar('\"');
        for(size_t i = 0; i < len; ++i) {
                if(isgraph(data[i]) || ' ' == data[i])
                        putchar(data[i]);
                else
                        printf("\\x%02x", data[i]);
        }
        if(quote)
                putchar('\"');
        putchar('\n');
}

void display_reply(reply_t *reply)
{
        int index = 1;
        uint32_t len = 0;
        uint8_t *pos = reply->data;
        uint8_t type;

        switch(reply->reply_type) {
        case RPLY_OK:
                printf("OK\n");
                break;
        case RPLY_FAIL:
                printf("(error) ");
                print_str(reply->data, reply->len, false);
                break;
        case RPLY_STRING:
                print_str(reply->data, reply->len, true);
                break;
        case RPLY_INT:
                printf("(interger) %d\n", be64toh(*(int64_t*)reply->data));
                break;
        case RPLY_NIL:
                printf("(nil)\n");
                break;
        case RPLY_ARR:
                for(;;) {
                        type = *pos++;

                        if(STR_END & type)
                                break;

                        if(!(STR_NI & type))
                                printf("%d) ", index);

                        if(STR_NIL & type) {
                                printf("(nil)\n", index);
                        } else if(STR_NOT_QUOTED & type) {
                                len = *(uint32_t *)pos;
                                pos += sizeof(len);
                                print_str(pos, len, false);

                                pos += len;
                        } else {
                                len = *(uint32_t *)pos;
                                pos += sizeof(len);
                                print_str(pos, len, true);

                                pos += len;
                        }

                        index++;
                }

                if(1 == index)
                        printf("(empty list or set)\n");

                break;
        case RPLY_TYPE:
                print_str(type_str[reply->data[0]],
                          strlen(type_str[reply->data[0]]),
                          false);
                break;
        default:
                break;
        }
}
int main()
{
        size_t len;
        char name[NAME_LEN];
        reply_t *reply = malloc(sizeof(reply_t) + BUF_SIZE);

        if(-1 == connect_server(12345, "127.0.0.1")) {
                perror("connect");
                return -1;
        }

        if(read(fd, &id, sizeof id) <= 0)
                return -1;

        do {
                size_t start, end;

                printf("%s:%d> ","127.0.0.1",12345);
                fgets(buf, BUF_SIZE, stdin);
                len = strlen(buf);
                buf[len-1] = 0;

                start = 0;
                while((start != len) && isspace(buf[start]))
                        start++;

                end = start;
                while((end != len) && isalpha(buf[end]) &&
                      (end - start) < 16)
                        end++;

                if(end - start + 1 >= 16)
                        printf("invalid command!\n");

                memcpy(name, buf + start, end - start);
                name[end - start] = 0;

                send_request(name, buf+end, len - end - 1);

                read(fd, reply, sizeof(reply_t) + BUF_SIZE);

                display_reply(reply);


        }while(strcasecmp(buf, "quita"));

        close(fd);

        return 0;
}
