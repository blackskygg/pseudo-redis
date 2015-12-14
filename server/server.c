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
#define CMD_ENT(name) {#name, name##_command, NOT_BLOCKED}
#define CMD_BENT(name) {#name, name##_command, BLOCKED}
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
                                 CMD_ENT(hkeys), CMD_ENT(hvals),
                                 CMD_BENT(blpop), CMD_ENT(lrange),
                                 CMD_BENT(brpop), CMD_ENT(lrem),
                                 CMD_BENT(brpoplpush), CMD_ENT(lset),
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
                                 CMD_ENT(smembers), CMD_ENT(setnx),
                                 CMD_ENT(incrbyfloat)};


/* ======server local variables========= */
static int _listening_fd;
static int _efd;        /* epoll fd */
static request_t *_curr_request;
static action_t _action_head;        /* pending list */
static bss_t *_time_bss;      /* used to stored the elapsed time string */

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

/* ======== below are some shorthands for constructing replies ======== */
int create_empty_reply(int type)
{
        _curr_reply->reply_type = type & TYPE_MASK;
        _curr_reply->len = 0;

        return 0;
}

int create_str_reply(char *s, size_t len, int type)
{
        /* is it too long? */
        if(len > MAX_RPLY_SIZE)
                return E_TOO_LONG;

        /* is it an empty string? */
        if(0 == len) {
                _curr_reply->reply_type = type & TYPE_MASK;
                _curr_reply->len = 0;
                return 0;
        }

        _curr_reply->reply_type = type & TYPE_MASK;
        _curr_reply->len = len;
        memcpy(_curr_reply->data, s, len);

        return 0;
}

int create_int_reply(int64_t n)
{
        _curr_reply->reply_type = RPLY_INT;
        _curr_reply->len = sizeof(int64_t);
        n = htobe64(n);
        memcpy(_curr_reply->data, &n, sizeof(int64_t));

        return 0;
}

int create_type_reply(uint8_t n)
{
        _curr_reply->reply_type = RPLY_TYPE;
        _curr_reply->len = sizeof(uint8_t);
        _curr_reply->data[0] = n;

        return 0;
}

void reset_reply_arr()
{
        _rply_arr_len = 0;
}

/* fill in the reply array
 * return E_TOO_LONG if the array is full
 */
int addto_reply_arr(bss_t *bss_ptr, uint8_t type)
{
        if(_rply_arr_len == MAX_ARGS)
                return E_TOO_LONG;

        _reply_arr[_rply_arr_len] = bss_ptr;
        _arr_ele_type[_rply_arr_len] = type;
        _rply_arr_len++;

        return 0;
}

int create_arr_reply(int arr_type)
{
        uint8_t *pos = _curr_reply->data;
        uint8_t type;
        uint32_t total_len = 0;
        uint32_t len = 0;

        for(int i = 0; i < _rply_arr_len; ++i) {
                type = _arr_ele_type[i];

                if(STR_NIL & type) {
                        /* check if it will exceed the limit */
                        if((total_len + 2) > MAX_RPLY_SIZE)
                                return E_TOO_LONG;

                        *pos++ = type & TYPE_MASK;
                        total_len++;
                } else {
                        len = _reply_arr[i]->len;
                        /* check if it will exceed the limit */
                        if((total_len + len + SIZE_TL + 1) > MAX_RPLY_SIZE)
                                return E_TOO_LONG;

                        *pos++ = type & TYPE_MASK;
                        memcpy(pos, &len, sizeof(len));
                        pos += sizeof(len);
                        memcpy(pos, _reply_arr[i]->str, len);
                        pos += len;

                        total_len += len + SIZE_TL;

                        /* need free? */
                        if(type & NEED_FREE)
                                bss_destroy(_reply_arr[i]);
                }
        }

        /* add the ending STR_END */
        *pos++ = STR_END;
        total_len++;
        _curr_reply->reply_type = arr_type;
        _curr_reply->len = total_len;
        if(RPLY_ARR == type)
                _curr_reply->arr_num = _rply_arr_len;
        else
                _curr_reply->arr_num = _rply_arr_len - 1;

        return 0;
}

