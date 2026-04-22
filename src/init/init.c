#include <git2.h>
#include <glib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <glib/gstdio.h>
#include "init/init.h"
#include "utils/utils.h"

/**
 * @brief Setup GitTor specific configurations.
 *
 * @param repo The repository to configure
 * @return int error code
 */
static int repo_config(git_repository* repo) {
    int error = 0;
    git_config* cfg = NULL;

    // Initialize libgit2
    int ret = git_libgit2_init();
    if (ret < 0) {
        error = ret;
    }

    // Open the configurations file
    if (!error) {
        error = git_repository_config(&cfg, repo);
    }

    // Configure seed
    if (!error) {
        error = git_config_set_string(cfg, "alias.seed", "!gittor seed");
    }

    // Configure leech
    if (!error) {
        error = git_config_set_string(cfg, "alias.leech", "!gittor leech");
    }

    git_config_free(cfg);
    git_libgit2_shutdown();
    return error;
}

extern int create_bare_repo(char url[FILE_URL_MAX]) {
    int error = 0;
    const git_error* e = NULL;
    git_treebuilder* builder = NULL;
    git_tree* tree = NULL;
    git_signature* me = NULL;
    git_repository* repo = NULL;

    // Make sure parent directories exist
    gchar* tmp_dir = g_build_filename(gittor_remote_dir(), NULL);
    if (g_mkdir_with_parents(tmp_dir, 0700)) {
        g_error("Failed to create directory for remotes (%s): %s", tmp_dir,
                g_strerror(errno));
        error = 1;
    }

    // Create temporary directory for repo
    gchar* tmp_remote_path = g_build_filename(tmp_dir, ".XXXXXX", NULL);
    free(tmp_dir);
    if (!g_mkdtemp(tmp_remote_path) || tmp_remote_path == NULL) {
        g_error("Failed to create temporary directory: %s", g_strerror(errno));
        return 1;
    }

    // Initialize libgit2
    int ret = git_libgit2_init();
    if (ret < 0) {
        error = ret;
    }

    // Initialize a new bare repo
    if (!error) {
        error = git_repository_init(&repo, tmp_remote_path, true);
    }

    // Load up a new git tree
    git_oid tree_oid;
    if (!error) {
        error = git_treebuilder_new(&builder, repo, NULL);
    }
    if (!error) {
        error = git_treebuilder_write(&tree_oid, builder);
    }
    if (!error) {
        error = git_tree_lookup(&tree, repo, &tree_oid);
    }

    // Load up users personal info
    if (!error) {
        error = git_signature_default(&me, repo);

        // If they have no default author set, use these
        if (error == -3 && git_error_last()->klass == 7) {
            error = git_signature_new(&me, "Default User",
                                      "default@example.com", time(NULL), 0);
        }
    }

    // Create an intial commit with users info
    git_oid commit_oid;
    if (!error) {
        error = git_commit_create(&commit_oid, repo, "HEAD", me, me, "UTF-8",
                                  "init", tree, 0, NULL);
    }
    char repo_id_str[GIT_OID_HEXSZ + 1] = {0};
    git_oid_tostr(repo_id_str, sizeof(repo_id_str), &commit_oid);

    // Configure repo
    if (!error) {
        error = repo_config(repo);
    }

    // Close the repository
    git_repository_free(repo);
    git_treebuilder_free(builder);
    git_tree_free(tree);
    git_signature_free(me);

    if (!error) {
        // Build the permanent path to the repo
        gchar remote_path[PATH_MAX];
        gittor_remote_path(remote_path, repo_id_str);

        // Move repo to permanent path
        if (g_rename(tmp_remote_path, remote_path)) {
            g_error("Failed to move repository folder (%s -> %s): %s",
                    tmp_remote_path, remote_path, g_strerror(errno));
            error = 3;
        }
        g_snprintf(url, FILE_URL_MAX, "file://%s", remote_path);
    }

    if (error < 0) {
        e = git_error_last();
        printf("Error %d/%d: %s\n", error, e->klass, e->message);
    }

    free(tmp_remote_path);
    git_libgit2_shutdown();
    return error;
}

extern int clone_bare_repo(char url[FILE_URL_MAX], char path[PATH_MAX]) {
    int error = 0;
    const git_error* e = NULL;
    git_repository* repo = NULL;

    // Initialize libgit2
    int ret = git_libgit2_init();
    if (ret < 0) {
        error = ret;
    }

    // Clone bare repository
    if (!error) {
        error = git_clone(&repo, url, path, NULL);
    }

    // Configure repo
    if (!error) {
        error = repo_config(repo);
    }

    if (error < 0) {
        e = git_error_last();
        printf("Error %d/%d: %s\n", error, e->klass, e->message);
    }

    git_repository_free(repo);
    git_libgit2_shutdown();
    return error;
}
