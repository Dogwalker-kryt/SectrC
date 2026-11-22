#pragma once

#include "../defs.h"

/**
 * @brief Mounts a partition ("sda1", "/dev/sda1") at @p mountpoint. Needs root.
 * @return true on success, false on error (ERR is set)
 */
bool disk_mount(const char *disk_name, const char *mountpoint);

/**
 * @brief Unmounts a partition, or every mounted partition of a disk. Needs root.
 * @return true on success, false on error (ERR is set)
 */
bool disk_unmount(const char *disk_name);
