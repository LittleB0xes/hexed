#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

#define PAGE_SIZE 0x100

void render_file(uint8_t *data, int file_size, char *file_name, uint32_t page, int cursor_index, bool edit_mode);

int valid_entry(char c);
bool is_printable_code(uint8_t c);
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
    bool quit = false;

    int cursor_index = 0;

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
    bool refresh = true;
    bool exit = false;

    uint32_t page = 0;

    uint8_t clipboard = 0;
    int nibble_index = 0;


    // Main loop
    // while (read(STDIN_FILENO, &c, 1) == 1 && c != 'q') {
    while (!exit) {

        // Keep the cursor inside the bound
        if (cursor_index > file_size - 1) { cursor_index = file_size - 1; }

        // Change page if necessary
        if (cursor_index >= (page + 1) * PAGE_SIZE) {
            page += 1;
            refresh = true;
        }
        // if (cursor_index < page * PAGE_SIZE) {
        //     page -= 1;
        //     refresh = true;
        // }
        if (refresh) {
            // Clear string
            printf("\033[2J");

            // Restore se saved position (staring postion, upper left)
            printf("\033[u");

            render_file(data, file_size, argv[1], page, cursor_index, edit_mode);
            refresh = false;
        }

        char c;
        read(STDIN_FILENO, &c, 1);
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
                data[cursor_index] &= mask;
                data[cursor_index] |= nibble_bits;
                nibble_index += 1;
                if (nibble_index > 1) {
                    nibble_index = 0;
                    cursor_index += 1;
                }
            }
            refresh = true;
            switch (c) {
                case 27:
                    nibble_index = 0;
                    refresh = true;
                    edit_mode = false;
                    break;
                case 'l':
                    if (cursor_index < file_size - 1) {
                        cursor_index += 1;
                    }
                    nibble_index = 0;
                    refresh = true;
                    break;

                case 'h':
                    if (cursor_index > 0) {
                        cursor_index -= 1;
                    }
                    nibble_index = 0;
                    refresh = true;
                    break;

                case 'j':
                    if (cursor_index < file_size - 16) {
                        cursor_index += 16;
                    }
                    nibble_index = 0;
                    refresh = true;
                    break;

                case 'k':
                    if (cursor_index >= 16) {
                        cursor_index -= 16;
                    }
                    nibble_index = 0;
                    refresh = true;
                    break;

            }
        } else {

            switch (c) {
                case 'l':
                    if (cursor_index < file_size - 1) {
                        cursor_index += 1;
                    }
                    refresh = true;
                    break;

                case 'h':
                    if (cursor_index > 0) {
                        cursor_index -= 1;
                    }
                    refresh = true;
                    break;

                case 'j':
                    if (cursor_index < file_size - 16) {
                        cursor_index += 16;
                    }
                    refresh = true;
                    break;

                case 'k':
                    if (cursor_index >= 16) {
                        cursor_index -= 16;
                    }
                    refresh = true;
                    break;

                case 'g':
                    // Go to the beginning of the file
                    cursor_index = 0;
                    refresh = true;
                    break;

                case 'G':
                    // Go to the end of the file
                    cursor_index = file_size - 1;
                    refresh = true;
                    break;
                case '(':
                    // Go to the beginning of the line
                    cursor_index = cursor_index / 16 * 16;
                    refresh = true;
                    break;

                case ')':
                    // Go to the end of the line
                    cursor_index = cursor_index / 16 * 16 + 15;
                    refresh = true;
                    break;
                case '[':
                    // Go to beginning of the page
                    cursor_index = page * PAGE_SIZE;
                    refresh = true;
                    break;
                case ']':
                    // Go to the end of the page
                    cursor_index = page * PAGE_SIZE + 0xff;
                    refresh = true;
                    break;
                case 'n':
                    // Go to next page

                    if (page < file_size / PAGE_SIZE) {
                        cursor_index += PAGE_SIZE;
                        page += 1;
                        refresh = true;
                    }
                    break;
                case 'b':
                    // Go to previous page
                    if (page > 0) {
                        cursor_index -= PAGE_SIZE;
                        page -= 1;
                        refresh = true;
                    }
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
                    clipboard = data[cursor_index];
                    for (int i = cursor_index; i < file_size - 1; i++) {
                        data[i] = data[i+1];
                    }
                    file_size -= 1;
                    data = realloc(data, file_size);
                    refresh = true;

                case 'y':
                    // Copy byte
                    clipboard = data[cursor_index];
                    break;

                case 'p':
                    // Paste byte
                    data[cursor_index] = clipboard;
                    cursor_index += 1;
                    refresh = true;
                    break;

                case 'a':
                    // Add byte after cursor position
                    file_size += 1;
                    data = realloc(data, file_size);
                    data[file_size - 1] = 0;
                    for (int i = file_size - 1; i > cursor_index + 1; i--) {
                        data[i] = data[i-1];
                    }
                    data[cursor_index + 1] = 0;
                    refresh = true;
                    break;
                case 'q':
                    exit = true;
                    break;

                case 27:
                    edit_mode = false;
                    refresh = true;
                    break;
            }
        }
    }

    free(data);
    return 0;
}

