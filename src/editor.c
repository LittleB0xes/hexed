#include "editor.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <wchar.h>

void escape(Editor *editor) {
    editor->nibble_index = 0;
    editor->mode = Normal;
}
void move_right(Editor *editor) {
    if (editor->cursor_index < editor->size - 1) {
        editor->cursor_index += 1;
    }
    editor->nibble_index = 0;

}
void move_up(Editor *editor) {
    if (editor->cursor_index >= 16) {
        editor->cursor_index -= 16;
    }
    editor->nibble_index = 0;
}
void move_down(Editor *editor) {

    if (editor->size > 16 && editor->cursor_index < editor->size - 16) {
        editor->cursor_index += 16;
    }
    editor->nibble_index = 0;
}
void move_left(Editor *editor) {

    if (editor->cursor_index > 0) {
        editor->cursor_index -= 1;
    }
    editor->nibble_index = 0;
}
void go_to_file_beginning(Editor *editor) {
    // Go to the beginning of the file
    editor->cursor_index = 0;
}
void go_to_file_end(Editor *editor) {
    editor->cursor_index = editor->size - 1;
}
void go_to_line_beginning(Editor *editor) {
    editor->cursor_index = editor->cursor_index / 16 * 16;
}
void go_to_line_end(Editor *editor) {
    editor->cursor_index = editor->cursor_index / 16 * 16 + 15;
}
void go_to_page_beginning(Editor *editor, uint32_t page_size) {
    editor->cursor_index = editor->page * page_size;

}
void go_to_page_end(Editor *editor, uint32_t page_size) {
    editor->cursor_index = editor->page * page_size + page_size - 1;
}
void go_to_next_page(Editor *editor, uint32_t page_size) {
    if (editor->page < editor->size / page_size) {
        editor->cursor_index += page_size;
        editor->page += 1;
    }
}
void go_to_previous_page(Editor *editor, uint32_t page_size) {
    if (editor->page > 0) {
        editor->cursor_index -= page_size;
        editor->page -= 1;
    }
}
uint8_t next_file(uint8_t current_file, uint8_t file_number) {
    if (current_file < file_number - 1) {
        current_file += 1;
    }
    return current_file;

}
uint8_t previous_file(uint8_t current_file) {
    if (current_file > 0) {
        current_file -= 1;
    }
    return current_file;
}
void switch_to_edit(Editor *editor) {
    editor->mode = Edit;
    editor->nibble_index = 0;
}
void switch_to_ascii_edit(Editor *editor) {
    editor->mode = AsciiEdit;
}
void switch_to_jump(Editor *editor) {
    editor->mode = Jump;
}
void switch_to_normal(Editor *editor) {
    editor->mode = Normal;
}
void save_file(Editor *editor, const char *file_name) {
    FILE *f = fopen(file_name, "wb");
    fwrite(editor->data, sizeof(uint8_t), editor->size, f);
    fclose(f);
}
void add_byte(Editor *editor) {
    editor->size += 1;
    editor->data = realloc(editor->data, editor->size);
    editor->data[editor->size - 1] = 0;
    for (uint32_t i = editor->size - 1; i > editor->cursor_index; i--) {
        editor->data[i] = editor->data[i-1];
    }
    editor->data[editor->cursor_index] = 0;
}
uint8_t cut_byte(Editor *editor) {
    if (editor->size < 1) return 0;
    uint8_t clipboard = editor->data[editor->cursor_index];
    for (int i = editor->cursor_index; i < editor->size - 1; i++) {
        editor->data[i] = editor->data[i+1];
    }
    editor->size -= 1;
    editor->data = realloc(editor->data, editor->size);

    return clipboard;
}

uint8_t copy_byte(Editor *editor) {
    return editor->data[editor->cursor_index];
}

void paste_byte(Editor *editor, uint8_t clipboard) {
    editor->data[editor->cursor_index] = clipboard;
    editor->cursor_index += 1;
}
void jump(Editor *editor) {
    editor->mode = Normal;
    editor->cursor_index = editor->jump_address;
    editor->jump_address = 0;
}
void delete_address(Editor *editor) {
    editor->jump_address >>= 4;
}

bool enter_address(Editor *editor, char c) {
    if (c >= 48 && c <= 57) {
        uint8_t n = c - 48;
        editor->jump_address <<= 4;
        editor->jump_address += n;
        return true;
    } else if (c >= 97 && c <= 102) {
        uint8_t n = c - 87;
        editor->jump_address <<= 4;
        editor->jump_address += n;
        return true;
    }
    return false;
}
bool enter_edit_hex(Editor *editor, char c) {
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
        editor->data[editor->cursor_index] &= mask;
        editor->data[editor->cursor_index] |= nibble_bits;
        editor->nibble_index += 1;
        if (editor->nibble_index > 1) {
            editor->nibble_index = 0;
            editor->cursor_index += 1;
        }
        return true;
    }
    return false;
}

bool enter_edit_ascii(Editor *editor, uint32_t c) {
    if (printable_ascii(c)) {
        editor->data[editor->cursor_index] = (uint8_t) c;
        editor->cursor_index += 1;
        return true;
    }

    return false;

}
    
bool printable_ascii(uint32_t c) {
    return (c >= 32 && c < 127) || (c > 128 && c < 254);
}

    

