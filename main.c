#include "bss.h"
#include "dict.h"
#include <stdio.h>
#include <stdlib.h>

int main()
{
        bss_t* bss;
        obj_t* obj;
        obj_t* arr[1024];
        dict_t *dict =  dict_create(3);

        bss = malloc(sizeof(bss_t) + 1024);
        for(int i = 0; i < 1000; ++i) {
                bss->len = sprintf(bss->str, "%dsssssssssss%d", i, i) + 1;
                arr[i] = bss_create_obj(bss);
        }
        for(int i = 0; i < 1000; ++i){
                bss->len = sprintf(bss->str, "%d", i) + 1;
                dict_add(dict, bss, arr[i]);
        }

        for(int i = 0; i < 1000; i+=2) {
                bss->len = sprintf(bss->str, "%d", i) + 1;
                dict_rm(dict, bss);
        }

        for(int i = 0; i < 1000; ++i){
                bss->len = sprintf(bss->str, "%d", i) + 1;
                if(NULL != (obj = dict_look_up(dict, bss)))
                        printf("%s\n", ((bss_t *)obj->val)->str);
        }

        dict_destroy(dict);
        free(bss);

        return 0;
}
