#include "../defs.h"
#include "../disk_data/disk_data.h"
#include "../sectr_helper/sectr_helper.h"
#include "../Debug/sectr_err.h"
#include "fingerprint.h"
#include <openssl/sha.h>

#define FINGERPRINT_TEXT_BUFFER_SIZE 2048

static const char *device_basename(const char *device_name) {
    const char *basename = strrchr(device_name, '/');
    return basename ? basename + 1 : device_name;
}

static bool create_fingerprint_of_disk(char *dest, size_t dest_size, const drive_mt *metadata) {
    int written = snprintf(dest, dest_size,
            "bus=%s\nfilesystem=%s\nlabel=%s\nmodel=%s\n"
            "mountpoint=%s\nname=%s\nparent=%s\nvendor=%s\n"
            "serial=%s\nstatus=%s\ntype=%s\nuuid=%s\nsize_bytes=%llu\n",
            metadata->bus, metadata->fstype, metadata->label, metadata->model,
            metadata->mountpoint, metadata->name, metadata->parent, metadata->vendor,
            metadata->serial, metadata->status, metadata->type, metadata->uuid,
            (unsigned long long)metadata->size);

    if (written < 0 || (size_t)written >= dest_size) {
        ERR(ERRC_ProcessFailure, "Failed to serialize disk metadata for fingerprinting", "Fingerprint metadata exceeded the buffer size");
        return false;
    }

    return true;
}

bool fingerprint_disk(const char *disk_name, char hash_hex[FINGERPRINT_SHA256_HEX_SIZE + 1]) {
    if (!disk_name || disk_name[0] == '\0' || !hash_hex) {
        ERR(ERRC_InvalidInput, "Invalid disk fingerprint arguments", "A disk name and hash output buffer are required");
        return false;
    }

    hash_hex[0] = '\0';
    Vec vec;
    if (!vec_init(&vec, 1, sizeof(drive_mt))) {
        ERR(ERRC_AllocationFault, NULL, "Failed to init vec for fingerprinting");
        return false;
    }

    Vec *disk_vec = get_physical_disk_info(&vec);
    if (!disk_vec) {
        vec_free(&vec);
        return false;
    }

    const char *requested_name = device_basename(disk_name);
    const drive_mt *metadata = NULL;

    for (uint32_t i = 0; i < disk_vec->element_count; ++i) {
        const drive_mt *candidate = (const drive_mt *)vec_get(disk_vec, i);

        if (candidate && strcmp(device_basename(candidate->name), requested_name) == 0) {
            metadata = candidate;
            break;
        }
    }

    if (!metadata) {
        ERR(ERRC_DeviceNotFound, "Disk not found for fingerprinting", "%s", disk_name);
        vec_free(&vec);
        return false;
    }

    char text[FINGERPRINT_TEXT_BUFFER_SIZE];
    if (!create_fingerprint_of_disk(text, sizeof(text), metadata)) {
        vec_free(&vec);
        return false;
    }

    unsigned char digest[SHA256_DIGEST_LENGTH];
    if (!SHA256((const unsigned char *)text, strlen(text), digest)) {
        ERR(ERRC_ProcessFailure, "SHA-256 calculation failed", "%s", disk_name);
        vec_free(&vec);
        return false;
    }

    static const char hex[] = "0123456789abcdef";
    
    for (size_t i = 0; i < sizeof(digest); ++i) {
        hash_hex[i * 2] = hex[digest[i] >> 4];
        hash_hex[i * 2 + 1] = hex[digest[i] & 0x0f];
    }
    hash_hex[SHA256_DIGEST_LENGTH * 2] = '\0';

    vec_free(&vec);
    return true;
}