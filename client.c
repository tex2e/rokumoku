#include "session.h"

int main(int argc, char const *argv[]) {
    int soc;
    char hostname[HOSTNAME_LENGTH];

    printf("Input sever's hostname: ");
    fgets(hostname, HOSTNAME_LENGTH, stdin);
    chop_newline(hostname, HOSTNAME_LENGTH);

    if ((soc = setup_client(hostname, PORT)) == -1) {
        exit(1);
    }

    session_init(soc);

    session_loop();

    return 0;
}
