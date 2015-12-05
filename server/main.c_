#include "bss.h"
#include "dict.h"
#include "list.h"
#include <stdio.h>
#include <stdlib.h>

void print_entry(list_entry_t *entry)
{
        printf("%s\n", entry->val->str);
}

int main()
{
        char s[1024];
        size_t len;
        bss_t *bss;
        list_entry_t *entry;
        list_t *list = list_create();
        list_t *list2 = list_create();

        for(int i = 0; i < 10; ++i) {
                len = sprintf(s, "%dssssssssssss%d", i, i);
                bss = bss_create(s, len);
                entry = list_create_entry(bss);
                list_insert_back(list, entry);
        }

        while(!list_move_fb(list, list2));

        list_iter(list2, print_entry);
        list_iter(list, print_entry);

        list_destroy(list);
        list_destroy(list2);

        return 0;
}
