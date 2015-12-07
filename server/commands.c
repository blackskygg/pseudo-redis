#include <endian.h>
#include "commands.h"

#define FREE_ARG(n) free(args[(n)])

#define FREE_ARGS(a, b) ({                              \
                        for(uint32_t i = a; i < b; ++i) \
                                FREE_ARG(i);            \
                })


#define CHECK_ARGS(n) ({                                        \
                        if(num != (n)) {                        \
                                FREE_ARGS(0, num);              \
                                fail_reply(NUM_ARGS);           \
                        }})

#define CHECK_TYPE(obj, t) ({                                   \
                        if((obj)->type != (t)){                 \
                                FREE_ARGS(0, num);              \
                                fail_reply(WRONG_TYPE);         \
                        }})

#define CHECK_NULL(ptr) ({                                      \
                        if(NULL == (ptr)) {                     \
                                FREE_ARGS(0, num);              \
                                fail_reply(MEM_OUT);            \
                        }})


/* <--!!! this comment applies too ALL the functions in this file-->
 * the xxx_reply() calls are actually marcros
 * and they ALL imply a "return" statement
 */

_CMD_PROTO(del)
{
        CHECK_ARGS(1);

        if(0 != dict_rm(dict, args[0])) {
                FREE_ARG(0);
                false_reply();
        } else {
                FREE_ARG(0);
                true_reply();
        }
}

_CMD_PROTO(exists)
{
        CHECK_ARGS(1);

        if(NULL == dict_look_up(dict, args[0])) {
                FREE_ARG(0);
                false_reply();
        } else {
                FREE_ARG(0);
                true_reply();
        }

}

/* helper function used by dict_iter to add keys to an array */
int addkey_to_arr(const dict_iter_t *dict_iter, void *dummy)
{
        return addto_reply_arr(dict_iter->curr->key);
}

int add_key_val_to_arr(const dict_iter_t *dict_iter, void *dummy)
{

        if(0 != addto_reply_arr(dict_iter->curr->key) ||
           0 != addto_reply_arr((bss_t *)dict_iter->curr->val->val))
                return E_TOO_LONG;
}

_CMD_PROTO(get)
{
        CHECK_ARGS(1);

        obj_t *val_obj;
        size_t len;

        val_obj = dict_look_up(dict, args[0]);
        if(NULL != val_obj) {
                CHECK_TYPE(val_obj, STRING);
                len = ((bss_t *)val_obj->val)->len + 1;

                FREE_ARG(0);
                string_reply(((bss_t *)val_obj->val)->str, len);
        } else {
                FREE_ARG(0);
                nil_reply();
        }
}


_CMD_PROTO(set)
{
        CHECK_ARGS(2);

        obj_t *val_obj;

        CHECK_NULL(val_obj = bss_create_obj(args[1]));
        if(0 != dict_add(dict, args[0], val_obj)) {
                FREE_ARG(0);
                FREE_ARG(1);
                fail_reply(MEM_OUT);
        } else {
                ok_reply();
        }

}

_CMD_PROTO(incrby)
{
        CHECK_ARGS(2);

        obj_t *str_obj;
        bss_int_t count;
        bss_int_t number;

        /* first check args[1] */
        if(0 != bss2int(args[1], &number))
                goto ERR_INV_INT;

        str_obj = dict_look_up(dict, args[0]);

        /* does it exist? */
        if(NULL == str_obj) {
                /* create a new entry and set it to args[1] */
                CHECK_NULL(str_obj = bss_create_obj(args[1]));
                if(0 != dict_add(dict, args[0], str_obj)) {
                        FREE_ARG(0);
                        FREE_ARG(1);
                        fail_reply(MEM_OUT);
                }

                int_reply(number);
        } else {
                CHECK_TYPE(str_obj, STRING);

                /* is it a valid number? */
                if(bss2int((bss_t*)(str_obj->val), &count) != 0)
                        goto ERR_INV_INT;

                /* overflow? */
                if(bss_incr((bss_t*)(str_obj->val), number) != 0)
                        goto ERR_INV_INT;

                count += number;
                FREE_ARG(0);
                FREE_ARG(1);
                int_reply(count);
        }

ERR_INV_INT:
        FREE_ARG(0);
        FREE_ARG(1);
        fail_reply(INV_INT);

}

