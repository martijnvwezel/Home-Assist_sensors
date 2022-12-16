#include <stdio.h>
#include "cazzo_msg.h"
#include "cazzo.h"

int mEVLT = 0;
int main(void) {

    int len = 223;
    while (len-- > 0) {
        char* cazzo_line = readBytesUntil('\n', cazzo_msg, 64);
        // printf("%s", cazzo_line);

        // 1-0:1.8.1 = Elektra verbruik laag tarief (DSMR v4.0)
        if (strncmp(cazzo_line, "1-0:1.8.1", strlen("1-0:1.8.1")) == 0)
            mEVLT = getValue(cazzo_line, strlen(cazzo_line));

        if (mEVLT) {
            printf("%d\n", mEVLT);
        }
    }

    return 1;
}