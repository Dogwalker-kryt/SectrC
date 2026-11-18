#include "permission.h"

bool is_root() {
    return (getuid() == 0);
}