/* ======== above are some shorthands for constructing replies ======== */

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

                        if(rptr == end) {
                                /* error occurred, clean and return */
                                for(size_t i = 0; i < *ret_size; ++i)
                                        bss_destroy(args[i]);
                                return E_INV_ARGS;
                        } else {
                                args[(*ret_size)++] = bss_create(curr,
                                                                 rptr - curr);
                        }

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

/* duplicate the current request for future use */
request_t *dup__curr_request()
{
        request_t *request;

        if(NULL == (request = malloc(sizeof(request_t) + _curr_request->len)))
                return NULL;

        memcpy(request, _curr_request, sizeof(request_t) + _curr_request->len);
        return request;
}

/* add a request to a pending list */
int addto_pending(request_t *request, int timeout)
{
        action_t *action = malloc(sizeof(action_t));

        /* fill in informations */
        action->request = dup__curr_request(request);
        gettimeofday(&action->tv, NULL);
        action->timeout = timeout;

        /* insert it to the head of the list */
        _action_head.next->prev = action;
        action->next = _action_head.next;
        action->prev = &_action_head;
        _action_head.next = action;

        return 0;
}

/* process the raw request
 * if this returns E_BLOCK, no reply will be sent
 * else, at least an error reply will be sent
 */
int process_request(request_t *request, bool is_new)
{
        bss_t *args[MAX_ARGS];
        uint32_t args_len;
        int ret_val;
        bss_int_t timeout;


        printf("%s\n", request->command);
        for(int i = 0 ; i < NUM_COMMANDS; ++i) {
                if(!strcasecmp(request->command, commad_tbl[i].name)) {
                        ret_val = parse_args(request->data, request->len,
                                   args, &args_len);
                        if(0 != ret_val) {
                                fail_reply(INV_ARG);
                        }

                        /* this call itself would decide which args to use
                         * -and which args to free, and modify _curr_reply
                         */
                        if(BLOCKED == commad_tbl[i].type) {
                                if(bss2int(args[args_len - 1], &timeout)
                                   || timeout < 0
                                   || timeout > INT32_MAX) {
                                        for(size_t i = 0; i < args_len; ++i)
                                                bss_destroy(args[i]);
                                        if(timeout < 0)
                                                fail_reply(NEG_TM);
                                        else
                                                fail_reply(INV_TM);
                                }
                                ret_val = commad_tbl[i].func(args, args_len - 1);
                        } else {
                                ret_val = commad_tbl[i].func(args, args_len);
                        }

                        /* reply too long? blocked? */
                        if(E_TOO_LONG == ret_val) {
                                if(BLOCKED == commad_tbl[i].type)
                                        bss_destroy(args[args_len - 1]);
                                fail_reply(TOO_LONG);
                        } else if(E_BLOCKED == ret_val) {
                                if(is_new)
                                        addto_pending(request, (int)timeout);
                                bss_destroy(args[args_len - 1]);
                                return E_BLOCKED;
                        } else {
                                /* here we've made an assumption,
                                 * all the blocking things return an array
                                 */
                                if(BLOCKED == commad_tbl[i].type) {
                                        bss_destroy(args[args_len - 1]);
                                        arr_reply();
                                }
                                return ret_val;
                        }
                }
        }

        fail_reply(INV_CMD);
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

        _curr_request = malloc(sizeof(struct request) + IN_BUF_SIZE);
        if(NULL == _curr_request)
                return E_MEM_OUT;

        _curr_reply = malloc(sizeof(reply_t) + MAX_RPLY_SIZE);
        if(NULL == _curr_reply)
                return E_MEM_OUT;

        _action_head.next = _action_head.prev = &_action_head;

        if(NULL == (_time_bss = bss_create_empty(TIME_LEN)))
                return E_MEM_OUT;

        /* network initialization */
        _listening_fd = create_server_socket(SERVER_PORT, MAX_QUEUED);
        if(E_NET_ERR == _listening_fd)
                return E_NET_ERR;

        if(-1 == (_efd = epoll_create1(0)))
                return E_UNKNOWN;

        return 0;

}