_CMD_PROTO(decrby)
{
                CHECK_ARGS(2);

        obj_t *str_obj;
        bss_int_t count;
        bss_int_t number;

        /* first check args[1] */
        if(0 != bss2int(args[1], &number))
                goto ERR_INV_INT;


        str_obj = dict_look_up(dict, args[0]);

        /* does it exist? */
        if(NULL == str_obj) {
                /* create a new entry and set it to args[1] */
                sprintf(args[1]->str, "%d", -number);
                args[1]->len++;

                CHECK_NULL(str_obj = bss_create_obj(args[1]));

                if(0 != dict_add(dict, args[0], str_obj)) {
                        FREE_ARG(0);
                        FREE_ARG(1);
                        fail_reply(MEM_OUT);
                }

                int_reply(-number);
        } else {
                CHECK_TYPE(str_obj, STRING);

                /* is it a valid number? */
                if(bss2int((bss_t*)(str_obj->val), &count) != 0)
                        goto ERR_INV_INT;
                /* overflow? */
                if(bss_decr((bss_t*)(str_obj->val), number) != 0)
                        goto ERR_INV_INT;

                count -= number;
                FREE_ARG(0);
                FREE_ARG(1);
                int_reply(count);
        }

ERR_INV_INT:
        FREE_ARG(0);
        FREE_ARG(1);
        fail_reply(INV_INT);
}

CMD_PROTO(del)
{
        return _del_command(key_dict, args, num);
}

CMD_PROTO(exists)
{
        return _exists_command(key_dict, args, num);
}


CMD_PROTO(randomkey)
{
}


CMD_PROTO(keys)
{
        CHECK_ARGS(1);

        reset_reply_arr();
        if(0 != dict_iter(key_dict, addkey_to_arr, NULL))
                fail_reply(TOO_LONG);
        else
                arr_reply();
}

CMD_PROTO(rename)
{
        CHECK_ARGS(2);

        if(0 != dict_rename(key_dict, args[0], args[1])) {
                FREE_ARG(0);
                FREE_ARG(1);
                fail_reply(NO_KEY);
        } else {
                FREE_ARG(0);
                ok_reply();
        }

}


CMD_PROTO(type)
{
        CHECK_ARGS(1);

        obj_t *val_obj;

        if(NULL == (val_obj = dict_look_up(key_dict, args[0]))) {
                FREE_ARG(0);
                type_reply(NONE);
        } else {
                FREE_ARG(0);
                type_reply(val_obj->type);
        }
}

CMD_PROTO(append)
{
        CHECK_ARGS(2);

        obj_t *str_obj;
        bss_t *bss;
        dict_iter_t iter;

        iter = _dict_look_up(key_dict, args[0]);

        if(NULL == iter.curr) {
                /* no such key..., create one */
                str_obj = bss_create_obj(args[1]);
                if(0 != dict_add(key_dict, args[0], str_obj)) {
                        FREE_ARG(0);
                        FREE_ARG(1);
                        fail_reply(MEM_OUT);
                } else {
                        bss = args[1];
                }
        } else {
                str_obj = iter.curr->val;
                CHECK_TYPE(str_obj, STRING);

                bss = (bss_t *)str_obj->val;
                bss = bss_append(bss, args[1]->str, args[1]->len);
                str_obj = bss_create_obj(bss);
                iter.curr->val = str_obj;

                FREE_ARG(1);
        }

        int_reply(bss->len);
}

CMD_PROTO(getbit)
{
        CHECK_ARGS(2);

        obj_t *str_obj;
        bss_t *bss;
        bss_int_t offset;

        /* check the offset */
        if(0 != bss2int(args[1], &offset)) {
                FREE_ARG(0);
                FREE_ARG(1);
                fail_reply(INV_OFFSET);
        }

        if(NULL == (str_obj = dict_look_up(key_dict, args[0]))) {
                FREE_ARG(0);
                FREE_ARG(1);
                false_reply();
        }

        CHECK_TYPE(str_obj, STRING);
        bss = str_obj->val;

        int_reply(bss_getbit(bss, offset));
}

