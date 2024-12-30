#include <string.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <wchar.h>

#include "editor.h"

void render_file(uint8_t *data, Editor *editor, char *file_name, uint32_t page_size);

int valid_entry(char c);
bool is_printable_code(uint8_t c);
void render_title();


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

    int file_number = argc - 1;
    Editor *all_editors = (Editor*) malloc(file_number * sizeof(Editor));
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
        all_editors[i-1] = (Editor) {
                .mode= Normal,
                .page = 0,
                .size = file_size,
                .cursor_index = 0,
                .nibble_index = 0,
                .jump_address = 0,
                .data = data,
        };
    }

    bool refresh = true;
    bool show_title = true;
    bool exit = false;


    struct winsize terminal_size;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &terminal_size);
    int previous_width = terminal_size.ws_row;
    int page_size =  (terminal_size.ws_row - 12) * 16;
    
    uint8_t clipboard = 0;
    uint8_t current_file = 0;


    //Main loop
    while (!exit) {

        Editor *editor = &all_editors[current_file];

        // Keep the cursor inside the bound
        if (editor->cursor_index > editor->size - 1) { editor->cursor_index = editor->size - 1; }

        if (editor->cursor_index < 0) { editor->cursor_index = 0; }
        // Change page if necessary
        if (editor->cursor_index >= (editor->page + 1) * page_size || editor->cursor_index < editor->page * page_size) {
            editor->page = editor->cursor_index / page_size;
            refresh = true;
        }


        // check terminal size
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &terminal_size);
        if (terminal_size.ws_row != previous_width) {
            page_size =  (terminal_size.ws_row - 12) * 16;

            refresh = true;
            
            previous_width = terminal_size.ws_row;
        }
        if (refresh) {
            // Clear screen
            printf("\033[2J");
            // Set the starting postion
            printf("\033[0;0f");
            // Store the starting position
            // printf("\033[s");

            // Restore se saved position (staring postion, upper left)
            // printf("\033[u");
            if (show_title) {
                render_title();
            }

            render_file(editor->data, editor, argv[current_file + 1],page_size);
            refresh = false;
        }

        wchar_t c;
        // read(STDIN_FILENO, &c, 1);
        c = getwchar();
        if (editor->mode == Edit) {
            if (enter_edit_hex(editor, c)) refresh = true;
            switch (c) {
                case 27:
                    escape(editor);
                    refresh = true;
                    break;
                case 17:
                case 'l':
                    move_right(editor);
                    refresh = true;
                    break;

                case 'h':
                    move_left(editor);
                    refresh = true;
                    break;

                case 'j':
                    move_down(editor);
                    refresh = true;
                    break;

                case 'k':
                    move_up(editor);
                    refresh = true;
                    break;
                case 32:
                    refresh = true;
                    break;

            }
        } else if (editor->mode == Jump) {
            if (enter_address(editor, c)) refresh = true;
            switch(c) {
                case 27:
                    switch_to_normal(editor);
                    refresh = true;;
                    break;
                case '\n':
                    jump(editor);
                    refresh = true;;
                    break;
                case 127:
                    delete_address(editor);
                    refresh = true;
                    break;
                case 'q':
                    exit = true;
                    break;
                case 32:
                    refresh = true;
                    break;
            }
        } else if (editor->mode == AsciiEdit) {
            switch (c) {
                case 27:
                    switch_to_normal(editor);
                    refresh = true;
                    break;
                default: 
                    refresh = enter_edit_ascii(editor, c);

            
            }


        } else if (editor->mode == Normal) {

            switch (c) {
                case 'l':
                    move_right(editor);
                    refresh = true;
                    break;

                case 'h':
                    move_left(editor);
                    refresh = true;
                    break;

                case 'j':
                    move_down(editor);
                    refresh = true;
                    break;

                case 'k':
                    move_up(editor);
                    refresh = true;
                    break;

                case 'g':
                    go_to_file_beginning(editor);
                    refresh = true;
                    break;

                case 'G':
                    // Go to the end of the file
                    go_to_file_end(editor);
                    refresh = true;
                    break;
                case '(':
                    // Go to the beginning of the line
                    go_to_line_beginning(editor);
                    refresh = true;
                    break;

                case ')':
                    go_to_file_end(editor);
                    // Go to the end of the line
                    refresh = true;
                    break;
                case '[':
                    // Go to beginning of the editor->page
                    go_to_page_beginning(editor, page_size);
                    refresh = true;
                    break;
                case ']':
                    // Go to the end of the editor->page
                    go_to_page_end(editor, page_size);
                    refresh = true;
                    break;
                case 'n':
                    // Go to next editor->page
                    go_to_next_page(editor, page_size);

                    refresh = true;
                    break;
                case 'b':
                    // Go to previous editor->page
                    go_to_previous_page(editor, page_size);
                    refresh = true;
                    break;

                case 'B': 
                    current_file = previous_file(current_file);
                    refresh = true;
                    break;
                case 'N':
                    current_file = next_file(current_file, file_number);
                    refresh = true;
                    break;
                case 'i':
                    switch_to_edit(editor);
                    refresh = true;
                    break;
                case 'I':
                    switch_to_ascii_edit(editor);
                    refresh = true;
                    break;
                case 'w':
                    save_file(editor, argv[current_file + 1]);
                    break;
                case 'x':
                    // Cut byte on cursor position
                    clipboard = cut_byte(editor);
                    refresh = true;
                    break;

                case 'y':
                    // Copy byte
                    clipboard = copy_byte(editor);
                    break;

                case 'p':
                    // Paste byte
                    paste_byte(editor, clipboard);
                    refresh = true;
                    break;
                case 'J':
                    switch_to_jump(editor);
                    refresh = true;
                    break;

                case 'a':
                    add_byte(editor);
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
        free(all_editors[i].data);
    }
    free(all_editors);
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

void render_file(uint8_t *data, Editor *editor, char *file_name, uint32_t page_size) {

    if (editor->mode == Edit) {
        printf("\033[30C\033[38;5;208m -- EDIT --");
    }
    else if (editor->mode == Jump) {
        printf("\033[30C\033[38;5;208mJump to %08x", editor->jump_address);
    }
    printf("\n\033[38;5;43m%s\n", file_name);
    printf("size: \033[38;5;45m%i bytes\033[38;5;43m -- adress: \033[38;5;45m%08x \033[38;5;43m-- editor->page: \033[38;5;45m%i / %i\n\033[0m",
            editor->size, editor->cursor_index, editor->page, editor->size / page_size);

    for (uint32_t i = editor->page * page_size; i < (editor->page + 1) * page_size && i < editor->size; i++) {
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
        if (editor->cursor_index == i && (editor->mode == Normal || editor->mode == Jump || editor->mode == AsciiEdit)) {
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
            uint32_t current_line = i / 0x10;
            for (uint32_t c = current_line * 0x10; c < 0x10 + current_line * 0x10 && c < editor->size;
                    c++) {

                // Adjust cursor's color
                if (editor->cursor_index == c && editor->mode == AsciiEdit) {
                    printf("\033[48;5;60m");
                } else if (editor->cursor_index == c) {
                    printf("\033[48;5;88m");
                }
                if (printable_ascii(data[c])) {
                // if (is_printable_code(data[c])) {
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
}
