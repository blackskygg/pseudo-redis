# pseudo-redis
a simplified, useless db

I was trying to make this pseudo-redis LOOK exactely the same with the real one,

(i.e. the same input series would cause the same output series, EVEN ERRORS are the same!!!),

Some commands(mostly zset commands) and options for some specific commands(set, randomkey, srandmember etc.) are not yet avaliable though.

You can use this to fool your friend(say, "Hey, you know what? Redis is stupid! See? It has memory leaks!", etc. (but what's the point?))


##supported commands
###keys 
setnx del exists rename keys type randomkey(partial)

###string
append getbit setbit mget bitcount incr decr incrby decrby incrbyfloat msetnx get set(partial) strlen setrange getrange

###hash
hdel hlen hexists hmget hget hmset hgetall hincrby hset hsetnx hkeys hvals linsert hincrbyfloat

###list
rpop llen lpop  rpush lpush rpushx lpushx lrange rpoplpush lset lindex lrem ltrim brpop brpoplpush blpop

###set 
scard sadd sismember smembers spop srandmember(partial) sdiff srem sunion smove sinter 