CMD_PROTO(setbit)
{
        CHECK_ARGS(3);

        dict_iter_t iter;
        obj_t *str_obj;
        bss_t *bss, *t_bss;
        bss_int_t offset;
        bss_int_t bit;
        int result;

        /* check the offset */
        if(0 != bss2int(args[1], &offset)) {
                FREE_ARGS(0, 3);
                fail_reply(INV_OFFSET);
        }

        /* check the bit */
        if((0 != bss2int(args[2], &bit)) ||
           (bit != 0 && bit != 1)) {
                FREE_ARGS(0, 3);
                fail_reply(INV_SYNX);

        }

        iter = _dict_look_up(key_dict, args[0]);
        if(NULL == iter.curr) {
                /* create a new key, and set its value to args[0] */
                CHECK_NULL(bss = bss_create_empty(0));
                if((NULL == (t_bss = bss_setbit(bss, offset, bit)))
                   || (NULL == (str_obj = bss_create_obj(t_bss)))) {
                        free(bss);
                        goto ERR_MEM_OUT;
                }

                if(0 != dict_add(key_dict, args[0], str_obj)) {
                        free(bss);
                        goto ERR_MEM_OUT;

                }

                FREE_ARG(1);
                FREE_ARG(2);
                false_reply();
        } else {
                str_obj = iter.curr->val;
                CHECK_TYPE(str_obj, STRING);
                bss = str_obj->val;

                /* we will obtain the bit first */
                result = bss_getbit(bss, offset);

                if(NULL == (t_bss = bss_setbit(bss, offset, bit)))
                        goto ERR_MEM_OUT;

                str_obj->val = t_bss;

                int_reply(result);
        }

ERR_MEM_OUT:
        FREE_ARGS(0, 3);
        fail_reply(MEM_OUT);
}

CMD_PROTO(mget)
{
}

CMD_PROTO(bitcount)
{
        CHECK_ARGS(1);

        obj_t *str_obj;
        size_t count;

        str_obj = dict_look_up(key_dict, args[0]);
        if(NULL == str_obj) {
                FREE_ARG(0);
                false_reply();
        }

        CHECK_TYPE(str_obj, STRING);
        count = bss_count_bit((bss_t *)str_obj->val);

        FREE_ARG(0);
        int_reply(count);

}

CMD_PROTO(setrange)
{
}

CMD_PROTO(getrange)
{
}

CMD_PROTO(incr)
{
        CHECK_ARGS(1);

        obj_t *str_obj;
        bss_t *bss;
        bss_int_t count;

        str_obj = dict_look_up(key_dict, args[0]);

        /* does it exist? */
        if(NULL == str_obj) {
                /* create a new entry and set it to args[1] */
                CHECK_NULL(bss = bss_create("1", 1));
                if(NULL == (str_obj = bss_create_obj(bss)))
                        goto ERR_MEM_OUT;

                if(0 != dict_add(key_dict, args[0], str_obj))
                        goto ERR_MEM_OUT;

                int_reply(1);
        } else {
                CHECK_TYPE(str_obj, STRING);

                /* is it a valid number? */
                if(bss2int((bss_t*)(str_obj->val), &count) != 0)
                        goto ERR_INV_INT;

                /* overflow? */
                if(bss_incr((bss_t*)(str_obj->val), 1) != 0)
                        goto ERR_INV_INT;


                count ++;
                FREE_ARG(0);
                int_reply(count);
        }

ERR_MEM_OUT:
        free(bss);
        FREE_ARG(0);
        fail_reply(MEM_OUT);

ERR_INV_INT:
        FREE_ARG(0);
        fail_reply(INV_INT);

}

