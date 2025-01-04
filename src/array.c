#include "array.h"
#include <stdint.h>
#include <stdlib.h>


Array32* new_dynamic_array(void) {
  Array32* new_array = (Array32*)malloc(sizeof(Array32));
  uint32_t *data = (uint32_t*)malloc(ARRAY_SLOT_SIZE * sizeof(uint32_t));
  for (size_t i = 0; i < ARRAY_SLOT_SIZE; i++)
    data[i] = 0;

  new_array->capacity = ARRAY_SLOT_SIZE;
  new_array->size     = 0;
  new_array->data     = data;

  return new_array;
}

void append(Array32 *a, uint32_t value) {
    if (a->size + 1 > a->capacity) {
        a->capacity += ARRAY_SLOT_SIZE;
        a->data = realloc(a->data, a->capacity * sizeof(uint32_t));
    }
    a->data[a->size] = value;
    a->size++;
}
uint32_t pop(Array32 *a) {
    if (a->size < 1)
        return 0;
    if (a->size - 1 < a->capacity - ARRAY_SLOT_SIZE) {
        a->capacity -= ARRAY_SLOT_SIZE;
        a->data = realloc(a->data, a->capacity * sizeof(uint32_t));
    }
    

    uint32_t value = a->data[a->size - 1];
    a->size -= 1;

    return value;
}

void clear(Array32 *a) {
    a->size = 0;
    if (a->capacity != ARRAY_SLOT_SIZE) {
        a->capacity = ARRAY_SLOT_SIZE;
        a->data = realloc(a->data, a->capacity * sizeof(uint32_t));
    }
}

void free_array(Array32 *a) {
    free(a->data);
    free(a);
}

