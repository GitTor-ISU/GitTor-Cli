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

extern int gittor_remote_path(char buf[PATH_MAX], const char* repo_id) {
    gchar* dir = g_build_filename(gittor_remote_dir(), repo_id, NULL);
    g_strlcpy(buf, dir, PATH_MAX);
    free(dir);
    return 0;
}
