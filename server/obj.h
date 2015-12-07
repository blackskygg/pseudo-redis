#ifndef OBJ_H_
#define OBJ_H_

#include <stdint.h>

struct obj_op;
struct obj;

#define STRING 0
#define INTEGER 1
#define SET 2
#define HASH 3
#define LIST 4
#define NONE 5

/* this is the base type for almost everything */
struct obj {
        struct obj_op *op; /* points to the type-specific operations */
        uint8_t type;
        void *val;
};
typedef struct obj obj_t;

/* by convention, the create() function will accept obj.val as its argument */
struct obj_op {
        obj_t *(*create)(const void *val);
        void (*destroy)(obj_t *obj);
        obj_t *(*cpy)(obj_t *a, const obj_t *b);
        int (*cmp)(const obj_t *a, const obj_t *b);
};

#endif
