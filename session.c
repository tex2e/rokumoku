#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <ncurses.h>

#define BUF_LEN 80

#define SEND_WIN_WIDTH  60
#define SEND_WIN_HEIGHT 1

#define RECV_WIN_WIDTH  60
#define RECV_WIN_HEIGHT 13

#define GOBAN_SCREEN_HEIGHT 20
#define GOBAN_SCREEN_WIDTH  40

static char goban_my_stone = 'o';
static char goban_peer_stone = 'x';

static char goban_plane[GOBAN_SCREEN_HEIGHT][GOBAN_SCREEN_WIDTH] = {
    ". . . . . . . . . . . . . . . . . . . . ",
    ". . . . . . . . . . . . . . . . . . . . ",
    ". . . . . . . . . . . . . . . . . . . . ",
    ". . . . . . . . . . . . . . . . . . . . ",
    ". . . . . . . . . . . . . . . . . . . . ",
    ". . . . . . . . . . . . . . . . . . . . ",
    ". . . . . . . . . . . . . . . . . . . . ",
    ". . . . . . . . . . . . . . . . . . . . ",
    ". . . . . . . . . . . . . . . . . . . . ",
    ". . . . . . . . . . . . . . . . . . . . ",
    ". . . . . . . . . . . . . . . . . . . . ",
    ". . . . . . . . . . . . . . . . . . . . ",
    ". . . . . . . . . . . . . . . . . . . . ",
    ". . . . . . . . . . . . . . . . . . . . ",
    ". . . . . . . . . . . . . . . . . . . . ",
    ". . . . . . . . . . . . . . . . . . . . ",
    ". . . . . . . . . . . . . . . . . . . . ",
    ". . . . . . . . . . . . . . . . . . . . ",
    ". . . . . . . . . . . . . . . . . . . . ",
    ". . . . . . . . . . . . . . . . . . . . "
};

static WINDOW *win_send, *win_recv;
static WINDOW *frame_send, *frame_recv;

static char send_buf[BUF_LEN];
static char recv_buf[BUF_LEN];
static int session_soc;
static fd_set mask;
static int width;

static void die();

void session_init(int soc)
{
    session_soc = soc;
    width = soc + 1;
    FD_ZERO(&mask);
    FD_SET(0, &mask);
    FD_SET(soc, &mask);

    initscr();
    signal(SIGINT, die);

    // frame_send = newwin(SEND_WIN_HEIGHT + 2, SEND_WIN_WIDTH + 2, 21, 0);
    win_send = newwin(SEND_WIN_HEIGHT, SEND_WIN_WIDTH, 22, 1);
    // box(frame_send, '|', '-');
    scrollok(win_send, FALSE);
    wmove(win_send, 0, 0);

    frame_recv = newwin(GOBAN_SCREEN_HEIGHT + 2, GOBAN_SCREEN_WIDTH + 2, 0, 0);
    win_recv = newwin(GOBAN_SCREEN_HEIGHT, GOBAN_SCREEN_WIDTH, 1, 1);
    box(frame_recv, '|', '-');
    scrollok(win_recv, FALSE);
    wmove(win_recv, 0, 0);

    cbreak();
    noecho();

    wrefresh(frame_send);
    wrefresh(win_send);
    wrefresh(frame_recv);
    wrefresh(win_recv);
}

void session_loop()
{
    int c;
    int flag = 1;
    fd_set readOk;
    int len = 0;
    int i;
    int y, x;
    int n;

    while (1) {
        readOk = mask;
        select(width, (fd_set *)&readOk, NULL, NULL, NULL);

        if (FD_ISSET(0, &readOk)) {
            // c = getchar();
            //
            // // backspace
            // if (c == '\b' || c == 0x10 || c == 0x7F) {
            //     if (len == 0) {
            //         continue;
            //     }
            //     len--;
            //     getyx(win_send, y, x);
            //     wmove(win_send, y, x-1);
            //     waddch(win_send, ' ');
            //     wmove(win_send, y, x-1);
            // }
            // else if (c == '\n' || c == '\r') {
            //     send_buf[len++] = '\n';
            //     write(session_soc, send_buf, len);
            //
            //     wclear(win_send);
            //     len = 0;
            // }
            // else {
            //     send_buf[len++] = c;
            //     waddch(win_send, c);
            // }
            // wrefresh(win_send);

            // TODO: puts stone and write to socket
            // getchar() -> hjkl -> move cursor
            // getchar() -> space -> put stone
            // send_buf := "%d %d\n".format(x, y)
            // wmove(win_recv, x, y);
            // waddch(win_recv, 'o' or 'x');
            // write(session_soc, send_buf, len);

            c = getchar();
            getyx(win_recv, y, x);
            switch (c) {
            case 'j':
                wmove(win_recv, y+1, x);
                break;
            case 'k':
                wmove(win_recv, y-1, x);
                break;
            case 'h':
                wmove(win_recv, y, x-2);
                break;
            case 'l':
                wmove(win_recv, y, x+2);
                break;
            case ' ':
                // TODO: puts stone
                sprintf(send_buf, "(%d,%d)\n", y, x);
                write(session_soc, send_buf, strlen(send_buf));
                break;
            case 'q':
                sprintf(send_buf, "quit\n");
                write(session_soc, send_buf, strlen(send_buf));
                break;
            }
            wrefresh(win_send);
            wrefresh(win_recv);
        }

        if (FD_ISSET(session_soc, &readOk)) {
            n = read(session_soc, recv_buf, BUF_LEN);
            werase(win_send);
            for (i = 0; i < n; i++) {
                waddch(win_send, recv_buf[i]);
            }

            // TODO: puts stone
            // x, y := scanf("%d %d\n")
            // wmove(win_recv, x, y);
            // waddch(win_recv, 'o' or 'x');

            if (strstr(recv_buf, "quit") != NULL) {
                flag = 0;
            }
            wrefresh(win_send);
            wrefresh(win_recv);
        }

        if (flag == 0) break;
    }

    die();
}

static void die()
{
    endwin();
    close(session_soc);
    exit(0);
}
