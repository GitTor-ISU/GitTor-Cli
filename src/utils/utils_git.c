#include <git2.h>
#include <glib.h>
#include <stdio.h>
#include "utils/utils.h"

extern int gittor_get_repo_id(git_oid* repo_id, git_repository* repo) {
    __attribute__((__unused__)) int error = 0;
    git_revwalk* walk = NULL;
    git_commit* commit = NULL;

    // Initialize libgit2
    int ret = git_libgit2_init();
    if (ret < 0) {
        error = ret;
    }

    if (!error) {
        error = git_revwalk_new(&walk, repo);
    }
    if (!error) {
        error = git_revwalk_sorting(walk, GIT_SORT_TIME);
    }
    if (!error) {
        error = git_revwalk_push_head(walk);
    }

    if (!error) {
        while (!git_revwalk_next(repo_id, walk)) {
        }
        error = git_commit_lookup(&commit, repo, repo_id);
    }

    git_revwalk_free(walk);
    git_commit_free(commit);
    git_libgit2_shutdown();
    return error;
}

extern int gittor_git_push(git_repository* repo) {
    int error = 0;
    git_remote* remote = NULL;
    git_reference* head = NULL;
    git_reference* resolved = NULL;
    git_push_options opts;

    // Initialize libgit2
    int ret = git_libgit2_init();
    if (ret < 0) {
        error = ret;
    }

    // Load the remote
    if (!error) {
        error = git_remote_lookup(&remote, repo, "origin");
    }

    // Load the push options
    if (!error) {
        error = git_push_options_init(&opts, GIT_PUSH_OPTIONS_VERSION);
    }

    // Get the HEAD of the repository
    if (!error) {
        error = git_repository_head(&head, repo);
    }
    if (!error) {
        error = git_reference_resolve(&resolved, head);
    }

    // Push to the remote
    git_strarray refspecs = {0};
    if (!error) {
        const char* branch = git_reference_name(resolved);
        size_t len = strlen(branch) * 2 + 2;
        char* spec = malloc(len);
        g_snprintf(spec, len, "%s:%s", branch, branch);
        refspecs.strings = (char**)malloc(sizeof(char*));
        refspecs.strings[0] = spec;
        refspecs.count = 1;

        error = git_remote_push(remote, &refspecs, &opts);
        free((void*)refspecs.strings);
    }

    if (error < 0) {
        const git_error* e = git_error_last();
        printf("Error %d/%d: %s\n", error, e->klass, e->message);
    }

    git_reference_free(resolved);
    git_reference_free(head);
    git_remote_free(remote);
    git_libgit2_shutdown();
    return error;
}
