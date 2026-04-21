#include <git2.h>
#include <glib.h>
#include <limits.h>
#include "utils/utils.h"

static char remotes_dir[PATH_MAX] = "";

extern const char* gittor_remote_dir() {
    if (!remotes_dir[0]) {
        char* dir =
            g_build_filename(g_get_user_config_dir(), "gittor", "repos", NULL);
        g_strlcpy(remotes_dir, dir, PATH_MAX);
        free(dir);
    }
    return remotes_dir;
}

extern int gittor_remote_path(char buf[PATH_MAX], const git_oid* repo_id) {
    char oid_str[GIT_OID_HEXSZ + 1];
    git_oid_tostr(oid_str, sizeof(oid_str), repo_id);
    gchar* dir = g_build_filename(gittor_remote_dir(), oid_str, NULL);
    g_strlcpy(buf, dir, PATH_MAX);
    free(dir);
    return 0;
}
