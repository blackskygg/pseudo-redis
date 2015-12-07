#include "dict.h"

#define HASH_SEED 5381

/* static functions */
static void destroy_entry(const dict_iter_t *iter);
static int destroy_entry_callback(const dict_iter_t *iter, void *dummy);
static dict_iter_t _dict_look_up_hash(const dict_t *dict, uint32_t hash,
                                 const bss_t *key);


/* the op structure for objs whose type is HASH */
static struct obj_op dict_op = {.create = dict_create_obj,
                                .destroy = dict_destroy_obj,
                                .cpy = NULL,
                                .cmp = NULL};

/* create a new dict wiht an initial size of 2^power */
dict_t *dict_create(int power)
{
        dict_t *dict_ptr;
        uint32_t size;

        if(NULL == (dict_ptr = malloc(sizeof(dict_t))))
                return NULL;

        power = power > MAX_UINT32 ? MAX_UINT32 : power;
        dict_ptr->power = power;

        if(32 == power) {
                size = MAX_UINT32;
                dict_ptr->bit_mask = MAX_UINT32;
        } else {
                size = pwr2size(power);
                /* ~x&(x-1) will change 0001000 to 0000111 */
                dict_ptr->bit_mask = (uint32_t)(~size & (size - 1));
        }

        dict_ptr->hash_tbl = calloc(sizeof(dict_entry_t *), size);
        if(NULL == dict_ptr->hash_tbl) {
                free(dict_ptr);
                return NULL;
        }

        /* all the slots are now empty */
        memset(dict_ptr->hash_tbl, 0, sizeof(dict_entry_t *) * size);
        dict_ptr->entry_num = 0;

        return dict_ptr;
}

obj_t *dict_create_obj(const void *data)
{
        const dict_t *dict_ptr_old = data;
        dict_t *dict_ptr;
        obj_t *obj_ptr;


        if(NULL == (obj_ptr = malloc(sizeof(obj_t))))
                return NULL;

        if(NULL == (dict_ptr = dict_create(dict_ptr_old->power))) {
                free(obj_ptr);
                return NULL;
        }

        obj_ptr->type = HASH;
        obj_ptr->op = &dict_op;
        obj_ptr->val = dict_ptr;

        return obj_ptr;
}

/* iterate over evry entry, and perform func on it;
 * this function promises a safe iteration,
 *  so you can delete entries during the call
 */
int dict_iter(const dict_t *dict, dict_callback_t func, void *data)
{
        size_t size;
        dict_iter_t iter;
        dict_entry_t *next_ent;
        int ret_val;
        register uint32_t i;

        size = pwr2size(dict->power);
        for(i = 0; i < size; ++i)
        {
                iter.prev = NULL;
                iter.curr = dict->hash_tbl[i];
                while(iter.curr != NULL) {
                        next_ent = iter.curr->next;

                        /* is it enough? */
                        if(0 != (ret_val = func(&iter, data)))
                                return ret_val;

                        iter.prev = iter.prev ? iter.prev->next : NULL;
                        iter.curr = next_ent;
                }

        }

        return 0;
}

/* destroy an entry pointed to by iter->curr */
static void destroy_entry(const dict_iter_t *iter)
{
        /* notice that we don't need to set the slot to NULL,
         * nor do we need to deal with the links,
         * for we'll keep looking forward and will never look back
         */
        free(iter->curr->key);
        iter->curr->val->op->destroy(iter->curr->val);
        free(iter->curr);
}

/* used as a callback function for dict_iter()
 * in function dict_destroy(), will always return 0
 */
static int destroy_entry_callback(const dict_iter_t *iter, void *dummy)
{
        destroy_entry(iter);
        return 0;
}

/* lookup a dict_iter by hashnum and return an iterator,
 * the existence of this function can prevent counting the hashnum twice.
 */
static dict_iter_t _dict_look_up_hash(const dict_t *dict, uint32_t hash,
                                 const bss_t *key)
{
        dict_iter_t iter = {NULL, NULL};

        iter.curr = dict->hash_tbl[hash];
        while(NULL != iter.curr) {
                if(0 == bss_cmp(key, iter.curr->key))
                        break;
                iter.prev = iter.curr;
                iter.curr = iter.curr->next;
        }

        return iter;
}

