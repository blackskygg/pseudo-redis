#include "bss.h"

static struct obj_op bss_obj_op = {.create= bss_create_obj,
                                    .destroy = bss_destroy_obj,
                                    .cpy = bss_cpy_obj,
                                    .cmp = bss_cmp_obj};


bss_t *bss_create(const char *data, size_t len)
{
        bss_t *bss_ptr = malloc(sizeof(bss_t) + len);

        if(NULL == bss_ptr)
                return NULL;

        memcpy(bss_ptr->str, data, len);
        bss_ptr->len = len;
        bss_ptr->free = 0;

        return bss_ptr;
}

obj_t *bss_create_obj(const void *data)
{
        obj_t *obj_ptr;
        bss_t *bss_ptr;
        const bss_t *bss_ptr_old;

        bss_ptr_old = data;
        if(NULL == (obj_ptr = malloc(sizeof(obj_t))))
                return NULL;

        if(NULL == (bss_ptr = bss_create(bss_ptr_old->str, bss_ptr_old->len))) {
                free(obj_ptr);
                return NULL;
        }

        obj_ptr->op = &bss_obj_op;
        obj_ptr->type = STRING;
        obj_ptr->val = bss_ptr;

        return obj_ptr;

}

void bss_destroy(bss_t *bss_ptr)
{
        free(bss_ptr);
}

void bss_destroy_obj(obj_t *obj)
{
        bss_destroy(obj->val);
        free(obj);
}

/* assign a new value to dst,
   memory will ba automatically taken care of,
   but the returned pointer might not be the same as dst. */
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

obj_t *bss_cpy_obj(obj_t *a, const obj_t *b)
{
        bss_t *bss;
        a->type = b->type;
        a->op = b->op;

        bss = bss_set(a->val, ((bss_t *)b->val)->str, ((bss_t *)b->val)->len);
        a->val = bss;

        return a;
}

int bss_cmp(const bss_t *a, const bss_t *b)
{
        size_t len = a->len < b->len ? a->len : b->len;
        return memcmp(a->str, b->str, len);
}

int bss_cmp_obj(const obj_t *a, const obj_t *b)
{
        return bss_cmp(a->val, b->val);
}
