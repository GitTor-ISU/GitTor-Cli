#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include "utils/utils.h"

extern int tempdir_init(char* buf, size_t buflen) {
    snprintf(buf, buflen, "%s", "/tmp/gittor_tmpXXXXXX");
    mkdtemp(buf);
    return 0;
}

extern int tempdir_destroy(char* dir) {
    return rmdir(dir);
}

extern bool tempdir_exists(char* dir) {
    struct stat st;
    if (stat(dir, &st) == 0 && S_ISDIR(st.st_mode)) {
        return true;
    }
    return false;
}
