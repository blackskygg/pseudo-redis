#include "commands.h"

CMD_PROTO(del)
{
}

CMD_PROTO(exists)
{
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
}

CMD_PROTO(setrange)
{
}

CMD_PROTO(getrange)
{
}

CMD_PROTO(incr)
{
}

CMD_PROTO(decr)
{
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
                return NULL;
        }

        reply_t *reply;
        obj_t *val_obj;
        size_t len;

        val_obj = dict_look_up(key_dict, args[0]);
        if(NULL != val_obj) {
                len = ((bss_t *)val_obj->val)->len + 1;
                reply =  malloc(sizeof(reply_t) + len);

                if(NULL == reply) {
                        free(args[0]);
                        return NULL;
                }

                reply->reply_type = RPLY_STRING;
                reply->len = len;
                memcpy(reply->data, ((bss_t *)val_obj->val)->str, len);

                return reply;
        } else {
                free(args[0]);
                return NULL;
        }
}

CMD_PROTO(set)
{
        if(num != 2) {
                for(uint32_t i = 0; i < num; ++i)
                        free(args[i]);
                return NULL;
        }

        reply_t *reply = malloc(sizeof(reply_t));
        obj_t *val_obj;

        if(NULL == reply) {
                for(uint32_t i = 0; i < num; ++i)
                        free(args[i]);
                return NULL;
        }


        if(NULL == (val_obj = bss_create_obj(args[1]))) {
                for(uint32_t i = 0; i < num; ++i)
                        free(args[i]);

                free(reply);
                return NULL;
        }

        if(0 != dict_add(key_dict, args[0], val_obj)) {
                for(uint32_t i = 0; i < num; ++i)
                        free(args[i]);

                free(reply);
                return NULL;
        } else {
                free(args[0]);
                reply->len = 0;
                reply->reply_type = RPLY_OK;
                return reply;
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

