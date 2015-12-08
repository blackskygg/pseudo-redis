#ifndef COMMAND_H_
#define COMMAND_H_

#include "server.h"

#define CMD_PROTO(name) int name##_command(bss_t *args[], size_t num)
#define _CMD_PROTO(name) int _##name##_command(dict_t *dict, \
                                                   bss_t *args[], size_t num)

CMD_PROTO(del);
CMD_PROTO(exists);
CMD_PROTO(randomkey);
CMD_PROTO(rename);
CMD_PROTO(keys);
CMD_PROTO(type);
CMD_PROTO(append);
CMD_PROTO(getbit);
CMD_PROTO(setbit);
CMD_PROTO(mget);
CMD_PROTO(bitcount);
CMD_PROTO(setrange);
CMD_PROTO(getrange);
CMD_PROTO(incr);
CMD_PROTO(decr);
CMD_PROTO(incrby);
CMD_PROTO(decrby);
CMD_PROTO(msetnx);
CMD_PROTO(get);
CMD_PROTO(set);
CMD_PROTO(setnx);
CMD_PROTO(strlen);
CMD_PROTO(hdel);
CMD_PROTO(hlen);
CMD_PROTO(hexists);
CMD_PROTO(hmget);
CMD_PROTO(hget);
CMD_PROTO(hmset);
CMD_PROTO(hgetall);
CMD_PROTO(hincrby);
CMD_PROTO(hset);
CMD_PROTO(hincrbyfloat);
CMD_PROTO(hsetnx);
CMD_PROTO(hkeys);
CMD_PROTO(hvals);
CMD_PROTO(blpop);
CMD_PROTO(lrange);
CMD_PROTO(brpop);
CMD_PROTO(lrem);
CMD_PROTO(brpoplpush);
CMD_PROTO(lset);
CMD_PROTO(lindex);
CMD_PROTO(ltrim);
CMD_PROTO(linsert);
CMD_PROTO(rpop);
CMD_PROTO(llen);
CMD_PROTO(rpoplpush);
CMD_PROTO(lpop);
CMD_PROTO(rpush);
CMD_PROTO(lpush);
CMD_PROTO(rpushx);
CMD_PROTO(lpushx);
CMD_PROTO(sadd);
CMD_PROTO(smove);
CMD_PROTO(scard);
CMD_PROTO(spop);
CMD_PROTO(sdiff);
CMD_PROTO(srandmember);
CMD_PROTO(srem);
CMD_PROTO(sinter);
CMD_PROTO(sscan);
CMD_PROTO(sunion);
CMD_PROTO(sismember);
CMD_PROTO(smembers);

#endif /* COMMAND_H_ */
