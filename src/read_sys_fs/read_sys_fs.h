#include "../defs.h"


static inline bool read_sysfs_text(const char *path, char *buffer, __uint64_t capacity);

static inline bool read_sysfs_u64( const char *path, __uint64_t *out);