#ifndef ARRAY_H

#define ARRAY_H

#include <stdint.h>
#include <stdbool.h>

#define ARRAY_SLOT_SIZE 100

typedef struct Array32 {
    uint32_t *data;
    uint16_t capacity;
    uint16_t size;
    
} Array32;
typedef struct Array8 {
    uint8_t *data;
    uint16_t capacity;
    uint16_t size;
    
} Array8;

typedef struct Result {
    uint32_t value;
    bool res;
} Result;


Array32 *new_dynamic_array();
void free_array(Array32 *a);
void append(Array32 *a, uint32_t value);
uint32_t pop(Array32 *a);
void clear(Array32 *a);
bool is_empty(Array32 *a);
Result find(Array32 *a, uint32_t value);
#endif // !ARRAY_H


