#ifndef OBJ_H_
#define OBJ_H_

struct obj;
typedef struct obj obj_t;

enum obj_type {
        STRING,
        INTEGER,
        SET,
        HASH,
        LIST,
};

/* by default, the create() function will accept obj.val as the lone argument */
struct obj_op {
        obj_t *(*create)(const void *val);
        void (*destroy)(obj_t *obj);
        obj_t *(*cpy)(obj_t *a, const obj_t *b);
        int (*cmp)(const obj_t *a, const obj_t *b);
};

struct obj {
        struct obj_op *op;
        enum obj_type type;
        void *val;
};

#endif
