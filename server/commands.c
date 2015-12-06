#include <endian.h>
#include "commands.h"

#define FREE_ARG(n) free(args[(n)])

#define CHECK_ARGS(n) ({if(num != (n)) {                                \
                                for(uint32_t i = 0; i < num; ++i)       \
                                        free(args[i]);                  \
                                return fail_reply(NUM_ARGS);            \
                        }})

#define CHECK_TYPE(obj, t) ({if((obj)->type != (t)){                    \
                                        for(uint32_t i = 0; i < num; ++i) \
                                                free(args[i]);            \
                                                                          \
                                        return fail_reply(WRONG_TYPE);}})


CMD_PROTO(del)
{
        CHECK_ARGS(1);

        if(0 != dict_rm(key_dict, args[0])) {
                FREE_ARG(0);
                return false_reply();
        } else {
                FREE_ARG(0);
                return true_reply();
        }
}

CMD_PROTO(exists)
{
        CHECK_ARGS(1);

        if(NULL == dict_look_up(key_dict, args[0])) {
                FREE_ARG(0);
                return false_reply();
        } else {
                FREE_ARG(0);
                return true_reply();
        }

}

CMD_PROTO(randomkey)
{
}

CMD_PROTO(rename)
{
        CHECK_ARGS(2);

        if(0 != dict_rename(key_dict, args[0], args[1])) {
                FREE_ARG(0);
                FREE_ARG(1);
                return fail_reply(NO_KEY);
        } else {
                FREE_ARG(0);
                return ok_reply();
        }

}

CMD_PROTO(keys)
{
}

CMD_PROTO(type)
{
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
                        return fail_reply(MEM_OUT);
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

        return create_int_reply(bss->len);
}

CMD_PROTO(getbit)
{
}

CMD_PROTO(setbit)
{
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
                return false_reply();
        }

        CHECK_TYPE(str_obj, STRING);
        count = bss_count_bit((bss_t *)str_obj->val);

        FREE_ARG(0);
        return create_int_reply(count);

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
        bss_int_t count;

        str_obj = dict_look_up(key_dict, args[0]);
        if(NULL == str_obj) {
                FREE_ARG(0);
                return false_reply();
        }

        CHECK_TYPE(str_obj, STRING);

        /* is it a valid number? */
        if(bss2int((bss_t*)(str_obj->val), &count) != 0) {
                FREE_ARG(0);
                return fail_reply(INV_INT);
        }

        /* overflow? */
        if(bss_incr((bss_t*)(str_obj->val), 1) != 0) {
                FREE_ARG(0);
                return fail_reply(INV_INT);
        }

        count++;
        FREE_ARG(0);
        return create_int_reply(count);
}

CMD_PROTO(decr)
{
        CHECK_ARGS(1);

        obj_t *str_obj;
        bss_int_t count;

        str_obj = dict_look_up(key_dict, args[0]);
        if(NULL == str_obj) {
                FREE_ARG(0);
                return false_reply();
        }

        CHECK_TYPE(str_obj, STRING);

        /* is it a valid number? */
        if(bss2int((bss_t*)(str_obj->val), &count) != 0) {
                FREE_ARG(0);
                return fail_reply(INV_INT);
        }

        /* overflow? */
        if(bss_decr((bss_t*)(str_obj->val), 1) != 0) {
                FREE_ARG(0);
                return fail_reply(INV_INT);
        }

        count--;
        FREE_ARG(0);
        return create_int_reply(count);
}

CMD_PROTO(incrby)
{
}

CMD_PROTO(decrby)
{
}

CMD_PROTO(msetnx)
{
}

CMD_PROTO(get)
{
        CHECK_ARGS(1);

        obj_t *val_obj;
        size_t len;

        val_obj = dict_look_up(key_dict, args[0]);
        if(NULL != val_obj) {
                CHECK_TYPE(val_obj, STRING);
                len = ((bss_t *)val_obj->val)->len + 1;

                FREE_ARG(0);
                return string_reply(((bss_t *)val_obj->val)->str, len);
        } else {
                FREE_ARG(0);
                return nil_reply();
        }
}

CMD_PROTO(set)
{
        CHECK_ARGS(2);

        obj_t *val_obj;


        if(NULL == (val_obj = bss_create_obj(args[1]))) {
                FREE_ARG(0);
                FREE_ARG(1);
                return fail_reply(MEM_OUT);
        }

        if(0 != dict_add(key_dict, args[0], val_obj)) {
                FREE_ARG(1);
                return fail_reply(MEM_OUT);
        } else {
                free(args[0]);
                return ok_reply();
        }

}

CMD_PROTO(strlen)
{
}

CMD_PROTO(hdel)
{
}

CMD_PROTO(hlen)
{
}

CMD_PROTO(hexists)
{
}

CMD_PROTO(hmget)
{
}

CMD_PROTO(hget)
{
}

CMD_PROTO(hmset)
{
}

CMD_PROTO(hgetall)
{
}

CMD_PROTO(hincrby)
{
}

CMD_PROTO(hset)
{
}

CMD_PROTO(hincrbyfloat)
{
}

CMD_PROTO(hsetnx)
{
}

CMD_PROTO(hkeys)
{
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

