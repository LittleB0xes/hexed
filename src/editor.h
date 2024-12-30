#ifndef EDITOR_H


#define EDITOR_H

#include <stdint.h>
#include <stdbool.h>
#include <wchar.h>

typedef enum Mode {
    Normal,
    Edit,
    AsciiEdit,
    Jump,
} Mode;

typedef struct Editor {
    Mode mode;
    uint32_t jump_address;
    uint32_t page;
    uint8_t nibble_index;
    uint32_t cursor_index;
    uint32_t size;
    uint8_t *data;
} Editor;

void escape(Editor *editor);
void move_right(Editor *editor);
void move_up(Editor *editor);
void move_down(Editor *editor);
void move_left(Editor *editor);
void go_to_file_beginning(Editor *editor);
void go_to_file_end(Editor *editor);
void go_to_line_beginning(Editor *editor);
void go_to_line_end(Editor *editor);
void go_to_page_beginning(Editor *editor, uint32_t page_size);
void go_to_page_end(Editor *editor, uint32_t page_size);
void go_to_next_page(Editor *editor, uint32_t page_size);
void go_to_previous_page(Editor *editor, uint32_t page_size);
uint8_t next_file(uint8_t current_file, uint8_t file_number);
uint8_t previous_file(uint8_t current_file);
void switch_to_edit(Editor *editor);
void switch_to_ascii_edit(Editor *editor);
void switch_to_jump(Editor *editor);
void switch_to_normal(Editor *editor);
void save_file(Editor *editor, const char *file_name);
void add_byte(Editor *editor);
uint8_t cut_byte(Editor *editor);
uint8_t copy_byte(Editor *editor);
void paste_byte(Editor *editor, uint8_t clipboard);
void jump(Editor *editor);
void delete_address(Editor *editor);
bool enter_address(Editor *editor, char c);
bool enter_edit_hex(Editor *editor, char c);
bool enter_edit_ascii(Editor *editor, uint32_t c);
bool printable_ascii(uint32_t c);
#endif // !EDITOR_H