CMD_PROTO(decr)
{
        CHECK_ARGS(1);

        obj_t *str_obj;
        bss_t *bss;
        bss_int_t count;

        str_obj = dict_look_up(key_dict, args[0]);

        /* does it exist? */
        if(NULL == str_obj) {
                /* create a new entry and set it to args[1] */
                CHECK_NULL(bss = bss_create("-1", 2));
                if(NULL == (str_obj = bss_create_obj(bss))) {
                        free(bss);
                        goto ERR_MEM_OUT;
                }

                if(0 != dict_add(key_dict, args[0], str_obj)) {
                        free(bss);
                        goto ERR_MEM_OUT;
                }

                int_reply(-1);
        } else {
                CHECK_TYPE(str_obj, STRING);

                /* is it a valid number? */
                if(bss2int((bss_t*)(str_obj->val), &count) != 0)
                        goto ERR_INV_INT;
                /* overflow? */
                if(bss_decr((bss_t*)(str_obj->val), 1) != 0)
                        goto ERR_INV_INT;

                count --;
                FREE_ARG(0);
                int_reply(count);
        }

ERR_MEM_OUT:
        FREE_ARGS(0, num);
        fail_reply(MEM_OUT);

ERR_INV_INT:
        FREE_ARG(0);
        fail_reply(INV_INT);

}

CMD_PROTO(incrby)
{
        return _incrby_command(key_dict, args, num);
}

CMD_PROTO(decrby)
{
        return _decrby_command(key_dict, args, num);
}

CMD_PROTO(msetnx)
{
}

CMD_PROTO(get)
{
        return _get_command(key_dict, args, num);
}

CMD_PROTO(set)
{
        return _set_command(key_dict, args, num);
}

CMD_PROTO(strlen)
{
        CHECK_ARGS(1);

        obj_t *val_obj;
        bss_t *bss;
        if(NULL == (val_obj = dict_look_up(key_dict, args[0]))) {
                FREE_ARG(0);
                int_reply(0);
        } else {
                CHECK_TYPE(val_obj, STRING);

                bss = val_obj->val;
                FREE_ARG(0);
                int_reply(bss->len);
        }
}

CMD_PROTO(hdel)
{
        CHECK_ARGS(2);

        obj_t *val_obj;
        bss_t *bss;

        if(NULL == (val_obj = dict_look_up(key_dict, args[0]))) {
                FREE_ARG(0);
                FREE_ARG(1);
                false_reply();
        } else {
                CHECK_TYPE(val_obj, HASH);
                FREE_ARG(0);
                return _del_command((dict_t *)val_obj->val, args + 1, num - 1);
        }
}

CMD_PROTO(hlen)
{
        CHECK_ARGS(1);

        obj_t *val_obj;

        if(NULL == (val_obj = dict_look_up(key_dict, args[0]))) {
                FREE_ARG(0);
                false_reply();
        } else {
                CHECK_TYPE(val_obj, HASH);
                FREE_ARG(0);
                int_reply(((dict_t *)val_obj->val)->entry_num);
        }
}

CMD_PROTO(hexists)
{
        CHECK_ARGS(2);

        obj_t *val_obj;

        if(NULL == (val_obj = dict_look_up(key_dict, args[0]))) {
                FREE_ARG(0);
                FREE_ARG(1);
                false_reply();
        } else {
                CHECK_TYPE(val_obj, HASH);
                FREE_ARG(0);
                return _exists_command((dict_t *)val_obj->val, args + 1, num - 1);
        }

}

CMD_PROTO(hmget)
{
}

CMD_PROTO(hget)
{
        CHECK_ARGS(2);

        obj_t *val_obj;

        if(NULL == (val_obj = dict_look_up(key_dict, args[0]))) {
                FREE_ARGS(0, 2);
                nil_reply();
        } else {
                CHECK_TYPE(val_obj, HASH);
                FREE_ARG(0);
                return _get_command((dict_t *)val_obj->val, args + 1, num - 1);
        }
}

CMD_PROTO(hmset)
{
}

