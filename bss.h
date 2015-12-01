/*simple binary-safe string implementation*/

#ifndef BSS_H_
#define BSS_H_

#include <stdlib.h>
#include <string.h>
#include "obj.h"

struct bss{
        size_t len;
        size_t free;
        char str[];
}__attribute__((__packed__));

typedef struct bss bss_t;

#define bsslen(bss_ptr) (bss_ptr->len)
#define bsssize(bss_ptr) (bss_ptr->len + bss_ptr->free)

/* functions dealing with bss structure */
bss_t *bss_create(const char *data, size_t len);
void bss_destroy(bss_t *bss);
bss_t *bss_set(bss_t *dst, const char *str, size_t len);
int bss_cmp(const bss_t *a, const bss_t *b);

/* functions on the obj-layer */
obj_t *bss_create_obj(const void *data);
void bss_destroy_obj(obj_t *obj);
/* when using cpy the caller has to be sure
   that a has enough room to accomadate b */
obj_t *bss_cpy_obj(obj_t *a, const obj_t *b);
int bss_cmp_obj(const obj_t *a, const obj_t *b);

#endif
