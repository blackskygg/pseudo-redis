#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define BUF_SIZE 4096
#define NAME_LEN 16

/* raw request structure */
struct request {
        char command[16];
        uint32_t client_id; /* allocated on connection, by the server */
        uint32_t len;  /* length of data */
        uint8_t  data[]; /* the request string */
}__attribute__((__packed__));
typedef struct request request_t;

/* the t field in a struct tlv can be one of the followings */
#define RPLY_COMMAND 0
#define RPLY_OK 1
#define RPLY_NIL 2
#define RPLY_FAIL 3
#define RPLY_INT 4
#define RPLY_FLOAT 5
#define RPLY_STRING 6

/* reply structure */
struct reply {
        uint8_t reply_type;
        uint32_t len;
        uint8_t data[];
}__attribute__((__packed__));
typedef struct reply reply_t;



int id; /* client id */
int fd; /* used to communicate with the server */
char buf[BUF_SIZE];


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

void display_reply(reply_t *reply)
{
        switch(reply->reply_type) {
        case RPLY_OK:
                printf("OK\n");
                break;
        case RPLY_FAIL:
                printf("(error)%s\n", reply->data);
                break;
        case RPLY_STRING:
                printf("\"%s\"\n", reply->data);
                break;
        case RPLY_INT:
                printf("(interger)%d\n", be64toh(*(int64_t*)reply->data));
                break;
        case RPLY_NIL:
                printf("(nil)\n");
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

        if(read(fd, &id, sizeof id) >0)
                printf("connected! My id is %d.\n", id);
        else
                return -1;

        do {
                size_t start, end;

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
