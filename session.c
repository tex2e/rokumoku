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
    ". . . . . . . . . . . . . . . . . . . .",
    ". . . . . . . . . . . . . . . . . . . .",
    ". . . . . . . . . . . . . . . . . . . .",
    ". . . . . . . . . . . . . . . . . . . .",
    ". . . . . . . . . . . . . . . . . . . .",
    ". . . . . . . . . . . . . . . . . . . .",
    ". . . . . . . . . . . . . . . . . . . .",
    ". . . . . . . . . . . . . . . . . . . .",
    ". . . . . . . . . . . . . . . . . . . .",
    ". . . . . . . . . . . . . . . . . . . .",
    ". . . . . . . . . . . . . . . . . . . .",
    ". . . . . . . . . . . . . . . . . . . .",
    ". . . . . . . . . . . . . . . . . . . .",
    ". . . . . . . . . . . . . . . . . . . .",
    ". . . . . . . . . . . . . . . . . . . .",
    ". . . . . . . . . . . . . . . . . . . .",
    ". . . . . . . . . . . . . . . . . . . .",
    ". . . . . . . . . . . . . . . . . . . .",
    ". . . . . . . . . . . . . . . . . . . .",
    ". . . . . . . . . . . . . . . . . . . ."
};
static char goban_plane_orig[GOBAN_SCREEN_HEIGHT][GOBAN_SCREEN_WIDTH];

static WINDOW *win_info, *win_goban;
static WINDOW *frame_info, *frame_goban;

static char send_buf[BUF_LEN];
static char recv_buf[BUF_LEN];
static int session_soc;
static fd_set mask;
static int width;

static void init_goban();
static int is_my_turn(int, char);
static int put_stone(int, int, char);
static void die();
static int detect_rokumoku(char);

void session_init(int soc)
{
    int i;
    int x, y;
    session_soc = soc;
    width = soc + 1;
    FD_ZERO(&mask);
    FD_SET(0, &mask);
    FD_SET(soc, &mask);

    initscr();
    signal(SIGINT, die);

    win_info = newwin(INFO_WIN_HEIGHT, INFO_WIN_WIDTH, 22, 1);
    scrollok(win_info, FALSE);
    wmove(win_info, 0, 0);

    frame_goban = newwin(GOBAN_SCREEN_HEIGHT + 2, GOBAN_SCREEN_WIDTH + 2, 0, 0);
    win_goban = newwin(GOBAN_SCREEN_HEIGHT, GOBAN_SCREEN_WIDTH, 1, 1);
    box(frame_goban, '|', '-');
    scrollok(win_goban, FALSE);
    wmove(win_goban, 0, 0);

    cbreak();
    noecho();

    memcpy(goban_plane_orig, goban_plane, sizeof(goban_plane));
    init_goban();

    wrefresh(frame_info);
    wrefresh(win_info);
    wrefresh(frame_goban);
    wrefresh(win_goban);
}

void session_loop()
{
    int c;
    fd_set readOk;
    int i;
    int y, x;
    char message[BUF_LEN];
    int status;
    int is_game_loop   = 1;
    int is_game_finish = 0;
    int game_step = 0;

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
                if (!is_my_turn(game_step, goban_my_stone)) break;
                if (!put_stone(y, x, goban_my_stone)) break;

                sprintf(send_buf, "(%d,%d) %c\n", x, y, goban_my_stone);
                write(session_soc, send_buf, strlen(send_buf));

                break;
            case 'r':
            case 'c':
                sprintf(send_buf, "reset\n");
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
            status = read(session_soc, recv_buf, BUF_LEN);
            if (recv_buf[0] == ':') {
                // Game start!
                int id;
                sscanf(recv_buf, ":%d", &id);
                if (id == 0) {
                    goban_my_stone = 'x';
                    goban_peer_stone = 'o';
                    strcpy(message, "Wait.");
                } else {
                    goban_my_stone = 'o';
                    goban_peer_stone = 'x';
                    strcpy(message, "It's your turn!");
                }
                sprintf(recv_buf, "Game start! %s\n", message);
                werase(win_info);
                waddstr(win_info, recv_buf);
            }
            else if (recv_buf[0] == '(') {
                // Player put stone.
                char stone_char;
                sscanf(recv_buf, "(%d,%d) %c", &x, &y, &stone_char);
                put_stone(y, x, stone_char);
                game_step++;
                if ((status = is_my_turn(game_step, goban_my_stone)) > 0) {
                    sprintf(message, "It's your turn! (remains: %d)\n", status);
                } else {
                    sprintf(message, "%s\n", "Wait");
                }
                werase(win_info);
                waddstr(win_info, message);

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
            else if (strstr(recv_buf, "reset") != NULL) {
                // Reset game.
                init_goban();
                game_step = 0;
                is_game_finish = 0;
                if (goban_my_stone == 'x') {
                    strcpy(message, "Wait.");
                } else {
                    strcpy(message, "It's your turn!");
                }
                sprintf(recv_buf, "Game start! %s\n", message);
                werase(win_info);
                waddstr(win_info, recv_buf);
            }
            else if (strstr(recv_buf, "quit") != NULL) {
                // Quit game.
                is_game_loop = 0;
            }
            else {
                // Received broadcast message.
                werase(win_info);
                waddstr(win_info, recv_buf);
            }

            wrefresh(win_info);
            wrefresh(win_goban);
        }

        if (is_game_loop == 0) break;
    }

    die();
}

