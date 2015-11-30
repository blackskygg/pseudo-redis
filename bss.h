/*simple binary-safe string implementation*/

#ifndef BSS_H_
#define BSS_H_

#include <stdlib.h>
#include <string.h>

struct bss{
        size_t len;
        size_t free;
        char str[];
}__attribute__((__packed__));

typedef struct bss bss_t;

#define bsslen(bss_ptr) (bss_ptr->len)
#define bsssize(bss_ptr) (bss_ptr->len + bss_ptr->free)

bss_t *bss_create(const char *data, size_t len);
void bss_destroy(bss_t *bss);
bss_t *bss_set(bss_t *dst, const char *str, size_t len);
#endif
