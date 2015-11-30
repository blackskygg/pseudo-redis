#include <stdlib.h>
#include <string.h>
#include "bss.h"

bss_t *bss_create(const char *data, size_t len)
{
        bss_t *bss_ptr = malloc(sizeof(bss_t) + len);

        if(NULL == bss_ptr)
                return NULL;

        memmove(bss_ptr->str, data, len);
        bss_ptr->len = len;
        bss_ptr->free = 0;

        return bss_ptr;
}

void bss_destroy(bss_t *bss_ptr)
{
        free(bss_ptr);
}

bss_t *bss_set(bss_t *dst, const char *str, size_t len)
{
        bss_t *result;

        result = dst;
        /*if no enough free space left, obtain a new one*/
        if(len > bsssize(dst)) {
                if(NULL == (result = bss_create(str, len))) {
                        return NULL;
                }

                bss_destroy(dst);
        } else {
                memcpy(result->str, str, len);
                /*notice the update order here*/
                result->free = bsssize(result) - len;
                result->len = len;
        }

        return result;
}