bool is_printable_code(uint8_t c) {
    return c >= 32 && c <= 126;


}
int valid_entry(char c) {
    int n;
    if (c >= 48 && c <= 57) {
        n = c - 48;
    } else if (c >= 97 && c <= 102) {
        n = c - 87;
    } else {
        n = -1;
    }

    return n;
}

void render_file(uint8_t *data, int file_size, char *file_name, uint32_t page, int cursor_index, bool edit_mode) {
    bool end_of_line = false;


    render_title();

    if (!edit_mode) {
        printf("\033[38;5;43m%s -- size: \033[38;5;45m%i bytes\033[38;5;43m -- page: \033[38;5;45m%i / %i\n\033[0m", file_name,
                file_size, page, file_size / PAGE_SIZE);

    } else {

        printf("\033[38;5;43m%s -- size: \033[38;5;45m%i bytes\033[38;5;43m -- page: \033[38;5;45m%i / %i\033[38;5;208m -- "
                "EDIT --\n",
                file_name, file_size, page, file_size / PAGE_SIZE);
    }
    for (int i = page * PAGE_SIZE; i < (page + 1) * PAGE_SIZE && i < file_size; i++) {
        // Adress display
        if (i % 0x10 == 0 && i != 0) {
            printf("\033[38;5;43m%08x:\033[0m ", i);
        } else if (i % 0x10 == 0 && i == 0) {
            printf("\033[38;5;43m00000000:\033[0m ");
        
        }

        if (i % 2 == 0) {
            printf(" ");
        }
        // Set color for cursor
        if (cursor_index == i && !edit_mode) {
            printf("\033[48;5;88m");
        } else if (cursor_index == i && edit_mode) {
            printf("\033[48;5;60m");
        } else if (is_printable_code(data[i])) {
            printf("\033[38;5;230m");
        }
        printf("%02x", data[i]);
        printf("\033[0m");

        // Print the char line on the right
        if (i % 0x10 == 15 || i == file_size - 1) {
            printf("\033[54G \033[38;5;43m| \033[0m ");
            int current_line = i / 0x10;
            for (int c = current_line * 0x10; c < 0x10 + current_line * 0x10 && c < file_size;
                    c++) {

                // Adjust cursor's color
                if (cursor_index == c) {
                    printf("\033[48;5;88m");
                }
                if (is_printable_code(data[c])) {
                    printf("\033[38;5;230m");
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


void render_title() {
    printf("    db   db d88888b db    db d88888b d8888b.\n");
    printf("    88   88 88'     `8b  d8' 88'     88  `8D\n");
    printf("    88ooo88 88ooooo  `8bd8'  88ooooo 88   88\n");
    printf("    88~~~88 88~~~~~  .dPYb.  88~~~~~ 88   88\n");
    printf("    88   88 88.     .8P  Y8. 88.     88  .8D\n");
    printf("    YP   YP Y88888P YP    YP Y88888P Y8888D'\n");

    printf("\n\033[38;5;43m");
    printf("move: hjkl - beginning: g - end: G - beginning of line: ( - end of line: )\n");
    printf("beginning of page: [ - end of page: ] - next page: n - previous page: b\n");
    printf("edit: i - quit edit: ESC - save: w - quit: q\n");
    printf("copy byte: y - cut byte: x - paste byte: p - add byte: a\n");

    printf("\n");
}