/* lookup a dict_iter by key and return an iterator*/
dict_iter_t _dict_look_up(const dict_t *dict, const bss_t *key)
{
        uint32_t hash;

        hash = dict_get_hash(key) & dict->bit_mask;

        return _dict_look_up_hash(dict, hash, key);
}

/* the real loopup API, return a obj_t */
obj_t *dict_look_up(const dict_t *dict, const bss_t *key)
{
        dict_iter_t iter;

        iter = _dict_look_up(dict, key);

        return (NULL == iter.curr) ? NULL : iter.curr->val;
}

/* remove an entry from dict,
 * and WILL NOT free the obj stored in the corresponding entry
 */
dict_iter_t dict_rm_nf(dict_t *dict, const bss_t *key)
{
        dict_iter_t target_iter;
        uint32_t hash;

        hash = dict_get_hash(key) & dict->bit_mask;

        target_iter = _dict_look_up_hash(dict, hash, key);
        if(NULL == target_iter.curr)
                return target_iter;

        if(NULL == target_iter.prev)
                dict->hash_tbl[hash] = target_iter.curr->next;
        else
                target_iter.prev->next = target_iter.curr->next;

        dict->entry_num--;

        return target_iter;
}

/* remove an entry from dict,
 * and free the obj stored in the corresponding entry
 */
int dict_rm(dict_t *dict, const bss_t *key)
{
        dict_iter_t target_iter;

        target_iter = dict_rm_nf(dict, key);
        if(NULL == target_iter.curr)
                return E_NOT_FOUND;

        destroy_entry(&target_iter);

        return 0;

}

/* destroy every entry, and destroy the dict itself */
void dict_destroy(dict_t *dict)
{
        dict_iter(dict, destroy_entry_callback, NULL);
        free(dict->hash_tbl);
        free(dict);
}

void dict_destroy_obj(obj_t *dict_obj)
{
        dict_destroy(dict_obj->val);
        free(dict_obj);
}

dict_entry_t *dict_entry_create(const bss_t *key, obj_t *obj)
{
        dict_entry_t *de_ptr;

        de_ptr = malloc(sizeof(dict_entry_t));
        if(NULL == de_ptr)
                return NULL;

        de_ptr->key = (bss_t *)key;
        de_ptr->val = obj;
        de_ptr->next = NULL;

        return de_ptr;
}

/* add an entry to dict
 * the dict might be expanded if the number of entries is too large
 * but expanding will probably fail
 */
int dict_add(dict_t *dict, const bss_t *key, obj_t *obj)
{
        uint32_t hash;
        dict_iter_t iter = {NULL, NULL};

#ifdef DICT_RESIZE
        /* before hashing, we want to check if we need to expand dict */
        if( ((pwr2size(dict->power) / 4) < (dict->entry_num)) &&
           (dict->power < MAX_POWER)) {
                dict_expand(dict, dict->power + 1);
        }
#endif

        hash = dict_get_hash(key) & dict->bit_mask;

        iter = _dict_look_up_hash(dict, hash, key);

        /* if key exists, replace it with the new one*/
        if(NULL != iter.curr) {
                bss_destroy(iter.curr->key);
                iter.curr->key = (bss_t *)key;
                iter.curr->val->op->destroy(iter.curr->val);
                iter.curr->val = obj;
                return 0;
        } else {

                if(NULL == (iter.curr = dict_entry_create(key, obj)))
                        return E_MEM_OUT;

                if(NULL == iter.prev)
                        dict->hash_tbl[hash] = iter.curr;
                else
                        iter.prev->next = iter.curr;

                /* now the entry is safely added to the dict */
                dict->entry_num++;

                return 0;
        }
}

