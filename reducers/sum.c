#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
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

                printf("%s %d\n", key, sum);
                sum = 0;
                free(key);
        }

        return 0;
}
