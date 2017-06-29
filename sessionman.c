#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

#define MAX_ATTENDANTS 5
#define BUF_LEN        80

static char buf[BUF_LEN];
static fd_set mask;
static int width;
static int attendants;

typedef struct {
    int fd;
    // char name[16];
} ATTENDANT;

static ATTENDANT p[MAX_ATTENDANTS];

static void send_all(int i, int n);
static void ending();

void enter(int i, int fd)
{
    int len;
    static char *mesg = "Wait.\n";

    p[i].fd = fd;

    // Send "Wait." to player who is first entered room.
    if (i == 0) {
        write(fd, mesg, strlen(mesg));
    }
}

void sessionman_init(int num, int maxfd)
{
    int i;
    // static char *mesg = "Game Start.\n";
    char message[20];
    attendants = num;

    width = maxfd + 1;
    FD_ZERO(&mask);
    FD_SET(0, &mask);
    for (i = 0; i < num; i++) {
        FD_SET(p[i].fd, &mask);
    }

    for (i = 0; i < num; i++) {
        sprintf(message, ":%d Game Start.\n", i);
        write(p[i].fd, message, strlen(message));
    }
}

void sessionman_loop()
{
    fd_set readOk;
    int i;

    while (1) {
        readOk = mask;
        select(width, (fd_set *)&readOk, NULL, NULL, NULL);

        // Is there are input from keyboard?
        if (FD_ISSET(0, &readOk)) {
            ending();
        }

        for (i = 0; i < attendants; i++) {
            if (FD_ISSET(p[i].fd, &readOk)) {
                int n;
                n = read(p[i].fd, buf, BUF_LEN);
                send_all(i, n);
            }
        }
    }
}

// Sub routine

static void ending()
{
    int i;
    for (i = 0; i < attendants; i++) {
        write(p[i].fd, "q", 1);
    }
    for (i = 0; i < attendants; i++) {
        close(p[i].fd);
    }
    exit(0);
}

static void send_all(int i, int n)
{
    int j;
    for (j = 0; j < attendants; j++) {
        write(p[j].fd, buf, n);
    }
}
