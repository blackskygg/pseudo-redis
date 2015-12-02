#include "bss.h"
#include "dict.h"
#include <stdio.h>
#include <stdlib.h>

int main()
{
        char s[1024];
        size_t len;
        bss_t* arr[1024];

        for(int i = 0; i < 10; ++i) {
                len = sprintf(s, "%d%d", i, i);
                arr[i] = bss_create(s, len);
        }

        for(int i = 0; i < 10; ++i) {
                bss_decr(arr[i], 1);
                arr[i] = bss_append(arr[i], "fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff", 80);
        }

        for(int i = 0; i < 10; ++i) {
                printf("%s, %d\n", arr[i]->str, bss_count_bit(arr[i]));
        }

        for(int i = 0; i < 10; ++i) {
                bss_destroy(arr[i]);
        }

        return 0;
}
