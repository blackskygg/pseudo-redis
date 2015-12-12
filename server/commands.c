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

#define CHECK_ARGS_GE(n) ({                                     \
                        if(num < (n)) {                         \
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
 * for non blocking functions, they have to make up an reply completely
 * for blocking functions, they only report whether they're blocked and
 * the result that they finally get
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
static int addkey_to_arr(const dict_iter_t *dict_iter, void *dummy)
{
        return addto_reply_arr(dict_iter->curr->key, STR_NORMAL);
}

static int addval_to_arr(const dict_iter_t *dict_iter, void *dummy)
{
        return addto_reply_arr((bss_t *)dict_iter->curr->val->val, STR_NORMAL);
}

static int add_key_val_to_arr(const dict_iter_t *dict_iter, void *dummy)
{

        if(0 != addto_reply_arr(dict_iter->curr->key, STR_NORMAL) ||
           0 != addto_reply_arr((bss_t *)dict_iter->curr->val->val, STR_NORMAL))
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
                len = ((bss_t *)val_obj->val)->len;

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

_CMD_PROTO(mget)
{
        CHECK_ARGS_GE(1);

        obj_t *tar_obj;
        int ret;

        reset_reply_arr();
        for(int i = 0; i < num; ++i) {
                tar_obj = dict_look_up(dict, args[i]);

                if((NULL != tar_obj) && (STRING == tar_obj->type))
                        ret = addto_reply_arr(tar_obj->val, STR_NORMAL);
                else
                        ret = addto_reply_arr(NULL, STR_NIL);

                if(ret != 0) {
                        FREE_ARGS(0, num);
                        fail_reply(TOO_LONG);
                }
        }

        FREE_ARGS(0, num);
        arr_reply();
}

_CMD_BI_PROTO(decrby, incrby)
{
        CHECK_ARGS(2);

        obj_t *str_obj;
        bss_int_t count;
        bss_int_t number;
        char str_num[BSS_MIN_LEN];
        size_t num_len;

        /* first check args[1] */
        if(0 != bss2int(args[1], &number))
                goto ERR_INV_INT;


        /* does it exist? */
        str_obj = dict_look_up(dict, args[0]);
        if(NULL == str_obj) {
                /* create a new entry and set it to args[1] */
                if(NEG_OP == type) {
                        num_len = sprintf(str_num, "%d", -number);
                        bss_set(args[1], str_num, num_len);
                }

                CHECK_NULL(str_obj = bss_create_obj(args[1]));
                if(0 != dict_add(dict, args[0], str_obj)) {
                        FREE_ARG(0);
                        FREE_ARG(1);
                        fail_reply(MEM_OUT);
                }

                if(NEG_OP == type)
                        int_reply(-number);
                else
                        int_reply(number);
        } else {
                CHECK_TYPE(str_obj, STRING);

                /* is it a valid number? */
                if(bss2int((bss_t*)(str_obj->val), &count) != 0)
                        goto ERR_INV_INT;
                /* overflow? */
                if(NEG_OP == type) {
                        if(bss_decr((bss_t*)(str_obj->val), number) != 0)
                                goto ERR_INV_INT;
                } else {
                        if(bss_incr((bss_t*)(str_obj->val), number) != 0)
                                goto ERR_INV_INT;

                }

                if(NEG_OP == type)
                        count -= number;
                else
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
        return _decrbyincrby_command(dict, args, num, NEG_OP);
}

_CMD_PROTO(incrby)
{
        return _decrbyincrby_command(dict, args, num, POS_OP);
}

_CMD_PROTO(setnx)
{
        CHECK_ARGS(2);

        obj_t *val_obj;

        if(NULL !=  dict_look_up(dict, args[0])) {
                FREE_ARG(0);
                FREE_ARG(1);
                false_reply();
        } else {
                return _set_command(dict, args, num);
        }

}

CMD_PROTO(setnx)
{
        return _setnx_command(key_dict, args, num);
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
        CHECK_ARGS(0);

        dict_entry_t *entry;

        if(0 == key_dict->entry_num) {
                nil_reply();
        } else {
                entry = dict_random_elem(key_dict);
                string_reply(entry->key->str, entry->key->len);
        }
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
                /* create a new key */
                CHECK_NULL(bss = bss_create_empty(0));
                if(NULL == (t_bss = bss_setbit(bss, offset, bit))){
                        free(bss);
                        goto ERR_MEM_OUT;

                }

                if(NULL == (str_obj = bss_create_obj(t_bss))){
                        free(t_bss);
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

                FREE_ARGS(0, 3);
                int_reply(result);
        }

ERR_MEM_OUT:
        FREE_ARGS(0, 3);
        fail_reply(MEM_OUT);
}

CMD_PROTO(mget)
{
        return _mget_command(key_dict, args, num);
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
        CHECK_ARGS(3);

        dict_iter_t iter;
        obj_t *str_obj;
        bss_t *bss, *t_bss;
        bss_int_t offset;

        /* check the offset */
        if(0 != bss2int(args[1], &offset)) {
                FREE_ARGS(0, num);
                fail_reply(INV_OFFSET);
        }

        iter = _dict_look_up(key_dict, args[0]);
        if(NULL == iter.curr) {
                CHECK_NULL(bss = bss_create_empty(0));
                if(NULL == (t_bss = bss_setrange(bss, offset,
                                                 args[2]->str, args[2]->len)))
                {
                        free(bss);
                        goto ERR_MEM_OUT;
                }

                if(NULL == (str_obj = bss_create_obj(t_bss))) {
                        free(t_bss);
                        goto ERR_MEM_OUT;
                }

                if(0 != dict_add(key_dict, args[0], str_obj)) {
                        free(t_bss);
                        free(str_obj);
                        goto ERR_MEM_OUT;

                }

                FREE_ARG(1);
                FREE_ARG(2);
                int_reply(t_bss->len);
        } else {
                str_obj = iter.curr->val;
                CHECK_TYPE(str_obj, STRING);
                bss = str_obj->val;

                if(NULL == (t_bss = bss_setrange(bss, offset,
                                                 args[2]->str, args[2]->len)))
                        goto ERR_MEM_OUT;

                FREE_ARG(0);
                FREE_ARG(1);
                str_obj->val = t_bss;
                int_reply(t_bss->len);
        }

ERR_MEM_OUT:
        FREE_ARGS(0, 3);
        fail_reply(MEM_OUT);
}

CMD_PROTO(getrange)
{
        CHECK_ARGS(3);

        obj_t *str_obj;
        bss_t *bss;
        bss_int_t offset1, offset2;
        size_t len;

        /* obtain the two offsets */
        if(0 != bss2int(args[1], &offset1)
           || 0 != bss2int(args[2], &offset2)) {
                FREE_ARGS(0, num);
                fail_reply(INV_INT);
        }

        if(NULL == (str_obj = dict_look_up(key_dict, args[0]))) {
                FREE_ARGS(0, num);
                string_reply(NULL, 0);
        } else {
                CHECK_TYPE(str_obj, STRING);
                bss = str_obj->val;

                /* check empty */
                if(0 == bss->len) {
                        FREE_ARGS(0, num);
                        string_reply(NULL, 0);
                }

                /* regularize the offsets */
                if(offset1 < 0)
                        offset1 = offset1 + (bss_int_t)bss->len;
                if(offset1 < 0)
                        offset1 = 0;
                if(offset2 < 0)
                        offset2 = offset2 + (bss_int_t)bss->len;

                /* check the range */
                if(offset2 < offset1 || offset1 >= bss->len) {
                        FREE_ARGS(0, num);
                        string_reply(NULL, 0);
                }

                len = offset2 > ((bss_int_t)bss->len - 1)
                        ? (bss_int_t)bss->len  - offset1
                        : offset2 - offset1 + 1;

                FREE_ARGS(0, num);
                string_reply(bss->str + offset1, len);
        }
}

CMD_PROTO(incr)
{
        CHECK_ARGS(1);

        bss_t *bss = bss_create("1", 1);
        args[num++] = bss;

        return incrby_command(args, num);
}

CMD_PROTO(decr)
{
        CHECK_ARGS(1);

        bss_t *bss = bss_create("1", 1);
        args[num++] = bss;

        return decrby_command(args, num);
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
        if((num < 2) || (num % 2 != 0)) {
                FREE_ARGS(0, num);
                fail_reply(NUM_ARGS);
        }

        /* check if any of the keys exists */
        for(int i = 0; i < num; i+=2) {
                if(NULL != dict_look_up(key_dict, args[i])) {
                        FREE_ARGS(0, num);
                        false_reply();
                }
        }

        int ret;
        for(int i = 0; i < num; i+=2) {
                /* set for each entry and check if it succeeded */
                ret = _set_command(key_dict, args + i, 2);

                /* error occurred, clean up and report the error */
                if(_curr_reply->reply_type != RPLY_OK || 0 != ret) {
                        FREE_ARGS(i + 2, num);
                        return ret;
                }
        }

        /* no clean up is needed, safely return */
        true_reply();
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
        CHECK_ARGS_GE(2);

        obj_t *val_obj, *tar_obj;
        dict_t *dict;
        int ret;

        /* all nil? */
        if(NULL == (val_obj = dict_look_up(key_dict, args[0]))) {
                FREE_ARGS(0, num);

                reset_reply_arr();
                for(int i = 1; i < num; ++i) {
                        if(0 != (ret = addto_reply_arr(NULL, STR_NIL))) {
                                fail_reply(TOO_LONG);
                        }
                }
                arr_reply();
        } else {
                CHECK_TYPE(val_obj, HASH);
                dict = val_obj->val;
                FREE_ARG(0);

                return _mget_command(dict, args + 1, num - 1);
        }
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


CMD_PROTO(hmset)
{
        if((num < 3) || (num % 2 != 1)) {
                FREE_ARGS(0, num);
                fail_reply(NUM_ARGS);
        }

        obj_t *val_obj;
        dict_t *dict;
        int ret;

        /* if it dose not exist, create one */
        if(NULL == (val_obj = dict_look_up(key_dict, args[0]))) {
                dict = dict_create(NEW_DICT_POW);
                val_obj = dict_create_obj(dict);
                dict_add(key_dict, args[0], val_obj);
        } else {

                dict = val_obj->val;
                CHECK_TYPE(val_obj, HASH);
                FREE_ARG(0);
        }

        /* now we've got enough information, let's roll */
        for(int i = 1; i < num; i+=2) {
                /* set for each entry and check if it succeeded */
                ret = _set_command((dict_t *)val_obj->val, args + i, 2);

                /* error occurred, clean up and report the error */
                if(_curr_reply->reply_type != RPLY_OK || 0 != ret) {
                        FREE_ARGS(i + 2, num);
                        return ret;
                }
        }

        /* no clean up is needed, safely return */
        return ret;
}

CMD_PROTO(hincrbyfloat)
{
}

CMD_PROTO(hsetnx)
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
                if(NULL == dict_look_up((dict_t *)val_obj->val, args[1])) {
                        return _set_command((dict_t *)val_obj->val,
                                            args + 1, num - 1);
                } else {
                        false_reply();
                }
        }

}

CMD_PROTO(hkeys)
{
        CHECK_ARGS(1);

        obj_t *val_obj;

        reset_reply_arr();
        if(NULL == (val_obj = dict_look_up(key_dict, args[0])))
                arr_reply();

        CHECK_TYPE(val_obj, HASH);

        FREE_ARG(0);
        if(0 != dict_iter((dict_t *)val_obj->val, addkey_to_arr, NULL))
                fail_reply(TOO_LONG);
        else
                arr_reply();
}

CMD_PROTO(hvals)
{
        CHECK_ARGS(1);

        obj_t *val_obj;

        reset_reply_arr();
        if(NULL == (val_obj = dict_look_up(key_dict, args[0])))
                arr_reply();

        CHECK_TYPE(val_obj, HASH);

        FREE_ARG(0);
        if(0 != dict_iter((dict_t *)val_obj->val, addval_to_arr, NULL))
                fail_reply(TOO_LONG);
        else
                arr_reply();

}

_CMD_BI_PROTO(blpop, brpop)
{
        CHECK_ARGS_GE(1);

        int flag;  /* blocked? */
        dict_iter_t iter;
        obj_t *list_obj;
        list_t *list;
        list_entry_t *entry;
        size_t i;

        /* look around for available food */
        flag = 0;
        for(i = 0; i < num; ++i) {
                iter = _dict_look_up(key_dict, args[i]);
                if(NULL != iter.curr) {
                        list_obj = iter.curr->val;
                        CHECK_TYPE(list_obj, LIST);

                        /* got an entry available */
                        list = list_obj->val;
                        if(LEFT_OP == type)
                                entry = list_pop_front(list);
                        else
                                entry = list_pop_back(list);

                        /* if the list is empty, end it's life */
                        if(0 == list->num)
                                dict_rm(key_dict, args[0]);


                        flag = 1;
                        break;
                }
        }

        reset_reply_arr();
        if(flag) {
                /* ok, fill the information and let the caller
                 * wrap all the things up
                 */
                addto_reply_arr(args[0], STR_NI | NEED_FREE);
                addto_reply_arr(entry->val, STR_NI | NEED_FREE);

                FREE_ARGS(1, num);
                return 0;
        } else {
                FREE_ARGS(0, num);
                return E_BLOCKED;
        }

}

CMD_PROTO(blpop)
{
        return _blpopbrpop_command(NULL, args, num, LEFT_OP);
}

CMD_PROTO(brpop)
{
        return _blpopbrpop_command(NULL, args, num, RIGHT_OP);
}

CMD_PROTO(lrange)
{
        CHECK_ARGS(3);

        obj_t *list_obj;
        list_t *list;
        list_entry_t *entry;
        bss_int_t offset1, offset2;

        /* obtain the two offsets */
        if(0 != bss2int(args[1], &offset1)
           || 0 != bss2int(args[2], &offset2)) {
                FREE_ARGS(0, num);
                fail_reply(INV_INT);
        }

        reset_reply_arr();
        if(NULL == (list_obj = dict_look_up(key_dict, args[0]))) {
                FREE_ARGS(0, num);
                arr_reply();
        } else {
                CHECK_TYPE(list_obj, LIST);
                list = list_obj->val;

                /* regularize the offsets */
                if(offset1 < 0)
                        offset1 = offset1 + list->num;
                if(offset1 < 0)
                        offset1 = 0;
                if(offset2 < 0)
                        offset2 = offset2 + list->num;

                /* check the range */
                if(offset2 < offset1 || offset1 >= list->num) {
                        FREE_ARGS(0, num);
                        arr_reply();
                }

                /* find the starting point */
                entry = &(list->head);
                for(size_t i = 0; i <= offset1; ++i)
                        entry = entry->next;

                for(size_t i = offset1; i <= offset2 && i < list->num; ++i)
                {
                        addto_reply_arr(entry->val, STR_NORMAL);
                        entry = entry->next;
                }

                FREE_ARGS(0, num);
                arr_reply();
        }
}


static int32_t _lrem(list_t *list, bss_int_t target, bss_t* bss)
{
        list_entry_t *entry, *next;
        int32_t count;

        count = 0;
        if(target > 0) {
                entry = &(list->head);
                next = entry->next;
                while(next != &(list->head)) {
                        entry = next;
                        next = entry->next;

                        if(!bss_cmp(entry->val, bss)
                           && count < target) {
                                list_rm(list, entry);

                                if(++count == target)
                                        break;
                        }

                }
        } else if (0 == target) {
                entry = &(list->head);
                next = entry->next;
                while(next != &(list->head)) {
                        entry = next;
                        next = entry->next;

                        if(!bss_cmp(entry->val, bss)) {
                                list_rm(list, entry);
                                count++;
                        }
                }
        } else {
                target = -target;

                entry = &(list->head);
                next = entry->prev;
                while(next != &(list->head)) {
                        entry = next;
                        next = entry->prev;

                        if(!bss_cmp(entry->val, bss)
                           && count < target) {
                                list_rm(list, entry);

                                if(++count == target)
                                        break;
                        }
                }

        }

        return count;
}

CMD_PROTO(lrem)
{
        CHECK_ARGS(3);

        obj_t *list_obj;
        list_t *list;
        bss_int_t target;
        int32_t count;

        if(0 != bss2int(args[1], &target)) {
                FREE_ARGS(0, 3);
                fail_reply(INV_INT);
        }

        if(NULL == (list_obj = dict_look_up(key_dict, args[0]))) {
                FREE_ARGS(0, 3);
                false_reply();
        } else {
                CHECK_TYPE(list_obj, LIST);
                list = list_obj->val;

                count = _lrem(list, target, args[2]);

                /* if the list is empty, end it's life */
                if(0 == list->num)
                        dict_rm(key_dict, args[0]);


                FREE_ARGS(0, 3);
                int_reply(count);
        }
}

CMD_PROTO(brpoplpush)
{
        CHECK_ARGS(2);

        obj_t *list_obj1, *list_obj2;
        list_t *list1, *list2;
        list_entry_t *entry;

        /* rpop */
        if(NULL == (list_obj1 = dict_look_up(key_dict, args[0]))) {
                FREE_ARG(0);
                FREE_ARG(1);
                return E_BLOCKED;
        } else {
                CHECK_TYPE(list_obj1, LIST);
                list1 = list_obj1->val;

                entry = list_pop_back(list1);

                /* if the list is empty, end it's life */
                if(0 == list1->num)
                        dict_rm(key_dict, args[0]);

        }
        FREE_ARG(0);

        /* lpush */
        if(NULL == (list_obj2 = dict_look_up(key_dict, args[1]))) {
                /* list dose not exist, create one */
                list2 = list_create();
                list_obj2 = list_create_obj(list2);
                dict_add(key_dict, args[1], list_obj2);

                list_insert_front(list2, entry);
        } else {
                CHECK_TYPE(list_obj2, LIST);
                list2 = list_obj2->val;

                list_insert_front(list2, entry);

                FREE_ARG(1);
        }

        reset_reply_arr();
        addto_reply_arr(entry->val, STR_NI);

}

CMD_PROTO(lset)
{
        CHECK_ARGS(3);

        obj_t *list_obj;
        list_t *list;
        list_entry_t *entry;
        bss_int_t index;

        if(0 != bss2int(args[1], &index)) {
                FREE_ARGS(0, 3);
                fail_reply(INV_INT);
        }

        if(NULL == (list_obj = dict_look_up(key_dict, args[0]))) {
                goto NIL;
        } else {
                CHECK_TYPE(list_obj, LIST);
                list = list_obj->val;

                if(index < 0)
                        index = index + list->num;

                /* is it out of the scope? */
                if(index >= list->num || index < 0)
                        goto NIL;

                entry = &(list->head);
                for(int i = 0; i <= index; ++i)
                        entry = entry->next;

                /* replace it with args[2] */
                bss_destroy(entry->val);
                entry->val = args[2];

                FREE_ARG(0);
                FREE_ARG(1);
                string_reply(entry->val->str, entry->val->len);
        }

NIL:
        FREE_ARGS(0, 3);
        nil_reply();
}

CMD_PROTO(lindex)
{
        CHECK_ARGS(2);

        obj_t *list_obj;
        list_t *list;
        list_entry_t *entry;
        bss_int_t index;

        if(0 != bss2int(args[1], &index)) {
                FREE_ARG(0);
                FREE_ARG(1);

                fail_reply(INV_INT);
        }

        if(NULL == (list_obj = dict_look_up(key_dict, args[0]))) {
                goto NIL;
        } else {
                CHECK_TYPE(list_obj, LIST);
                list = list_obj->val;

                if(index < 0)
                        index = index + list->num;

                /* is it out of the scope? */
                if(index >= list->num || index < 0)
                        goto NIL;

                entry = &(list->head);
                for(int i = 0; i <= index; ++i)
                        entry = entry->next;

                FREE_ARG(0);
                FREE_ARG(1);
                string_reply(entry->val->str, entry->val->len);
        }

NIL:
        FREE_ARG(0);
        FREE_ARG(1);
        nil_reply();
}

CMD_PROTO(ltrim)
{
        CHECK_ARGS(3);

        obj_t *list_obj;
        list_t *list, *new_list;
        list_entry_t *entry, *next;
        bss_int_t offset1, offset2;

        /* obtain the two offsets */
        if(0 != bss2int(args[1], &offset1)
           || 0 != bss2int(args[2], &offset2)) {
                FREE_ARGS(0, num);
                fail_reply(INV_INT);
        }

        if(NULL == (list_obj = dict_look_up(key_dict, args[0]))) {
                FREE_ARGS(0, num);
                ok_reply();
        } else {
                CHECK_TYPE(list_obj, LIST);
                list = list_obj->val;

                /* regularize the offsets */
                if(offset1 < 0)
                        offset1 = offset1 + list->num;
                if(offset1 < 0)
                        offset1 = 0;
                if(offset2 < 0)
                        offset2 = offset2 + list->num;

                /* check the range */
                if(offset2 < offset1 || offset1 >= list->num) {
                        dict_rm(key_dict, args[0]);
                        FREE_ARGS(0, num);
                        ok_reply();
                }

                /* find the starting point */
                entry = &(list->head);
                for(size_t i = 0; i <= offset1; ++i)
                        entry = entry->next;

                /* fill the new list with elements within the range*/
                new_list = list_create();
                while(entry != &(list->head)) {
                        next = entry->next;

                        list_move_back(entry, list, new_list);
                        if(offset1++ == offset2)
                                break;

                        entry = next;
                }

                /* replace the original list with the new list */
                list_destroy(list);
                list_obj->val = new_list;

                FREE_ARGS(0, num);
                ok_reply();
        }

}

CMD_PROTO(linsert)
{
        CHECK_ARGS(4);

        obj_t *list_obj;
        list_t *list;
        list_entry_t *entry, *new_entry;
        int flag;
        int direction; /* 0 means before and 1 after */

        if(!strcasecmp(args[1]->str, "before")) {
                direction = 0;
        } else if(!strcasecmp(args[1]->str, "after")) {
                direction = 1;
        } else {
                /* a syntax error */
                FREE_ARGS(0, 4);
                fail_reply(INV_SYNX);
        }

        if(NULL == (list_obj = dict_look_up(key_dict, args[0]))) {
                FREE_ARG(0);
                false_reply();
        } else {
                CHECK_TYPE(list_obj, LIST);
                list = list_obj->val;

                flag = 0;
                entry = list->head.next;
                while(entry != &(list->head)) {
                        if(!bss_cmp(entry->val, args[2])) {
                                flag = 1;
                                break;
                        }
                        entry = entry->next;
                }

                if(!flag) {
                        /* not found */
                        FREE_ARGS(0, 4);
                        int_reply(-1);
                } else {
                        new_entry = list_create_entry(args[3]);

                        /* insert it */
                        if(direction)
                                list_insert_after(list, entry, new_entry);
                        else
                                list_insert_before(list, entry, new_entry);

                        FREE_ARGS(0, 3); /* args[3] can't be freed */
                        int_reply(list->num);
                }
        }
}

_CMD_BI_PROTO(lpop, rpop)
{
        CHECK_ARGS(1);

        obj_t *list_obj;
        list_t *list;
        list_entry_t *entry;
        int ret_val;

        if(NULL == (list_obj = dict_look_up(key_dict, args[0]))) {
                FREE_ARG(0);
                nil_reply();
        } else {
                CHECK_TYPE(list_obj, LIST);
                list = list_obj->val;

                if(LEFT_OP == type)
                        entry = list_pop_front(list);
                else
                        entry = list_pop_back(list);

                /* if the list is empty, end it's life */
                if(0 == list->num)
                        dict_rm(key_dict, args[0]);

                FREE_ARG(0);
                ret_val = create_str_reply(entry->val->str,
                                           entry->val->len, RPLY_STRING);
                list_destroy_entry(entry);

                return ret_val;
        }
}

CMD_PROTO(lpop)
{
        _lpoprpop_command(NULL, args, num, LEFT_OP);
}

CMD_PROTO(rpop)
{
        _lpoprpop_command(NULL, args, num, RIGHT_OP);
}

CMD_PROTO(llen)
{
        CHECK_ARGS(1);

        obj_t *list_obj;
        list_t *list;

        if(NULL == (list_obj = dict_look_up(key_dict, args[0]))) {
                FREE_ARG(0);
                false_reply();
        } else {
                CHECK_TYPE(list_obj, LIST);
                list = list_obj->val;
                int_reply(list->num);
        }
}

CMD_PROTO(rpoplpush)
{
        CHECK_ARGS(2);

        obj_t *list_obj1, *list_obj2;
        list_t *list1, *list2;
        list_entry_t *entry;

        /* rpop */
        if(NULL == (list_obj1 = dict_look_up(key_dict, args[0]))) {
                FREE_ARG(0);
                FREE_ARG(1);
                nil_reply();
        } else {
                CHECK_TYPE(list_obj1, LIST);
                list1 = list_obj1->val;

                entry = list_pop_back(list1);

                /* if the list is empty, end it's life */
                if(0 == list1->num)
                        dict_rm(key_dict, args[0]);

        }
        FREE_ARG(0);

        /* lpush */
        if(NULL == (list_obj2 = dict_look_up(key_dict, args[1]))) {
                /* list dose not exist, create one */
                list2 = list_create();
                list_obj2 = list_create_obj(list2);
                dict_add(key_dict, args[1], list_obj2);

                list_insert_front(list2, entry);
        } else {
                CHECK_TYPE(list_obj2, LIST);
                list2 = list_obj2->val;

                list_insert_front(list2, entry);

                FREE_ARG(1);
        }

        string_reply(entry->val->str, entry->val->len);
}

_CMD_BI_PROTO(lpush, rpush)
{
        CHECK_ARGS_GE(2);

        obj_t *list_obj;
        list_t *list;
        list_entry_t *entry;

        if(NULL == (list_obj = dict_look_up(key_dict, args[0]))) {
                /* list dose not exist, create one */
                list = list_create();
                list_obj = list_create_obj(list);
                dict_add(key_dict, args[0], list_obj);
        } else {
                CHECK_TYPE(list_obj, LIST);
                list = list_obj->val;
                FREE_ARG(0);
        }

        for(int i = 1; i < num; ++i) {
                entry = list_create_entry(args[i]);
                if(LEFT_OP == type)
                        list_insert_front(list, entry);
                else
                        list_insert_back(list, entry);
        }

        int_reply(list->num);

}
CMD_PROTO(rpush)
{
        _lpushrpush_command(NULL, args, num, RIGHT_OP);
}

CMD_PROTO(lpush)
{
        _lpushrpush_command(NULL, args, num, LEFT_OP);
}

_CMD_BI_PROTO(lpushx, rpushx)
{
        CHECK_ARGS(2);

        obj_t *list_obj;
        list_t *list;
        list_entry_t *entry;

        if(NULL == (list_obj = dict_look_up(key_dict, args[0]))) {
                FREE_ARG(0);
                FREE_ARG(1);

                int_reply(0);
        } else {
                CHECK_TYPE(list_obj, LIST);
                list = list_obj->val;

                entry = list_create_entry(args[1]);
                if(LEFT_OP == type)
                        list_insert_front(list, entry);
                else
                        list_insert_back(list, entry);

                FREE_ARG(0);
                int_reply(list->num);
        }

}
CMD_PROTO(rpushx)
{
        _lpushrpush_command(NULL, args, num, RIGHT_OP);
}

CMD_PROTO(lpushx)
{
        _lpushrpush_command(NULL, args, num, LEFT_OP);
}

CMD_PROTO(sadd)
{
        CHECK_ARGS_GE(2);

        obj_t *set_obj;
        dict_t *set;
        size_t count;

        if(NULL == (set_obj = dict_look_up(key_dict, args[0]))) {
                /* create a new set */
                set = dict_create(NEW_DICT_POW);
                set_obj = set_create_obj(set);
                dict_add(key_dict, args[0], set_obj);
        } else {
                CHECK_TYPE(set_obj, SET);
                set = set_obj->val;
                FREE_ARG(0);
        }

        count = 0;
        for(int i = 1; i < num; ++i) {
                if(NULL == dict_look_up(set, args[i])) {
                        set_add(set, args[i]);
                        count++;
                } else {
                        FREE_ARG(i);
                }
        }

        int_reply(count);
}

CMD_PROTO(smove)
{
}

CMD_PROTO(scard)
{
        CHECK_ARGS(1);

        obj_t *val_obj;

        if(NULL == (val_obj = dict_look_up(key_dict, args[0]))) {
                FREE_ARG(0);
                false_reply();
        } else {
                CHECK_TYPE(val_obj, SET);
                FREE_ARG(0);
                int_reply(((dict_t *)val_obj->val)->entry_num);
        }
}

CMD_PROTO(spop)
{
}

CMD_PROTO(sdiff)
{
}

CMD_PROTO(srandmember)
{
        if(1 != num &&  2 != num) {
                FREE_ARGS(0, num);
                fail_reply(INV_ARG);
        }

        obj_t *set_obj;
        bss_int_t count;
        dict_entry_t *entry;

        if(bss2int(args[1], &count)) {
                FREE_ARGS(0, num);
                fail_reply(INV_INT);
        }

        if(2 == num) {
                if(bss2int(args[1], &count)) {
                        FREE_ARGS(0, num);
                        fail_reply(INV_INT);
                }
        }

        reset_reply_arr();
        if(NULL == (set_obj = dict_look_up(key_dict, args[0]))) {
                FREE_ARGS(0, num);
                if(2 == num)
                        arr_reply();
                else
                        nil_reply();
        }

        CHECK_TYPE(set_obj, SET);
        if(2 == num) {

        }
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
        CHECK_ARGS(2);

        obj_t *set_obj;
        dict_t *set;

        if(NULL == (set_obj = dict_look_up(key_dict, args[0]))) {
                FREE_ARG(0);
                FREE_ARG(1);
                false_reply();
        } else {
                CHECK_TYPE(set_obj, SET);
                set = set_obj->val;
                FREE_ARG(0);

                if(NULL == dict_look_up(set, args[1])) {
                        FREE_ARG(1);
                        false_reply();
                } else {
                        FREE_ARG(1);
                        true_reply();
                }

        }
}

int smembers_callback(const dict_iter_t *dict_iter, void *data)
{
        addto_reply_arr(dict_iter->curr->key, STR_NORMAL);
}

CMD_PROTO(smembers)
{
        CHECK_ARGS(1);

        obj_t *set_obj;
        dict_t *set;

        reset_reply_arr();
        if(NULL == (set_obj = dict_look_up(key_dict, args[0]))) {
                FREE_ARG(0);
        } else {
                CHECK_TYPE(set_obj, SET);
                set = set_obj->val;
                FREE_ARG(0);

                dict_iter(set, smembers_callback, NULL);
        }

        arr_reply();
}

