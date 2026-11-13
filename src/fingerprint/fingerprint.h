#pragma once

#include <stdbool.h>

#define FINGERPRINT_SHA256_HEX_SIZE 64

/* Writes the selected disk's SHA-256 fingerprint as 64 lowercase hex characters. */
bool fingerprint_disk(const char *disk_name, char hash_hex[FINGERPRINT_SHA256_HEX_SIZE + 1]);