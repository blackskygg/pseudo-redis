#include <stdlib.h>

#include "list.h"

static struct obj_op list_op = {.create = list_create_obj,
                                .destroy = list_destroy_obj,
                                .cpy = NULL,
                                .cmp = NULL};

/* create an empty list */
list_t *list_create()
{
        list_t *result;

        if(NULL == (result = malloc(sizeof(list_t))))
                return NULL;

        result->head.next = result->head.prev = &(result->head);
        result->head.val = NULL;
        result->num = 0;

        return result;
}

/* wrap list to an obj_t */
obj_t *list_create_obj(const void *list)
{
        obj_t *result;

        if(NULL == (result = malloc(sizeof(obj_t))))
                return NULL;

        result->type = LIST;
        result->op = &list_op;
        result->val = (void *)list;

        return result;
}

/* create an entry given val */
list_entry_t *list_create_entry(const bss_t *val)
{
        list_entry_t *result;

        if(NULL == (result = malloc(sizeof(list_entry_t))))
                return NULL;
        result->val = (bss_t *)val;

        return result;
}

/* destroy an entry */
void list_destroy_entry(list_entry_t *entry)
{
        bss_destroy(entry->val);
        free(entry);
}

/* remove an entry but will not destroy it */
list_entry_t *list_rm_nf(list_t *list, list_entry_t *entry)
{
        entry->prev->next = entry->next;
        entry->next->prev = entry->prev;

        list->num--;

        return entry;
}

/* remove an entry and destroy it */
void list_rm(list_t *list, list_entry_t *entry)
{
        entry->prev->next = entry->next;
        entry->next->prev = entry->prev;
        list_destroy_entry(entry);

        list->num--;
}

/* iterate over the list, safely, and perform func() on each entry */
void list_iter(list_t *list, void (*func)(list_entry_t *))
{
        list_entry_t *curr, *next;

        curr = &(list->head);
        next = list->head.next;
        while(next != &(list->head)) {
                curr = next;
                next = curr->next;
                func(curr);
        }
}

/* destroy a list and free all the entries */
void list_destroy(list_t *list) {
        list_iter(list, list_destroy_entry);
        free(list);
}

/* destroy a list obj and free all the entries */
void list_destroy_obj(obj_t *list_obj)
{
        list_destroy(list_obj->val);
        free(list_obj);
}

/* insert an element to the begining of a list */
void list_insert_front(list_t *list, list_entry_t *entry)
{
        list->head.next->prev = entry;
        entry->next = list->head.next;
        list->head.next = entry;
        entry->prev = &(list->head);

        list->num++;
}

/* insert an element to the back of a list */
void list_insert_back(list_t *list, list_entry_t *entry)
{
        list->head.prev->next = entry;
        entry->prev = list->head.prev;
        list->head.prev = entry;
        entry->next = &(list->head);

        list->num++;
}

/* insert target AFTER entry */
void list_insert_after(list_t *list, list_entry_t *entry, list_entry_t *target)
{
        entry->next->prev = target;
        target->next = entry->next;
        entry->next = target;
        target->prev = entry;

        list->num++;
}

/* insert target BEFORE entry */
void list_insert_before(list_t *list, list_entry_t *entry, list_entry_t *target)
{
        entry->prev->next = target;
        target->prev = entry->prev;
        entry->prev = target;
        target->next = entry;

        list->num++;
}

/* remove the first entry in a list and return it
 * returns NULL if the list is empty
 */
list_entry_t* list_pop_front(list_t *list)
{
        if(list->head.next == &(list->head))
                return NULL;

        return list_rm_nf(list, list->head.next);
}

/* remove the last entry in a list and return it
 * returns NULL if the list is empty
 */
list_entry_t* list_pop_back(list_t *list)
{
        if(list->head.next == &(list->head))
                return NULL;

        return list_rm_nf(list, list->head.prev);
}

/* move an entry from one list to the back of another list */
void list_move_back(list_entry_t *entry, list_t *src, list_t *dst)
{
        list_rm_nf(src, entry);
        list_insert_back(dst, entry);
}

/* move an entry from one list to the begining of another list */
void list_move_front(list_entry_t *entry, list_t *src, list_t *dst)
{
        list_rm_nf(src, entry);
        list_insert_front(dst, entry);
}

/* bf means from back to front, so on so forth */
int list_move_bf(list_t *src, list_t *dst)
{
        if(src->head.next == &(src->head))
                return E_EMPTY;

        list_move_front(src->head.prev, src, dst);

        return 0;
}

int list_move_fb(list_t *src, list_t *dst)
{
        if(src->head.next == &(src->head))
                return E_EMPTY;

        list_move_back(src->head.next, src, dst);

        return 0;
}

int list_move_ff(list_t *src, list_t *dst)
{
        if(src->head.next == &(src->head))
                return E_EMPTY;

        list_move_front(src->head.next, src, dst);

        return 0;
}

int list_move_bb(list_t *src, list_t *dst)
{
        if(src->head.next == &(src->head))
                return E_EMPTY;

        list_move_back(src->head.prev, src, dst);

        return 0;
}