/* release all the resources allocated by server */
void server_destroy()
{
        dict_destroy(key_dict);
        free(_curr_request);
        free(_curr_reply);
        bss_destroy(_time_bss);
        close(_listening_fd);
}

void send_reply(int fd)
{
        int ret;
        printf("sending to %d\n", fd);
        ret = write(fd, _curr_reply, sizeof(reply_t) + _curr_reply->len);
}

/* process the incoming data */
int process_in_data(struct epoll_event *ev)
{
        int n_read;

        /* read all the possible data */
        while((n_read = read(ev->data.fd, (void *)_curr_request,
                             sizeof(request_t) + IN_BUF_SIZE)) > 0) {
                /* if command blocks, don't send anything */
                _curr_request->client_id = ev->data.fd;
                if(E_BLOCKED !=  process_request(_curr_request, true))
                        send_reply(ev->data.fd);
        }

        /* diconnected */
        if(0 == n_read) {
                printf("%d is disconnected\n", ev->data.fd);
                epoll_ctl(_efd, EPOLL_CTL_DEL, ev->data.fd, NULL);
                close(ev->data.fd);
        }

        return 0;
}

/* accept pending connection requests */
void accept_connection()
{
        int tfd;
        int ret;
        struct sockaddr_in addr;
        struct epoll_event ev;
        socklen_t len;

        for(;;) {
                tfd = accept(_listening_fd, (struct sockaddr*)&addr, &len);
                if(tfd == -1) {
                        break;
                } else {
                        make_socket_non_blocking(tfd);

                        ev.events = EPOLLIN | EPOLLET;
                        ev.data.fd = tfd;
                        epoll_ctl(_efd, EPOLL_CTL_ADD, tfd, &ev);

                        /* tell the client who he is
                         * we use the fd number as a the client id
                         */
                        do {
                               ret = write(tfd, &tfd, sizeof tfd);
                        }while(EAGAIN == ret || EWOULDBLOCK == ret);

                        printf("connection established with %d\n", tfd);
                }

        }

}

/* deal with the hanging jobs
 * called by main_loop() on every hear-beat
 */
void do_hanging_jobs()
{
        printf("...\n");

        action_t *curr, *next;
        char strtm[TIME_LEN];
        size_t tm_len;
        float interval;
        int id;
        bool done;
        struct timeval now;

        curr = _action_head.next;
        while(curr != &_action_head) {
                next = curr->next;

                gettimeofday(&now, NULL);
                interval = now.tv_sec - curr->tv.tv_sec;
                interval += (now.tv_usec -  curr->tv.tv_usec) * 1E-6;

                done = 0;
                printf("%f\n", interval);
                printf("%d\n", interval - curr->timeout);
                if(0 != curr->timeout && interval - curr->timeout >= 0) {
                        /* set up the reply string to say sorry */
                        reset_reply_arr();
                        addto_reply_arr(NULL, STR_NIL | STR_NI);

                        done = 1;
                } else if(E_BLOCKED != process_request(curr->request, false)) {
                        done = 1;
                }

                /* this action is done, reply and free it */
                if(done) {
                        tm_len = sprintf(strtm, "(%.2f)", interval);
                        bss_set(_time_bss, strtm, tm_len);
                        addto_reply_arr(_time_bss, STR_NOT_QUOTED | STR_NI);

                        /* we are not going to wait any longer
                         * remove this action
                         */
                        id = curr->request->client_id;

                        curr->prev->next = curr->next;
                        curr->next->prev = curr->prev;
                        free(curr->request);
                        free(curr);

                        create_arr_reply(RPLY_ARR);
                        send_reply(id);
                }

                curr = next;
        }
}

/* the real event loop */
void main_loop()
{
        struct epoll_event ev;
        struct epoll_event events[MAX_EVENTS];
        int nfds;

        ev.events = EPOLLIN | EPOLLET;
        ev.data.fd = _listening_fd;
        epoll_ctl(_efd, EPOLL_CTL_ADD, _listening_fd, &ev);

        for(;;) {
                nfds = epoll_wait(_efd, events, MAX_EVENTS, 1000);

                for (int i = 0; i < nfds; ++i) {
                        if(events[i].data.fd == _listening_fd) {
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
