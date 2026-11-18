#pragma once

#include "../defs.h"

/* Destroys all data on a disk or partition ("nvme0n1", "/dev/nvme0n1" or "/dev/nvme0n1/").
   Refuses mounted / in-use devices, shows what will be lost and asks first
   (unless globals.bypass_security_code is set).
   Method: secure discard → discard → writing zeros, whichever the device supports first.
   Returns true when the disk was overwritten or the user said no; false on error (ERR is set). */
bool disk_overwrite(const char *disk_name);
