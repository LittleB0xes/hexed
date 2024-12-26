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
    

    
} Editor;

struct Editor *create_editor(char *file_name, uint8_t *data);
#endif // !EDITOR_H

