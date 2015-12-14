
#ifndef SERVER_H_
#define SERVER_H_

#include <stdlib.h>
#include <sys/time.h>

#include "obj.h"
#include "bss.h"
#include "dict.h"
#include "list.h"

#ifdef SERVER_C_
#define EXTERN
#else
#define EXTERN extern
#endif

/* the listening port */
#define SERVER_PORT 12345

/* some limits */
#define MAX_QUEUED  5
#define MAX_EVENTS  64
#define IN_BUF_SIZE 1024*1024
#define MAX_RPLY_SIZE 1024*1024
#define MAX_RPLY_ARR 1024
#define MAX_ARGS  1024
#define NEW_DICT_POW 7
#define TIME_LEN 32

/* the reply types
 * NOTE : the arr protocal is simple : (type|len|val) denotes an element
 * the higher 4 bits of 'type' is for internal use, and won't be transferred
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
/* the higher 4 bits */
#define NEED_FREE 0x10
#define NO_FREE 0x00
#define TYPE_MASK 0x0f


/* the internal representation of a client */
struct client {
};
typedef struct client client_t;

/* reply structure */
struct reply {
        uint8_t reply_type;
        uint32_t len;
        uint8_t data[];
}__attribute__((__packed__));
typedef struct reply reply_t;
#define SIZE_TL (sizeof(uint32_t) + sizeof(uint8_t))


/* the command structure
 */
#define NOT_BLOCKED 0
#define BLOCKED 1
typedef int (*command_func)(bss_t *args[], size_t num);

struct command {
        char *name;
        command_func func;
        uint8_t type;
};
typedef struct command command_t;


/* raw request structure */
struct request {
        char command[16];
        uint32_t client_id; /* allocated on connection, by the server */
        uint32_t len;  /* length of data */
        uint8_t  data[]; /* the request string */
}__attribute__((__packed__));
typedef struct request request_t;


/* pending request list entry structure */
struct action {
        struct action *next;
        struct action *prev;
        request_t *request;
        struct timeval tv;
        int timeout;
};
typedef struct action action_t;


/* some exported functions */
int create_empty_reply(int type);
int create_str_reply(char *s, size_t len, int type);
int create_int_reply(int64_t n);
int create_arr_reply();
int create_type_reply(uint8_t n);
int addto_reply_arr(bss_t *bss, uint8_t type);
void reset_reply_arr();
/* shortcuts */
#define NUM_ARGS "ERR wrong number of arguments"
#define INV_CMD "ERR unknown command"
#define INV_ARG "ERR Invalid Arguments"
#define INV_INT "ERR value is not an integer or out of range"
#define INV_FLT "ERR Value is not a valid float"
#define INV_TM "ERR timeout is not an integer or out of range"
#define NEG_TM "ERR timeout is negative"
#define INV_OFFSET "ERR bit offset is not an integer or out of range"
#define NO_KEY "ERR no such key"
#define INV_SYNX "ERR syntax error"
#define TOO_LONG "ERR request or reply too long"
#define OUT_RANGE "ERR index out of range"
#define WRONG_TYPE "WRONG TYPE "                                \
        "Operation against a key holding a wrong kind of value"
#define MEM_OUT "Server Memory Out"


#define fail_reply(s) return create_str_reply((s), strlen(s), RPLY_FAIL)
#define string_reply(s, len) return create_str_reply((s), len, \
                                                     RPLY_STRING)
#define ok_reply() return create_empty_reply(RPLY_OK)
#define nil_reply() return create_empty_reply(RPLY_NIL)
#define true_reply() return create_int_reply(1)
#define false_reply() return create_int_reply(0)
#define int_reply(n) return create_int_reply(n)
#define arr_reply() return create_arr_reply()
#define type_reply(n) return create_type_reply(n)

/* global server data
 * but modifying the ones with leading underscore is NOT recommended
 * UNLESS you know exactely what you're doing
 */
EXTERN dict_t *key_dict;  /* this dictionary is the MAIN dictionary holding
                           * all the keys and their data
                           */
EXTERN reply_t *_curr_reply;  /* since we are single-threaded,
                              * we can share one reply structure
                              */
EXTERN bss_t *_reply_arr[MAX_ARGS]; /* when replying an array, the only thing to
                                     * do is just to fill in this array, and
                                     * a call to arr_reply() will get everything
                                     * done
                                     */
EXTERN int _arr_ele_type[MAX_ARGS]; /* quoted or unquoted */
EXTERN uint8_t _rply_arr_len;  /* used to keep track of the length of reply_arr */
EXTERN bss_t *reserved_bss_ptr;


#endif /* SERVER_H_ */
