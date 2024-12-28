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


void render_file(uint8_t *data, Editor *editor, char *file_name, int page_size);

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

    Editor *all_editors = (Editor*) malloc(file_number * sizeof(Editor));
    for (int i = 0; i < file_number; i++) {
        all_editors[i] = (Editor) {
            .mode= Normal,
                .page = 0,
                .size = all_size[i],
                .cursor_index = 0,
                .nibble_index = 0,
                .jump_address = 0,
        };
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

        Editor *editor = &all_editors[current_file];
        int file_size = all_size[current_file];

        // Keep the cursor inside the bound
        if (editor->cursor_index > editor->size - 1) { editor->cursor_index = editor->size - 1; }

        if (cursor_index < 0) { cursor_index = 0; }
        // Change page if necessary
        if (editor->cursor_index >= (editor->page + 1) * page_size || editor->cursor_index < editor->page * page_size) {
            editor->page = editor->cursor_index / page_size;
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

            render_file(all_data[current_file], editor, argv[current_file + 1],page_size);
            refresh = false;
        }

        char c;
        read(STDIN_FILENO, &c, 1);
        if (editor->mode == Edit) {
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
                uint8_t nibble_bits = n << 4 * (1 - editor->nibble_index);
                uint8_t mask = 0x0F << 4 * editor->nibble_index;
                all_data[current_file][editor->cursor_index] &= mask;
                all_data[current_file][editor->cursor_index] |= nibble_bits;
                editor->nibble_index += 1;
                if (editor->nibble_index > 1) {
                    editor->nibble_index = 0;
                    editor->cursor_index += 1;
                }
            }
            refresh = true;
            switch (c) {
                case 27:
                    editor->nibble_index = 0;
                    refresh = true;
                    editor->mode = Normal;
                    break;
                case 'l':
                    if (editor->cursor_index < editor->size - 1) {
                        editor->cursor_index += 1;
                    }
                    editor->nibble_index = 0;
                    refresh = true;
                    break;

                case 'h':
                    if (editor->cursor_index > 0) {
                        editor->cursor_index -= 1;
                    }
                    editor->nibble_index = 0;
                    refresh = true;
                    break;

                case 'j':
                    if (editor->cursor_index < editor->size - 16) {
                        editor->cursor_index += 16;
                    }
                    editor->nibble_index = 0;
                    refresh = true;
                    break;

                case 'k':
                    if (editor->cursor_index >= 16) {
                        editor->cursor_index -= 16;
                    }
                    editor->nibble_index = 0;
                    refresh = true;
                    break;
                case 32:
                    refresh = true;
                    break;

            }
        } else if (editor->mode == Jump) {
            if (c >= 48 && c <= 57) {
                int n = c - 48;
                editor->jump_address <<= 4;
                editor->jump_address += n;
                refresh = true;
            } else if (c >= 97 && c <= 102) {
                int n = c - 87;
                editor->jump_address <<= 4;
                editor->jump_address += n;
                refresh = true;
            }
            switch(c) {
                case 27:
                    editor->mode = Normal;
                    refresh = true;;
                    break;
                case '\n':
                    editor->mode = Normal;
                    editor->cursor_index = editor->jump_address;
                    editor->jump_address = 0;
                    refresh = true;;
                    break;
                case 127:
                    editor->jump_address >>= 4;
                    refresh = true;
                    break;
                case 'q':
                    exit = true;
                    break;
                case 32:
                    refresh = true;
                    break;
            }

        } else if (editor->mode == Normal) {

            switch (c) {
                case 'l':
                    if (editor->cursor_index < editor->size - 1) {
                        editor->cursor_index += 1;
                    }
                    refresh = true;
                    break;

                case 'h':
                    if (editor->cursor_index > 0) {
                        editor->cursor_index -= 1;
                    }
                    refresh = true;
                    break;

                case 'j':
                    if (editor->cursor_index < editor->size - 16) {
                        editor->cursor_index += 16;
                    }
                    refresh = true;
                    break;

                case 'k':
                    if (editor->cursor_index >= 16) {
                        editor->cursor_index -= 16;
                    }
                    refresh = true;
                    break;

                case 'g':
                    // Go to the beginning of the file
                    editor->cursor_index = 0;
                    refresh = true;
                    break;

                case 'G':
                    // Go to the end of the file
                    editor->cursor_index = editor->size - 1;
                    refresh = true;
                    break;
                case '(':
                    // Go to the beginning of the line
                    editor->cursor_index = editor->cursor_index / 16 * 16;
                    refresh = true;
                    break;

                case ')':
                    // Go to the end of the line
                    editor->cursor_index = editor->cursor_index / 16 * 16 + 15;
                    refresh = true;
                    break;
                case '[':
                    // Go to beginning of the editor->page
                    editor->cursor_index = editor->page * page_size;
                    refresh = true;
                    break;
                case ']':
                    // Go to the end of the editor->page
                    editor->cursor_index = editor->page * page_size + 0xff;
                    refresh = true;
                    break;
                case 'n':
                    // Go to next editor->page

                    if (editor->page < editor->size / page_size) {
                        editor->cursor_index += page_size;
                        editor->page += 1;
                        refresh = true;
                    }
                    break;
                case 'b':
                    // Go to previous editor->page
                    if (editor->page > 0) {
                        editor->cursor_index -= page_size;
                        editor->page -= 1;
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
                    // Switch to insert editor->mode
                    editor->mode = Edit;
                    editor->nibble_index = 0;
                    refresh = true;
                    break;

                case 'w':
                    // Save file
                    f = fopen(argv[current_file + 1], "wb");
                    fwrite(all_data[current_file], sizeof(uint8_t), editor->size, f);
                    fclose(f);
                    break;
                case 'x':
                    // Cut byte on cursor position
                    clipboard = all_data[current_file][editor->cursor_index];
                    for (int i = editor->cursor_index; i < editor->size - 1; i++) {
                        all_data[current_file][i] = all_data[current_file][i+1];
                    }
                    editor->size -= 1;
                    all_data[current_file] = realloc(all_data[current_file], editor->size);
                    refresh = true;
                    break;

                case 'y':
                    // Copy byte
                    clipboard = all_data[current_file][editor->cursor_index];
                    break;

                case 'p':
                    // Paste byte
                    all_data[current_file][editor->cursor_index] = clipboard;
                    editor->cursor_index += 1;
                    refresh = true;
                    break;
                case 'J':
                    editor->mode = Jump;
                    refresh = true;
                    break;

                case 'a':
                    // Add byte after cursor position
                    editor->size += 1;
                    all_data[current_file] = realloc(all_data[current_file], editor->size);
                    all_data[current_file][editor->size - 1] = 0;
                    for (int i = editor->size - 1; i > editor->cursor_index + 1; i--) {
                        all_data[current_file][i] = all_data[current_file][i-1];
                    }
                    all_data[current_file][editor->cursor_index + 1] = 0;
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

    free(all_editors);
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

void render_file(uint8_t *data, Editor *editor, char *file_name, int page_size) {
    bool end_of_line = false;



    if (editor->mode == Edit) {
        printf("\033[30C\033[38;5;208m -- EDIT --");
    }
    else if (editor->mode == Jump) {
        printf("\033[30C\033[38;5;208mJump to %08x", editor->jump_address);
    }
    printf("\n\033[38;5;43m%s\n", file_name);
    printf("size: \033[38;5;45m%i bytes\033[38;5;43m -- adress: \033[38;5;45m%08x \033[38;5;43m-- editor->page: \033[38;5;45m%i / %i\n\033[0m",
            editor->size, editor->cursor_index, editor->page, editor->size / page_size);

    for (int i = editor->page * page_size; i < (editor->page + 1) * page_size && i < editor->size; i++) {
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
        if (editor->cursor_index == i && (editor->mode == Normal || editor->mode == Jump)) {
            printf("\033[48;5;88m");
        } else if (editor->cursor_index == i && editor->mode == Edit) {
            printf("\033[48;5;60m");
        } else if (is_printable_code(data[i])) {
            printf("\033[38;5;230m");
        }
        printf("%02x", data[i]);
        printf("\033[0m");

        // Print the char line on the right
        if (i % 0x10 == 15 || i == editor->size - 1) {
            printf("\033[54G \033[38;5;43m| \033[0m ");
            int current_line = i / 0x10;
            for (int c = current_line * 0x10; c < 0x10 + current_line * 0x10 && c < editor->size;
                    c++) {

                // Adjust cursor's color
                if (editor->cursor_index == c) {
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
