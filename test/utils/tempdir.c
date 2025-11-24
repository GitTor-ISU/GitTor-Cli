#include <glib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include "utils/utils.h"

extern char* tempdir_init() {
    return g_dir_make_tmp("gittor-XXXXXX", NULL);
}

extern void tempdir_destroy(char* dir) {
    rmdir(dir);
    free(dir);
}

extern bool tempdir_exists(char* dir) {
    struct stat st;
    if (stat(dir, &st) == 0 && S_ISDIR(st.st_mode)) {
        return true;
    }
    return false;
}
