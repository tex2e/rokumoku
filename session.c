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

static char goban_my_stone;
static char goban_peer_stone;

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

static int put_stone(int, int, char);
static void die();
static int detect_rokumoku(char);

void session_init(int soc)
{
    int x, y;
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

    // Init goban
    x = 0;
    for (y = 0; y < GOBAN_SCREEN_HEIGHT; y++) {
        wmove(win_goban, y, x);
        waddstr(win_goban, goban_plane[y]);
    }
    wmove(win_goban, GOBAN_SCREEN_HEIGHT/2, GOBAN_SCREEN_WIDTH/2);

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
    int is_game_finish = 0;

    while (1) {
        readOk = mask;
        select(width, (fd_set *)&readOk, NULL, NULL, NULL);

        if (FD_ISSET(0, &readOk)) {
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
                if (is_game_finish) break;
                if (!put_stone(y, x, goban_my_stone)) break;

                sprintf(send_buf, "(%d,%d) %c\n", x, y, goban_my_stone);
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
            if (recv_buf[0] == ':') {
                // Game start!
                int id;
                char message[BUF_LEN];
                sscanf(recv_buf, ":%d %s", &id, message);
                if (id == 0) {
                    goban_my_stone = 'x';
                    goban_peer_stone = 'o';
                } else {
                    goban_my_stone = 'o';
                    goban_peer_stone = 'x';
                }
                sprintf(recv_buf, "%s (your stone: %c)\n", message, goban_my_stone);
                werase(win_info);
                waddstr(win_info, recv_buf);
            }
            else if (recv_buf[0] == '(') {
                // Player put stone.
                char stone_char;
                sscanf(recv_buf, "(%d,%d) %c", &x, &y, &stone_char);
                put_stone(y, x, stone_char);
                werase(win_info);
                waddstr(win_info, recv_buf);

                if (stone_char == goban_my_stone && detect_rokumoku(stone_char)) {
                    werase(win_info);
                    waddstr(win_info, "You win!");
                    is_game_finish = 1;
                }
                if (stone_char == goban_peer_stone && detect_rokumoku(stone_char)) {
                    werase(win_info);
                    waddstr(win_info, "You lose!");
                    is_game_finish = 1;
                }
            }
            else {
                // Received broadcast message.
                werase(win_info);
                waddstr(win_info, recv_buf);
            }

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

static int put_stone(int y, int x, char stone_char)
{
    int plane_x = x * 2;
    int plane_y = y;
    if (goban_plane[plane_y][plane_x] != '.') return 0;
    goban_plane[plane_y][plane_x] = stone_char;

    wmove(win_goban, y, x);
    waddch(win_goban, stone_char);
    wmove(win_goban, y, x);
    return 1;
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
