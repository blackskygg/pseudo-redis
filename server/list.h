#ifndef LIST_H_
#define LIST_H_

#include "obj.h"
#include "bss.h"
#include "err.h"

struct list_entry {
        struct list_entry *next;
        struct list_entry *prev;
        bss_t *val;
};

typedef struct list_entry list_entry_t;

struct list {
        list_entry_t head;
        size_t num;
};
typedef struct list list_t;

/* list APIs */
list_t *list_create();
void list_destroy(list_t *list);
list_entry_t *list_create_entry(const bss_t *val);
void list_iter(list_t *list, void (*func)(list_entry_t *));
void list_destroy_entry(list_entry_t *entry);
void list_insert_front(list_t *list, list_entry_t *entry);
void list_insert_back(list_t *list, list_entry_t *entry);
void list_insert_after(list_t *list, list_entry_t *entry, list_entry_t *target);
void list_insert_before(list_t *list, list_entry_t *entry, list_entry_t *target);
list_entry_t *list_rm_nf(list_t *list, list_entry_t *entry);
void list_rm(list_t *list, list_entry_t *entry);
list_entry_t* list_pop_back(list_t *list);
list_entry_t* list_pop_front(list_t *list);
void list_move_back(list_entry_t *entry, list_t *src, list_t *dst);
void list_move_front(list_entry_t *entry, list_t *src, list_t *dst);
//int list_move_bf(list_t *src, list_t *dst);
//int list_move_fb(list_t *src, list_t *dst);
//int list_move_ff(list_t *src, list_t *dst);
//int list_move_bb(list_t *src, list_t *dst);

#define list_is_empty(list_ptr) (list_ptr->head.next == &(list_ptr->head))


/* list obj layer APIs */
obj_t *list_create_obj(const void *list);
void list_destroy_obj(obj_t *list_obj);

#endif
