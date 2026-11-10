#pragma once

#include "../defs.h"

#define DEFAULT_EXPECTED_ELEMENTS_COUNT 7

typedef struct Vec {
    __uint32_t element_count;
    __uint32_t expected_element_count;
    
    __uint64_t element_size;

    void *data;
    __uint64_t buffer_size;
} Vec;

bool vec_init(Vec *vec, __uint32_t expected_element_count, __uint64_t size);

/**
 * @brief Adds an element to the vector.
 * @param vec The vector to add the element to.
 * @param element The element to add.
 * @return true on success, false if the arguments or allocation are invalid.
 * @note the adding element should be the same type as the vector's element type, otherwise it will cause undefined behavior.
 */
bool vec_add_element(Vec *vec, const void *element);

/**
 * @brief Gets an element by index, or NULL when the index is outside the vector.
 */
void *vec_get(Vec *vec, __uint32_t index);

void vec_free(Vec *vec);