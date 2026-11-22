#pragma once

#include "../ui/ui.h"
#include "../disk_data/disk_data.h"
#include "health.h"

void print_nvme_health(const drive_mt *disk, const nvme_health_t *health);

void print_ata_smart(const drive_mt *disk, const ata_smart_t *smart);
