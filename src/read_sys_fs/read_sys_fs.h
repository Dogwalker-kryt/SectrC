#include "../defs.h"


bool read_sysfs_text(const char *path, char *buffer, __uint_least64_t capacity);

bool read_sysfs_u64(const char *path, __uint64_t *out);


__uint64_t read_sysfs_uint64(const char *__restrict__ path) ;
