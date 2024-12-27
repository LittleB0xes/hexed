#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "editor.h"

#define PAGE_SIZE 0x100
#define MAX_FILE 10


void render_file(uint8_t *data, int file_size, char *file_name, uint32_t page, int cursor_index, Mode mode, uint32_t jump_address, int page_size);

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

    FILE *f;

    enableRawMode();

    int file_number = argc - 1;
    int *all_size = (int*) malloc(file_number * sizeof(int));
    uint8_t **all_data = (uint8_t **) malloc(file_number * sizeof(uint8_t*));
    for (int i = 1; i <= file_number; i++) {
        FILE *f;
        f = fopen(argv[i], "rb");
        if (f == NULL) {
            printf("ERROR - Could not open file : %s", argv[1]);
            return -1;
        }

        // Find the file's size
        fseek(f, 0, SEEK_END);
        int file_size = ftell(f);

        // // Rewind
        fseek(f, 0, SEEK_SET);
        // Allocate some memory for the reading file
        uint8_t *data = (uint8_t *)malloc(sizeof(uint8_t) * file_size);

        // Read the data from file
        // And store it in data
        fread(data, sizeof(uint8_t), file_size, f);
        fclose(f);

        // Store file data
        all_data[i-1] = data;
        all_size[i - 1] = file_size;
    }


    // Clear screen
    printf("\033[2J");
    // Set the starting postion
    printf("\033[0;0f");
    // Store the starting position
    printf("\033[s");

    bool refresh = true;
    bool show_title = true;
    bool exit = false;


    Mode mode = Normal;
    struct winsize terminal_size;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &terminal_size);
    int previous_width = terminal_size.ws_row;
    int page_size =  (terminal_size.ws_row - 12) * 16;
    
    uint32_t jump_address = 0;


    uint32_t page = 0;

    uint8_t clipboard = 0;
    int nibble_index = 0;

    uint8_t current_file = 0;



    int cursor_index = 0;
    //Main loop
    // while (read(STDIN_FILENO, &c, 1) == 1 && c != 'q') {
    while (!exit) {
        int file_size = all_size[current_file];

        // Keep the cursor inside the bound
        if (cursor_index > file_size - 1) { cursor_index = file_size - 1; }

        if (cursor_index < 0) { cursor_index = 0; }
        // Change page if necessary
        if (cursor_index >= (page + 1) * page_size || cursor_index < page * page_size) {
            page = cursor_index / page_size;
            refresh = true;
        }
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &terminal_size);
        if (terminal_size.ws_row != previous_width) {
            page_size =  (terminal_size.ws_row - 12) * 16;

            refresh = true;
            
            previous_width = terminal_size.ws_row;
        }
        if (refresh) {
            // Clear string
            printf("\033[2J");

            // Restore se saved position (staring postion, upper left)
            printf("\033[u");
            if (show_title) {
                render_title();
            }

            // render_file(data, file_size, argv[1], page, cursor_index, edit_mode, jump_mode, jump_address);
            render_file(all_data[current_file], all_size[current_file], argv[current_file + 1], page, cursor_index, mode, jump_address, page_size);
            refresh = false;
        }

        char c;
        read(STDIN_FILENO, &c, 1);
        if (mode == Edit) {
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
                all_data[current_file][cursor_index] &= mask;
                all_data[current_file][cursor_index] |= nibble_bits;
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
                    mode = Normal;
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
                case 32:
                    refresh = true;
                    break;

            }
        } else if (mode == Jump) {
            if (c >= 48 && c <= 57) {
                int n = c - 48;
                jump_address <<= 4;
                jump_address += n;
                refresh = true;
            } else if (c >= 97 && c <= 102) {
                int n = c - 87;
                jump_address <<= 4;
                jump_address += n;
                refresh = true;
            }
            switch(c) {
                case 27:
                    mode = Normal;
                    refresh = true;;
                    break;
                case '\n':
                    mode = Normal;
                    cursor_index = jump_address;
                    jump_address = 0;
                    refresh = true;;
                    break;
                case 127:
                    jump_address >>= 4;
                    refresh = true;
                    break;
                case 'q':
                    exit = true;
                    break;
                case 32:
                    refresh = true;
                    break;
            }

        } else if (mode == Normal) {

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
                    cursor_index = page * page_size;
                    refresh = true;
                    break;
                case ']':
                    // Go to the end of the page
                    cursor_index = page * page_size + 0xff;
                    refresh = true;
                    break;
                case 'n':
                    // Go to next page

                    if (page < file_size / page_size) {
                        cursor_index += page_size;
                        page += 1;
                        refresh = true;
                    }
                    break;
                case 'b':
                    // Go to previous page
                    if (page > 0) {
                        cursor_index -= page_size;
                        page -= 1;
                        refresh = true;
                    }
                    break;

                case 'B': 
                    if (current_file > 0) {
                        current_file -= 1;
                        refresh = true;
                    }
                    break;
                case 'N':
                    if (current_file < file_number - 1) {
                        current_file += 1;
                        refresh = true;
                    }
                    break;
                case 'i':
                    // Switch to insert mode
                    mode = Edit;
                    nibble_index = 0;
                    refresh = true;
                    break;

                case 'w':
                    // Save file
                    f = fopen(argv[current_file + 1], "wb");
                    fwrite(all_data[current_file], sizeof(uint8_t), all_size[current_file], f);
                    fclose(f);
                    break;
                case 'x':
                    // Cut byte on cursor position
                    clipboard = all_data[current_file][cursor_index];
                    for (int i = cursor_index; i < file_size - 1; i++) {
                        all_data[current_file][i] = all_data[current_file][i+1];
                    }
                    all_size[current_file] -= 1;
                    all_data[current_file] = realloc(all_data[current_file], all_size[current_file]);
                    refresh = true;
                    break;

                case 'y':
                    // Copy byte
                    clipboard = all_data[current_file][cursor_index];
                    break;

                case 'p':
                    // Paste byte
                    all_data[current_file][cursor_index] = clipboard;
                    cursor_index += 1;
                    refresh = true;
                    break;
                case 'J':
                    mode = Jump;
                    refresh = true;
                    break;

                case 'a':
                    // Add byte after cursor position
                    all_size[current_file] += 1;
                    all_data[current_file] = realloc(all_data[current_file], all_size[current_file]);
                    all_data[current_file][all_size[current_file] - 1] = 0;
                    for (int i = all_size[current_file] - 1; i > cursor_index + 1; i--) {
                        all_data[current_file][i] = all_data[current_file][i-1];
                    }
                    all_data[current_file][cursor_index + 1] = 0;
                    refresh = true;
                    break;
                case 9:
                    show_title = !show_title;
                    refresh = true;
                    break;
                case 'q':
                    exit = true;
                    break;
                case 32:
                    refresh = true;
                    break;
                    
            }
        }
    }

    for (int i=0; i < argc - 1; i++) {
        free(all_data[i]);
    }
    free(all_data);
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

