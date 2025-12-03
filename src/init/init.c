#include <git2.h>
#include <glib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "init/init.h"

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

    // Create temporary directory for repo
    gchar* dir = g_dir_make_tmp("gittor-XXXXXX", NULL);
    if (dir == NULL) {
        g_error("Failed to create temporary directory");
        return 1;
    }

    // Initialize libgit2
    int ret = git_libgit2_init();
    if (ret < 0) {
        error = ret;
    }

    // Initialize a new bare repo
    if (!error) {
        error = git_repository_init(&repo, dir, true);
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
        // Get the initial commit hash
        char oid_str[GIT_OID_HEXSZ + 1];
        git_oid_tostr(oid_str, sizeof(oid_str), &commit_oid);

        // Build the permanent path to the repo
        gchar* new_dir = g_build_filename(g_get_user_config_dir(), "gittor",
                                          "repos", oid_str, NULL);

        // Make sure parent directories exist
        gchar* parent = g_path_get_dirname(new_dir);
        if (g_mkdir_with_parents(parent, 0700) != 0) {
            g_error("Failed to create parent directories");
            error = 2;
        }
        g_free(parent);

        // Move repo to permanent path
        rename(dir, new_dir);
        g_snprintf(url, FILE_URL_MAX, "file://%s", new_dir);
        free(new_dir);
    }

    if (error < 0) {
        e = git_error_last();
        printf("Error %d/%d: %s\n", error, e->klass, e->message);
    }

    free(dir);
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
