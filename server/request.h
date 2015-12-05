#ifndef SERVER_H_
#define SERVER_H_

#ifdef SERVER_C_
#define EXTERN
#else
#define EXTERN extern
#endif

/* data structures */
struct client {
};
typedef struct client client_t;

typedef int (*command_func)(client_t *);

struct command {
        char *name;
        command_func *func;
};

typedef struct command command_t;
typedef struct request request_t;

/* the command table */
command

#endif /* SERVER_H_ */
