#include "array.h"
#define TB_IMPL
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "termbox2.h"

#include "editor.h"
#include "array.h"

void render_file(uint8_t *data, Editor *editor, char *file_name, uint32_t page_size, int line);

int valid_entry(char c);
bool is_printable_code(uint8_t c);
int render_title(int line);


int main(int argc, char *argv[]) {

    int file_number = argc - 1;
    Editor *all_editors = (Editor*) malloc(file_number * sizeof(Editor));
    for (int i = 1; i <= file_number; i++) {
        all_editors[i-1] = load_editor(argv[i]);
    }

    tb_init();
    bool refresh = true;
    bool show_title = true;
    bool exit = false;


    struct winsize terminal_size;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &terminal_size);
    int previous_width = terminal_size.ws_row;
    int page_size =  (terminal_size.ws_row - 12) * 16;
    
    uint8_t clipboard = 0;
    uint8_t current_file = 0;
    
    struct tb_event e;
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
            tb_clear();
            int line = 0;
            if (show_title) {
                line = render_title(line);
            }

            render_file(editor->data, editor, argv[current_file + 1],page_size, line);

            tb_present();
            refresh = false;
        }

        tb_poll_event(&e);
        if (editor->mode == Edit) {
            if (enter_edit_hex(editor, e.ch)) refresh = true;
            switch (e.key) {
                case TB_KEY_ESC:
                    switch_to_normal(editor);
                    refresh = true;;
                    break;
                case TB_KEY_ARROW_RIGHT:
                    move_right(editor);
                    refresh = true;
                    break;

                case TB_KEY_ARROW_LEFT:
                    move_left(editor);
                    refresh = true;
                    break;

                case TB_KEY_ARROW_DOWN:
                    move_down(editor);
                    refresh = true;
                    break;

                case TB_KEY_ARROW_UP:
                    move_up(editor);
                    refresh = true;
                    break;
            }
        } else if (editor->mode == Jump) {
            if (enter_address(editor, e.ch)) refresh = true;
            switch (e.key) {
                case TB_KEY_ESC:
                    switch_to_normal(editor);
                    refresh = true;;
                    break;
                case TB_KEY_ENTER:
                    jump(editor);
                    refresh = true;;
                    break;
                case TB_KEY_BACKSPACE2:
                case TB_KEY_BACKSPACE:
                    delete_address(editor);
                    refresh = true;
                    break;
            
            }
        } else if (editor->mode == Search) {
            if (search_input(editor, e.ch)) refresh = true;
            switch (e.key) {
                case TB_KEY_ESC:
                    switch_to_normal(editor);
                    refresh = true;;
                    break;
                case TB_KEY_ENTER:
                    refresh = true;;
                    switch_to_normal(editor);
                    search_pattern(editor);
                    break;
                case TB_KEY_BACKSPACE2:
                case TB_KEY_BACKSPACE:
                    pop(editor->search_pattern);
                    refresh = true;
                    break;
            
            }
        } else if (editor->mode == AsciiEdit) {
            refresh = enter_edit_ascii(editor, e.ch);
            switch (e.key) {
                case TB_KEY_ARROW_RIGHT:
                    move_right(editor);
                    refresh = true;
                    break;

                case TB_KEY_ARROW_LEFT:
                    move_left(editor);
                    refresh = true;
                    break;

                case TB_KEY_ARROW_DOWN:
                    move_down(editor);
                    refresh = true;
                    break;

                case TB_KEY_ARROW_UP:
                    move_up(editor);
                    refresh = true;
                    break;
                case TB_KEY_ESC:
                    switch_to_normal(editor);
                    refresh = true;;
                    break;
            }

        } else if (editor->mode == Normal) {

            switch (e.key) {
                case TB_KEY_ARROW_RIGHT:
                    move_right(editor);
                    refresh = true;
                    break;

                case TB_KEY_ARROW_LEFT:
                    move_left(editor);
                    refresh = true;
                    break;

                case TB_KEY_ARROW_DOWN:
                    move_down(editor);
                    refresh = true;
                    break;

                case TB_KEY_ARROW_UP:
                    move_up(editor);
                    refresh = true;
                    break;
            }
            switch (e.ch) {
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
                case '>':
                    // Go to the next Result
                    go_to_next_result(editor);
                    refresh = true;
                    break;
                case '<':
                    // Go to the previous result 
                    go_to_previous_result(editor);
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
                case 's':
                    switch_to_search(editor);
                    refresh = true;
                    break;

                case 'a':
                    add_byte(editor);
                    refresh = true;
                    break;
                case 'u':
                    undo(editor);
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
        free_editor(all_editors[i]);
    }
    free(all_editors);
    tb_shutdown();
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

void render_file(uint8_t *data, Editor *editor, char *file_name, uint32_t page_size,int line) {

    if (editor->mode == Edit) {
        tb_print(30, line, TB_MAGENTA, TB_DEFAULT, "-- EDIT --");
        line++;
    }
    else if (editor->mode == Jump) {
        tb_printf(30, line, TB_MAGENTA, TB_DEFAULT, "Jump to %08x", editor->jump_address);
        line++;
    }
    else if (editor->mode == Search) {
        tb_printf(30, line, TB_MAGENTA, TB_DEFAULT, "Search ");
        for (int i = 0; i < editor->search_pattern->size; i++)
            tb_printf(37 + 3*i, line, TB_MAGENTA, TB_DEFAULT, "%02x ", editor->search_pattern->data[i]);
        line++;
    } else {
        line++;
    }
    tb_printf(0, line++, TB_GREEN, TB_DEFAULT, "%s", file_name);
    tb_printf(0, line++,TB_GREEN, TB_DEFAULT, "size: %i bytes -- adress: %08x -- page: %i / %i", editor->size, editor->cursor_index, editor->page, editor->size / page_size);

    for (uint32_t i = editor->page * page_size; i < (editor->page + 1) * page_size && i < editor->size; i++) {
        // Adress display
        if (i % 0x10 == 0 && i != 0) {
            tb_printf(0, line + (i - editor->page * page_size) / 0x10, TB_GREEN, TB_DEFAULT,"%08x: ", i);

        } else if (i % 0x10 == 0 && i == 0) {
            // line++;
            tb_printf(0, line + (i - editor->page * page_size) / 0x10, TB_GREEN, TB_DEFAULT, "00000000: ");

        }
        if (i % 2 == 0) {
            // tb_print(i * 3, line, TB_DEFAULT, TB_DEFAULT," ");
        }
        // Set color for cursor
        int fg_color = TB_DEFAULT;
        int bg_color = TB_DEFAULT;
        if (editor->cursor_index == i && (editor->mode == Normal || editor->mode == Jump || editor->mode == AsciiEdit)) {
            bg_color = TB_RED;
        } else if (editor->cursor_index == i && editor->mode == Edit) {
            bg_color = TB_YELLOW;
        } else if (printable_ascii(data[i])) {
            fg_color = TB_YELLOW;
        }
        

        // Highlight search_result-
        if (i != editor->cursor_index && editor->mode == Normal && !is_empty(editor->search_pattern) && !is_empty(editor->search_result)) {
            for (int j = 0; j < editor->search_pattern->size; j++) {
                if (i-j >= 0) {
                    Result pat_index = find(editor->search_result,i-j);
                    if (pat_index.res) {
                            bg_color = TB_YELLOW;
                            fg_color = TB_BLACK;
                    }
                }
            }
        }


        tb_printf(15 + (i % 16) * 3, line + (i - editor->page * page_size) / 0x10, fg_color, bg_color, "%02x", data[i]);
        tb_print(15 + (i%16) * 3 + 2, line + (i - editor->page * page_size) / 0x10, TB_DEFAULT, TB_DEFAULT, " ");


        // Print the char line on the right
        if (i % 0x10 == 15 || i == editor->size - 1) {
            tb_print(15 + 16 * 3 + 1, line + (i - editor->page * page_size) / 0x10, TB_RED, TB_DEFAULT, "|");
            uint32_t current_line = (i - editor->page * page_size) / 0x10 ;
            for (uint32_t c = current_line * 0x10; c < 0x10 + current_line * 0x10 && c < editor->size; c++) {
                int fg_color = TB_DEFAULT;
                int bg_color = TB_DEFAULT;
                // Adjust cursor's color
                if (editor->cursor_index == c && editor->mode == AsciiEdit) {
                    bg_color = TB_YELLOW;
                } else if (editor->cursor_index == c) {
                    bg_color = TB_RED;
                }
                if (printable_ascii(data[c])) {
                    fg_color = TB_YELLOW;
                    // tb_printf(15 + 16 * 3 + 3 + c % 16, line + current_line,fg_color, bg_color, "%c", data[c]);
                    uint32_t lc = data[c];
                    tb_set_cell_ex(15 + 16 * 3 + 3 + c % 16, line + current_line, &lc,2,fg_color, bg_color);
                } else {
                    tb_print(15 + 16 * 3 + 3 + c % 16, line + current_line,fg_color, bg_color, ".");
                }
            }
        }

    }
}


int render_title(int line) {
    tb_print(0, line, TB_MAGENTA, TB_DEFAULT,     "    db   db d88888b db    db d88888b d8888b.");
    tb_print(0, line + 1, TB_MAGENTA, TB_DEFAULT, "    88   88 88'     `8b  d8' 88'     88  `8D");
    tb_print(0, line + 2, TB_MAGENTA, TB_DEFAULT, "    88ooo88 88ooooo  `8bd8'  88ooooo 88   88");
    tb_print(0, line + 3, TB_MAGENTA, TB_DEFAULT, "    88~~~88 88~~~~~  .dPYb.  88~~~~~ 88   88");
    tb_print(0, line + 4, TB_MAGENTA, TB_DEFAULT, "    88   88 88.     .8P  Y8. 88.     88  .8D");
    tb_print(0, line + 5, TB_MAGENTA, TB_DEFAULT, "    YP   YP Y88888P YP    YP Y88888P Y8888D'");

    return line + 6;
}
