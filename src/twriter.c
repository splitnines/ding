#include <stdio.h>
#include <unistd.h>


void twriter(char *str, size_t usecs) {
    for (int i = 0; str[i] != '\0'; i++) {
        printf("%c", str[i]);
        fflush(stdout);
        usleep(usecs);
    }
}