int dict_rename(dict_t *dict, const bss_t *key, bss_t *new_key)
{
        dict_iter_t iter, new_iter;
        uint32_t hash;

        /* check if the original key exists */
        iter = dict_rm_nf(dict, key);
        if(NULL == iter.curr)
                return E_NOT_FOUND;

        /* check if new_key already exists
         * if it dose, replace it with our new key and value
         */
        hash = dict_get_hash(new_key) & dict->bit_mask;
        new_iter = _dict_look_up_hash(dict, hash, new_key);
        if(new_iter.curr != NULL) {
                /* destroy the old, install the new */
                bss_destroy(new_iter.curr->key);
                new_iter.curr->key = new_key;
                new_iter.curr->val->op->destroy(new_iter.curr->val);
                new_iter.curr->val = iter.curr->val;

                /* clean the old entry */
                bss_destroy(iter.curr->key);
                free(iter.curr);

                return 0;
        } else {
                /* replace the key */
                bss_destroy(iter.curr->key);
                iter.curr->key = new_key;

                /* insert */
                if(NULL == new_iter.prev) {
                        iter.curr->next = NULL;
                        dict->hash_tbl[hash]  = iter.curr;
                } else {
                        iter.curr->next = new_iter.prev->next;
                        new_iter.prev->next = iter.curr;
                }

                return  0;
        }
}

#ifdef DICT_RESIZE
/* expand the size of a dict to 2^power
 * if it fails, nothing will be changed
 */
int dict_expand(dict_t *dict, uint8_t power)
{
        size_t size, old_size;
        uint32_t new_bitmask, hash;
        dict_entry_t **new_tbl;
        register uint32_t i;

        if(32 == power) {
                size = MAX_UINT32;
                new_bitmask = MAX_UINT32;
        } else {
                size = pwr2size(power);
                /* ~x&(x-1) will change 0001000 to 0000111 */
                new_bitmask = (uint32_t)(~size & (size - 1));
        }

        new_tbl = calloc(sizeof(dict_entry_t *), size);
        if(NULL == new_tbl)
                return E_MEM_OUT;

        /* rehashing */
        old_size = pwr2size(dict->power);
        for(i = 0; i < old_size; ++i) {
                if(dict->hash_tbl[i]) {
                        hash = dict_get_hash(dict->hash_tbl[i]->key)
                                & new_bitmask;
                        new_tbl[hash] = dict->hash_tbl[i];
                }
        }
        /* update dict with the new hashtable */
        free(dict->hash_tbl);
        dict->hash_tbl = new_tbl;
        dict->power = power;
        dict->bit_mask = new_bitmask;

        return 0;
}
#endif /*DICT_RESIZE*/

/* this function is borrowed from redis's source code */
/* MurmurHash2, by Austin Appleby
 * Note - This code makes a few assumptions about how your machine behaves -
 * 1. We can read a 4-byte value from any address without crashing
 * 2. sizeof(int) == 4
 *
 * And it has a few limitations -
 *
 * 1. It will not work incrementally.
 * 2. It will not produce the same results on little-endian and big-endian
 *    machines.
 */
uint32_t dict_get_hash(const bss_t *bss)
{
        /* 'm' and 'r' are mixing constants generated offline.
           They're not really 'magic', they just happen to work well.  */
        const char *key = bss->str;
        size_t len = bss->len;
        uint32_t seed = HASH_SEED;
        const uint32_t m = 0x5bd1e995;
        const int r = 24;

        /* Initialize the hash to a 'random' value */
        uint32_t h = seed ^ len;

        /* Mix 4 bytes at a time into the hash */
        const unsigned char *data = (const unsigned char *)key;

        while(len >= 4) {
                uint32_t k = *(uint32_t*)data;

                k *= m;
                k ^= k >> r;
                k *= m;

                h *= m;
                h ^= k;

                data += 4;
                len -= 4;
        }

        /* Handle the last few bytes of the input array  */
        switch(len) {
        case 3: h ^= data[2] << 16;
        case 2: h ^= data[1] << 8;
        case 1: h ^= data[0]; h *= m;
        };

        /* Do a few final mixes of the hash to ensure the last few
         * bytes are well-incorporated. */
        h ^= h >> 13;
        h *= m;
        h ^= h >> 15;

        return (uint32_t)h;
}
