#include "bss.h"
#include <stdio.h>
#include <stdlib.h>

int main()
{
        bss_t* bss;
        obj_t* arr[1024];

        bss = malloc(sizeof(bss_t) + 1024);
        for(int i = 0; i < 100; ++i) {
                bss->len = sprintf(bss->str, "%dsssssssssss%d", i, i) + 1;
                arr[i] = bss_create_obj(bss);
        }

        for(int i = 0; i < 40; ++i)
                arr[i] = arr[i]->op->cpy(arr[i], arr[i+40]);
        for(int i = 0; i < 100; ++i){
                printf("%s\n", ((bss_t *)arr[i]->val)->str);
        }

        for(int i = 0; i < 100; ++i)
                arr[i]->op->destroy(arr[i]);

        free(bss);

        return 0;
}
