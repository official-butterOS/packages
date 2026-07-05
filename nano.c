#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <ctype.h>

#define CTRL_KEY(k) ((k) & 0x1f)
#define MAX_LINES 10000
#define MAX_LINE_LEN 1024

struct termios orig_termios;
char *lines[MAX_LINES];
int num_lines = 0;
int cursor_x = 0, cursor_y = 0;
int screen_rows = 24, screen_cols = 80;
char filename[256] = "";
int dirty = 0;

void die(const char *s) {
    write(STDOUT_FILENO, "\x1b[2J\x1b[H", 7);
    perror(s);
    exit(1);
}

void disable_raw_mode(void) {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void enable_raw_mode(void) {
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(disable_raw_mode);
    struct termios raw = orig_termios;
    raw.c_iflag &= ~(unsigned)(IXON | ICRNL);
    raw.c_oflag &= ~(unsigned)(OPOST);
    raw.c_lflag &= ~(unsigned)(ECHO | ICANON | ISIG | IEXTEN);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void get_window_size(void) {
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) != -1 && ws.ws_col != 0) {
        screen_rows = ws.ws_row - 2;
        screen_cols = ws.ws_col;
    }
}

void load_file(const char *fname) {
    strncpy(filename, fname, sizeof(filename) - 1);
    FILE *fp = fopen(fname, "r");
    if (!fp) {
        lines[0] = calloc(1, 1);
        num_lines = 1;
        return;
    }
    char buf[MAX_LINE_LEN];
    while (fgets(buf, sizeof(buf), fp) && num_lines < MAX_LINES) {
        buf[strcspn(buf, "\n")] = '\0';
        lines[num_lines] = strdup(buf);
        num_lines++;
    }
    if (num_lines == 0) {
        lines[0] = calloc(1, 1);
        num_lines = 1;
    }
    fclose(fp);
}

void save_file(void) {
    FILE *fp = fopen(filename, "w");
    if (!fp) return;
    for (int i = 0; i < num_lines; i++) {
        fprintf(fp, "%s\n", lines[i]);
    }
    fclose(fp);
    dirty = 0;
}

void insert_char(int c) {
    char *line = lines[cursor_y];
    int len = (int)strlen(line);
    if (len >= MAX_LINE_LEN - 1) return;
    char *newline = malloc((size_t)len + 2);
    memcpy(newline, line, (size_t)cursor_x);
    newline[cursor_x] = (char)c;
    memcpy(newline + cursor_x + 1, line + cursor_x, (size_t)(len - cursor_x + 1));
    free(line);
    lines[cursor_y] = newline;
    cursor_x++;
    dirty = 1;
}

void insert_newline(void) {
    if (num_lines >= MAX_LINES - 1) return;
    char *line = lines[cursor_y];
    char *rest = strdup(line + cursor_x);
    line[cursor_x] = '\0';
    for (int i = num_lines; i > cursor_y + 1; i--) {
        lines[i] = lines[i - 1];
    }
    lines[cursor_y + 1] = rest;
    num_lines++;
    cursor_y++;
    cursor_x = 0;
    dirty = 1;
}

void delete_char(void) {
    if (cursor_x == 0 && cursor_y == 0) return;
    if (cursor_x == 0) {
        int prevlen = (int)strlen(lines[cursor_y - 1]);
        char *merged = malloc((size_t)prevlen + strlen(lines[cursor_y]) + 1);
        strcpy(merged, lines[cursor_y - 1]);
        strcat(merged, lines[cursor_y]);
        free(lines[cursor_y - 1]);
        free(lines[cursor_y]);
        lines[cursor_y - 1] = merged;
        for (int i = cursor_y; i < num_lines - 1; i++) {
            lines[i] = lines[i + 1];
        }
        num_lines--;
        cursor_y--;
        cursor_x = prevlen;
    } else {
        char *line = lines[cursor_y];
        int len = (int)strlen(line);
        memmove(line + cursor_x - 1, line + cursor_x, (size_t)(len - cursor_x + 1));
        cursor_x--;
    }
    dirty = 1;
}

void refresh_screen(void) {
    char out[65536];
    int pos = 0;
    pos += sprintf(out + pos, "\x1b[H");

    for (int i = 0; i < screen_rows; i++) {
        if (i < num_lines) {
            int len = (int)strlen(lines[i]);
            if (len > screen_cols) len = screen_cols;
            pos += sprintf(out + pos, "%.*s\x1b[K\r\n", len, lines[i]);
        } else {
            pos += sprintf(out + pos, "~\x1b[K\r\n");
        }
    }

    pos += sprintf(out + pos, "\x1b[7m %s%s | Ctrl-S save | Ctrl-Q quit \x1b[K\x1b[0m\r\n",
                   filename[0] ? filename : "[No Name]", dirty ? " (modified)" : "");

    pos += sprintf(out + pos, "\x1b[%d;%dH", cursor_y + 1, cursor_x + 1);

    write(STDOUT_FILENO, out, (size_t)pos);
}

int read_key(void) {
    char c;
    while (read(STDIN_FILENO, &c, 1) != 1) {}
    return (unsigned char)c;
}

void move_cursor(int key) {
    switch (key) {
        case 'h':
            if (cursor_x > 0) cursor_x--;
            break;
        case 'l':
            if (cursor_x < (int)strlen(lines[cursor_y])) cursor_x++;
            break;
        case 'k':
            if (cursor_y > 0) cursor_y--;
            break;
        case 'j':
            if (cursor_y < num_lines - 1) cursor_y++;
            break;
    }
    int len = (int)strlen(lines[cursor_y]);
    if (cursor_x > len) cursor_x = len;
}

void process_keypress(void) {
    int c = read_key();

    switch (c) {
        case CTRL_KEY('q'):
            write(STDOUT_FILENO, "\x1b[2J\x1b[H", 7);
            exit(0);
            break;
        case CTRL_KEY('s'):
            save_file();
            break;
        case '\r':
            insert_newline();
            break;
        case 127: /* backspace */
            delete_char();
            break;
        case '\x1b': {
            char seq[3];
            if (read(STDIN_FILENO, &seq[0], 1) != 1) break;
            if (read(STDIN_FILENO, &seq[1], 1) != 1) break;
            if (seq[0] == '[') {
                if (seq[1] == 'A') move_cursor('k');
                else if (seq[1] == 'B') move_cursor('j');
                else if (seq[1] == 'C') move_cursor('l');
                else if (seq[1] == 'D') move_cursor('h');
            }
            break;
        }
        default:
            if (!iscntrl(c)) {
                insert_char(c);
            }
            break;
    }
}

int main(int argc, char *argv[]) {
    enable_raw_mode();
    get_window_size();

    if (argc >= 2) {
        load_file(argv[1]);
    } else {
        lines[0] = calloc(1, 1);
        num_lines = 1;
    }

    while (1) {
        refresh_screen();
        process_keypress();
    }

    return 0;
}
