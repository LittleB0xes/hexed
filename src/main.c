#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

void render_file(uint8_t *data, int file_size, char *file_name, int cursor_line,
        int cursor_char, bool edit_mode);

void render_title();
int data_index(int cursor_line, int cursor_char);


struct termios orig_termios;

void disableRawMode() { tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios); }
void enableRawMode() {
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(disableRawMode);
    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

int main(int argc, char *argv[]) {
    enableRawMode();

    FILE *file;
    char c;
    bool quit = false;

    int cursor_line = 0;
    int cursor_char = 0;

    // Try to open the file
    file = fopen(argv[1], "rb");
    if (file == NULL) {
        printf("ERROR - Could not open file : %s", argv[1]);
        return -1;
    }

    // Find the file's size
    fseek(file, 0, SEEK_END);
    int file_size = ftell(file);

    // Rewind
    fseek(file, 0, SEEK_SET);

    // Allocate some memory for the reading file
    uint8_t *data = (uint8_t *)malloc(sizeof(uint8_t) * file_size);

    // Read the data from file
    // And store it in data
    fread(data, sizeof(uint8_t), file_size, file);
    fclose(file);

    // Clear screen
    printf("\033[2J");
    // Set the starting postion
    printf("\033[0;0f");
    // Store the starting position
    printf("\033[s");

    bool edit_mode = false;
    bool refresh = false;

    uint8_t clipboard = 0;
    int nibble_index = 0;
    render_file(data, file_size, argv[1], cursor_line, cursor_char, edit_mode);
    while (read(STDIN_FILENO, &c, 1) == 1 && c != 'q') {
        if (edit_mode) {
            uint8_t n;
            bool valid_entry = false;
            if (c >= 48 && c <= 57) {
                n = c - 48;
                valid_entry = true;
            } else if (c >= 97 && c <= 102) {
                n = c - 87;
                valid_entry = true;
            }
            if (valid_entry) {
                uint8_t nibble_bits = n << 4 * (1 - nibble_index);
                uint8_t mask = 0x0F << 4 * nibble_index;
                data[cursor_char + 16 * cursor_line] &= mask;
                data[cursor_char + 16 * cursor_line] |= nibble_bits;
                nibble_index += 1;
                if (nibble_index > 1) {
                    nibble_index = 0;
                    cursor_char += 1;
                }
            }
            refresh = true;
            if (c == 27) {edit_mode = false;}
        } else {

            switch (c) {
                case 'l':
                    cursor_char += 1;
                    if (data_index(cursor_line, cursor_char) > file_size - 1) {
                        cursor_char -=1;
                    }
                    refresh = true;
                    break;

                case 'h':
                    cursor_char -= 1;
                    if (data_index(cursor_line, cursor_char) < 0) {
                        cursor_line = 0;
                        cursor_char = 0;
                    }
                    refresh = true;
                    break;

                case 'j':
                    cursor_line += 1;
                    if (data_index(cursor_line, cursor_char) > file_size - 1) {
                        cursor_line -= 1;
                    }
                    refresh = true;
                    break;

                case 'k':
                    cursor_line -= 1;
                    if (cursor_line < 0) {cursor_line = 0;}
                    refresh = true;
                    break;

                case 'g':
                    // Go to the beginning of the file
                    cursor_line = 0;
                    cursor_char = 0;
                    refresh = true;
                    break;

                case 'G':
                    // Go to the end of the file
                    cursor_line = file_size / 16;
                    cursor_char = file_size - cursor_line * 16 - 1;
                    refresh = true;
                    break;
                case '0':
                    // Go to the beginning of the line
                    cursor_char = 0;
                    refresh = true;
                    break;

                case '$':
                    // Got to the end of the line
                    cursor_char = 15;
                    refresh = true;
                    break;
                case 'i':
                    // Switch to insert mode
                    edit_mode = true;
                    nibble_index = 0;
                    refresh = true;
                    break;

                case 'w':
                    // Save file
                    file = fopen(argv[1], "wb");
                    fwrite(data, sizeof(uint8_t), file_size, file);
                    fclose(file);
                    break;

                case 'x':
                    // Cut byte on cursor position
                    clipboard = data[cursor_char + 16 * cursor_line];
                    for (int i = cursor_char + 16 * cursor_line; i < file_size - 1; i++) {
                        data[i] = data[i+1];
                    }
                    refresh = true;

                case 'y':
                    // Copy byte
                    clipboard = data[cursor_char + 16 * cursor_line];
                    break;

                case 'p':
                    // Paste byte
                    data[cursor_char + 16 * cursor_line] = clipboard;
                    cursor_char += 1;
                    refresh = true;
                    break;

                case 'a':
                    // Add byte after cursor position
                    file_size += 1;
                    data = realloc(data, file_size);
                    data[file_size - 1] = 0;
                    for (int i = file_size - 1; i > cursor_char + 16 * cursor_line + 1; i--) {
                        data[i] = data[i-1];
                    }
                    data[cursor_char + 16 * cursor_line + 1] = 0;
                    refresh = true;
                    break;

                case 27:
                    edit_mode = false;
                    refresh = true;
                    break;
            }
        }

        if (cursor_char > 16) {
            cursor_line += 1;
            cursor_char = 0;
        }
        if (refresh) {
            // Clear string
            printf("\033[2J");

            // Restore se saved position (staring postion, upper left)
            printf("\033[u");

            render_file(data, file_size, argv[1], cursor_line, cursor_char,
                    edit_mode);
            refresh = false;
        }
    }

    free(data);
    return 0;
}
void render_file(uint8_t *data, int file_size, char *file_name, int cursor_line,
        int cursor_char, bool edit_mode) {
    int line = 0;
    bool end_of_line = false;


    render_title();

    if (!edit_mode) {
        printf("\033[38;5;43m%s -- size: \033[38;5;45m%i bytes\n\033[0m", file_name,
                file_size);

    } else {

        printf("\033[38;5;43m%s -- size: \033[38;5;45m%i bytes\033[38;5;208m -- "
                "EDIT --\n",
                file_name, file_size);
    }
    printf("\033[38;5;43m00000000:\033[0m ");
    for (int i = 0; i < file_size; i++) {
        if (i % 0x10 == 0 && i != 0) {
            printf("\033[38;5;43m%08x:\033[0m ", i);
            line += 1;
        }

        if (i % 2 == 0) {
            printf(" ");
        }
        // Set color for cursor
        if (cursor_line * 16 + cursor_char == i && !edit_mode) {
            printf("\033[48;5;88m");
        } else if (cursor_line * 16 + cursor_char == i && edit_mode) {
            printf("\033[48;5;60m");
        }
        printf("%02x", data[i]);
        printf("\033[0m");

        // Last charcter of the line
        // Print the char line
        // if (i % 0x10 == 15 || data[i] == 0x0a || i == file_size - 1) {
        if (i % 0x10 == 15 || i == file_size - 1) {
            printf("\033[54G \033[38;5;43m| \033[0m ");
            for (int c = 0 + line * 0x10; c < 0x10 + line * 0x10 && c < file_size;
                    c++) {

                // Adjust cursor's color
                if (cursor_line * 16 + cursor_char == c) {
                    printf("\033[48;5;88m");
                }
                if (data[c] >= 32 && data[c] <= 126) {
                    printf("%c", data[c]);
                } else {
                    printf(".");
                }

                // reset cursor color
                printf("\033[0m");
            }
            printf("\n");
        }
    }
}

int data_index(int cursor_line, int cursor_char) {
    return cursor_line * 16 + cursor_char;
}

void render_title() {
    printf("    db   db d88888b db    db d88888b d8888b.\n");
    printf("    88   88 88'     `8b  d8' 88'     88  `8D\n");
    printf("    88ooo88 88ooooo  `8bd8'  88ooooo 88   88\n");
    printf("    88~~~88 88~~~~~  .dPYb.  88~~~~~ 88   88\n");
    printf("    88   88 88.     .8P  Y8. 88.     88  .8D\n");
    printf("    YP   YP Y88888P YP    YP Y88888P Y8888D'\n");

    printf("\n\033[38;5;43m");
    printf("move: hjkl - beginning: g - end: G - end of line: $ - beginning of line: 0\n");
    printf("edit: i - quit edit: ESC - save: w - quit: q\n");
    printf("copy byte: y - cut byte: x - paste byte: p - add byte: a\n");

    printf("\n");
}
