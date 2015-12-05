#define SERVER_C_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "server.h"
#include "commands.h"

#define INIT_DICT_SIZE 16  /* the initial key dict size */

/* the command table */
#define CMD_ENT(name) {#name, name##_command}
#define NUM_COMMANDS 63

EXTERN command_t commad_tbl[] = {CMD_ENT(del), CMD_ENT(exists),
                                 CMD_ENT(randomkey), CMD_ENT(rename),
                                 CMD_ENT(keys), CMD_ENT(type),
                                 CMD_ENT(append), CMD_ENT(getbit),
                                 CMD_ENT(setbit), CMD_ENT(mget),
                                 CMD_ENT(bitcount), CMD_ENT(setrange),
                                 CMD_ENT(getrange), CMD_ENT(incr),
                                 CMD_ENT(decr), CMD_ENT(incrby),
                                 CMD_ENT(decrby), CMD_ENT(msetnx),
                                 CMD_ENT(get), CMD_ENT(set),
                                 CMD_ENT(strlen), CMD_ENT(hdel),
                                 CMD_ENT(hlen), CMD_ENT(hexists),
                                 CMD_ENT(hmget), CMD_ENT(hget),
                                 CMD_ENT(hmset), CMD_ENT(hgetall),
                                 CMD_ENT(hincrby), CMD_ENT(hset),
                                 CMD_ENT(hincrbyfloat), CMD_ENT(hsetnx),
                                 CMD_ENT(hkeys), CMD_ENT(havls),
                                 CMD_ENT(blpop), CMD_ENT(lrange),
                                 CMD_ENT(brpop), CMD_ENT(lrem),
                                 CMD_ENT(brpoplpush), CMD_ENT(lset),
                                 CMD_ENT(lindex), CMD_ENT(ltrim),
                                 CMD_ENT(linsert), CMD_ENT(rpop),
                                 CMD_ENT(llen), CMD_ENT(rpoplpush),
                                 CMD_ENT(lpop), CMD_ENT(rpush),
                                 CMD_ENT(lpush), CMD_ENT(rpushx),
                                 CMD_ENT(lpushx), CMD_ENT(sadd),
                                 CMD_ENT(smove), CMD_ENT(scard),
                                 CMD_ENT(spop), CMD_ENT(sdiff),
                                 CMD_ENT(srandmember), CMD_ENT(srem),
                                 CMD_ENT(sinter), CMD_ENT(sscan),
                                 CMD_ENT(sunion), CMD_ENT(sismember),
                                 CMD_ENT(smembers) };


/* ======server local variables========= */
static int listening_fd;
static int efd; /* epoll fd */
static request_t *curr_request;



/* we use non-bloking io with edge_trigger */
int make_socket_non_blocking(int sfd)
{
        int flags, s;

        flags = fcntl (sfd, F_GETFL, 0);
        if(flags == -1) {
                perror ("fcntl");
                return -1;
        }

        flags |= O_NONBLOCK;
        s = fcntl (sfd, F_SETFL, flags);
        if(s == -1) {
                perror ("fcntl");
                return -1;
        }

        return 0;
}

/* create socket for listening incoming connections*/
int create_server_socket(unsigned short port, int nqueued)
{
        int fd;
        struct sockaddr_in addr;

        fd = socket(AF_INET, SOCK_STREAM, 0);
        if(fd == -1) {
                perror(NULL);
                return E_NET_ERR;
        }

        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        addr.sin_port = htons(port);

        if(0 != bind(fd, (struct sockaddr *)&addr, sizeof(addr))) {
                perror(NULL);
                return E_NET_ERR;
        }

        make_socket_non_blocking(fd);

        if(listen(fd, nqueued) == -1) {
                perror(NULL);
                return E_NET_ERR;
        }

        return fd;
}

/* parse the data field in request_t, convert it into bss_ts */
int parse_args(uint8_t *data, uint32_t len, bss_t *args[], uint32_t *ret_size)
{
        uint8_t *curr, *end;
        uint8_t buffer[IN_BUF_SIZE];


        *ret_size = 0;
        curr = data;
        end = data + len;
        while(curr < end) {
                if(isspace(*curr)) {
                        curr++;
                        continue;
                }

                if(*curr == '\"') {
                        uint8_t *rptr = ++curr;
                        while(rptr != end && *rptr != '\"')
                                rptr++;

                        if(rptr == end)
                                return E_INV_ARGS;
                        else
                                args[(*ret_size)++] = bss_create(curr,
                                                                 rptr - curr);
                        curr = rptr + 1;
                } else {
                        uint8_t *rptr = curr + 1;
                        while(rptr != end && !isspace(*rptr))
                                rptr++;

                        args[(*ret_size)++] = bss_create(curr, rptr - curr);

                        curr = rptr;

                }

        }

        return 0;
}

