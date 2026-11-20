#pragma once

#include "../defs.h"

/**
 * @brief Copies a whole disk or partition onto another one, byte for byte.
 * Names can be "sda", "/dev/sda" or "/dev/sda/". Refuses mounted or in-use devices, overlapping
 * devices (a disk and one of its partitions), targets smaller than the source and whole-disk
 * clones between different sector sizes. Shows both drives and asks first, unless
 * globals.bypass_security_code is set.
 * @param disk_dest the disk or partition to overwrite
 * @param disk_src  the disk or partition to read from
 * @return true when the clone finished or the user said no; false on error (ERR is set)
 */
bool disk_clone(const char *disk_dest, const char *disk_src);
