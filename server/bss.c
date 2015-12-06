/* bss.c : simple resizeable binary-safe string implementation */

#include <assert.h>
#include "bss.h"

static struct obj_op bss_obj_op = {.create= bss_create_obj,
                                    .destroy = bss_destroy_obj,
                                    .cpy = bss_cpy_obj,
                                    .cmp = bss_cmp_obj};

/* for faster bit-counting */
static uint8_t bit_table[] = {0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4,
                              1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
                              1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
                              2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
                              1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
                              2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
                              2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
                              3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
                              1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
                              2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
                              2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
                              3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
                              2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
                              3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
                              3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
                              4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8};

/* create a new bss
 * will always make enough room to accomadate an integer
 * the new length will always be aleast as large as max(BSS_MIN_LEN + 1, len +1)
 */
bss_t *bss_create(const char *data, size_t len)
{
        bss_t *bss_ptr;
        size_t real_len;

        real_len = len > BSS_MIN_LEN ? len : BSS_MIN_LEN;
        bss_ptr = malloc(sizeof(bss_t) + real_len + 1);
        if(NULL == bss_ptr)
                return NULL;

        memcpy(bss_ptr->str, data, len);
        bss_ptr->str[len] = 0;  /* bss->str is always ended with 0 */
        bss_ptr->len = len;
        bss_ptr->free = real_len - len;

        return bss_ptr;
}

/* create an all-0 bss of size len */
bss_t *bss_create_empty(size_t len)
{
        bss_t *bss_ptr;

        len = len > BSS_MIN_LEN ? len : BSS_MIN_LEN;

        bss_ptr = calloc(sizeof(bss_t) + len + 1, 1);
        if(NULL == bss_ptr)
                return NULL;

        bss_ptr->len = 0;
        bss_ptr->free = len;

        return bss_ptr;

}

obj_t *bss_create_obj(const void *data)
{
        obj_t *obj_ptr;

        if(NULL == (obj_ptr = malloc(sizeof(obj_t))))
                return NULL;

        obj_ptr->op = &bss_obj_op;
        obj_ptr->type = STRING;
        obj_ptr->val = (void *)data;

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
 * memory will be automatically taken care of,
 * but the returned pointer might not be the same as dst.
 */
bss_t *bss_set(bss_t *dst, const char *str, size_t len)
{
        bss_t *result;
        size_t new_len;

        result = dst;
        /* if no enough free space left, obtain a new one
         * we'll first try doubling the space
         * if it fails, we'll fall back to len
         * if it, again, fails, the whole call fails
         */
        if(len > bsssize(dst)) {
                new_len = bsssize(dst) * 2 > len ? bsssize(dst) * 2 : len;

                result = malloc(sizeof(bss_t) + new_len + 2);
                if(NULL == result) {
                        new_len = len;

                        result = malloc(sizeof(bss_t) + new_len);
                        if(NULL == result)
                                return NULL;
                }

                memcpy(result->str, str, len);
                result->str[len] = 0;
                result->free = new_len - len;
                result->len = len;

                bss_destroy(dst);
        } else {
                memcpy(result->str, str, len);
                result->str[len] = 0;
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

/* convert bss to bss_int_t
 * return non-zero if it fails
 */
int bss2int(const bss_t *bss, bss_int_t *result)
{
        char *end_ptr;

        if(0 == bss->len) {
                *result = 0;
                return 0;
        }

        errno = 0;
        *result = strtoll(bss->str, &end_ptr, 10);

        if((ERANGE == errno) || ( end_ptr != (bss->str + bss->len) ) ) {
                perror(" ");
                return E_INV_INT;
        }

        return 0;
}

/* add a number to or subtract a number from an bss if possible.
 * will return E_INV_INT if bss is not an integer
 * --or if the operation causes an overflow.
 */
int bss_add_sub(const bss_t *bss, bss_int_t num, int8_t factor)
{
        bss_int_t origin;
        char result[BSS_MIN_LEN];
        size_t len;

        if(0 != bss2int(bss, &origin))
                return E_INV_INT;

        if(factor == -1) {
                if(BSS_INT_MIN + num > origin)
                        return E_INV_INT;
        } else {
                if(BSS_INT_MIN - num < num)
                        return E_INV_INT;
        }

        len = sprintf(result, BSS_INT_FMT, factor * num + origin);

        /* we know that bss will not be changed */
        assert( bss == bss_set((bss_t *)bss, result, len) );

        return 0;
}

/* count the number of bits set in bss->str */
size_t bss_count_bit(const bss_t *bss)
{
        register size_t i;
        size_t count;

        for(count = i = 0; i < bss->len; ++i)
                count += bit_table[bss->str[i]];

        return count;
}

/* append a string after bss, and return the result bss_t pointer */
bss_t *bss_append(bss_t *bss, char *data, size_t len)
{
        bss_t *result;
        char *str;

        result = bss;
        if(bss->free >= len) {
                memcpy(bss->str + bss->len, data, len);
                bss->free -= len;
                bss->len += len;
                bss->str[bss->len] = 0;
        } else {
                str = malloc(bss->len + len);
                if(NULL == str)
                        return NULL;

                memcpy(str, bss->str, bss->len);
                memcpy(str + bss->len, data, len);
                result = bss_set(bss, str, len + bss->len);
                free(str);
        }

        return result;
}

/* get a bit from bss, if offset is out of range, return 0 */
int bss_getbit(const bss_t *bss, size_t offset)
{
        size_t byte, bit;

        offset++;
        bit = offset % 8;
        byte = offset / 8 + (bit ? 1 : 0);
        byte--;

        /* fall back to the previous byte */
        if(0 == bit)
                bit = 8;

        if(byte + 1> bss->len)
                return 0;
        else
                return bss->str[byte] & (1UL << (bit-1)) ? 1 : 0;
}

/* set a bit of bss, if offset is out of range, expand it
 * return the result bss or NULL if memory is out
 */
bss_t *bss_setbit(bss_t *bss, size_t offset, uint val)
{
        size_t byte, bit;
        bss_t *new_bss;

        offset++;
        bit = offset % 8;
        byte = offset / 8 + (bit ? 1 : 0);
        byte--;

        /* fall back to the previous byte */
        if(0 == bit)
                bit = 8;

        if(byte + 1 > bsssize(bss)) {
                if(NULL == (new_bss = bss_create_empty(byte + 1))) {
                        return NULL;
                } else {
                        bss_destroy(bss);

                        new_bss->free -= byte;
                        new_bss->len = byte;
                        bss = new_bss;
                }
        } else if(byte + 1 > bss->len) {
                memset(bss->str + bss->len, 0, bss->free);
                bss->free = bsssize(bss) - (byte + 1);
                bss->len = byte + 1;
        }

        if(val)
                bss->str[byte] |= val << (bit-1);
        else
                bss->str[byte] &= ~(1UL << (bit-1));

        return bss;

}
