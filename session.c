#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <ncurses.h>

#define BUF_LEN 80

#define INFO_WIN_WIDTH  40
#define INFO_WIN_HEIGHT 1

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

static WINDOW *win_info, *win_goban;
static WINDOW *frame_info, *frame_goban;

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

    // frame_info = newwin(INFO_WIN_HEIGHT + 2, INFO_WIN_WIDTH + 2, 21, 0);
    win_info = newwin(INFO_WIN_HEIGHT, INFO_WIN_WIDTH, 22, 1);
    // box(frame_info, '|', '-');
    scrollok(win_info, FALSE);
    wmove(win_info, 0, 0);

    frame_goban = newwin(GOBAN_SCREEN_HEIGHT + 2, GOBAN_SCREEN_WIDTH + 2, 0, 0);
    win_goban = newwin(GOBAN_SCREEN_HEIGHT, GOBAN_SCREEN_WIDTH, 1, 1);
    box(frame_goban, '|', '-');
    scrollok(win_goban, FALSE);
    wmove(win_goban, 0, 0);

    cbreak();
    noecho();

    wrefresh(frame_info);
    wrefresh(win_info);
    wrefresh(frame_goban);
    wrefresh(win_goban);
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
            // TODO: puts stone and write to socket
            // getchar() -> hjkl -> move cursor
            // getchar() -> space -> put stone
            // send_buf := "%d %d\n".format(x, y)
            // wmove(win_goban, x, y);
            // waddch(win_goban, 'o' or 'x');
            // write(session_soc, send_buf, len);

            c = getchar();
            getyx(win_goban, y, x);
            switch (c) {
            case 'j':
                wmove(win_goban, y+1, x);
                break;
            case 'k':
                wmove(win_goban, y-1, x);
                break;
            case 'h':
                wmove(win_goban, y, x-2);
                break;
            case 'l':
                wmove(win_goban, y, x+2);
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
            wrefresh(win_info);
            wrefresh(win_goban);
        }

        if (FD_ISSET(session_soc, &readOk)) {
            n = read(session_soc, recv_buf, BUF_LEN);
            werase(win_info);
            for (i = 0; i < n; i++) {
                waddch(win_info, recv_buf[i]);
            }

            // TODO: puts stone
            // x, y := scanf("%d %d\n")
            // wmove(win_goban, x, y);
            // waddch(win_goban, 'o' or 'x');

            if (strstr(recv_buf, "quit") != NULL) {
                flag = 0;
            }
            wrefresh(win_info);
            wrefresh(win_goban);
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

static int detect_rokumoku(char stone_char)
{
    int cnt = 0;
    int cnt2 = 0;
    int i, j, k;
    for (i = 0; i < GOBAN_SCREEN_HEIGHT; i++) {
        for (j = 0; j < GOBAN_SCREEN_WIDTH - 1; j += 2) {
            cnt = (goban_plane[i][j] == stone_char) ? (cnt + 1) : 0;
            if (cnt == 6) return 1;
        }
        cnt = 0;
    }

    for (i = 0; i < GOBAN_SCREEN_WIDTH - 1; i += 2) {
        for (j = 0; j < GOBAN_SCREEN_HEIGHT; j++) {
            cnt = (goban_plane[j][i] == stone_char) ? (cnt + 1) : 0;
            if (cnt == 6) return 1;
        }
        cnt = 0;
    }

    for (i = 0; i < GOBAN_SCREEN_HEIGHT; i++) {
        for (j = 0; j < GOBAN_SCREEN_WIDTH - 1; j += 2) {
            for (k = 0; k < GOBAN_SCREEN_WIDTH / 2 - 1; k++) {
                if (!(i + k >= 0 && i + k < GOBAN_SCREEN_HEIGHT)) continue;
                if (!(i + k * 2 < GOBAN_SCREEN_WIDTH - 1)) continue;
                if (!(GOBAN_SCREEN_WIDTH - 2 - j - k * 2 >= 0)) continue;
                cnt  = (goban_plane[i + k][j + k * 2] == stone_char) ? (cnt + 1) : 0;
                cnt2 = (goban_plane[i + k][GOBAN_SCREEN_WIDTH - 2 - j - k * 2] == stone_char) ? (cnt2 + 1) : 0;
                if (cnt == 6) return 1;
                if (cnt2 == 6) return 1;
            }
            cnt = 0;
            cnt2 = 0;
        }
    }

    return 0;
}
