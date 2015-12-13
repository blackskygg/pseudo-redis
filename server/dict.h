#ifndef DICT_H_
#define DICT_H_

#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include "obj.h"
#include "bss.h"
#include "err.h"

#define MAX_POWER 32
#define MAX_UINT32  UINT32_MAX
#define SET_VAL_PTR (void *)0x1

struct dict_entry {
        bss_t *key;
        obj_t *val;
        struct dict_entry *next;
};
typedef struct dict_entry dict_entry_t;

/* a backward iterator, which will make it easier to perform deletion */
struct dict_iter {
        dict_entry_t *curr;
        dict_entry_t *prev;
};
typedef struct dict_iter dict_iter_t;

struct dict {
        dict_entry_t **hash_tbl;

#ifdef DICT_RESIZE
        dict_entry_t **hash_tbl2;
#endif
        uint8_t power;
        size_t entry_num;
        uint32_t bit_mask;
};

typedef struct dict dict_t;
/* we use a 32-bit unsigned int as hash key */
typedef uint32_t hkey_t;
/* prototype of the callback function required by dict_iter() */
typedef int (*dict_callback_t)(const dict_iter_t *dict_iter, void *data);

/* convert power to uint32_t */
#define pwr2size(power) (((uint32_t)1) << (power))

/* dict APIs */
dict_t *dict_create(int power);
void dict_destroy(dict_t *dict);
void dict_destroy_shallow(dict_t *dict);
dict_entry_t *dict_entry_create(const bss_t *key, obj_t *obj);
uint32_t dict_get_hash(const bss_t *bss);
int dict_add(dict_t *dict, const bss_t *key, obj_t *obj);
int dict_add_shallow(dict_t *dict, const bss_t *key, obj_t *obj);
int dict_rename(dict_t *dict, const bss_t *key, bss_t *new_key);
int dict_rm(dict_t *dict, const bss_t *key);
dict_iter_t dict_rm_nf(dict_t *dict, const bss_t *key);
dict_iter_t _dict_look_up(const dict_t *dict, const bss_t *key);
obj_t *dict_look_up(const dict_t *dict, const bss_t *key);
int dict_iter(const dict_t *dict, dict_callback_t func, void *data);
dict_entry_t *dict_random_elem(dict_t *dict);

#ifdef DICT_RESIZE
int dict_expand(dict_t *dict, uint8_t power);
#endif

/* set APIs */
#define set_add(set, key) dict_add((set), (key), SET_VAL_PTR)

/* obj-layer set APIs */
obj_t *set_create_obj(const void *data);


/* obj-layer dict APIs */
obj_t *dict_create_obj(const void *data);
void dict_destroy_obj(obj_t *dict_obj);


#endif
