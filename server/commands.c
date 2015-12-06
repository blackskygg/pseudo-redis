#include <endian.h>
#include "commands.h"

CMD_PROTO(del)
{
}

CMD_PROTO(exists)
{
        if(num != 1) {
                for(uint32_t i = 0; i < num; ++i)
                        free(args[i]);
                return fail_reply(NUM_ARGS);
        }


        if(NULL == dict_look_up(key_dict, args[0]))
                return false_reply();
        else
                return true_reply();

}

CMD_PROTO(randomkey)
{
}

CMD_PROTO(rename)
{
}

CMD_PROTO(keys)
{
}

CMD_PROTO(type)
{
}

CMD_PROTO(append)
{
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
        if(num != 1) {
                for(uint32_t i = 0; i < num; ++i)
                        free(args[i]);
                return fail_reply(NUM_ARGS);
        }

        obj_t *str_obj;
        size_t count;

        str_obj = dict_look_up(key_dict, args[0]);
        if(NULL == str_obj)
                return false_reply();
        else if(STRING == str_obj->type) {
                count = bss_count_bit((bss_t *)str_obj->val);
                return create_int_reply(count);
        } else {
                return fail_reply(WRONG_TYPE);
        }

}

CMD_PROTO(setrange)
{
}

CMD_PROTO(getrange)
{
}

CMD_PROTO(incr)
{
        if(num != 1) {
                for(uint32_t i = 0; i < num; ++i)
                        free(args[i]);
                return fail_reply(NUM_ARGS);
        }

        obj_t *str_obj;
        bss_int_t count;

        str_obj = dict_look_up(key_dict, args[0]);
        if(NULL == str_obj)
                return false_reply();
        else if(STRING == str_obj->type) {
                /* is it a valid number? */
                if(bss2int((bss_t*)(str_obj->val), &count) != 0)
                        return fail_reply(INV_INT);

                /* overflow? */
                if(bss_incr((bss_t*)(str_obj->val), 1) != 0)
                        return fail_reply(INV_INT);

                count++;
                return create_int_reply(count);
        } else {
                return fail_reply(WRONG_TYPE);
        }

}

CMD_PROTO(decr)
{
        if(num != 1) {
                for(uint32_t i = 0; i < num; ++i)
                        free(args[i]);
                return fail_reply(NUM_ARGS);
        }

        obj_t *str_obj;
        bss_int_t count;

        str_obj = dict_look_up(key_dict, args[0]);
        if(NULL == str_obj)
                return false_reply();
        else if(STRING == str_obj->type) {
                /* is it a valid number? */
                if(bss2int((bss_t*)(str_obj->val), &count) != 0)
                        return fail_reply(INV_INT);

                /* overflow? */
                if(bss_decr((bss_t*)(str_obj->val), 1) != 0)
                        return fail_reply(INV_INT);

                count--;
                return create_int_reply(count);
        } else {
                return fail_reply(WRONG_TYPE);
        }

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
        if(num != 1) {
                for(uint32_t i = 0; i < num; ++i)
                        free(args[i]);
                return fail_reply(NUM_ARGS);
        }

        obj_t *val_obj;
        size_t len;

        val_obj = dict_look_up(key_dict, args[0]);
        if(NULL != val_obj) {
                if(val_obj->type != STRING)
                        return fail_reply(WRONG_TYPE);

                len = ((bss_t *)val_obj->val)->len + 1;

                return string_reply(((bss_t *)val_obj->val)->str, len);
        } else {
                free(args[0]);
                return nil_reply();
        }
}

CMD_PROTO(set)
{
        if(num != 2) {
                for(uint32_t i = 0; i < num; ++i)
                        free(args[i]);
                return fail_reply(NUM_ARGS);
        }

        obj_t *val_obj;


        if(NULL == (val_obj = bss_create_obj(args[1]))) {
                for(uint32_t i = 0; i < num; ++i)
                        free(args[i]);
                return fail_reply(MEM_OUT);
        }

        if(0 != dict_add(key_dict, args[0], val_obj)) {
                for(uint32_t i = 0; i < num; ++i)
                        free(args[i]);
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

