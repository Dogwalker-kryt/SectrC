#include "vector.h"

bool vec_init(Vec *vec, uint32_t expected_element_count, size_t size) {
    if (unlikely(size == 0 || expected_element_count == 0 || !vec)) return false;

    vec->data = NULL;
    vec->expected_element_count = expected_element_count;
    vec->element_count = 0;
    vec->element_size = size;
    vec->buffer_size = 0;

    if ((size_t)expected_element_count > SIZE_MAX / size) return false;

    vec->buffer_size = (size_t)expected_element_count * size;
    vec->data = malloc(vec->buffer_size);

    if (unlikely(!vec->data)) {
        vec->buffer_size = 0;
        vec->expected_element_count = 0;
        vec->element_size = 0;
        return false;
    }

    return true;
}

bool vec_add_element(Vec *vec, const void *element) {
    if (unlikely(!vec || !element || vec->element_size == 0 || vec->element_count == (uint32_t)-1)) return false;

    if (vec->element_count == vec->expected_element_count) {
        uint32_t new_capacity = vec->expected_element_count ? vec->expected_element_count * 2 : DEFAULT_EXPECTED_ELEMENTS_COUNT;
        if (new_capacity < vec->expected_element_count || (size_t)new_capacity > SIZE_MAX / vec->element_size) return false;

        size_t new_buffer_size = (size_t)new_capacity * vec->element_size;
        void *new_data = realloc(vec->data, new_buffer_size);
        if (unlikely(!new_data)) return false;

        vec->data = new_data;
        vec->expected_element_count = new_capacity;
        vec->buffer_size = new_buffer_size;
    }

    char *dest = (char *)vec->data + (size_t)vec->element_count * vec->element_size;
    memcpy(dest, element, vec->element_size);
    ++vec->element_count;

    return true;
}

void *vec_get(Vec *vec, uint32_t index) {
    if (unlikely(!vec || index >= vec->element_count || !vec->data)) return NULL;
    return (char *)vec->data + (size_t)index * vec->element_size;
}

void vec_free(Vec *vec) {
    if (!vec) return;
    free(vec->data);
    vec->data = NULL;
    vec->element_count = 0;
    vec->expected_element_count = 0;
    vec->element_size = 0;
    vec->buffer_size = 0;
}