#ifndef EDITOR_H


#define EDITOR_H

#include <stdint.h>
#include <stdbool.h>

typedef enum Mode {
    Normal,
    Edit,
    Jump,
} Mode;

typedef struct Editor {
    Mode mode;
    uint32_t jump_address;
    uint32_t page;
    int nibble_index;
    int cursor_index;
    int size;
} Editor;


#endif // !EDITOR_H

