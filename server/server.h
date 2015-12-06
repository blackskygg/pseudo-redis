#ifndef SERVER_H_
#define SERVER_H_

#include <stdlib.h>

#include "obj.h"
#include "bss.h"
#include "dict.h"
#include "list.h"

#ifdef SERVER_C_
#define EXTERN
#else
#define EXTERN extern
#endif

/* the network stuff */
#define SERVER_PORT 12345
#define MAX_QUEUED  5
#define MAX_EVENTS  64
#define IN_BUF_SIZE 4096

/* command limits */
#define MAX_ARGS  64


/* the t field in a struct tlv can be one of the followings */
#define RPLY_COMMAND 0
#define RPLY_OK 1
#define RPLY_NIL 2
#define RPLY_FAIL 3
#define RPLY_INT 4
#define RPLY_FLOAT 5
#define RPLY_STRING 6


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


/* the command structure
 */
typedef reply_t *(*command_func)(bss_t *args[], size_t num);

struct command {
        char *name;
        command_func func;
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


/* some exported functions */
reply_t* create_empty_reply(int type);
reply_t* create_str_reply(char *s, size_t len, int type);
reply_t* create_int_reply(int64_t n);
/* shortcuts */
#define NUM_ARGS "ERR wrong number of arguments"
#define INV_CMD "ERR unknown command"
#define INV_ARG "ERR Invalid Arguments"
#define MEM_OUT "Server Memory Out"
#define WRONG_TYPE "WRONG TYPE"                                 \
        "Operation against a key holding a wrong kind of value"
#define INV_INT "ERR value is not an integer or out of range"
#define NO_KEY "ERR no such key"

#define fail_reply(s) create_str_reply((s), strlen(s) + 1, RPLY_FAIL)
#define string_reply(s, len) create_str_reply((s), len, RPLY_STRING)
#define ok_reply() create_empty_reply(RPLY_OK)
#define nil_reply() create_empty_reply(RPLY_NIL)
#define true_reply() create_int_reply(1)
#define false_reply() create_int_reply(0)

/* global server data */
EXTERN dict_t *key_dict;  /* this dictionary is the MAIN dictionary holding
                           * all the keys and their data
                           */



#endif /* SERVER_H_ */
