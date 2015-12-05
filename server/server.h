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
#define RPLY_FAIL 2
#define RPLY_INT 3
#define RPLY_FLOAT 4
#define RPLY_STRING 5


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

/* global server data */
EXTERN dict_t *key_dict;  /* this dictionary is the MAIN dictionary holding
                           * all the keys and their data
                           */


#endif /* SERVER_H_ */