static void init_goban()
{
    int x, y;
    memcpy(goban_plane, goban_plane_orig, sizeof(goban_plane_orig));

    wclear(win_goban);
    x = 0;
    for (y = 0; y < GOBAN_SCREEN_HEIGHT; y++) {
        wmove(win_goban, y, x);
        waddstr(win_goban, goban_plane[y]);
    }
    wmove(win_goban, GOBAN_SCREEN_HEIGHT/2, GOBAN_SCREEN_WIDTH/2);
}

// Return true if it's my turn.
// game_step: 0 1 2 3 4 5 6 7 8 9 10 ...
// stone:     o x x o o x x o o x x  ...
static int is_my_turn(int game_step, char stone_char)
{
    int mod;
    if (stone_char == 'o' && game_step == 0) return 1;
    if (stone_char == 'x' && game_step == 0) return 0;
    mod = (game_step - 1) % 4;
    if (stone_char == 'o' && mod == 2) return 2;
    if (stone_char == 'o' && mod == 3) return 1;
    if (stone_char == 'x' && mod == 0) return 2;
    if (stone_char == 'x' && mod == 1) return 1;
    return 0;
}

static int put_stone(int y, int x, char stone_char)
{
    if (goban_plane[y][x] != '.') return 0;
    goban_plane[y][x] = stone_char;

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
    int x, y;
    int k;
    for (y = 0; y < GOBAN_SCREEN_HEIGHT; y++) {
        cnt = 0;
        for (x = 0; x < GOBAN_SCREEN_WIDTH - 1; x += 2) {
            cnt = (goban_plane[y][x] == stone_char) ? (cnt + 1) : 0;
            if (cnt == 6) return 1;
        }
    }

    for (x = 0; x < GOBAN_SCREEN_WIDTH - 1; x += 2) {
        cnt = 0;
        for (y = 0; y < GOBAN_SCREEN_HEIGHT; y++) {
            cnt = (goban_plane[y][x] == stone_char) ? (cnt + 1) : 0;
            if (cnt == 6) return 1;
        }
    }

    for (y = 0; y < GOBAN_SCREEN_HEIGHT; y++) {
        for (x = 0; x < GOBAN_SCREEN_WIDTH - 1; x += 2) {
            cnt = 0;
            cnt2 = 0;
            for (k = 0; k < GOBAN_SCREEN_WIDTH / 2 - 1; k++) {
                if (!(y + k >= 0 && y + k < GOBAN_SCREEN_HEIGHT)) continue;
                if (!(y + k * 2 < GOBAN_SCREEN_WIDTH - 1)) continue;
                if (!(GOBAN_SCREEN_WIDTH - 2 - x - k * 2 >= 0)) continue;
                cnt  = (goban_plane[y + k][x + k * 2] == stone_char) ? (cnt + 1) : 0;
                cnt2 = (goban_plane[y + k][GOBAN_SCREEN_WIDTH - 2 - x - k * 2] == stone_char) ? (cnt2 + 1) : 0;
                if (cnt == 6) return 1;
                if (cnt2 == 6) return 1;
            }
        }
    }

    return 0;
}
