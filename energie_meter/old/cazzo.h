#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include "cazzo_msg.h"

#define MAXLINE_READ 64

char       buffer[MAXLINE_READ];
static int buffer_loc = 0;

char* readBytesUntil(char end, char* msg, int length) {
    // if we need to start from value 0 again
    if (length == 0) {
        buffer_loc = 0;
    }

    // for the few last bytes
    if ((length + buffer_loc) > size_msg) {
        length = size_msg - buffer_loc;
    }
    // copy over buffer
    for (int i = 0; i < length; i++) {
        buffer[i] = cazzo_msg[buffer_loc++];
        if (buffer[i] == end) {
            buffer[i + 1] = '\0';
            break;
        }
    }

    return buffer;
}

#include <string.h>


bool isNumber(char* res, int len) {
    for (int i = 0; i < len; i++) {
        if (((res[i] < '0') || (res[i] > '9')) && (res[i] != '.' && res[i] != 0)) {
            return false;
        }
    }
    return true;
}


int FindCharInArrayRev(char array[], char c, int len) {
    for (int i = len - 1; i >= 0; i--) {
        if (array[i] == c) {
            return i;
        }
    }
    return -1;
}

long getValue(char* buffer, int maxlen) {
    int s = FindCharInArrayRev(buffer, '(', maxlen - 2);
    if (s < 8)
        return 0;
    if (s > 32)
        s = 32;
    int l = FindCharInArrayRev(buffer, '*', maxlen - 2) - s - 1;

    // for (int cnt = s; cnt < l; cnt++)
    //   printf(telegram[cnt]);
    //  printfln(' ');
    if (l < 3)
        return 0;
    if (l > 12)
        return 0;
    char res[16];
    memset(res, 0, sizeof(res));

    if (strncpy(res, buffer + s + 1, l)) {
        // printf(res);
        if (isNumber(res, l)) {

            long aap = (1000 * atof(res));

            return aap;
        }
    }
    return 0;
}
