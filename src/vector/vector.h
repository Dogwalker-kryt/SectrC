#pragma once

#include "../defs.h"

#define DEFAULT_EXPECTED_ELEMENTS_COUNT 7

typedef struct Vec {
    uint32_t element_count;
    uint32_t expected_element_count;
    
    size_t element_size;

    void *data;
    size_t buffer_size;
} Vec;

bool vec_init(Vec *vec, uint32_t expected_element_count, size_t size);

bool vec_add_element(Vec *vec, const void *element);

void *vec_get(Vec *vec, uint32_t index);

void vec_free(Vec *vec);