CMD_PROTO(hgetall)
{
        CHECK_ARGS(1);

        obj_t *val_obj;

        reset_reply_arr();
        if(NULL == (val_obj = dict_look_up(key_dict, args[0]))) {
                FREE_ARG(0);
                arr_reply();
        }

        CHECK_TYPE(val_obj, HASH);

        if(0 != dict_iter((dict_t *)val_obj->val, add_key_val_to_arr, NULL)) {
                FREE_ARG(0);
                fail_reply(TOO_LONG);
        } else {
                FREE_ARG(0);
                arr_reply();
        }

}

CMD_PROTO(hincrby)
{
        CHECK_ARGS(3);

        obj_t *val_obj;
        dict_t *dict;

        if(NULL == (val_obj = dict_look_up(key_dict, args[0]))) {
                /* if it dose not exist, create one */
                dict = dict_create(NEW_DICT_POW);
                val_obj = dict_create_obj(dict);
                dict_add(key_dict, args[0], val_obj);

                return _incrby_command(dict, args + 1, num - 1);
        } else {
                CHECK_TYPE(val_obj, HASH);
                FREE_ARG(0);
                return _incrby_command((dict_t *)val_obj->val, args + 1, num - 1);
        }
}

CMD_PROTO(hset)
{
        CHECK_ARGS(3);

        obj_t *val_obj;
        dict_t *dict;
        uint8_t flag;

        if(NULL == (val_obj = dict_look_up(key_dict, args[0]))) {
                /* if it dose not exist, create one */
                dict = dict_create(NEW_DICT_POW);
                val_obj = dict_create_obj(dict);
                dict_add(key_dict, args[0], val_obj);

                _set_command(dict, args + 1, num - 1);
                true_reply();
        } else {
                CHECK_TYPE(val_obj, HASH);

                FREE_ARG(0);

                /* check if the target already exists */
                if(NULL == dict_look_up((dict_t *)val_obj->val, args[1]))
                        flag = 0;
                else
                        flag = 1;

                _set_command((dict_t *)val_obj->val, args + 1, num - 1);

                if(flag)
                        false_reply();
                else
                        true_reply();
        }
}

CMD_PROTO(hincrbyfloat)
{
}

CMD_PROTO(hsetnx)
{
}

CMD_PROTO(hkeys)
{
        CHECK_ARGS(1);

        obj_t *val_obj;

        reset_reply_arr();
        if(NULL == (val_obj = dict_look_up(key_dict, args[0])))
                arr_reply();

        CHECK_TYPE(val_obj, HASH);

        if(0 != dict_iter((dict_t *)val_obj->val, addkey_to_arr, NULL))
                fail_reply(TOO_LONG);
        else
                arr_reply();
}

CMD_PROTO(havls)
{
}

CMD_PROTO(blpop)
{
}

CMD_PROTO(lrange)
{
}

CMD_PROTO(brpop)
{
}

CMD_PROTO(lrem)
{
}

CMD_PROTO(brpoplpush)
{
}

CMD_PROTO(lset)
{
}

CMD_PROTO(lindex)
{
}

CMD_PROTO(ltrim)
{
}

CMD_PROTO(linsert)
{
}

CMD_PROTO(rpop)
{
}

CMD_PROTO(llen)
{
}

CMD_PROTO(rpoplpush)
{
}

CMD_PROTO(lpop)
{
}

CMD_PROTO(rpush)
{
}

CMD_PROTO(lpush)
{
}

CMD_PROTO(rpushx)
{
}

CMD_PROTO(lpushx)
{
}

CMD_PROTO(sadd)
{
}

CMD_PROTO(smove)
{
}

CMD_PROTO(scard)
{
}

CMD_PROTO(spop)
{
}

CMD_PROTO(sdiff)
{
}

CMD_PROTO(srandmember)
{
}

CMD_PROTO(srem)
{
}

CMD_PROTO(sinter)
{
}

CMD_PROTO(sscan)
{
}

CMD_PROTO(sunion)
{
}

CMD_PROTO(sismember)
{
}

CMD_PROTO(smembers)
{
}