/* process the raw request */
int process_request(request_t *request, reply_t **reply_ptr)
{
        bss_t *args[MAX_ARGS];
        uint32_t args_len;
        int ret_val;

        printf("%s\n", request->command);
        for(int i = 0 ; i < NUM_COMMANDS; ++i) {
                if(!strcasecmp(request->command, commad_tbl[i].name)) {
                        ret_val = parse_args(request->data, request->len,
                                   args, &args_len);
                        if(0 != ret_val)
                                return ret_val;

                        /* this call itself would decide which args to use
                         * -and which args to free
                         */
                        *reply_ptr = commad_tbl[i].func(args, args_len);

                        return 0;
                }
        }

        return E_INV_CMD;
}


/* init server
 * prepare for the db and get ready for connetions
 */
int server_init()
{
        /* data intialization */
        key_dict = dict_create(INIT_DICT_SIZE);
        if(NULL == key_dict)
                return E_MEM_OUT;

        curr_request = malloc(sizeof(struct request) + IN_BUF_SIZE);
        if(NULL == curr_request)
                return E_MEM_OUT;

        /* network initialization */
        listening_fd = create_server_socket(SERVER_PORT, MAX_QUEUED);
        if(E_NET_ERR == listening_fd)
                return E_NET_ERR;

        if(-1 == (efd = epoll_create1(0)))
                return E_UNKNOWN;

        return 0;

}

/* release all the resources allocated by server */
void server_destroy()
{
        dict_destroy(key_dict);
        free(curr_request);
        close(listening_fd);
}

void send_reply(reply_t *reply, int fd)
{
        write(fd, reply, sizeof(reply_t) + reply->len);
}

void send_failure(int fd, char *s)
{
        reply_t *reply = malloc(sizeof(reply_t));

        if(reply) {
                reply->reply_type = RPLY_FAIL;
                reply->len = 0;
                send_reply(reply, fd);

                free(reply);
        }
}

/* process the incoming data */
int process_in_data(struct epoll_event *ev)
{
        int n_read;
        reply_t *rply_ptr;

        while((n_read = read(ev->data.fd, (void *)curr_request,
                             sizeof(request_t) + IN_BUF_SIZE)) > 0) {
                      /* TODO : remember to free *reply_ptr */
                if(process_request(curr_request, &rply_ptr) == 0) {
                        if(rply_ptr)
                                send_reply(rply_ptr, ev->data.fd);
                        else
                                send_failure(ev->data.fd, "invalid arguments");
                } else {
                        send_failure(ev->data.fd, "invalid command");
                }
        }

        if(0 == n_read) {
                printf("%d is disconnected\n", ev->data.fd);
                epoll_ctl(efd, EPOLL_CTL_DEL, ev->data.fd, NULL);
                close(ev->data.fd);
        }

        return 0;
}

/* accept pending connection requests */
void accept_connection()
{
        int tfd;
        struct sockaddr_in addr;
        struct epoll_event ev;
        socklen_t len;

        for(;;) {
                tfd = accept(listening_fd, (struct sockaddr*)&addr, &len);
                if(tfd == -1) {
                        break;
                } else {
                        make_socket_non_blocking(tfd);

                        ev.events = EPOLLIN | EPOLLET;
                        ev.data.fd = tfd;
                        epoll_ctl(efd, EPOLL_CTL_ADD, tfd, &ev);

                        /* tell the client who he is
                         * we use the fd number as a the client id
                         */
                        write(tfd, &tfd, sizeof tfd);

                        printf("connection established with %d\n", tfd);
                }

        }

}

/* deal with the hanging jobs
 * called by main_loop() every on evert hear-beat
 */
void do_hanging_jobs()
{
        printf("...\n");
}

/* the real event loop */
void main_loop()
{
        struct epoll_event ev;
        struct epoll_event events[MAX_EVENTS];
        int nfds;

        ev.events = EPOLLIN | EPOLLET;
        ev.data.fd = listening_fd;
        epoll_ctl(efd, EPOLL_CTL_ADD, listening_fd, &ev);

        for(;;) {
                nfds = epoll_wait(efd, events, MAX_EVENTS, 1000);

                for (int i = 0; i < nfds; ++i) {
                        if(events[i].data.fd == listening_fd) {
                                accept_connection();
                        } else {
                                printf("received from %d\n", events[i].data.fd);
                                process_in_data(&events[i]);
                        }

                }
                do_hanging_jobs();
        }

}

int main(int argc, char **argv)
{
        if(0 != server_init())
                return -1;
        main_loop();
        return 0;
}