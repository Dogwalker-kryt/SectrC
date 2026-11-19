#include "globals.h"
#include "Debug/sectr_err.h"
#include <errno.h>
#include <pwd.h>
#include <sys/stat.h>

globals_t globals = {
    false, false, false, false,
    {NULL, NULL, NULL},
    {NULL, NULL, NULL}
};

void globals_set_ptr_fd(FILE **dest, FILE *src) {
    *dest = src;
}

char *build_path_beside_exe(const char *suffix) {
    char *exe = elf_run_path();
    if (!exe) return NULL;

    char *slash = strrchr(exe, '/');
    size_t dir_len = slash ? (size_t)(slash - exe) : 0;

    char *path_buffer = (char*)malloc(PATH_MAX);
    if (path_buffer == NULL) {
        free(exe);
        ERR(ERRC_AllocationFault, "failed to build path", "malloc() failed");
        return NULL;
    }

    int n = snprintf(path_buffer, PATH_MAX, "%.*s%s", (int)dir_len, exe, suffix);
    free(exe);

    if (n < 0 || (size_t)n >= PATH_MAX) {
        free(path_buffer);
        ERR(ERRC_OutOfRange, "built path is too long", "%s", suffix);
        return NULL;
    }

    return path_buffer;
}

static bool make_dirs(char *path) {
    for (char *p = path + 1; *p; ++p) {
        if (*p != '/') continue;
        *p = '\0';
        bool ok = mkdir(path, 0700) == 0 || errno == EEXIST;
        *p = '/';
        if (!ok) return false;
    }
    return mkdir(path, 0700) == 0 || errno == EEXIST;
}

char *build_state_path(const char *file) {
    const char *xdg = getenv("XDG_STATE_HOME");
    const char *home = getenv("HOME");

    char *path_buffer = (char*)malloc(PATH_MAX);
    if (path_buffer == NULL) {
        ERR(ERRC_AllocationFault, "failed to build state path", "malloc() failed");
        return NULL;
    }

    int n;
    if (xdg && xdg[0] == '/') n = snprintf(path_buffer, PATH_MAX, "%s/sectrc", xdg);
    else if (home && home[0] == '/') n = snprintf(path_buffer, PATH_MAX, "%s/.local/state/sectrc", home);
    else n = -1;

    if (n < 0 || (size_t)n >= PATH_MAX || !make_dirs(path_buffer)) {
        free(path_buffer);
        return NULL;
    }

    size_t dir_len = (size_t)n;
    n = snprintf(path_buffer + dir_len, PATH_MAX - dir_len, "/%s", file);
    if (n < 0 || (size_t)n >= PATH_MAX - dir_len) {
        free(path_buffer);
        return NULL;
    }

    return path_buffer;
}

static const char *config_home(void) {
    const char *sudo_user = getenv("SUDO_USER");
    if (geteuid() != 0 || !sudo_user || sudo_user[0] == '\0') return getenv("HOME");

    const struct passwd *user = getpwnam(sudo_user);
    if (!user || !user->pw_dir || user->pw_dir[0] != '/') return getenv("HOME");
    return user->pw_dir;
}

char *build_config_path(const char *file) {
    const char *home = config_home();
    if (!home || home[0] != '/') {
        ERR(ERRC_MissingData, "Failed to determine home directory", "HOME environment variable is not set or invalid");
        return NULL;
    }

    char *path_buffer = (char*)malloc(PATH_MAX);
    if (!path_buffer) {
        ERR(ERRC_AllocationFault, "failed to build config path", "malloc() failed");
        return NULL;
    }

    int64_t n = snprintf(path_buffer, PATH_MAX, "%s/.config/sectrc/%s", home, file);
    if (n < 0 || (uint64_t)n >= PATH_MAX) {
        free(path_buffer);
        ERR(ERRC_OutOfRange, "config path is too long", "%s/.config/sectrc/%s", home, file);
        return NULL;
    }

    return path_buffer;
}

bool set_bsc_true(const char *value, void *context) {
    (void)value;
    (void)context;
    globals.bypass_security_code = true;
    return true;
}

bool set_nolog_true(const char *value, void *context) {
    (void)value;
    (void)context;
    globals.no_log = true;
    return true; 
}

bool set_no_color_true(const char *value, void *context) {
    (void)value;
    (void)context;
    globals.no_color = true;
    return true;
}

bool set_standalone_true(const char *value, void *context) {
    (void)value;
    (void)context;
    globals.stand_alone = true;
    globals.no_log = true;
    globals.log_path.path = NULL;
    globals.config_path.path = NULL;
    return true;
}

bool set_no_config_true(const char *value, void *context) {
    (void)value;
    (void)context;
    globals.config_path.path = NULL;
    return true;
}

bool set_bypass_security_code_true(const char *value, void *context) {
    (void)value;
    (void)context;
    globals.bypass_security_code = true;
    return true;
}

const char *sectr_color(const char *code) {
    return globals.no_color ? "" : code;
}
