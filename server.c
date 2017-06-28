#include <stdio.h>
#include <stdlib.h>
#include "sessionman.h"
#include "mylib.h"

int main(int argc, char const *argv[]) {
    int num;
    int soc;
    int maxfd;

    num = 2; // player count

    if ((soc = mserver_socket(PORT, num)) == -1) {
        fprintf(stderr, "cannot setup server\n");
        exit(1);
    }

    maxfd = mserver_maccept(soc, num, enter);

    sessionman_init(num, maxfd);

    sessionman_loop();

    return 0;
}