void render_file(uint8_t *data, int file_size, char *file_name, uint32_t page, int cursor_index, Mode mode, uint32_t jump_address, int page_size) {
    bool end_of_line = false;



    if (mode == Edit) {
        printf("\033[30C\033[38;5;208m -- EDIT --");
    }
    else if (mode == Jump) {
        printf("\033[30C\033[38;5;208mJump to %08x", jump_address);
    }
    printf("\n\033[38;5;43m%s\n", file_name);
    printf("size: \033[38;5;45m%i bytes\033[38;5;43m -- adress: \033[38;5;45m%08x \033[38;5;43m-- page: \033[38;5;45m%i / %i\n\033[0m",
            file_size, cursor_index, page, file_size / page_size);

    for (int i = page * page_size; i < (page + 1) * page_size && i < file_size; i++) {
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
        if (cursor_index == i && (mode == Normal || mode == Jump)) {
            printf("\033[48;5;88m");
        } else if (cursor_index == i && mode == Edit) {
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
    // printf("move: hjkl - beginning: g - end: G - beginning of line: ( - end of line: )\n");
    // printf("beginning of page: [ - end of page: ] - next page: n - previous page: b\n");
    // printf("edit: i - quit edit: ESC - save: w - quit: q\n");
    // printf("copy byte: y - cut byte: x - paste byte: p - add byte: a\n");
    //
    // printf("\n");
}
