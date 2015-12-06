#ifndef BSS_H_
#define BSS_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <limits.h>

#include "obj.h"
#include "err.h"


/* limits */
#define BSS_INT_MAX LLONG_MAX
#define BSS_INT_MIN LLONG_MIN
#define BSS_MIN_LEN 22  /* this length can hold a bss_int_t */
#define BSS_INT_FMT "%lld"

/* type definitions */
struct bss{
        size_t len;
        size_t free;
        char str[];
};

typedef struct bss bss_t;
typedef long long  bss_int_t;


/* some shorthands */
#define bsslen(bss_ptr) (bss_ptr->len)
#define bsssize(bss_ptr) (bss_ptr->len + bss_ptr->free)
#define bss_incr(bss_ptr, num) bss_add_sub(bss_ptr, num, 1)
#define bss_decr(bss_ptr, num) bss_add_sub(bss_ptr, num, -1)


/* bss "low-level" APIS */
bss_t *bss_create(const char *data, size_t len);
bss_t *bss_create_empty(size_t len);
void bss_destroy(bss_t *bss);
bss_t *bss_set(bss_t *dst, const char *str, size_t len);
int bss_cmp(const bss_t *a, const bss_t *b);
int bss2int(const bss_t *bss, bss_int_t *result);
int bss_add_sub(const bss_t *bss, bss_int_t num, int8_t factor);
size_t bss_count_bit(const bss_t *bss);
bss_t *bss_append(bss_t *bss, char *data, size_t len);
bss_t *bss_setbit(bss_t *bss, size_t offset, uint val);
int bss_getbit(const bss_t *bss, size_t offset);


/* functions on the obj layer */
obj_t *bss_create_obj(const void *data);
void bss_destroy_obj(obj_t *obj);
/* when using cpy the caller has to be sure
 * that a has enough room to accomadate b
 */
obj_t *bss_cpy_obj(obj_t *a, const obj_t *b);
int bss_cmp_obj(const obj_t *a, const obj_t *b);

#endif
