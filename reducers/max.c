#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
        char *max_key = NULL;
        long long max_val = 0;

        while (!feof(stdin)) {
                getc(stdin); /* read dummy symbol */
                if (feof(stdin))
                        break;

                int key_len = 0;
                scanf("%d", &key_len);
                getc(stdin); /* read dummy symbol */

                char *key = (char *) malloc(key_len + 1);
                fread(key, key_len, 1, stdin);
                key[key_len] = '\0';

                int num_vals = 0;
                scanf("%d", &num_vals);
                getc(stdin); /* read dummy symbol */
                
                long long sum = 0;

                for (int i=0; i<num_vals; i++) {
                        int len, val;
                        scanf("%d", &len);
                        getc(stdin); /* read dummy symbol */
                        char dbuf[1024];
                        fread(dbuf, len, 1, stdin);
                        dbuf[len] = 0;
                        sscanf(dbuf, "%d", &val);
                        sum += val;
                }

                if (sum > max_val) {
                        if (max_key != NULL)
                                free(max_key);
                        max_val = sum;
                        max_key = key;
                } else {
                        free(key);
                }
                sum = 0;
        }

        if (max_key != NULL) {
                printf("%s %lld\n", max_key, max_val);
                free(max_key);
        }

        return 0;
}
